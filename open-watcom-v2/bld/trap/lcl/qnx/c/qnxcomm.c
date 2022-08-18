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


#include <stdlib.h>
#include <unistd.h>
#include <process.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/proxy.h>
#include <sys/kernel.h>
#include <sys/debug.h>
#include <sys/stat.h>
#include <sys/proc_msg.h>
#include <sys/osinfo.h>
#include <sys/psinfo.h>
#include <sys/seginfo.h>
#include <sys/sched.h>
#include <sys/vc.h>
#include <sys/magic.h>
#include <sys/wait.h>
#include <sys/dumper.h>
#include <sys/console.h>
#include <sys/dev.h>
#include "trpimp.h"
#include "trpcomm.h"
#include "qnxcomm.h"


char *StrCopy( const char *src, char *dst )
{
    while( *dst = *src ) {
        ++src;
        ++dst;
    }
    return( dst );
}

char *CollectNid( char *ptr, unsigned len, nid_t *nidp )
{
    char        *start;
    nid_t       nid;
    char        ch;

    *nidp = 0;
    start = ptr;
    if( ptr[0] != '/' || ptr[1] != '/' ) return( ptr );
    len -= 2;
    ptr += 2;
    nid = 0;
    //NYI: will need beefing up when NID's can be symbolic
    for( ;; ) {
        if( len == 0 ) break;
        ch = *ptr;
        if( ch < '0' || ch > '9' ) break;
        nid = (nid * 10) + ( ch - '0' );
        ++ptr;
        --len;
    }
    *nidp = nid;
    //NYI: how do I check to see if NID is valid?
    if( len == 0 ) return( ptr );
    switch( ptr[0] ) {
    case ' ':
    case '\t':
    case '\0':
        break;
    default:
        *nidp = 0;
        return( start );
    }
    return( ptr );
}

unsigned TryOnePath( const char *path, struct stat *tmp, const char *name, char *result )
{
    char        *end;
    char        *ptr;

    if( path == NULL )
        return( 0 );
    ptr = result;
    for( ;; ) {
        switch( *path ) {
        case ':':
        case '\0':
            if( ptr != result && ptr[-1] != '/' )
                *ptr++ = '/';
            end = StrCopy( name, ptr );
            //NYI: really should check permission bits
            if( stat( result, tmp ) == 0 )
                return( end - result );
            if( *path == '\0' )
                return( 0 );
            ptr = result;
            break;
        case ' ':
        case '\t':
            break;
        default:
            *ptr++ = *path;
            break;
        }
        ++path;
    }
}

unsigned FindFilePath( bool exe, const char *name, char *result )
{
    struct stat tmp;
    unsigned    len;
    char        *end;

    if( stat( (char *)name, &tmp ) == 0 ) {
        end = StrCopy( name, result );
        return( end - result );
    }
    if( exe ) {
        return( TryOnePath( getenv( "PATH" ), &tmp, name, result ) );
    } else {
        len = TryOnePath( getenv( "WD_PATH" ), &tmp, name, result );
        if( len != 0 ) return( len );
        len = TryOnePath( getenv( "HOME" ), &tmp, name, result );
        if( len != 0 ) return( len );
        return( TryOnePath( "/usr/watcom/wd", &tmp, name, result ) );
    }
}


trap_retval TRAP_CORE( Read_user_keyboard )( void )
{
    struct _console_ctrl    *con;
    unsigned                con_num;
    int                     con_hdl;
    int                     con_mode;
    char                    chr;
    //NYI: what about QNX windows?
    static char             con_name[] = "/dev/conXX";
    unsigned                timeout;
    read_user_keyboard_req      *acc;
    read_user_keyboard_ret      *ret;

#   define FIRST_DIGIT (sizeof( con_name ) - 3)

    acc = GetInPtr( 0 );
    ret = GetOutPtr( 0 );
    timeout = acc->wait * 10;
    if( timeout == 0 ) timeout = -1;
    ret->key = '\0';
    con = console_open( 2, O_WRONLY );
    if( con == NULL ) {
        return( sizeof( *ret ) );
    }
    con_num = console_active( con, -1 );
    console_close( con );
    con_name[ FIRST_DIGIT + 0 ] = (con_num / 10) + '0';
    con_name[ FIRST_DIGIT + 1 ] = (con_num % 10) + '0';

    con_hdl = open( con_name, O_RDONLY );
    if( con_hdl < 0 ) {
        if( timeout == -1 ) timeout = 50;
        sleep( timeout / 10 );
        return( sizeof( *ret ) );
    }
    con_mode = dev_mode( con_hdl, 0, _DEV_MODES );
    if( dev_read( con_hdl, &chr, 1, 1, 0, timeout, 0, 0 ) == 1 ) {
        if( chr == '\xff' ) {
            read( con_hdl, &chr, 1 );
            chr = '\0';
        }
        ret->key = chr;
    }
    dev_mode( con_hdl, con_mode, _DEV_MODES );
    close( con_hdl );
    return( sizeof( *ret ) );
}


trap_retval TRAP_CORE( Get_err_text )( void )
{
    get_err_text_req    *acc;
    char                *err_txt;

    acc = GetInPtr( 0 );
    err_txt = GetOutPtr( 0 );
    strcpy( err_txt, strerror( acc->err ) );
    return( strlen( err_txt ) + 1 );
}


trap_retval TRAP_CORE( Split_cmd )( void )
{
    char                ch;
    char                *cmd;
    char                *start;
    split_cmd_ret       *ret;
    unsigned            len;
    nid_t               nid;

    cmd = GetInPtr( sizeof( split_cmd_req ) );
    len = GetTotalSizeIn() - sizeof( split_cmd_req );
    start = cmd;
    ret = GetOutPtr( 0 );
    ret->parm_start = 0;
    cmd = CollectNid( cmd, len, &nid );
    len -= cmd - start;
    while( len != 0 ) {
        ch = *cmd;
        if( !( ch == '\0' || ch == ' ' || ch == '\t' ) ) break;
        ++cmd;
        --len;
    }
    while( len != 0 ) {
        switch( *cmd ) {
        case '\0':
        case ' ':
        case '\t':
            ret->parm_start = 1;
            len = 0;
            continue;
        }
        ++cmd;
    }
    ret->parm_start += cmd - start;
    ret->cmd_end = cmd - start;
    return( sizeof( *ret ) );
}

trap_retval TRAP_CORE( Get_next_alias )( void )
{
    get_next_alias_ret  *ret;

    ret = GetOutPtr( 0 );
    ret->seg = 0;
    ret->alias = 0;
    return( sizeof( *ret ) );
}

trap_retval TRAP_CORE( Set_user_screen )( void )
{
    return( 0 );
}

trap_retval TRAP_CORE( Set_debug_screen )( void )
{
    return( 0 );
}
