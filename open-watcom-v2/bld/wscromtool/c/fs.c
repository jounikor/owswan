/**
 * (c) 2022 by Jouni 'Mr.Spiv' Korhonen
 *
 ****************************************************************************
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

#include "wscromtool.h"
#include "wscerror.h"
#include "fs.h"

extern bool g_verbose;

static struct option longopts[] = {
    /* generic */
    {"verbose", no_argument,        NULL,       'v'},
    {"debug",   no_argument,        NULL,       'd'},

    /* these are optionf from 'rom' just to please getopt_long() */
    {"data",    required_argument,  NULL,       'D'},
    {"pub-id",  required_argument,  NULL,       'p'},
    {"game-id", required_argument,  NULL,       'g'},
    {"game-rev",required_argument,  NULL,       'r'},
    {"mono",    no_argument,        NULL,       'm'},
    {"sram-eeprom",    required_argument,  NULL,       's'},
    {"1-cycle", no_argument,        NULL,       '1'},
    {"8bit-bus",no_argument,        NULL,       '8'},
    {"vertical",no_argument,        NULL,       'V'},
    {"rtc",     no_argument,        NULL,       'R'},

    /* these are optionf from 'fs' just to please getopt_long() */
    {"keep-order",  no_argument,    NULL,       'k'},

    /* Done */
    {NULL,         0,               NULL,       0 }
};

static void usage(char** argv)
{
    printf("Usage: %s [options] output_bin outout_hdr file [files..]:\n\n",argv[0]);
    printf("Options:\n"
        "  --keep-order,-k  Do not rearrange files.\n"
        /*"  --align,-a       Align data to 16-bits.\n"*/
        "  output_bin       Output binary file containing data segments.\n"
        "  output_hdr       Generated C-header file for the output binary.\n"
        "  file,files..     Files to add into the output binary. Maximum\n"
        "                   allowed single file size is 65536 bytes.\n"
        );

    exit(EXIT_FAILURE);
}


static int build_file_table(char **file_names, int num_files, file_entry_t **build_file_table)
{
    int n;
    file_entry_t *table;
    struct stat st;

    if ((table = malloc(num_files*sizeof(file_entry_t))) == NULL) {
        return ERR_WSC_MALLOC;
    }

    for (n = 0; n < num_files; n++) {
        if (stat(file_names[n],&st) < 0) {
            free(table);
            return ERR_WSC_FILESTAT;
        }

        if (st.st_size > 65536) {
            free(table);
            return ERR_WSC_FILESIZE;
        }

        table[n].name = file_names[n];
        table[n].size = st.st_size;
        table[n].bin_id = -1;
    }

    *build_file_table = table;
    return 0;
}

/**
 * @brief "Next fit" bin-packing algorithm i.e. the simplest.
 *
 * @param[in] files     A prt to array of file names.
 * @param[in] num_files The size of the files array.
 * @param[out] bins.    A ptr to an array of bins, where files are 
 *                      to be stored.
 * 
 * @return 0 if OK, negative if an error.
 * 
 * @note If the function was successful the caller is responsible for
 *       freeing the memory for bins array.    
 *
 *       I apologise for naive algorithms but with the size and frequency
 *       of the inputs the efficiency does not matter ;)
 *       http://www.martinbroadhurst.com/bin-packing.html has good material
 *       of various bin-packing algorithms and my goto would have been
 *       Worst Fit Decreasing (WFD) algorithm.
 */

static int next_fit(file_entry_t *files, int num_files, bin_t **bins )
{
    int n;
    int num_bins = 1;
    bin_t *b;

    if ((b = malloc(num_files * sizeof(bin_t))) == NULL) {
        return ERR_WSC_MALLOC;
    }

    b[0].capacity = MAX_BIN_CAPACITY;

    for (n = 0; n < num_files; n++) {
        if (b[num_bins-1].capacity < files[n].size) {
            b[num_bins++].capacity = MAX_BIN_CAPACITY;
        }

        b[num_bins-1].capacity -= files[n].size;
        files[n].bin_id = num_bins-1;
    }

    *bins = b;
    return num_bins;
}

static int repeat_next_fit(file_entry_t *files, int num_files, bin_t **bins )
{
    int n,m;
    int num_bins = 1;
    bin_t *b;

    if ((b = malloc(num_files * sizeof(bin_t))) == NULL) {
        return ERR_WSC_MALLOC;
    }

    b[0].capacity = MAX_BIN_CAPACITY;

    for (n = 0; n < num_files; n++) {
        /* This is a lame algorithm but.. */
        bool found = false;

        for (m = 0; m < num_bins; m++) {
            if (b[m].capacity >= files[n].size) {
                /* greedily fit the file into the first available bin */
                files[n].bin_id = m;
                b[m].capacity -= files[n].size;
                found = true;
                break;
            }
        }

        if (found == false) {
            /* Nothing was found.. create a new bin */
            files[n].bin_id = num_bins;
            b[num_bins++].capacity = MAX_BIN_CAPACITY - files[n].size;
        }
    }

    *bins = b;
    return num_bins;
}

static int hdrprep(char *buf, int buflen, char *pfx, char *fmt, ...)
{
    int n;
    char tmp[128];

    va_list va;
    va_start(va,fmt);

    vsnprintf(tmp,128,fmt,va);
    va_end(va);

    for (n = 0; n < strlen(tmp); n++) {
        if (islower(tmp[n])) {
            tmp[n] = toupper(tmp[n]);
        }
        if (tmp[n] == '.') {
            tmp[n] = '_';
        }
    }

    snprintf(buf,buflen,"%s%s",pfx,tmp);
    return 0;
}

