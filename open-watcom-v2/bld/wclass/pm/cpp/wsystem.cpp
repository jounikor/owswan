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
* Description:  WSystemService class implementation.
*
****************************************************************************/


#include <process.h>
#include "wsystem.hpp"
#include "wstrlist.hpp"
#include "wfilenam.hpp"

#define PGM_DOSEXECPGM          0
#define PGM_DOSSTARTSESSION     1

#define SSF_TYPE_31_ENHSEAMLESSVDM      PROG_31_ENHSEAMLESSVDM

#define WINOS2_NAME     "WINOS2.COM"
#define WINOS2_PARM     "/K"


// Build an environment array suitable for DosExecPgm
char    *build_exec_env( char **env )
{
    char    **s;
    size_t  len;
    char    *env_copy;
    char    *d;

    if( env == NULL ) {
        return( NULL );
    }
    // figure out how much memory we need
    len = 0;
    for( s = env; *s != NULL; s++ ) {
        len += strlen( *s ) + 1;
    }
    ++len;  // for terminating NUL
    env_copy = (char *)malloc( len );
    if( env_copy == NULL ) {
        return( NULL );
    }
    // copy the environment strings
    d = env_copy;
    for( s = env; *s != NULL; s++ ) {
        d = strcpy( d, *s ) + strlen( *s ) + 1;
    }
    *d = '\0';  // terminate array
    return( env_copy );
}

int WEXPORT WSystemService::sysExec( const char *cmd,
                                     WWindowState state,
                                     WWindowType typ,
                                     bool background ) {
/******************************************************/

    WStringList args;
    const char  *arg_pgm;
    const char  *pgm;
    char        pgm_buf[_MAX_PATH];
    char        searchenv_buf[_MAX_PATH];
    char        *cmdline;
    APIRET      rc;
    RESULTCODES returncodes;
    ULONG       app_type;
    TIB         *ptib;
    PIB         *ppib;
    STARTDATA   sd;
    ULONG       session;
    PID         pid;
    bool        use_exec_pgm;
    int         exec_state;
    int         show;
    USHORT      sess_type;

    args.parseIn( cmd );
    arg_pgm = args.stringAt( 0 );
    // if the filename was quoted, we need to strip the quotes
    // or OS/2 won't like us
    if( arg_pgm[0] == '\"' ) {
        strncpy( pgm_buf, arg_pgm + 1, _MAX_PATH - 1 );
        pgm_buf[strlen( pgm_buf ) - 1] = '\0';
        pgm = pgm_buf;
    } else {
        pgm = arg_pgm;
    }

    WFileName exename( pgm );
    exename.setExtIfNone( "exe" );

    // Try to determine full pathname; the process PATH may not be what
    // we started with.
    _searchenv( (const char *)exename, "PATH", searchenv_buf );
    if( searchenv_buf[0] != '\0' )
        pgm = searchenv_buf;

    rc = DosQueryAppType( (char const *)pgm, &app_type );
    if( rc != 0 )
        return( -1 );

    use_exec_pgm = false;
    sess_type = SSF_TYPE_DEFAULT;
    if( typ == WWinTypeDefault ) {
        if( app_type & FAPPTYP_DOS ) {
            sess_type = SSF_TYPE_WINDOWEDVDM;
        } else if( app_type & (FAPPTYP_WINDOWSPROT31 | FAPPTYP_WINDOWSPROT) ) {
            sess_type = SSF_TYPE_31_ENHSEAMLESSVDM;
            args.insertAt( 0, new WString( WINOS2_NAME ) );
            args.insertAt( 1, new WString( WINOS2_PARM ) );
        } else {
            rc = DosGetInfoBlocks( &ptib, &ppib );
            if( rc != 0 )
                return( -1 );
            // we are starting a full screen or PM-compatible program
            switch( app_type & FAPPTYP_EXETYPE ) {
            case FAPPTYP_NOTSPEC:
                if( ppib->pib_ultype == PT_FULLSCREEN ) {
                    use_exec_pgm = true;
                }
                break;
            case FAPPTYP_NOTWINDOWCOMPAT:
                if( background || ppib->pib_ultype == PT_FULLSCREEN ) {
                    use_exec_pgm = true;
                }
                break;
            case FAPPTYP_WINDOWCOMPAT:
                if( background || ppib->pib_ultype == PT_FULLSCREEN ) {
                    use_exec_pgm = true;
                }
                break;
            case FAPPTYP_WINDOWAPI:
                if( ppib->pib_ultype == PT_PM ) {
                    use_exec_pgm = true;
                }
                break;
            }
        }
    } else {
        switch( typ ) {
        case WWinTypeFullScreen:
            if( app_type & FAPPTYP_DOS ) {
                sess_type = SSF_TYPE_VDM;
            } else if( app_type & (FAPPTYP_WINDOWSPROT31 | FAPPTYP_WINDOWSPROT) ) {
                sess_type = SSF_TYPE_DEFAULT;
                args.insertAt( 0, new WString( WINOS2_NAME ) );
                args.insertAt( 1, new WString( WINOS2_PARM ) );
            } else {
                sess_type = SSF_TYPE_FULLSCREEN;
            }
            break;
        case WWinTypeWindowed:
            if( app_type & FAPPTYP_DOS ) {
                sess_type = SSF_TYPE_WINDOWEDVDM;
            } else if( app_type & (FAPPTYP_WINDOWSPROT31 | FAPPTYP_WINDOWSPROT) ) {
                sess_type = SSF_TYPE_31_ENHSEAMLESSVDM;
            } else {
                sess_type = SSF_TYPE_WINDOWABLEVIO;
            }
            break;
        }
    }

    WFileName fn( pgm );
    fn.setExtIfNone( "exe" );
    if( use_exec_pgm ) {
        char    *exec_env;

        exec_state = EXEC_ASYNC;
        if( background ) {
            exec_state = EXEC_BACKGROUND;
        }
        if( args.count() == 1 ) {
            cmdline = NULL;
        } else {
            cmdline = (char *)args.cString();
            cmdline[strlen( args.stringAt( 0 ) )] = '\0';
        }
        exec_env = build_exec_env( environ );
        rc = DosExecPgm( (char *)NULL, 0, exec_state, (char const *)cmdline, (char const *)exec_env,
                         &returncodes, (char *)(const char *)fn );
        if( exec_env != NULL ) {
            free( exec_env );
        }
        if( rc != 0 )
            return( -1 );
        return( returncodes.codeTerminate );    // process id of child
    } else {
        switch( state ) {
        case WWinStateHide:
            show = SSF_CONTROL_INVISIBLE;
            break;
        case WWinStateShow:
            show = SSF_CONTROL_VISIBLE;
            break;
        case WWinStateShowNormal:
            show = SSF_CONTROL_VISIBLE;
            break;
        case WWinStateMinimized:
            show = SSF_CONTROL_MINIMIZE;
            break;
        case WWinStateMaximized:
            show = SSF_CONTROL_MAXIMIZE;
            break;
        }
        exec_state = SSF_FGBG_FORE;
        if( background ) {
            exec_state = SSF_FGBG_BACK;
        }
        if( args.count() == 1 ) {
            cmdline = NULL;
        } else {
            cmdline = (char *)args.cString( 1 );
        }
        sd.Length = sizeof( sd );
        sd.Related = SSF_RELATED_INDEPENDENT;
        sd.FgBg = (USHORT)exec_state;
        sd.TraceOpt = SSF_TRACEOPT_NONE;
        sd.PgmTitle = NULL;
        sd.PgmName = (char *)(const char *)fn;
        sd.PgmInputs = (PBYTE)cmdline;
        sd.TermQ = NULL;
        sd.Environment = NULL;
        sd.InheritOpt = SSF_INHERTOPT_PARENT;
        sd.SessionType = sess_type;
        sd.IconFile = NULL;
        sd.PgmHandle = 0;
        sd.PgmControl = (USHORT)show;
        sd.InitXPos = 0;
        sd.InitYPos = 0;
        sd.InitXSize = 0;
        sd.InitYSize = 0;
        sd.Reserved = 0;
        sd.ObjectBuffer = NULL;
        sd.ObjectBuffLen = 0;
        rc = DosStartSession( &sd, &session, &pid );
        if( rc != 0 && rc != ERROR_SMG_START_IN_BACKGROUND )
            return( -1 );
        return( pid );
    }
}


