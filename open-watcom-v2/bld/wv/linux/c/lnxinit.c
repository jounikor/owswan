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
* Description:  Linux initialization support routines for wd
*
****************************************************************************/


#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "dbgdefn.h"
#include "strutil.h"
#include "dbgmain.h"
#include "dbginit.h"
#include "dbgcmdln.h"
#include "dbgscrn.h"
#ifndef __NOUI__
#include "aui.h"
#endif

#ifdef __WATCOMC__
#include "clibint.h"
#endif


#ifdef __WATCOMC__
unsigned char   _8087 = 0;
unsigned char   _real87 = 0;
#endif

extern char     **_argv;
extern int      _argc;

#ifndef __WATCOMC__
extern char     **environ;
#endif

static char             *cmdStart;
static volatile bool    BrkPending;
static unsigned         NumArgs;

extern  void    (*__abort)( void );
extern  void    __sigabort( void );

/* following are to stop the C library from hauling in stuff we don't want */
void (*__abort)(void);
void __sigabort( void ) {}

#ifndef __NOUI__

static void BrkHandler( int signo )
{
    /* unused parameters */ (void)signo;

    BrkPending = true;
}

void GUImain( void )
{
    struct sigaction sa;

    cmdStart = _argv[1];
    NumArgs = _argc - 1;

    /*
       This is so that the debugger can be made set UID root to get ring 1 access
       for the parallel trap file, without being a security hole.
    */
    setegid( getgid() );
    seteuid( getuid() );
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = BrkHandler;
    sigemptyset( &sa.sa_mask );
    sigaction( SIGINT, &sa, NULL );
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

    exit( 0 );
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
    const char  *argv[4];
    const char  *shell;
    pid_t       pid;

    shell = getenv( "SHELL" );
    if( shell == NULL )
        shell = "/bin/sh";

    argv[0] = shell;
    if( len != 0 ) {
        argv[1] = "-c";
        memcpy( buff, cmd, len );
        buff[len] = NULLCHAR;
        argv[2] = buff;
        argv[3] = NULL;
    } else {
        argv[1] = NULL;
    }
    fcntl( DbgConHandle, F_SETFD, 0 );
    if ((pid = fork()) == 0) {
        /* make sure STDIN/STDOUT/STDERR are connected to a terminal */
        dup2( DbgConHandle, 0 );
        dup2( DbgConHandle, 1 );
        dup2( DbgConHandle, 2 );
        close( DbgConHandle );
        DbgConHandle = -1;
        setsid();
#if defined( __UNIX__ ) && !defined( __WATCOMC__ )
        execve( shell, (char * const *)argv, (char * const *)environ );
#else
        execve( shell, argv, (const char **)environ );
#endif
        exit( 1 );
    } else {
        fcntl( DbgConHandle, F_SETFD, FD_CLOEXEC );
        if( pid == -1 )
            return( 0xffff0000 | errno );
        do {
        } while( waitpid( pid, NULL, 0 ) == -1 && errno == EINTR );
    }
    return 0;
}