static uint8_t binbuf[MAX_BIN_CAPACITY];

static int build_data_file(file_entry_t *files, bin_t *bins, 
    char *output_file, char *output_hdr,
    int num_files, int num_bins)
{
    FILE *out_fh;
    FILE *hdr_fh;
    FILE *fh;
    char *hname;
    char *fname;
    char buf[512];
    int len, pos, n;

    /* reinitialize the bins capacity counter.. we use this for offsets */
    for (n = 0; n < num_bins; n++) {
        bins[n].capacity = 0;
    }

    if ((out_fh = fopen(output_file,"wb")) == NULL) {
        return ERR_WSC_FILEOPEN;
    }

    if ((hdr_fh = fopen(output_hdr,"wb")) == NULL) {
        fclose(out_fh);
        return ERR_WSC_FILEOPEN;
    }

    hname = strrchr(output_hdr,PATHSEP);

    if (hname == NULL) {
        hname = output_hdr;
    } else {
        hname++;
    }

    /* Write the prologue for the header file */
    
    hdrprep(buf,512,"#ifndef ","_%s_INCLUDED\n",hname);
    fwrite(buf,1,strlen(buf),hdr_fh);
    hdrprep(buf,512,"#define ","_%s_INCLUDED\n\n",hname);
    fwrite(buf,1,strlen(buf),hdr_fh);

    /* number of bins i.e., number of used segments */
    hdrprep(buf,512,"#define ","NUM_BANKS_%s %d\n\n",hname,num_bins);
    fwrite(buf,1,strlen(buf),hdr_fh);

    for (n = 0; n < num_files; n++) {
        fname = strrchr(files[n].name,PATHSEP);

        if (fname == NULL) {
            fname = files[n].name;
        } else {
            fname++;
        }

        /* #define BANK_%s %02x */

        hdrprep(buf,512,"#define ","BANK_%s %d\n",fname,files[n].bin_id);
        fwrite(buf,1,strlen(buf),hdr_fh);

        /* #define OFFS_%s %02x */
        hdrprep(buf,512,"#define ","OFFS_%s %d\n",fname,bins[files[n].bin_id].capacity);
        fwrite(buf,1,strlen(buf),hdr_fh);

        /* #define SIZE_%s %02x */
        hdrprep(buf,512,"#define ","SIZE_%s %d\n\n",fname,files[n].size);
        fwrite(buf,1,strlen(buf),hdr_fh);

        /* Write data */
        if ((fh = fopen(files[n].name,"rb")) == NULL) {
            fclose(hdr_fh);
            fclose(out_fh);
            return ERR_WSC_FILEOPEN;
        }

        len = fread(binbuf,1,files[n].size,fh);
        fclose(fh);

        if (len != files[n].size) {
            fclose(hdr_fh);
            fclose(out_fh);
            return ERR_WSC_FILEREAD;
        }

        pos = files[n].bin_id * MAX_BIN_CAPACITY + bins[files[n].bin_id].capacity;

        fseek(out_fh,pos,SEEK_SET);
        fwrite(binbuf,1,len,out_fh);
        bins[files[n].bin_id].capacity += files[n].size;
    }

    fclose(out_fh);

    /* Write the epilogue for the header file */
    hdrprep(buf,512,"#endif ","/* _%s_INCLUDED */\n",hname);
    fwrite(buf,1,strlen(buf),hdr_fh);
    fclose(hdr_fh);

    return 0;
}



int fs(int argc, char **argv)
{
    int ch;
    bool keep_order = false;
    char *output_file = NULL;
    char *output_hdr = NULL;
    int num_files;
    file_entry_t *files;
    bin_t *bins;
    int num_bins;
    int ret;

    optind = 0;

    while ((ch = getopt_long(argc, argv, "kvdDp:mr:s:38VR", longopts, NULL)) != -1) {
        switch (ch) {
        case 'v': case 'd': case 'D': case 'p':
        case 'g': case 'r': case 'm': case 's':
        case '3': case '8': case 'V': case 'R':
            break;
        case 'k':   // --keep-order,-k
            keep_order = true;
            break;
        default:
            usage(argv);
        }
    }

    if (argc - optind < 3) {
        usage(argv);
    }

    output_file = argv[optind++];
    output_hdr  = argv[optind++];
    num_files   = argc - optind;

    ret = build_file_table(&argv[optind],num_files,&files);

    if (ret < 0) {
        fprintf(stderr,"**Error %d: building file table failed\n",ret);
        return -1;
    }

    if (keep_order) {
        num_bins = next_fit(files, num_files, &bins);
    } else {
        num_bins = repeat_next_fit(files, num_files, &bins);
    }

    if (num_bins < 0) {
        fprintf(stderr,"**Error %d: building bins failed\n",num_bins);
        free(files);
        return -1;
    }

    if (g_verbose) {
        printf("Total %d files, number of bins %d\n",num_files,num_bins);

        for (int n = 0; n < num_bins; n++) {
            printf("bin %2d: capacity %d\n",n,bins[n].capacity);
        }

        for (int n = 0; n < num_files; n++) {
            printf("file %2d (%d) is in bin %d\n",n,
                files[n].size,files[n].bin_id);
        }
    }

    ret = build_data_file(files,bins,output_file,output_hdr,num_files,num_bins);

    free(bins);
    free(files);
    return ret;
}
