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
* Description:  File logging functions.
*
****************************************************************************/


#include "commonui.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "wio.h"
#include "bool.h"
#include "watcom.h"
#include "log.h"
#include "cguimem.h"
#ifndef NOUSE3D
    #include "ctl3dcvr.h"
#endif
#include "ldstr.h"
#include "uistr.grh"
#include "wclbproc.h"


typedef struct loginfo {
    LogConfig           config;
    void                (*headerfn)( FILE * );
    HANDLE              instance;
    HWND                hwnd;
    bool                init;
} LogInfo;

/* Window callback functions prototypes */
WINEXPORT UINT_PTR CALLBACK LogSaveOFNHookProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
WINEXPORT INT_PTR  CALLBACK LogExistsDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
WINEXPORT INT_PTR  CALLBACK ConfigLogDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static LogInfo          LogCurInfo;
static char             *BufLines[NO_BUF_LINES];
static WORD             LinesUsed;

#ifndef NOUSE3D

/*
 * LogSaveHook - hook used called by common dialog - for 3D controls
 */
UINT_PTR CALLBACK LogSaveOFNHookProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    wparam = wparam;
    lparam = lparam;
    hwnd = hwnd;
    switch( msg ) {
    case WM_INITDIALOG:
#ifndef NOUSE3D
        /*
         * We must call this to subclass the directory listbox even
         * if the app calls Ctl3dAutoSubclass (commdlg bug).
         */
        CvrCtl3dSubclassDlgAll( hwnd );
#endif
        return( TRUE );
    }
    return( FALSE );

} /* LogSaveOFNHookProc */

#endif

/*
 * getLogName - get a filename for the log and check if the file already exists
 */
static bool getLogName( char *buf, HWND hwnd )
{
    OPENFILENAME        of;
    int                 rc;
    static char         fname[LOG_MAX_FNAME];
    static char         filterList[] = "File (*.*)" \
                                       "\0" \
                                       "*.*" \
                                       "\0\0";

    strcpy( fname, LogCurInfo.config.name );
    memset( &of, 0, sizeof( OPENFILENAME ) );
    of.lStructSize = sizeof( OPENFILENAME );
    of.hwndOwner = hwnd;
    of.lpstrFilter = (LPSTR)filterList;
    of.lpstrDefExt = NULL;
    of.nFilterIndex = 1L;
    of.lpstrFile = fname;
    of.nMaxFile = LOG_MAX_FNAME;
    of.lpstrTitle = AllocRCString( LOG_CHOOSE_LOG_NAME );
    of.Flags = OFN_HIDEREADONLY;
#ifndef NOUSE3D
    of.Flags |= OFN_ENABLEHOOK;
    of.lpfnHook = MakeProcInstance_OFNHOOK( LogSaveOFNHookProc, LogCurInfo.instance );
#endif
    rc = GetSaveFileName( &of );
#ifndef NOUSE3D
    FreeProcInstance_OFNHOOK( of.lpfnHook );
#endif
    FreeRCString( (char *)of.lpstrTitle );
    if( !rc ) {
        return( false );
    }
    strcpy( buf, fname );
    return( true );

} /* getLogName */

/*
 * flushLog - write out everything in the log buffer
 */
static void flushLog( bool free )
{
    WORD        i;
    FILE        *f;

    f = fopen( LogCurInfo.config.curname, "at" );
    if( f == NULL ) {
        return;
    }
    for( i = 0; i < LinesUsed; i++ ) {
        fwrite( BufLines[i], 1, strlen( BufLines[i] ), f );
        fwrite( "\n", 1, 1, f );
    }
    fclose( f );
    if( free ) {
        for( i = 0; i < LinesUsed; i++ ) {
            MemFree( BufLines[i] );
        }
    }
    LinesUsed = 0;

} /* flushLog */

/*
 * LogExistsDlgProc - handle the log exists dialog
 */
