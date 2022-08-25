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
#include <getopt.h>

#include "rom.h"
#include "fs.h"
#include "wscromtool.h"
#include "wscerror.h"


/* Versoning of this tool
 */

#define VER_MAJOR 0
#define VER_MINOR 3

/* global flags */
bool g_verbose = false;

/* commandline options to be shared between all commands */

struct option longopts[] = {
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

const char *optstring = "kvdDp:mr:s:18VRg";

static void usage(char** argv)
{
    printf("WonderSwan ROM tool v%d.%d (c) 2022 Jouni 'Mr.Spiv' Korhonen\n\n"
	         "Usage: %s command [options] [files..]\n",
             VER_MAJOR,VER_MINOR,argv[0]);
	printf("Commands:\n"
        "  rom  Prepare final ROM.\n"
        "  fs   Create ROM bank(s) file for 'ROM file system'.\n"
        "Options:\n"
        "  --verbose,-v Print various information while processing.\n\n" 
        "For command specific options type:\n"
        "  %s command\n",argv[0]
        );

	exit(EXIT_FAILURE);
}

rom_size_t rom_size_table[] = {
    0x00, 2*65536,      // 1Mbit? (128KB)
    0x01, 4*65536,      // 2Mbit? (256KB)
    0x02, 8*65536,      // 4Mbit (512KB)
    0x03, 16*65536,     // 8Mbit (1MB)
    0x04, 32*65536,     // 16Mbit (2MB)
    0x05, 48*65536,     // 24Mbit (3MB)
    0x06, 64*65536,     // 32Mbit (4MB)
    0x07, 96*65536,     // 48Mbit (6MB)
    0x08, 128*65536,    // 64Mbit (8MB)
    0x09, 256*65536,    // 128Mbit (16MB)
};

/**
 * @brief Find the first valid ROM size.
 *
 * @param[in] romlen Rom size to round up.
 * @param[out] size  A prt to rom_size_t or NULL.
 *
 * @return Negative if ROM was too big. Otherwise the
 *   rounded up ROM size.
 */

int find_rom_size(int romlen, rom_size_t *size)
{
    for (int n = 0; n < sizeof(rom_size_table)/sizeof(rom_size_t); n++) {
        if (romlen > rom_size_table[n].size_max) {
            continue;
        } else {
            if (size) {
                size->size_byte = rom_size_table[n].size_byte;
                size->size_max  = rom_size_table[n].size_max;
            }
            return rom_size_table[n].size_max;
        }
    }

    return ERR_WSC_ROMSIZE;
}

int find_rom_size_min8Mb(int romlen, rom_size_t *size)
{
    for (int n = 3; n < sizeof(rom_size_table)/sizeof(rom_size_t); n++) {
        if (romlen > rom_size_table[n].size_max) {
            continue;
        } else {
            if (size) {
                size->size_byte = rom_size_table[n].size_byte;
                size->size_max  = rom_size_table[n].size_max;
            }
            return rom_size_table[n].size_max;
        }
    }

    return ERR_WSC_ROMSIZE;
}


/**
 * @brief Program entrypoint, which calls separate subcommands. 
 * 
 */

int main(int argc, char** argv)
{
    int ch;
    int ret = 0;

    if (argc < 2) {
        usage(argv);
    }

    optind = 2;

    while ((ch = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
        switch (ch) {
        case 'v':   // --verbose,-v
        case 'd':   // --debug,-d
            g_verbose = true;
            break;
        default:
            break;
        }
    }

    if (!strcmp(argv[1],"rom")) {
        ret = rom(argc-1,&argv[1]);
    } else if (!strcmp(argv[1],"fs")) {
        ret = fs(argc-1,&argv[1]);
    } else {
        usage(argv);
    }

    return ret;
}


