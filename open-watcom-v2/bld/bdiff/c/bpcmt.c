/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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


#include "bdiff.h"
#include "pathgrp2.h"

#include "clibext.h"


#define MXFNAME         130
#define BUFSIZE         0x4000

static char *Buffer;
static char UsageText[] = {
    "Usage: bpcmt comment_file patch_file\0"
};

static char TmpExt[] = "A";

static void Fatal( char *reason, char *insert )
/* the reason doesn't have to be good */
{
    for( ; *reason; ++reason ) {
        if( *reason == '*' ) {
            fputs( insert, stdout );
        } else {
            fputc( *reason, stdout );
        }
    }
    puts( "\nbpcmt aborted" );
    exit( EXIT_FAILURE );
}

static void Usage( void )
{
    const char *p;

    for( p = UsageText; *p != '\0'; ) {
        puts( p );
        while( *p++ != '\0' ) ;
    }
    exit( EXIT_FAILURE );
}

int main( int argc, char *argv[] )
{
    size_t      size, bufsize;
    char        outfile[_MAX_PATH], infile[_MAX_PATH];
    pgroup2     pg;
    FILE        *fpin, *fpout, *fpcmt;
    char        *pos;


    if( argc != 3 )
        Usage();
    if( argv[1][0] == '?' && argv[1][1] == '\0' )
        Usage();
    bufsize = BUFSIZE;
    while( ( Buffer = bdiff_malloc( bufsize ) ) == NULL ) {
        size = bufsize & (bufsize - 1);
        bufsize = size ? size : ( (bufsize << 1) | (bufsize << 2) );
        if( bufsize < MXFNAME ) {
            Fatal( "Too low on memory", NULL  );
        }
    }

    fpcmt = fopen( argv[1], "rb" );
    if( fpcmt == NULL ) {
        Fatal( "Unable to open '*' to read", argv[2] );
    }

    strcpy( infile, argv[2] );
    _splitpath2( infile, pg.buffer, &pg.drive, &pg.dir, &pg.fname, &pg.ext );
    for( ;; ) {
        _makepath( outfile, pg.drive, pg.dir, "__", TmpExt );
        if( access( outfile, 0 ) != 0 )
            break;
        TmpExt[0]++;
        if( TmpExt[0] > 'Z' ) {
            Fatal( "Cannot create temporary file", NULL );
        }
    }

    /* initialize input file */
    fpin = fopen( infile, "rb" );
    if( fpin == NULL ) {
        Fatal( "Unable to open '*' to read", infile );
    }

    /* initialize output file */
    fpout = fopen( outfile, "wb" );
    if( fpout == NULL ) {
        Fatal( "Unable to create output file '*'", outfile );
    }
    /* write new comment */
    fwrite( PATCH_SIGNATURE, 1, sizeof( PATCH_SIGNATURE ) - 1, fpout );
    for( ;; ) {
        size = fread( Buffer, 1, bufsize, fpcmt );
        if( size == 0 )
            break;
        pos = memchr( Buffer, EOF_CHAR, size );
        if( pos != NULL ) {
            size = pos - Buffer;
            fseek( fpcmt, 0L, SEEK_END ); /*cause it to quit next time round*/
        }
        fwrite( Buffer, 1, size, fpout );
    }
    /* strip old comment */
    for( ;; ) {
        size = fread( Buffer, 1, bufsize, fpin );
        pos = memchr( Buffer, EOF_CHAR, size );
        if( pos != NULL ) {
            break;
        }
    }
    size -= pos - Buffer;
    if( size != 0 )
        fwrite( pos, 1, size, fpout );
    /* transfer patch file */
    while( (size = fread( Buffer, 1, bufsize, fpin )) != 0 ) {
        fwrite( Buffer, 1, size, fpout );
    }
    fclose( fpin );
    fclose( fpout );
    fclose( fpcmt );

    if( remove( infile ) )
        Fatal( "Cannot erase file '*'", infile );
    if( rename( outfile, infile ) )
        Fatal( "Cannot rename file '*'", outfile );
    return( EXIT_SUCCESS );
}
