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
#include <stdarg.h>
#include "defgen.h"
#include "semantic.h"


char    *CurFile;
char    *CurLine;

void SemLine( char *fname, char *lineno ) {
//    DebugOut( "\nFile %s line %s", fname, lineno );
    if( CurFile != NULL ) free( CurFile );
    CurFile = fname;
    if( CurLine != NULL ) free( CurLine );
    CurLine = lineno;
}

void ReportError( const char *msg, ... ) {
    va_list     al;

    ErrorHasOccured = true;
    if( CurFile != NULL && CurLine != NULL ) {
        printf( "%s(%s): ", CurFile, CurLine, msg );
    }
    va_start( al, msg );
    vprintf( msg, al );
    printf( "\n" );
    va_end( al );
}

void ReportWarning( const char *msg, ... ) {
    va_list     al;

    if( CurFile != NULL && CurLine != NULL ) {
        printf( "%s(%s): ", CurFile, CurLine, msg );
    }
    va_start( al, msg );
    vprintf( msg, al );
    printf( "\n" );
    va_end( al );
}

void DebugOut( const char *msg, ... ) {
    va_list     al;

    va_start( al, msg );
    vprintf( msg, al );
    printf( "\n" );
    va_end( al );
}
