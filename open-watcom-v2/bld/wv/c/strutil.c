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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgio.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"


extern address GetRealSeg( address );

/*
 * StrCopy -- copy string
 */

char *StrCopy( char const *src, char *dest )
{

    if( src == NULL || dest == NULL )
        return( NULL );
    while( (*dest = *src) != NULLCHAR ) {
        ++src;
        ++dest;
    }
    return( dest );
}


char *StrTrim( char *str )
{
    char        *p;
    size_t      len;

    len = strlen( str );
    if( len == 0 )
        return( str );
    p = str + len - 1;
    while( *p == ' ' ) {
        --p;
    }
    *++p = NULLCHAR;
    return( p );
}

/*
 * FmtStr -- gut level formatter
 */

char *FmtStr( char *buff, const char *fmt, va_list args )
{
    char                *ptr;
    unsigned            len;
    long                val;
    address             addr;
    char                *res;
    sym_handle          *sym;
    error_handle        errh;
    char                save_buff[UTIL_LEN + 1];

    while( *fmt != NULLCHAR ) {
        if( *fmt != '%' ) {
            *buff = *fmt;
            ++buff;
        } else {
            ++fmt;
            switch( *fmt ) {
            case 'c':
                val = va_arg( args, int );
                *buff++ = val;
                break;
            case 's':
                ptr = va_arg( args, char * );
                buff = StrCopy( ptr, buff );
                break;
            case 't':
                ptr = va_arg( args, char * );
                len = va_arg( args, unsigned );
                memcpy( buff, ptr, len );
                buff += len;
                break;
            case 'l':
                addr = va_arg( args, address );
                buff = StrAddr( &addr, buff, TXT_LEN ); // nyi - overflow?
                res = LineAddr( &addr, save_buff, sizeof( save_buff ) );
                if( res != NULL ) {
                    *buff++ = '(';
                    buff = StrCopy( save_buff, buff );
                    *buff++ = ')';
                }
                break;
            case 'A':
                addr = va_arg( args, address );
                buff = UniqStrAddr( &addr, buff, TXT_LEN ); // nyi - overflow?
                break;
            case 'a':
                addr = va_arg( args, address );
                buff = StrAddr( &addr, buff, TXT_LEN ); // nyi - overflow?
                break;
            case 'p':
                addr = va_arg( args, address );
                buff = AddrToIOString( &addr, buff, TXT_LEN ); // nyi - overflow?
                break;
            case 'e':
                errh = va_arg( args, error_handle );
                buff = SysErrMsg( errh, buff );
                break;
            case 'U':
                buff = CnvULongDec( va_arg( args, unsigned long), buff, TXT_LEN );
                break;
            case 'd':
                val = va_arg( args, int );
                buff = CnvLongDec( val, buff, TXT_LEN );
                break;
            case 'u':
                val = va_arg( args, unsigned );
                buff = CnvULongDec( val, buff, TXT_LEN );
                break;
            case '%':
                *buff++ = '%';
                break;
            case 'o':
                val = va_arg( args, long );
                if( val < 0 ) {
                    *buff++ = '-';
                    val = -val;
                } else {
                    *buff++ = '+';
                }
                buff = AddHexSpec( buff );
                buff = CnvULongHex( val, buff, TXT_LEN );
                break;
            case 'S':
                sym = va_arg( args, sym_handle * );
                buff += DIPSymName( sym, NULL, SNT_SOURCE, buff, TXT_LEN );
                break;
            }
        }
        ++fmt;
    }
    *buff = NULLCHAR;
    return( buff );
}


/*
 * Format -- format up a string
 */

char *Format( char *buff, const char *fmt, ... )
{
    va_list args;
    char    *p;

    va_start( args, fmt );
    p = FmtStr( buff, fmt, args );
    va_end( args );
    return( p );
}

char *DupStrLen( const char *str, size_t len )
{
    char        *dup;

    dup = DbgAlloc( len + 1 );
    if( dup != NULL ) {
        memcpy( dup, str, len );
        dup[len] = NULLCHAR;
    }
    return( dup );
}


char *DupStr( const char *str )
{
    return( DupStrLen( str, strlen( str ) ) );
}
