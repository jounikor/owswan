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
* Description:  message processing utilities
*
****************************************************************************/


#include "ftnstd.h"
#include <ctype.h>
#include "wressetr.h"
#include "wresset2.h"
#include "wreslang.h"
#include "cioconst.h"
#include "wfc.rh"
#include "errutil.h"


#define MAX_SUBSTITUTABLE_ARGS  8

typedef union msg_arg {
    char                *s;
    int                 d;
    unsigned int        u;
    long                i;
} msg_arg;

static  HANDLE_INFO     hInstance = { 0 };
static  unsigned        MsgShift;

static bool LoadMsg( unsigned int msg, char *buffer, int buff_size )
{
    return( hInstance.status && ( WResLoadString( &hInstance, MSG_RC_BASE + msg + MsgShift, buffer, buff_size ) > 0 ) );
}

void ErrorInit( const char *pgm_name )
{
    hInstance.status = 0;
    if( OpenResFile( &hInstance, pgm_name ) ) {
        MsgShift = _WResLanguage() * MSG_LANG_SPACING;
        return;
    }
    CloseResFile( &hInstance );
}

void ErrorFini( void )
{
    CloseResFile( &hInstance );
}

static void OrderArgs( const char *msg, msg_arg *ordered_args, va_list args )
//===========================================================================
{
    char        chr;
    uint        idx;
    uint        arg_count;

    arg_count = 0;
    for(;;) {
        for(;;) {
            chr = *msg;
            if( chr == '%' )
                break;
            if( chr == NULLCHAR )
                return;
            ++msg;
        }
        ++arg_count;
        ++msg;                  // skip over '%'
        chr = *msg;
        if( isdigit( chr ) ) {  // only support a 1 digit field width
            ++msg;              // skip over width specifier
            chr = *msg;
        }
        ++msg;                  // skip over format specifier
        idx = *msg - '0' - 1;   // get positional information
        ++msg;
        if( arg_count > MAX_SUBSTITUTABLE_ARGS )
            continue;
        if( chr == 's' ) {
            ordered_args[idx].s = va_arg( args, char * );
        } else if( chr == 'd' ) {
            ordered_args[idx].d = va_arg( args, int );
        } else if( chr == 'u' ) {
            ordered_args[idx].u = va_arg( args, unsigned int );
        } else {   // assume -->  fflag == 'i'
            ordered_args[idx].i = va_arg( args, long );
        }
    }
}

static void Substitute( const char *msg, char *buffer, va_list args )
//===================================================================
// Do the necessary "%" substitutions.
{
    char        *subs_ptr;
    size_t      subs_len;
    size_t      width;
    char        chr;
    char        temp_buff[MAX_INT_SIZE];
    int         arg_count;
    int         idx;
    msg_arg     ordered_args[MAX_SUBSTITUTABLE_ARGS];
    bool        same_buff;

    same_buff = ( msg == buffer );
    OrderArgs( msg, ordered_args, args );
    arg_count = 0;
    for(;;) {
        for(;;) {
            chr = *msg;
            if( chr == '%' )
                break;
            if( chr == NULLCHAR )
                break;
            *buffer = chr;
            ++buffer;
            ++msg;
        }
        if( chr == NULLCHAR )
            break;
        ++arg_count;
        ++msg;                  // skip over '%'
        chr = *msg;
        width = 0;
        if( isdigit( chr ) ) {  // only support a 1 digit field width
            width = chr - '0';
            ++msg;              // skip over width specifier
            chr = *msg;
        }
        ++msg;                  // skip over format specifier
        idx = *msg - '0' - 1;   // get positional information
        ++msg;
        subs_ptr = temp_buff;
        if( arg_count > MAX_SUBSTITUTABLE_ARGS ) {
            temp_buff[0] = '?';
            temp_buff[1] = NULLCHAR;
        } else if( chr == 's' ) {
            subs_ptr = ordered_args[idx].s;
        } else if( chr == 'd' ) {
            sprintf( temp_buff, "%d", ordered_args[idx].d );
        } else if( chr == 'u' ) {
            sprintf( temp_buff, "%u", ordered_args[idx].u );
        } else {   // assume -->  fflag == 'i'
            sprintf( temp_buff, "%lu", ordered_args[idx].i );
        }
        subs_len = strlen( subs_ptr );
        if( width < subs_len ) {
            width = subs_len;
        }
        if( same_buff ) {
            memmove( buffer + width, msg, strlen( msg ) + sizeof( char ) );
            msg = buffer + width;
        }
        if( subs_len < width ) {
            memset( buffer, ( chr == 's' ) ? ' ' : '0', width - subs_len );
            buffer += width - subs_len;
        }
        memcpy( buffer, subs_ptr, subs_len );
        buffer += subs_len;
    }
    *buffer = NULLCHAR;
}

const char *GetMsg( unsigned msg )
{
    static char     msg_str[ERR_BUFF_SIZE];

    if( !LoadMsg( msg, msg_str, sizeof( msg_str ) ) ) {
        *msg_str = NULLCHAR;
    }
    return( msg_str );
}

void BldErrMsg( unsigned int msg, char *buffer, va_list args )
// Build error message.
{
    *buffer = NULLCHAR;
    if( LoadMsg( msg, &buffer[1], ERR_BUFF_SIZE - 1 ) ) {
        buffer[0] = ' ';
        Substitute( buffer, buffer, args );
    }
}

void    MsgFormat( const char *msg, char *buff, ... )
// Format a message.
{
    va_list     args;

    va_start( args, buff );
    Substitute( msg, buff, args );
    va_end( args );
}

void    MsgBuffer( uint msg, char *buff, ...  )
// Format message to buffer.
{
    va_list     args;

    va_start( args, buff );
    BldErrMsg( msg, buff, args );
    va_end( args );
}
