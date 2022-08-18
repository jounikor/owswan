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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <process.h>
#include "disasm.h"
#include "wdismsg.h"
#include "wressetr.h"
#include "wreslang.h"

#include "clibext.h"


static  HANDLE_INFO     hInstance = { 0 };
static  unsigned        MsgShift;

#define EXE_EXT         ".exe"

bool MsgInit( void )
{
    char        name[_MAX_PATH];

    hInstance.status = 0;
    if( _cmdname( name ) != NULL && OpenResFile( &hInstance, name ) ) {
        MsgShift = _WResLanguage() * MSG_LANG_SPACING;
        if( MsgGet( MSG_USE_BASE, name ) ) {
            return( true );
        }
    }
    CloseResFile( &hInstance );
    puts( NO_RES_MESSAGE );
    return( false );
}

bool MsgGet( int resourceid, char *buffer )
{
    if( hInstance.status == 0 || WResLoadString( &hInstance, resourceid + MsgShift, (lpstr)buffer, MAX_RESOURCE_SIZE ) <= 0 ) {
        buffer[0] = NULLCHAR;
        return( false );
    }
    return( true );
}

void MsgPutUsage( void )
{
    char        msg_buff[MAX_RESOURCE_SIZE];
    int         i;
    char        only_newline;
//    int         previous_null = 0;

    for( i = MSG_USE_BASE;; i++ ) {
        only_newline = 0;
        MsgGet( i, msg_buff );
        if( (msg_buff[0] == '~') && (msg_buff[1] == 0 ) ) {
            only_newline = 1;
        }
#if 0
This section of the code is killed for now.  If, sometime in the future,
there are multiple pages of usage messages for this program, include the code
and add a waitforkey() function.
        if( previous_null ) {
            if( msg_buff[0] != '\0' ) {
                waitforkey();
                if( only_newline ) {
                    puts( "" );
                } else {
                    puts( msg_buff );
                }
                previous_null = 0;
            } else break;

        } else if( msg_buff[0] == '\0' ) {
            previous_null = 1;
        } else {
#else
        if( msg_buff[ 0 ] == 0 ) {
            break;
        }
#endif
            if( only_newline ) {
                puts( "" );
            } else {
                puts( msg_buff );
            }
//        }
    }
}

void MsgFini( void )
{
    CloseResFile( &hInstance );
}

void MsgSubStr( char *strptr, char *para, char specifier )
{
    char        buff[MAX_RESOURCE_SIZE];
    char        *src;
    int         i;

    src = strptr;
    for( i = 0; *src != NULLCHAR; src++ ) {
        if( i == MAX_RESOURCE_SIZE - 1 ) break;
        if( *src != '%' ) {
            buff[i++] = *src;
        } else {
            if( *(src+1) == specifier ) {
                while( *para != NULLCHAR ) {
                    buff[i++] = *para++;
                    if( i == MAX_RESOURCE_SIZE - 1 ) break;
                }
                src++;
            } else if( specifier == '1' ) { /*keep '%' for next MsgSubStr*/
                buff[i++] = *src;
                buff[i++] = *(++src);
            } else if( *(src+1) == '%' ) {
                buff[i++] = *(++src);
            }
        }
    }
    buff[i] = NULLCHAR;
    strcpy( strptr, buff );
}

void MsgChgeSpec( char *strptr, char specifier )
{
    int         i;

    for( i = 0; strptr[i] != NULLCHAR; i++ ) {
        if( strptr[i] == '%' && strptr[i+1] == specifier ) {
            strptr[i+1] = 's';
            break;
        }
    }
}