INT_PTR CALLBACK LogExistsDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    lparam = lparam;
    switch( msg ) {
    case WM_COMMAND:
        switch( wparam ) {
        case LOG_APPEND:
        case LOG_REPLACE:
        case LOG_CANCEL:
            EndDialog( hwnd, wparam );
            break;
        default:
            return( FALSE );
        }
        break;
    default:
        return( FALSE );
    }
    return( TRUE );

} /* LogExistsDlgProc */

/*
 * ConfigLogDlgProc - handle the configure log dialog
 */
INT_PTR CALLBACK ConfigLogDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    char    *buf;

    lparam = lparam;
    switch( msg ) {
    case WM_INITDIALOG:
        if( LogCurInfo.config.query_for_name ) {
            CheckDlgButton( hwnd, LOG_CFG_QUERY_NAME, BST_CHECKED );
        } else {
            CheckDlgButton( hwnd, LOG_CFG_USE_NAME, BST_CHECKED );
        }
        if( LogCurInfo.config.type == LOG_TYPE_BUFFER ) {
            CheckDlgButton( hwnd, LOG_CFG_PERIODIC, BST_CHECKED );
        } else {
            CheckDlgButton( hwnd, LOG_CFG_CONT, BST_CHECKED );
        }
        if( LogCurInfo.config.def_action == LOG_ACTION_APPEND ) {
            CheckDlgButton( hwnd, LOG_CFG_APPEND, BST_CHECKED );
        } else if( LogCurInfo.config.def_action == LOG_ACTION_TRUNC ) {
            CheckDlgButton( hwnd, LOG_CFG_REPLACE, BST_CHECKED );
        } else if( LogCurInfo.config.def_action == LOG_ACTION_QUERY ) {
            CheckDlgButton( hwnd, LOG_CFG_QUERY, BST_CHECKED );
        }
        SetDlgItemText( hwnd, LOG_CFG_NAME_EDIT, LogCurInfo.config.name );
        break;
#ifndef NOUSE3D
    case WM_SYSCOLORCHANGE:
        CvrCtl3dColorChange();
        break;
#endif
    case WM_COMMAND:
        switch( wparam ) {
        case LOG_CFG_BROWSE:
            buf = MemAlloc( LOG_MAX_FNAME );
            if( getLogName( buf, hwnd ) ) {
                strlwr( buf );
                SetDlgItemText( hwnd, LOG_CFG_NAME_EDIT, buf );
            }
            MemFree( buf );
            break;
        case LOG_CFG_OK:
            if( IsDlgButtonChecked( hwnd, LOG_CFG_QUERY_NAME ) ) {
                LogCurInfo.config.query_for_name = true;
            } else if( IsDlgButtonChecked( hwnd, LOG_CFG_USE_NAME ) ) {
                LogCurInfo.config.query_for_name = false;
            }
            if( IsDlgButtonChecked( hwnd, LOG_CFG_PERIODIC ) ) {
                LogCurInfo.config.type = LOG_TYPE_BUFFER;
            } else if( IsDlgButtonChecked( hwnd, LOG_CFG_CONT ) ) {
                LogCurInfo.config.type = LOG_TYPE_CONTINUOUS;
            }
            if( IsDlgButtonChecked( hwnd, LOG_CFG_APPEND ) ) {
                LogCurInfo.config.def_action = LOG_ACTION_APPEND;
            } else if( IsDlgButtonChecked( hwnd, LOG_CFG_REPLACE ) ) {
                LogCurInfo.config.def_action = LOG_ACTION_TRUNC;
            } else if( IsDlgButtonChecked( hwnd, LOG_CFG_QUERY ) ) {
                LogCurInfo.config.def_action = LOG_ACTION_QUERY;
            }
            GetDlgItemText( hwnd, LOG_CFG_NAME_EDIT,
                            LogCurInfo.config.name, LOG_MAX_FNAME );
            EndDialog( hwnd, TRUE );
            break;
        case LOG_CFG_CANCEL:
            EndDialog( hwnd, FALSE );
            break;
        default:
            return( FALSE );
        }
        break;
    default:
        return( FALSE );
    }
    return( TRUE );

} /* ConfigLogDlgProc */