int WEXPORT WSystemService::sysExec( const char *cmd,
                                     WWindowState state, WWindowType typ ) {
/**************************************************************************/

    return( sysExec( cmd, state, typ, false ) );
}


int WEXPORT WSystemService::sysExecBackground( const char *cmd ) {
/****************************************************************/

    return( sysExec( cmd, WWinStateShowNormal, WWinTypeDefault, true ) );
}


void WEXPORT WSystemService::sysYield() {
/***************************************/

    QMSG     qmsg;

    while( WinPeekMsg( GUIMainHInst.hab, &qmsg, NULLHANDLE, 0, 0, PM_NOREMOVE ) ) {
        WinGetMsg( GUIMainHInst.hab, &qmsg, 0, 0, 0 );
        WinDispatchMsg( GUIMainHInst.hab, &qmsg );
    }
}


void WEXPORT WSystemService::sysSleep( unsigned long interval ) {
/***************************************************************/

    DosSleep( interval );
}


#define BUFF_LEN 512

WModuleHandle WEXPORT WSystemService::loadLibrary( const char *lib_name ) {
/*************************************************************************/

    APIRET      rc;
    HMODULE     lib_handle;
    CHAR        fail_name[BUFF_LEN];

    rc = DosLoadModule( (PSZ)fail_name, sizeof( fail_name ),
                        (PSZ)lib_name, &lib_handle );
    if( rc != 0 ) {
        return( NULLHANDLE );
    } else {
        return( lib_handle );
    }
}


WProcAddr WEXPORT WSystemService::getProcAddr( WModuleHandle mod_handle,
                                               const char *proc ) {
/*****************************************************************/

    APIRET      rc;
    PFN         proc_addr;

    rc = DosQueryProcAddr( mod_handle, 0, (PSZ)proc, &proc_addr );
    if( rc != 0 ) {
        return( NULL );
    } else {
        return( proc_addr );
    }
}


void WEXPORT WSystemService::freeLibrary( WModuleHandle mod_handle ) {
/********************************************************************/

    DosFreeModule( mod_handle );
}
