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
* Description:  A variety of printf-like functions for vi
*
****************************************************************************/


#include "vi.h"
#include "myprtf.h"


void Lead( char c, int num, char *buff )
// With "c", to "num" bytes, put leading bytes in "buff"
// This is a service for basePrintf() which is made available externally.
{
    int len, diff, i;

    len = (int)strlen( buff );
    diff = num - len;
    if( diff <= 0 ) {
        return;
    }
    for( i = len; i >= 0; i-- ) {
        buff[diff + i] = buff[i];
    }
    for( i = 0; i < diff; i++ ) {
        buff[i] = c;
    }
}

// barfChar() - called from basePrintf() - writes to (cFile) ? cFile : cStr
static FILE *cFile;
static char *cStr;

static void barfChar( char ch )
{
    if( cFile == NULL ) {
        *cStr++ = ch;
    } else {
        fputc( ch, cFile );
    }
}

static void basePrintf( const char *in, va_list args )
// With format "in" and argument list "al", use barfChar() to write an output
// Flags, minimum field width, precision, and length modifiers are unsupported.
// conversion specifiers are a superset of a subset of those printf supports.
// Unsupported specifiers are quietly ignored.
{
    char        cin;
    int         i, j;
    long        l;
    char        buff[MAX_STR], *tmp;

    while( (cin = *in) != '\0' ) {
        if( cin == '%' ) {
            in++;
            switch( *in ) {
            case '%':
                barfChar( '%' );
                break;
            case 'c':
                barfChar( (char)va_arg( args, int ) );
                break;
            case 'u':
                i = va_arg( args, int );
                sprintf( buff, "%u", (unsigned)i );
                goto copyloop1;
            case 'd':
                i = va_arg( args, int );
                sprintf( buff, "%d", (unsigned)i );
                goto copyloop1;
            case 'U':
                l = va_arg( args, long );
                sprintf( buff, "%lu", (unsigned long)l );
                goto copyloop1;
            case 'l':
                l = va_arg( args, long );
                sprintf( buff, "%ld", (long)l );
                goto copyloop1;
            case 's':
                tmp = va_arg( args, char * );
                goto copyloop2;
#ifndef NDEBUG
            case 'W':
#ifdef _M_I86
                i = va_arg( args, int );
                j = va_arg( args, int );
                sprintf( buff, "%04x:%04x", j, i );
#else
                i = va_arg( args, int );
                sprintf( buff, "%08x", i );
#endif
                goto copyloop1;
#endif
            case 'Z':   /* %02x */
                i = va_arg( args, int );
                sprintf( buff, "%02x", i );
                goto copyloop1;
            case 'D':   /* %02d */
                i = va_arg( args, int );
                sprintf( buff, "%02d", i );
                goto copyloop1;
            case 'L':   /* %8ld */
                l = va_arg( args, long );
                sprintf( buff, "%8ld", l );
                j = 8;
                goto copyloop;
            case 'M':   /* %5ld */
                l = va_arg( args, long );
                sprintf( buff, "%5ld", l );
                j = 5;
                goto copyloop;
            case 'N':   /* %6ld */
                l = va_arg( args, long );
                sprintf( buff, "%6ld", l );
                j = 6;
                goto copyloop;
            case 'O':   /* %6d */
                i = va_arg( args, int );
                sprintf( buff, "%6d", i );
                j = 6;
                goto copyloop;
            case 'S':   /* %-12s */
                j = 12;
                goto copyloopa;
            case 'X':   /* %-15s */
                j = 15;
                goto copyloopa;
            case 'Y':   /* %-32s */
                j = 32;
copyloopa:
                tmp = va_arg( args, char * );
                strcpy( buff, tmp );
                {
                    int k;

                    l = (long)strlen( buff );
                    k = j - l;
                    if( k > 0 ) {
                        tmp = &buff[l];
                        for( i = 0; i < k; i++ ) {
                            tmp[i] = ' ';
                        }
                        tmp[k] = '\0';
                    }
                }
                goto copyloop1;
copyloop:
                Lead( ' ', j, buff );
copyloop1:
                tmp = buff;
copyloop2:
                while( *tmp != '\0' ) {
                    barfChar( *tmp++ );
                }
                break;
            }
        } else {
            barfChar( cin );
        }
        in++;
    }
    if( cFile == NULL ) {
        *cStr++ = '\0';
    }
}

void MyPrintf( const char *str, ... )
{
    va_list     args;

#ifdef __WIN__
    char        tmp[MAX_STR];

    va_start( args, str );
    cFile = NULL;
    cStr = tmp;
    basePrintf( str, args );
    MessageBox( NO_WINDOW, tmp, EditorName, MB_OK | MB_TASKMODAL );
#else
    va_start( args, str );
    cFile = stdout;
    cStr = NULL;
    basePrintf( str, args );
#endif
    va_end( args );
}

void MySprintf( char *out, const char *str, ... )
// sprintf++ functionality
{
    va_list     args;

    va_start( args, str );
    cFile = NULL;
    cStr = out;
    basePrintf( str, args );
    va_end( args );
}

void MyVSprintf( char *out, const char *str, va_list args )
// vsprintf++ functionality
{
    cFile = NULL;
    cStr = out;
    basePrintf( str, args );
}

void MyVPrintf( const char *str, va_list args )
// vprintf++ functionality
{
    cFile = stdout;
    basePrintf( str, args );
}

void MyFprintf( FILE *fp, const char *str, ... )
// vfprintf++ functionality
{
    va_list args;

    va_start( args, str );
    cFile = fp;
    basePrintf( str, args );
    va_end( args );
}
