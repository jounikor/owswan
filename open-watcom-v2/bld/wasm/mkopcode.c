/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WASM keyword structures definition program.
*
****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "watcom.h"
#include "hash.h"
#include "mkopcode.h"
#include "asmops.h"
#define MKOPCODE
#include "asminsd.h"

#include "clibext.h"


char Chars[32000];

static unsigned inst_table[HASH_TABLE_SIZE] = { 0 };
static unsigned *index_table;
static unsigned *pos_table;

static int len_compare( const void *pv1, const void *pv2 )
{
    size_t          len1;
    size_t          len2;
    const sword     *p1 = pv1;
    const sword     *p2 = pv2;

    len1 = strlen( p1->word );
    len2 = strlen( p2->word );
    if( len1 < len2 )
        return( 1 );
    if( len1 > len2 )
        return( -1 );
    return( memcmp( p1->word, p2->word, len1 + 1 ) );
}

static void make_inst_hash_tables( unsigned int count, sword *Words )
/*******************************************************************/
{
    char            *name;
    unsigned        i;
    unsigned        *p;
    unsigned        pos;
    unsigned        size = sizeof( AsmOpTable ) / sizeof( AsmOpTable[0] );

    index_table = calloc( count, sizeof( *index_table ) );
    pos_table = calloc( count, sizeof( *pos_table ) );
    pos = 0;
    for( i = 0; i < count; i++ ) {
        // create indexes for hash item lists
        name = Words[i].word;
        for( p = &inst_table[hashpjw( name )]; *p; p = &index_table[*p - 1] ) {
            if( stricmp( name, Words[*p - 1].word ) == 0 ) {
                break;
            }
        }
        if( *p == 0 ) {
            index_table[i] = 0;
            *p = i + 1;
        }
        // create index for position in AsmOpTable
        for( ; pos < size; pos++ ) {
            if( AsmOpTable[pos] >= i ) {
                break;
            }
        }
        if( pos >= size || AsmOpTable[pos] != i ) {
            printf( "Wrong data in asminsd.h. position=%d, index=%d\n", pos, i );
            exit( EXIT_FAILURE );
        }
        pos_table[i] = pos;
    }
}


int main( int argc, char *argv[] )
{
    FILE            *in;
    FILE            *out;
    char            *out_name;
    int             i1;
    int             i2;
    unsigned        words_count;
    size_t          chars_count;
    size_t          len;
    size_t          len1;
    unsigned        idx;
    size_t          idxs;
    sword           *Words;
    char            *word;
    char            buf[KEY_MAX_LEN];

    out_name = argv[argc - 1];
    --argc;

    // Count the words in all the input files
    words_count = 0;
    for( i1 = 1; i1 < argc; ++i1 ) {
        in = fopen( argv[i1], "r" );
        if( in == NULL ) {
            printf( "Unable to open '%s'\n", argv[i1] );
            exit( EXIT_FAILURE );
        }
        for( ; fgets( buf, sizeof( buf ), in ) != NULL; ) {
            words_count++;
        }
        fclose( in );
    }
    Words = malloc( ( words_count + 1 ) * sizeof( sword ) );
    if( Words == NULL ) {
        printf( "Unable to allocate Words array\n" );
        exit( EXIT_FAILURE );
    }
    Words[words_count].word = NULL;
    words_count = 0;
    for( i1 = 1; i1 < argc; ++i1 ) {
        in = fopen( argv[i1], "r" );
        if( in == NULL ) {
            printf( "Unable to open '%s'\n", argv[i1] );
            exit( EXIT_FAILURE );
        }
        for( ; fgets( buf, sizeof( buf ), in ) != NULL; ) {
            for( i2 = 0; buf[i2] != '\0' && !isspace( buf[i2] ); i2++ )
                ;
            buf[i2] = '\0';
            Words[words_count].word = strdup( buf );
            if( Words[words_count].word == NULL ) {
                printf( "Out of memory\n" );
                exit( EXIT_FAILURE );
            }
            ++words_count;
        }
        fclose( in );
    }
    qsort( Words, words_count, sizeof( sword ), len_compare );
    chars_count = 0;
    Chars[0] = '\0';
    for( idx = 0; idx < words_count; idx++ ) {
        word = strstr( Chars, Words[idx].word );
        if( word == NULL ) {
            word = &Chars[chars_count];
            len1 = strlen( Words[idx].word );
            len = len1 - 1;
            if( chars_count < len )
                len = chars_count;
            for( ; len > 0; --len ) {
                if( memcmp( word - len, Words[idx].word, len ) == 0 ) {
                    break;
                }
            }
            len1 -= len;
            memcpy( word, Words[idx].word + len, len1 + 1 );
            chars_count += len1;
            word -= len;
        }
        Words[idx].index = (unsigned)( word - Chars );
    }
    qsort( Words, words_count, sizeof( sword ), str_compare );

    make_inst_hash_tables( words_count, Words );

    out = fopen( out_name, "w" );
    if( out == NULL ) {
        printf( "Unable to open '%s'\n", out_name );
        exit( EXIT_FAILURE );
    }
    fprintf( out, "const char AsmChars[] = {\n" );
    for( idxs = 0; idxs < chars_count; idxs++ ) {
        if( idxs % 10 == 0 )
            fprintf( out, "/*%4u*/ ", (unsigned)idxs );
        fprintf( out, "'%c',", Chars[idxs] );
        if( idxs % 10 == 9 ) {
            fprintf( out, "\n" );
        }
    }
    fprintf( out, "'\\0'\n};\n\n" );
    fprintf( out, "static const unsigned short inst_table[HASH_TABLE_SIZE] = {\n" );
    for( idx = 0; idx < HASH_TABLE_SIZE; idx++ )
        fprintf( out, "\t%u,\n", inst_table[idx] );
    fprintf( out, "};\n\n" );
    fprintf( out, "const struct AsmCodeName AsmOpcode[] = {\n" );
    for( idx = 0; idx < words_count; idx++ ) {
        word = Words[idx].word;
        fprintf( out, "\t{\t%u,\t%u,\t%u,\t%u\t},\t/* %s */\n",
                 pos_table[idx], (unsigned)strlen( word ), Words[idx].index,
                 index_table[idx], get_enum_key( word ) );
    }
    fprintf( out, "\t{\t0,\t0,\t0,\t0\t}\t/* T_NULL */\n" );
    fprintf( out, "};\n\n" );
    fclose( out );
    return( EXIT_SUCCESS );
}
