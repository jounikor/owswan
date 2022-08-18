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

#include "wscrom.h"
#include <wscromtool.h>
#include "wscerror.h"

extern bool g_verbose;

static struct option longopts[] = {
    {"verbose", no_argument,        NULL,       'v'},
    {"debug",   no_argument,        NULL,       'd'},

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

    {"keep-order", no_argument,    NULL,       'k'},
    /* Done */
    {NULL,         0,               NULL,       0 }
};

static void usage(char** argv)
{
    printf("Usage: %s [options] input_rom map_file [output_rom]:\n\n",argv[0]);
    printf("Options:\n"
        "  --data,-D        Data segments to be added into the ROM.\n"
        "  --pub-id,-p      Publisher ID.  Defaults to 0.\n"
        "  --game-id,-g     Game ID. Defaults to 0.\n"
        "  --game-rev,-r    Game revision.  Defaults to 0.\n"
        "  --mono,-m        Monochrome WonderSwan.\n"
        "  --sram-eeprom,-s SRAM/EEPROM size (1k..4M) bits. Default to none.\n"
        "  --1-cycle,-1     ROM access speed (1 cycle instead of 3 cycles).\n"
        "  --8bit-bus,-8    ROM bus width (8-bit instead of 16-bit).\n"
        "  --vertical,-V    Vertical orientation instead of horizontal.\n"
        "  --rtc,-R         Realtime clock present.\n"
        "  input_rom        The output ROM from the compiler/linker.\n"
        "  output_rom       The final ROM after patching. If not give no final\n"
        "                   ROM is saved but all checks are performed.\n"
        "  map_file         The map file produced by the linker.\n"
        );

    exit(EXIT_FAILURE);
}

static sram_size_t sram_size_table[] = {
    0x10,  "1K",          // 1Kbit EEPROM
    0x50,  "8K",          // 8Kbit EEPROM
    0x20,  "16K",         // 16Kbit EEPROM
    0x01,  "64K",         // 64Kbit SRAM (8KB)
    0x02,  "256K",        // 256Kbit SRAM (32KB)
    0x03,  "1M",     // 1Mbit SRAM (128KB)
    0x04,  "2M",     // 2Mbit SRAM (256KB)
    0x05,  "4M"      // 4Mbit SRAM (512KB)
};

static int find_sram_eeprom_size(char *len)
{
    int n;

    for (n = 0; n < sizeof(sram_size_table)/sizeof(sram_size_t); n++) {
        if (strcasecmp(len,sram_size_table[n].size_str)) {
            continue;
        } else {
            return sram_size_table[n].size_byte;
        }
    }

    return 0;
}

/** 
 * @brief Calculate the ROM checksum.
 *
 * @param[in] rom    A ptr to the ROM.
 * @param[in] romlen Length of the ROM. Must be a multiple of 64KB.
 *
 * @return Caclculated checksum.
 */
static uint16_t rom_checksum(uint8_t *rom, int romlen)
{
    uint16_t sum = 0;
    assert((romlen & 0xffff) == 0);

    for (int n=0; n < romlen-2; n++) {
        sum += rom[n];
    }

    return sum;
}

/**
 * @brief Init the ROM metadata structure.
 *
 * @param[out] meta         A ptr to meta to initialize.
 * @param[in] romlen        Length of the ROM. Must be multiple of 64KB and rounded 
 *                          up to supported ROM sizes.
 * @param[in] is_large      True if entire ROM0 bank is used for code.
 * @param[in] publisher     Publisher ID.
 * @param[in] mono          True is WS and False if WSC.
 * @param[in] game_id       Game ID.
 * @param[in] game_revision Game revision.
 * @param[in] save_size.    SRAM/EEPROM size. 0 if none.
 * @param[in] cycles_1      True if ROM access speed is 3 cycle and false if 1 cycle.
 * @param[in] bus_8bit      True if ROM bus width is 8-bit and false if 16-bit.
 * @param[in] vertical      True is orientation is vertical and false if horizontal.
 * @param[in] rtc           True if RTC is available.
 *
 * @return None
 */

