/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  Implements the functions declared in cffunc.h:
*                   get_code_blocks()
*                   get_p_buffer()
*                   parse_functions_block()
*
* Note:         The Wiki should be consulted for any term whose meaning is
*               not apparent. This should help in most cases.
*
****************************************************************************/

#define __STDC_WANT_LIB_EXT1__  1
#include <stdlib.h>
#include <string.h>
#include "cffunc.h"
#include "common.h"
#include "research.h"

/*  Extern function definitions. */

/*  Function get_code_blocks().
 *  Return a pointer to an array of code_block structs containing the
 *  CodeBlocks in a P-buffer.
 *
 *  Parameters:
 *      current points to the first byte of the data to be processed.
 *      cb_count contains the number of CodeBlocks expected.
 *      base points to the first byte of the underlying P-buffer.
 *      block points to a character string naming the block being parsed.
 *
 *  Parameter is changed:
 *      current points to the byte after the last byte processed.
 *      on failure, it's position is not meaningful.
 *
 *  Returns:
 *      a pointer to a code_block containing the CodeBlocks on success.
 *      NULL on failure.
 */

code_block * get_code_blocks( char **current, uint16_t cb_count, char *base, char *block )
{
    code_block *    out_block   = NULL;
    size_t          difference;
    size_t          position;
    char            junk_byte[3];
    char            first_byte[3];
    char            second_byte[3];
    uint8_t         i;

    /* Allocate the out_block. */

    out_block = (code_block * ) malloc( sizeof( code_block ) * cb_count );

    /* Initialize each code_block struct instance. */

    for( i = 0; i < cb_count; i++ ) {

        /* Get the position of the designator in the P-buffer. */

        difference = *current - base;
        position = difference % 80;

        /* Get the designator, shifting it if necessary. */

        if( position == 79 ) {
            printf_s( "Parsing %s block: CodeBlock %i has a shifted designator\n", block, i);
            display_hex_char( junk_byte, **current );
            display_hex_char( first_byte, *(*current + 1) );
            printf_s( "Values: %s %s\n", junk_byte, first_byte );
            *current += 1;
        }

        memcpy_s( &out_block[i].designator, 1, *current, 1 );
        *current += 1;

        /* Get the cb05_flag. */

        memcpy_s( &out_block[i].cb05_flag, 1, *current, 1 );
        *current += 1;

        /* Get the lp_flag. */

        memcpy_s( &out_block[i].lp_flag, 1, *current, 1 );
        *current += 1;

        /* Get the pass, shifting it if necessary. */

        if( position == 76 ) {
            printf_s( "Parsing %s block: CodeBlock %i has a shifted pass.\n", block, i);
            display_hex_char( junk_byte, **current );
            display_hex_char( first_byte, *(*current + 1) );
            display_hex_char( second_byte, *(*current + 2) );
            printf_s( "Values: %s %s %s\n", junk_byte, first_byte, second_byte );
            *current += 1;
        }

        memcpy_s( &out_block[i].pass, 2, *current, 2 );
        *current += 2;

        /* Get the count, shifting it if necessary. */

        if( position == 74 ) {
            printf_s( "Parsing %s block: CodeBlock %i has a shifted count\n", block, i);
            display_hex_char( junk_byte, **current );
            display_hex_char( first_byte, *(*current + 1) );
            display_hex_char( second_byte, *(*current + 2) );
            printf_s( "Values: %s %s %s\n", junk_byte, first_byte, second_byte );
            *current += 1;
        }

        memcpy_s( &out_block[i].count, 2, *current, 2 );
        *current += 2;

        /* Set text, which is the pointer to the actual compiled code. */

        if( &out_block[i].count == 0 ) {
            out_block[i].text = NULL;
        } else {
            out_block[i].text = *current;
            *current += out_block[i].count;
        }
    }

    return( out_block );
}

