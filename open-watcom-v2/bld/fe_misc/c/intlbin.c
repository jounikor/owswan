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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "lsspec.h"
#include "encodlng.h"
#include "pathgrp2.h"

#include "clibext.h"


char const * const componentName[] = {
    #define LS_DEF( name, sig ) #name ,
    LS_DEFS
    #undef LS_DEF
};

static void fatal( char *m ) {
    printf( "fatal: %s\n", m );
    exit( EXIT_FAILURE );
}

static void copyAligned( FILE *ofp, FILE *ifp ) {
    unsigned n;
    int c;

    n = 0;
    for(;;) {
        c = fgetc( ifp );
        if( c == EOF ) break;
        ++n;
        fputc( c, ofp );
    }
    while( n & 3 ) {
        fputc( 0, ofp );
        ++n;
    }
}

static void processLang( char *prefix, unsigned lang ) {
    unsigned    curr_offset;
    FILE        *fp;
    FILE        *ifp;
    unsigned    component;
    fpos_t      header_posn;
    LocaleData  header;
    pgroup2     pg;
    char        fname[PATH_MAX];
    char        name[9];

    _splitpath2( prefix, pg.buffer, &pg.drive, &pg.dir, &pg.fname, NULL );
    sprintf( name, "%.6s%02u", pg.fname, lang );
    _makepath( fname, pg.drive, pg.dir, name, LOCALE_DATA_EXT );
    fp = fopen( fname, "wb" );
    if( !fp ) {
        fatal( "cannot open output file" );
    }
    memset( &header, 0, sizeof( header ) );
    strcpy( header.text_header, LSF_TEXT_HEADER );
    header.signature = !LSF_SIGNATURE;
    header.major = LSF_MAJOR;
    header.minor = LSF_MINOR;
    header.units = ( LS_MAX - LS_MIN );
    fgetpos( fp, &header_posn );
    fwrite( &header, sizeof( header ), 1, fp );
    curr_offset = 0;
    for( component = LS_MIN; component < LS_MAX; ++component ) {
        header.offset[ curr_offset++ ] = ftell( fp );
        sprintf( name, "%.6s%02u", componentName[component], lang );
        name[0] = (char)tolower(name[0]);
        _makepath( fname, NULL, NULL, name, LOCALE_DATA_EXT );
        ifp = fopen( fname, "rb" );
        if( !ifp ) {
            fatal( "cannot open output file" );
        }
        copyAligned( fp, ifp );
        fclose( ifp );
    }
    header.offset[ curr_offset ] = ftell( fp );
    fsetpos( fp, &header_posn );
    header.signature = LSF_SIGNATURE;
    fwrite( &header, sizeof( header ), 1, fp );
    fclose( fp );
}

int main( int argc, char **argv )
{
    unsigned lang;

    if( argc != 2 ) {
        fatal( "usage: intlbin <prefix>" );
    }
    for( lang = LANG_FIRST_INTERNATIONAL; lang < LANG_MAX; ++lang ) {
        processLang( argv[1], lang );
    }
    return( EXIT_SUCCESS );
}