static void init_metadata(rom_metadata_t *meta,
    int romlen,
    bool is_large,
    uint8_t text_seg,
    uint8_t data_seg,
    uint8_t publisher,
    bool mono,
    uint8_t game_id,
    uint8_t game_revision,
    uint8_t save_size,
    bool cycles_1,
    bool bus_8bit,
    bool vertical,
    bool rtc)
{
    uint8_t flags = 0;
    rom_size_t size;

    find_rom_size(romlen,&size);

    if (is_large) {
        memcpy(&meta->bootstrap[0],bootstrap_large,sizeof(bootstrap_large));
    } else {
        memcpy(&meta->bootstrap[0],bootstrap,sizeof(bootstrap));
    }
    
    meta->null = 0;


    /* calculate the BANK where our startup code resides.. */
    meta->bootstrap[BOOTSTRAP_BANK_OFFSET] = text_seg;
    meta->bootstrap[DATA_BANK_OFFSET] = data_seg;
    meta->publisher = publisher;
    meta->system = 0 ? mono : 1;
    meta->game_id = game_id;
    meta->game_revision = game_revision;
    
    meta->rom_size = (uint8_t)size.size_byte;
    
    if (cycles_1) { flags |= 0x04; }
    if (bus_8bit) { flags |= 0x02; }
    if (vertical) { flags |= 0x01; }

    meta->save_size = save_size;
    meta->rtc = 1 ? rtc : 0;
    meta->checksum[0] = 0;
    meta->checksum[1] = 0;
}

/**
 * @brief attempt patching ROM metadata and bootstrap code into ROM.
 *
 * @param[inout] rom  A ptr to the ROM to be patched.
 * @param[in] romlen  Length of the ROM. Must be a multiple of 64KB.
 * @param[inout] meta A ptr to metadata to patch into the ROM.
 *
 */

static void patch_rom_metadata(uint8_t *rom, int romlen, rom_metadata_t *meta)
{
    uint16_t checksum;

    /* patch in place */
    memcpy(rom+romlen-sizeof(rom_metadata_t),meta,sizeof(rom_metadata_t)-2);
    
    /* calculate the checksum */

    checksum = rom_checksum(rom,romlen);
    meta->checksum[0] = checksum;
    meta->checksum[1] = checksum >> 8;
    rom[romlen-2] = checksum;
    rom[romlen-1] = checksum >> 8;
}

/**
 * @brief Load a ROM, round up its length and return the buffer, 
 *
 * @param[in] rom_name  ROM to load.
 * @param[in] aux_name  Additional segments to load.
 * @param[in] symbols   A ptr to linker symbols putput.
 * @param[out] rom      A ptr to retuned rom buffer. The caller is responsible 
 *                      for freeing the buffer when done.
 * @param[out] text_seg A ptr to start of the ROM text segment (where
 *                      execution starts.
 * @param[out] data_seg A ptr to start of the aux data segment.
 *
 * @return ROM length rounded up to legal ROM size.
 */