/*
 * LogConfigure - display a dialog to let the user configure log features
 */
void LogConfigure( void )
{
    DLGPROC     dlgproc;

    if( !LogCurInfo.init ) {
        LogSetDef();
    }
    if( LogCurInfo.config.type == LOG_TYPE_BUFFER ) {
        flushLog( true );
    }
    dlgproc = MakeProcInstance_DLG( ConfigLogDlgProc, LogCurInfo.instance );
    DialogBox( LogCurInfo.instance, "LOG_CFG_DLG", LogCurInfo.hwnd, dlgproc );
    FreeProcInstance_DLG( dlgproc );

} /* LogConfigure */

/*
 * LogSetDef - set the log configuration to the defaults
 */
void LogSetDef( void )
{
    strcpy( LogCurInfo.config.name, "dflt.log" );
    LogCurInfo.config.type = LOG_TYPE_CONTINUOUS;
    LogCurInfo.config.def_action = LOG_ACTION_QUERY;
    LogCurInfo.config.query_for_name = true;
    LogCurInfo.init = true;

} /* LogSetDef */

/*
 * LogGetConfig - copy the current log configuration information to config
 */
void LogGetConfig( LogConfig *config )
{
    if( !LogCurInfo.init ) {
        LogSetDef();
    }
    *config = LogCurInfo.config;

} /* LogGetConfig */

/*
 * LogSetConfig - set current log configuration
 */
void LogSetConfig( LogConfig *config )
{
    LogCurInfo.config = *config;
    LogCurInfo.init = true;

} /* LogSetConfig */

#define LOG_TYPE                "LOGtype"
#define LOG_NAME                "LOGname"
#define LOG_ACTION              "LOGaction"
#define LOG_QUERY               "LOGquery"

/*
 * LogSaveConfig - save the current log configuration
 */
void LogSaveConfig( char *fname, char *section )
{
    char        buf[10];

    if( !LogCurInfo.init ) {
        LogSetDef();
    }

    itoa( LogCurInfo.config.type, buf, 10 );
    WritePrivateProfileString( section, LOG_TYPE, buf, fname );

    itoa( LogCurInfo.config.def_action, buf, 10 );
    WritePrivateProfileString( section, LOG_ACTION, buf, fname );

    WritePrivateProfileString( section, LOG_QUERY, ( LogCurInfo.config.query_for_name ) ? "1" : "0", fname );

    WritePrivateProfileString( section, LOG_NAME, LogCurInfo.config.name, fname );

} /* LogSaveConfig */

/*
 * LogLoadConfig - read log configuration information from the .ini file
 */

void LogLoadConfig( char *fname, char *section )
{
    LogSetDef();
    LogCurInfo.config.type = GetPrivateProfileInt( section, LOG_TYPE, LogCurInfo.config.type, fname );
    LogCurInfo.config.def_action = GetPrivateProfileInt( section, LOG_ACTION, LogCurInfo.config.def_action, fname );
    LogCurInfo.config.query_for_name = GetPrivateProfileInt( section, LOG_QUERY, LogCurInfo.config.query_for_name, fname ) != 0;
    GetPrivateProfileString( section, LOG_NAME, LogCurInfo.config.name, LogCurInfo.config.name, LOG_MAX_FNAME, fname );

} /* LogLoadConfig */

/*
 * LogOut - dump a message to the log file
 */
void LogOut( char *res )
{
    size_t  len;

    if( !LogCurInfo.config.logging || LogCurInfo.config.paused ) {
        return;
    }
    if( LogCurInfo.config.type == LOG_TYPE_CONTINUOUS ) {
        BufLines[LinesUsed] = res;
        LinesUsed++;
        flushLog( false );
    } else {
        len = strlen( res ) + 1;
        BufLines[LinesUsed] = MemAlloc( len );
        strcpy( BufLines[LinesUsed], res );
        LinesUsed++;
        if( LinesUsed == NO_BUF_LINES ) {
            flushLog( true );
        }
    }

} /* LogOut */

