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


#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <process.h>
#include <sys/types.h>
#include <sys/magic.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <signal.h>
#include "dbgdefn.h"
#include "strutil.h"
#include "dbgmain.h"
#include "dbginit.h"
#include "dbgcmdln.h"
#include "owqnx.h"
#include "dbgscrn.h"
#ifndef __NOUI__
#include "aui.h"
#endif

#include "clibint.h"


extern char     **_argv;
extern int      _argc;
extern void     __sigabort();

unsigned char   _8087 = 0;
unsigned char   _real87 = 0;

static char             *cmdStart;
static volatile bool    BrkPending;
static unsigned         NumArgs;

/* following are to stop the C library from hauling in stuff we don't want */
void (*__abort)();
void __sigabort() {}

#ifndef __NOUI__

static void BrkHandler( int signo )
{
    signo = signo;
    BrkPending = true;
}

void GUImain( void )
{
    cmdStart = _argv[1];
    NumArgs = _argc - 1;

    /*
       This is so that the debugger can be made set UID root to get ring 1 access
       for the parallel trap file, without being a security hole.
    */
    setegid( getgid() );
    seteuid( getuid() );
    signal( SIGINT, &BrkHandler );
    DebugMain();
}


bool GUISysInit( init_mode install )
{
    /* unused parameters */ (void)install;

    return( true );
}

void GUISysFini( void  )
{
    DebugFini();
}

void WndCleanUp( void )
{
}

#endif

char *GetCmdArg( int num )
{
    char    *cmd;

    if( num >= NumArgs || cmdStart == NULL )
        return( NULL );
    for( cmd = cmdStart; num != 0; --num ) {
        cmd += strlen( cmd ) + 1;
    }
    return( cmd );
}

void SetCmdArgStart( int num, char *ptr )
{
    NumArgs -= num;
    if( ptr != NULL && *ptr == NULLCHAR )
        ++ptr;
    cmdStart = ptr;
}

void KillDebugger( int rc )
{
    /* unused parameters */ (void)rc;

    __qnx_exit( 0 );
    // never return
}

void GrabHandlers( void )
{
}

void RestoreHandlers( void )
{
}

bool TBreak( void )
{
    bool    ret;

    ret = BrkPending;
    BrkPending = false;
    return( ret );
}

long _fork( const char *cmd, size_t len )
{
    char        buff[256];
    char        *argv[4];
    const char  *shell;
    pid_t       pid;
    int         i;
    char        iov[10];

    shell = getenv( "SHELL" );
    if( shell == NULL )
        shell = "/bin/sh";

    argv[0] = (char *)shell;
    if( len != 0 ) {
        argv[1] = "-c";
        memcpy( buff, cmd, len );
        buff[len] = NULLCHAR;
        argv[2] = buff;
        argv[3] = NULL;
    } else {
        argv[1] = NULL;
    }
    /* make sure STDIN/STDOUT/STDERR are connected to a terminal */
    iov[0] = DbgConHandle;
    iov[1] = DbgConHandle;
    iov[2] = DbgConHandle;
    for( i = 3; i < 10; ++i )
        iov[i] = '\xFF';
    fcntl( DbgConHandle, F_SETFD, 0 );
    pid = qnx_spawn( 0, 0, 0, -1, -1,
                _SPAWN_NEWPGRP | _SPAWN_TCSETPGRP | _SPAWN_SETSID,
                shell, argv, environ, iov, DbgConHandle );
    fcntl( DbgConHandle, F_SETFD, FD_CLOEXEC );
    if( pid == -1 )
        return( 0xffff0000 | errno );
    do {
    } while( waitpid( pid, NULL, 0 ) == -1 && errno == EINTR );
    return( 0 );
}