static int load_rom(char *rom_name, char *aux_name, int *symbols, 
    uint8_t **rom, uint8_t *text_seg, uint8_t *data_seg, bool *is_large)
{
    FILE *fh_rom = NULL;
    FILE *fh_aux = NULL;
    int romlen, orglen, romsta;
    int offset = symbols[__romstart];
    int auxlen, orgauxlen;
    int datlen = symbols[__enddata] - symbols[__begdata];
    int datpos = symbols[__begdata];
    
    *is_large = offset >= LARGE_OFFSET ? true : false;

    assert(datlen >= 0);
    assert(datpos+datlen >= 65536);

    if (g_verbose) {
        printf("offset: 0x%08x, datlen 0x%08x, datpos 0x%08x\n",
            offset,datlen,datpos);
    }

    if (offset & 0xffff) {
        return ERR_WSC_ROMALIGNMENT;
    }

    if ((fh_rom = fopen(rom_name,"rb")) == NULL) {
        return ERR_WSC_FILEOPEN;
    }

    fseek(fh_rom,0,SEEK_END);
    romlen = ftell(fh_rom);

    if (romlen <= offset) {
        fclose(fh_rom);
        return ERR_WSC_ROMSIZE;
    }

    fseek(fh_rom,offset,SEEK_SET);
    romlen -= offset;
    orglen = romlen;

    if (aux_name && ((fh_aux = fopen(aux_name,"rb")) == NULL)) {
        fclose(fh_rom);
        return ERR_WSC_FILEOPEN;
    }

    if (fh_aux) {
        fseek(fh_aux,0,SEEK_END);
        auxlen = ftell(fh_aux);
        fseek(fh_aux,0,SEEK_SET);
    } else {
        auxlen = 0;
    }
    
    orgauxlen = auxlen;

    /* Round up to next full 64KB */
    romlen = (romlen + 0xffff) & ~0xffff;
    auxlen = (auxlen + 0xffff) & ~0xffff;

    /* If we have a large code ROM then pad if needed..
       The ROM will always fill $4000:0000 to $ffff:ffff
    */
    if (*is_large) {
        if (romlen < (0x100000 - offset)) {
            romlen = 0x100000 - offset;
        }
    }

    if (orglen > (romlen - datlen - sizeof(rom_metadata_t))) {
        if (*is_large) {
            fclose(fh_rom);
            fclose(fh_aux);
            return ERR_WSC_ROMSIZE; 
        }
    
        romlen += 65536;
    }

    /* now we can calculate the start of code.. */
    *text_seg = (256 - (romlen >> 16));
    romsta = romlen;

    /* Add auxiliary data segments.. it can be 0 bytes */
    romlen += auxlen;

    /* Round up to next legal ROM size.. */
    romlen = find_rom_size(romlen,NULL);

    if (romlen < 0) {
        fclose(fh_rom);
        fclose(fh_aux);
        return romlen;        
    }

    /* Now we know the start of raw data banks */
    *data_seg = (256 - (romlen >> 16));


    if (g_verbose) {
        printf( "romlen: %08x, orglen: %08x, romsta: %08x\n"
                "datlen: %08x, auxlen: %08x, orgauxlen: %08x\n",
                romlen,orglen,romsta,datlen,auxlen,orgauxlen);
    }

    /* Allocate memory for the rounded up ROM + offset bytes of RAM+SRAM
     */

    if ((*rom = calloc(1,romlen)) == NULL) {
        fclose(fh_rom);
        fclose(fh_aux);
        return ERR_WSC_MALLOC;
    }

    /* Read the aux data.. position at the beginnin of the ROM */
    if (auxlen > 0 && (fread(*rom,1,orgauxlen,fh_aux) != orgauxlen)) {
        fclose(fh_rom);
        fclose(fh_aux);
        free(*rom);
        return ERR_WSC_FILEREAD;
    }

    /* Read the game ROM.. position at the end of the ROM */
    fseek(fh_rom,offset,SEEK_SET);

    if (fread(*rom+romlen-romsta,1,orglen,fh_rom) != orglen) {
        fclose(fh_rom);
        fclose(fh_aux);
        free(*rom);
        return ERR_WSC_FILEREAD;
    }

    /* Read the initializable data */
    fseek(fh_rom,datpos,SEEK_SET);

    if (fread(*rom+romlen-datlen-sizeof(rom_metadata_t),1,datlen,fh_rom) != datlen) {
        fclose(fh_rom);
        fclose(fh_aux);
        free(*rom);
        return ERR_WSC_FILEREAD;
    }


    fclose(fh_rom);
    fclose(fh_aux);

    return romlen;
}

static int save_rom(char *output_file, uint8_t *rom, int romlen)
{
    FILE *fh;

    if ((fh = fopen(output_file,"wb")) == NULL) {
        return ERR_WSC_FILEOPEN;
    }

    if (fwrite(rom,1,romlen,fh) != romlen) {
        romlen = ERR_WSC_FILESAVE;
    }

    fclose(fh);
    return romlen;
}


