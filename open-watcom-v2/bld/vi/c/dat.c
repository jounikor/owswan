/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "vi.h"
#include "specio.h"

/*
 * ReadDataFile - do just that
 */
#ifdef VICOMP
vi_rc ReadDataFile( const char *file, char **buffer, bool (*fn_alloc)(int), bool (*fn_save)(int, const char *) )
#else
vi_rc ReadDataFile( const char *file, char **buffer, bool (*fn_alloc)(int), bool (*fn_save)(int, const char *), bool bounddata )
#endif
{
    GENERIC_FILE        gf;
    int                 i, dcnt;
    size_t              size;
    size_t              len;
    char                token[MAX_STR], buff[MAX_STR];
    char                *buffdata;
    bool                hasvals;
    const char          *ptr;

    /*
     * get file and buffer
     */
#ifdef VICOMP
    if( !SpecialOpen( file, &gf ) ) {
#else
    if( !SpecialOpen( file, &gf, bounddata ) ) {
#endif
        return( ERR_FILE_NOT_FOUND );
    }

    /*
     * get counts
     */
    if( (ptr = SpecialFgets( buff, sizeof( buff ) - 1, &gf )) == NULL ) {
        SpecialFclose( &gf );
        return( ERR_INVALID_DATA_FILE );
    }
    dcnt = atoi( ptr );
    hasvals = fn_alloc( dcnt );

    /*
     * read all tokens
     *
     * create list of tokens separated by '\0'
     * list is terminated by two '\0' characters
     */
    buffdata = NULL;
    size = 0;
    for( i = 0; i < dcnt; i++ ) {
        if( (ptr = SpecialFgets( buff, sizeof( buff ) - 1, &gf )) == NULL ) {
            /* error */
            break;
        }
        if( hasvals ) {
            ptr = GetNextWord1( ptr, token );
            if( *token == '\0' ) {
                /* error */
                break;
            }
        } else {
            strcpy( token, ptr );
        }
        // add space for token terminator
        len = strlen( token ) + 1;
        // add space for list terminator
        buffdata = MemRealloc( buffdata, size + len + 1 );
        // copy token with terminator
        memcpy( buffdata + size, token, len );
        size += len;
        // write list terminator
        buffdata[size] = '\0';
        if( hasvals ) {
            ptr = GetNextWord1( ptr, token );
            if( *token == '\0' ) {
                /* error */
                break;
            }
            fn_save( i, token );
        }
    }
    SpecialFclose( &gf );
    if( i < dcnt ) {
        if( buffdata != NULL )
            MemFree( buffdata );
        return( ERR_INVALID_DATA_FILE );
    }
    *buffer = buffdata;
    return( ERR_NO_ERR );

} /* ReadDataFile */
