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
* Description:  LINEPRT.C - line print routines
*
****************************************************************************/

/*
   LINEPRT.C - line print routines

   Date         By              Reason
   ====         ==              ======
   11-apr-90    C.G.Eisler      defined
   25-jul-90    C.G.Eisler      more work
   19-oct-91    C.G.Eisler      cleaned up
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "lineprt.h"
#include "bool.h"
#include "misc.h"


static bool     _dropped = true;
static size_t   _lastlinelen = 0;
//                           12345678 1 2345678 2 2345678 3 2345678 4 2345678 5 2345678 6 23456
static char     _spaces[] = "                                                                  ";
char            *buffer;

/* PrintALine - printf text on the current line */
void PrintALine( const char *str, ... )
{
    va_list     args;
    char        bob[256];
    size_t      len;
    size_t      i = 0;

    va_start( args, str );
    vsprintf( bob, str, args );
    va_end( args );
    len = strlen( bob );
    if( _lastlinelen > len ) {
        i = _lastlinelen - len;
    }
    _spaces[i] = 0;
    printf( "%c%s%s", '\r', bob, _spaces );
    _spaces[i] = ' ';
    _lastlinelen = len;
    _dropped = false;
}

/* PrintALineThenDrop - printf text on the current line, then drop */
void PrintALineThenDrop( const char *str, ... )
{
    va_list     args;
    char        bob[256];
    size_t      len;
    size_t      i = 0;

    va_start( args, str );
    vsprintf( bob, str, args );
    va_end( args );
    len = strlen( bob );
    if( _lastlinelen > len ) {
        i = _lastlinelen - len;
    }
    _spaces[i] = 0;
    printf( "%c%s%s\n", '\r', bob, _spaces );
    _spaces[i] = ' ';
    _lastlinelen = 0;
    _dropped = true;
}

/* DropPrintALine - printf text on the next line */
void DropPrintALine( const char *str, ... )
{
    va_list     args;
    char        bob[256];

    va_start( args, str );
    vsprintf( bob, str, args );
    va_end( args );
    _lastlinelen = strlen( bob );
    if( !_dropped )
        printf( "\n" );
    printf( "%s", bob );
    _dropped = false;
}

/* DropALineDammit - drop down one line, for sure */
void DropALineDammit( void )
{
    _dropped = true;
    _lastlinelen = 0;
    printf( "\n" );
}

/* DropALine - drop down one line */
void DropALine( void )
{
    if( _dropped )
        return;
    DropALineDammit();
}

/* StartPrint - allocate buffer for unbufferd io */
void StartPrint( void )
{

    buffer = MemAlloc( BUFSIZ );
    setvbuf( stdout, buffer, _IONBF, BUFSIZ );
}

/* EndPrint - deallocate buffer for unbufferd io */
void EndPrint( void )
{
    MemFree( buffer );
}