static int read_map_file(const char* const mapfile, int syms[__sym_type_max])
{
    FILE *fh;
    unsigned int seg, off;
    char sym[512];
    char skp[512];
    char *line = NULL;
    size_t cap = 0;
    int n;

    for (n = 0; n < __sym_type_max; n++) {
        syms[n] = -1;
    }

    if ((fh = fopen(mapfile,"r"))) {
        while (getline(&line,&cap,fh) > 0) {
            if (sscanf(line,"%4x:%4x%[ \t+*]%s",&seg,&off,skp,sym) >= 3) {
                /* Mighty brute force search for symbols.. these should
                 * at least be tabled or something.. but oh well..
                 */
                if (!strcmp("__begdata",sym)) {
                    syms[__begdata] = (int)(seg*16 + off);
                    continue;
                }
                if (!strcmp("__enddata",sym)) {
                    syms[__enddata] = (int)(seg*16 + off);
                    continue;
                }
                if (!strcmp("__begbss",sym)) {
                    syms[__begbss] = (int)(seg*16 + off);
                    continue;
                }
                if (!strcmp("__endbss",sym)) {
                    syms[__endbss] = (int)(seg*16 + off);
                    continue;
                }
                if (!strcmp("__romstart",sym)) {
                    syms[__romstart] = (int)(seg*16 + off);
                    continue;
                }
            #if 0
                if (!strcmp("__romend",sym)) {
                    syms[__romend] = (int)(seg*16 + off);
                    continue;
                }
                if (!strcmp("__begromdata",sym)) {
                    syms[__begromdata] = (int)(seg*16 + off);
                    continue;
                }
            #endif
            }
        }

        if (line) {
            free(line);
        }

        fclose(fh);
    }

    for (n = 0; n < __sym_type_max; n++) {
        if (syms[n] < 0) {
            return ERR_WSC_SYMNOTFOUND-n;
        }
    }

    return 0;
}


int rom(int argc, char** argv)
{
    char *input_file = NULL;
    char *output_file = NULL;
    char *map_file = NULL;
    char *aux_file = NULL;
    int ch;

    /* switches */
    uint8_t pub_id = 0;     // invalid
    uint8_t game_id = 0;
    uint8_t game_rev = 0;
    uint8_t save_size = 0;

    bool mono = false;
    bool rtc = false;
    bool vertical = false;
    bool bus_8bit = false;
    bool cycle_1 = false;

    uint8_t data_seg = 0;
    uint8_t text_seg = 0;
    bool is_large;

    int symbols[__sym_type_max];
    uint8_t *rom;
    int ret,romlen;
    rom_metadata_t meta;

    optind = 0;

    while ((ch = getopt_long(argc, argv, "vdDp:mr:s:38VR", longopts, NULL)) != -1) {
        switch (ch) {
        case 'v': case 'd': case 'k':
            break;

        case 'D':   // --data,-D
            aux_file = optarg;
            break;
        case 'p':   // --pub-id,-p
            pub_id = strtoul(optarg,NULL,0);
            break;
        case 'g':   // --game-id,-g
            game_id = strtoul(optarg,NULL,0);
            break;
        case 'r':   // --game-rev,-r
            game_rev = strtoul(optarg,NULL,0);
            break;
        case 'm':   // --mono,-m
            mono = true;
            break;
        case 's':   // --sram-eeprom,-s
            save_size = find_sram_eeprom_size(optarg);

            if (g_verbose && save_size == 0) {
                printf("No SRAM/EEPROM available.\n");
            }
            break;
        case '3':   // --1-cycle,-1
            cycle_1 = true;
            break;
        case '8':   // --8bit-bus,-8
            bus_8bit = true;        
            break;
        case 'V':   // --vertical,-V
            vertical = true;
            break;
        case 'R':   // --rtc,-R
            rtc = true;
            break;
        default:
            usage(argv);
        }
    }
    
    if (argc - optind < 3) {
        usage(argv);
    }
    
    input_file = argv[optind++];
    map_file = argv[optind++];

    if (optind < argc) {
        output_file = argv[optind++];
    }

    if ((ret = read_map_file(map_file,symbols)) < 0) {
        fprintf(stderr,"**Error %d: failed to load map file '%s'\n",
            ret,map_file);
        exit(EXIT_FAILURE);
    }

    if ((romlen = load_rom(input_file,aux_file,symbols,&rom,&text_seg,&data_seg,&is_large)) < 0) {
        fprintf(stderr,"**Error %d: failed to load rom file '%s'\n",
            romlen,argv[optind+0]);
        exit(EXIT_FAILURE);
    }

    init_metadata(&meta,romlen,is_large,text_seg,data_seg,pub_id,mono,game_id,game_rev,
        save_size,cycle_1,bus_8bit,vertical,rtc);

    patch_rom_metadata(rom,romlen,&meta);

    if (output_file) {
        if ((ret = save_rom(output_file,rom,romlen)) < 0) {
            fprintf(stderr,"**Error %d: failed to save rom file '%s'\n",
                ret,output_file);    
            free(rom);
            exit(EXIT_FAILURE);
        }
    }

    free(rom);
    return 0;
}