/* Function get_p_buffer().
 * Extract one or more contiguous P-buffers from in_file.
 *
 * Parameter:
 *     in_file points to the start of a P-buffer.
 *
 * Parameter is changed:
 *     in_file will point to the first non-P-buffer byte in the file on success.
 *     the status of in_file is unpredictable on failure.
 *
 * Returns:
 *     a pointer to a p_buffer containing the raw data on success.
 *     NULL on failure.
 */

p_buffer * get_p_buffer( FILE * in_file )
{
    char        *current    = NULL;
    uint8_t     i;
    p_buffer *  out_buffer  = NULL;
    uint8_t     p_count;
    char        test_char;

    /* Determine the number of contiguous P-buffers in the file. */

    p_count = 0;
    test_char = fgetc( in_file );
    if( ferror( in_file ) || feof( in_file ) ) return( out_buffer );
    while( test_char == 80 ) {
        ++p_count;
        fseek( in_file, 80, SEEK_CUR );
        if( ferror( in_file ) || feof( in_file ) ) return( out_buffer );
        test_char = fgetc( in_file );
        if( ferror( in_file ) || feof( in_file ) ) return( out_buffer );
    }

    /* There should always be at least one P-buffer. */

    if( p_count == 0 ) {
        puts( "No P-buffer found." );
        return( out_buffer );
    }

    /* Rewind the file by 81 bytes per P-buffer plus 1. */

    fseek( in_file, -1 * ((81 * p_count) + 1), SEEK_CUR );
    if( ferror( in_file ) || feof( in_file ) ) return( out_buffer );

    /* Allocate the out_buffer. */

    out_buffer = (p_buffer *) malloc( sizeof( p_buffer ) + 80 * p_count);
    if( out_buffer == NULL ) return( out_buffer );

    out_buffer->count = 80 * p_count;
    out_buffer->buffer = (char *)out_buffer + sizeof( p_buffer );
    current = out_buffer->buffer;

    /* Now get the data into the out_buffer. */

    for( i = 0; i < p_count; i++ ) {
        test_char = fgetc( in_file );
        if( ferror( in_file ) || feof( in_file ) ) {
            free( out_buffer );
            out_buffer = NULL;
            return( out_buffer );
        }

        if( test_char != 80 ) {
            puts( "P-buffer terminated prematurely." );
            free( out_buffer );
            out_buffer = NULL;
            return( out_buffer );
        }

        fread( current, 80, 1, in_file );
        if( ferror( in_file ) || feof( in_file ) ) {
            free( out_buffer );
            out_buffer = NULL;
            return( out_buffer );
        }

        current = current + 80;
    }

    return( out_buffer );
}

/*  Function parse_functions_block().
 *  Construct a functions_block containing the data in the P-buffer.
 *
 *  Parameter:
 *      current contains the first byte of the data to be processed.
 *      base points to the first byte of the underlying P-buffer.
 *      block points to a character string naming the block being parsed.
 *
 *  Parameter is changed:
 *      current will point to the first byte after the last byte parsed.
 *      on failure, the value of current will be meaningless.
 *
 *  Note:
 *      if not NULL, out_block->code_blocks is allocated separately from
 *      out_block and so must be freed separately as well.
 *
 *  Returns:
 *      a pointer to a functions_block containing the processed data on success.
 *      NULL on failure.
 */

functions_block * parse_functions_block( char **current, char *base, char * block )
{
    uint16_t            code_count;
    functions_block *   out_block   = NULL;

    /* Get the number of CodeBlocks. */

    memcpy_s( &code_count, 2, *current, 2 );
    *current += 2;

    /* Allocate the out_block. */

    out_block = (functions_block *) malloc( sizeof( functions_block ) );
    if( out_block == NULL) return( out_block );

    out_block->count = code_count;

    /* Now extract the CodeBlocks, if any. */

    if( out_block->count == 0 ) {
        out_block->code_blocks = NULL;
    } else {
        out_block->code_blocks = get_code_blocks( current, out_block->count, base, block );
    }

    return( out_block );
}