/*
 * LogOpen - open the log file
 */
static bool LogOpen( void )
{
    FILE        *f;
    DLGPROC     dlgproc;
    INT_PTR     ret;
    char        *msgtitle;
    char        *fmode = "wt";

    if( !LogCurInfo.init ) {
        LogSetDef();
    }
    if( LogCurInfo.config.query_for_name ) {
        if( !getLogName( LogCurInfo.config.name, LogCurInfo.hwnd ) ) {
            return( false );
        }
    }
    strcpy( LogCurInfo.config.curname, LogCurInfo.config.name );
    switch( LogCurInfo.config.def_action ) {
    case LOG_ACTION_TRUNC:
        break;
    case LOG_ACTION_APPEND:
        fmode = "at";
        break;
    case LOG_ACTION_QUERY:
        if( !access( LogCurInfo.config.curname, F_OK ) ) {
            dlgproc = MakeProcInstance_DLG( LogExistsDlgProc, LogCurInfo.instance );
            ret = DialogBox( LogCurInfo.instance, "LOG_EXISTS_DLG", LogCurInfo.hwnd, dlgproc );
            FreeProcInstance_DLG( dlgproc );
            switch( ret ) {
            case LOG_APPEND:
                fmode = "at";
                break;
            case LOG_REPLACE:
                break;
            case LOG_CANCEL:
                return( false );
            }
        }
        break;
    }
    f = fopen( LogCurInfo.config.curname, fmode );
    if( f == NULL ) {
        msgtitle = AllocRCString( LOG_LOG_ERROR );
        RCMessageBox( LogCurInfo.hwnd, LOG_CANT_OPEN_LOG, msgtitle, MB_OK | MB_ICONEXCLAMATION );
        FreeRCString( msgtitle );
        return( false );
    }
    if( LogCurInfo.headerfn != NULL ) {
        LogCurInfo.headerfn( f );
    }
    fclose( f );
    LogCurInfo.config.logging = true;
    LogCurInfo.config.paused = false;
    return( true );

} /* LogOpen */

/*
 * LogClose - close the log file
 */
static void LogClose( void )
{
    flushLog( true );
    LogCurInfo.config.logging = false;
    LogCurInfo.config.paused = false;

} /* LogClose */

/*
 * LogPauseToggle - switch between paused/unpaused state
 */
bool LogPauseToggle( void )
{
    if( LogCurInfo.config.logging ) {
        if( !LogCurInfo.config.paused ) {
            flushLog( true );
        }
        LogCurInfo.config.paused = !LogCurInfo.config.paused;
    }
    return( LogCurInfo.config.paused );

} /* LogPauseToggle */

/*
 * LogToggle - toggle between logging and not logging modes
 */
bool LogToggle( void )
{
    if( !LogCurInfo.init ) {
        LogSetDef();
    }
    if( !LogCurInfo.config.logging ) {
        return( LogOpen() );
    }
    LogClose();
    return( LogCurInfo.config.logging );

} /* LogToggle */

/*
 * LogInit - must be called before any other log functions to do
 *           initialization
 *
 *      hwnd - is a window to be the parent to dialog boxes generated by
 *             the log function
 *      inst - is the INSTANCE of the calling application
 *      headerfn - function that creates the log header or NULL if no
 *                header is desired
 */
void LogInit( HWND hwnd, HANDLE inst, void (*headerfn)( FILE * ) )
{
    LogCurInfo.hwnd = hwnd;
    LogCurInfo.instance = inst;
    LogCurInfo.headerfn = headerfn;
    LogCurInfo.init = false;

} /* LogInit */

/*
 * LogFini - must be called as last log functions to do finalization
 *             flush data etc.
 *
 */
void LogFini( void )
{
    LogClose();

} /* LogFini */
