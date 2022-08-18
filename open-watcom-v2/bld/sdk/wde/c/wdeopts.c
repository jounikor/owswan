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


#include "wdeglbl.h"
#include "wdemsgbx.h"
#include "wderesin.h"
#include "wdegeted.h"
#include "wdedebug.h"
#include "wdesdup.h"
#include "wdefutil.h"
#include "wdegetfn.h"
#include "wdemain.h"
#include "wdestat.h"
#include "wdectl3d.h"
#include "wde.rh"
#include "wdeopts.h"
#include "jdlg.h"
#include "watini.h"
#include "inipath.h"
#include "wclbproc.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define MAIN_WIN_START_X  20
#define MAIN_WIN_START_Y  20
#define MAIN_WIN_SIZE_X   620
#define MAIN_WIN_SIZE_Y   440

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct {
    bool    is_wres_fmt;
    bool    use_def_dlg;
    int     grid_x;
    int     grid_y;
    bool    ignore_inc;
    char    *inc_path;
    RECT    screen_pos;
    RECT    control_toolbar_pos;
    bool    is_screen_maximized;
    bool    is_ctoolbar_visible;
    bool    is_ribbon_visible;
    char    *last_directory;
    char    *last_file_filter;
    bool    use_3d_effects;
} WdeOptState;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT INT_PTR CALLBACK WdeOptionsDlgProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/*****************************************************************************/
//static void WdeResetOpts( void ); // prevent warning
void WdeResetOpts( void );

/****************************************************************************/
/* external variables                                                       */
/****************************************************************************/
char WdeProfileName[_MAX_PATH] = WATCOM_INI;
char WdeSectionName[] = "wde";

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WdeOptState WdeCurrentState;
static HWND        WdeOptWin        = NULL;

static WdeOptState WdeDefaultState = {
    FALSE,                      /* initial resource format is MS    */
    TRUE,                       /* initial define dialog is default */
    1, 1,                       /* initial grid size                */
    TRUE,                       /* ignore INCLUDE env variable      */
    NULL,                       /* extra include path               */
    { MAIN_WIN_START_X,         /* initial screen pos               */
      MAIN_WIN_START_Y,
      MAIN_WIN_START_X +
      MAIN_WIN_SIZE_X,
      MAIN_WIN_START_Y +
      MAIN_WIN_SIZE_Y
    },
    { 0, 0, 0, 0 },             /* initial controls toolbar pos     */
    FALSE,                      /* is the window maximized          */
    TRUE,                       /* is the controls toolbar visible  */
    TRUE,                       /* is the ribbon visible            */
    NULL,                       /* last open/save directory         */
    NULL,                       /* last file filter                 */
    TRUE                        /* use 3d effects                   */
};

static bool WdeWriteIntOpt( char *entry, int i )
{
    char  str[12];
    bool  ret;

    ltoa( i, str, 10 );

    ret = ( WritePrivateProfileString( WdeSectionName, entry, str, WdeProfileName ) != 0 );

    return( ret );
}

static bool WdeGetBoolOpt( char *entry, bool *i )
{
    int val;

    val = (int)GetPrivateProfileInt( WdeSectionName, entry, 0x7fff, WdeProfileName );

    if( val != 0x7fff ) {
        *i = ( val != 0 );
        return( TRUE );
    } else {
        return( FALSE );
    }
}

static bool WdeGetIntOpt( char *entry, int *i )
{
    int val;

    val = (int)GetPrivateProfileInt( WdeSectionName, entry, 0x7fff, WdeProfileName );

    if( val != 0x7fff ) {
        *i = val;
        return( TRUE );
    } else {
        return( FALSE );
    }
}

static bool WdeWriteRectOpt( char *entry, RECT *r )
{
    char    *str;
    bool    ret;

    ret = FALSE;
    str = WdeRectToStr( r );
    if( str != NULL ) {
        ret = ( WritePrivateProfileString( WdeSectionName, entry, str, WdeProfileName ) != 0 );
        WRMemFree( str );
    }

    return( ret );
}

static bool WdeGetRectOpt( char *entry, RECT *r )
{
    char  str[41];
    bool  ret;

    ret = GetPrivateProfileString( WdeSectionName, entry, "0, 0, 0, 0",
                                   str, 40, WdeProfileName );
    if( ret && strcmp( "0, 0, 0, 0", str ) ) {
        WdeStrToRect( str, r );
        return( TRUE );
    }

    return( FALSE );
}

static bool WdeGetStrOpt( char *entry, char **opt )
{
    char        str[_MAX_PATH];
    bool        ret;

    ret = GetPrivateProfileString( WdeSectionName, entry, "",
                                   str, _MAX_PATH - 1, WdeProfileName );

    if( ret ) {
        if( !WdeIsStrSpace( str ) ) {
            ret = ((*opt = WdeStrDup( str )) != NULL);
        }
    }

    return( ret );
}

static bool WdeReadOpts( WdeOptState *s )
{
    bool ret;

    ret  = WdeGetBoolOpt( "WResFmt", &s->is_wres_fmt );
    ret &= WdeGetBoolOpt( "UseDefDlg", &s->use_def_dlg );
    ret &= WdeGetIntOpt( "GridX", &s->grid_x );
    ret &= WdeGetIntOpt( "GridY", &s->grid_y );
    ret &= WdeGetBoolOpt( "IgnoreIncPath", &s->ignore_inc );
    ret &= WdeGetRectOpt( "ScreenPos", &s->screen_pos );
    ret &= WdeGetRectOpt( "CTBarPos", &s->control_toolbar_pos );
    ret &= WdeGetBoolOpt( "ScreenMaxed", &s->is_screen_maximized );
    ret &= WdeGetBoolOpt( "CTBarVis", &s->is_ctoolbar_visible );
    ret &= WdeGetBoolOpt( "RibbonVis", &s->is_ribbon_visible );
    ret &= WdeGetBoolOpt( "Use3DEffects", &s->use_3d_effects );
    ret &= WdeGetStrOpt( "FileFilter", &s->last_file_filter );
    ret &= WdeGetStrOpt( "IncPath", &s->inc_path );
    ret &= WdeGetStrOpt( "LastDir", &s->last_directory );

    return( ret );
}

static void WdeWriteOpts( WdeOptState *o )
{
    WdeWriteIntOpt( "WResFmt", o->is_wres_fmt );
    WdeWriteIntOpt( "UseDefDlg", o->use_def_dlg );
    WdeWriteIntOpt( "GridX", o->grid_x );
    WdeWriteIntOpt( "GridY", o->grid_y );
    WdeWriteIntOpt( "IgnoreIncPath", o->ignore_inc );
    WdeWriteRectOpt( "ScreenPos", &o->screen_pos );
    WdeWriteRectOpt( "CTBarPos", &o->control_toolbar_pos );
    WdeWriteIntOpt( "ScreenMaxed", o->is_screen_maximized );
    WdeWriteIntOpt( "CTBarVis", o->is_ctoolbar_visible );
    WdeWriteIntOpt( "RibbonVis", o->is_ribbon_visible );
    WdeWriteIntOpt( "Use3DEffects", o->use_3d_effects );
    WritePrivateProfileString( WdeSectionName, "FileFilter",
                               o->last_file_filter, WdeProfileName );
    WritePrivateProfileString( WdeSectionName, "IncPath",
                               o->inc_path, WdeProfileName );
    WritePrivateProfileString( WdeSectionName, "LastDir",
                               o->last_directory, WdeProfileName );
}

void WdeOptsShutdown( void )
{
    if( WdeCurrentState.last_directory != NULL ) {
        WRMemFree( WdeCurrentState.last_directory );
    }
    if( WdeCurrentState.last_file_filter != NULL ) {
        WRMemFree( WdeCurrentState.last_file_filter );
    }

    WdeCurrentState.last_directory = WdeStrDup( WdeGetInitialDir() );
    WdeCurrentState.last_file_filter = WdeGetFileFilter();

    WdeWriteOpts( &WdeCurrentState );

    if( WdeCurrentState.last_directory != NULL ) {
        WRMemFree( WdeCurrentState.last_directory );
    }
    if( WdeCurrentState.inc_path != NULL ) {
        WRMemFree( WdeCurrentState.inc_path );
    }
}

void WdeInitOpts( void )
{
    WdeCurrentState = WdeDefaultState;
    GetConfigFilePath( WdeProfileName, sizeof( WdeProfileName ) );
    strcat( WdeProfileName, "\\" WATCOM_INI );
    WdeReadOpts( &WdeCurrentState );
    if( WdeCurrentState.last_directory ) {
        WdeSetInitialDir( WdeCurrentState.last_directory );
    }
    WdeSetFileFilter( WdeCurrentState.last_file_filter );
}

void WdeResetOpts( void )
{
    if( WdeCurrentState.inc_path != NULL ) {
        WRMemFree( WdeCurrentState.inc_path );
    }
    WdeCurrentState.is_wres_fmt = WdeDefaultState.is_wres_fmt;
    WdeCurrentState.use_def_dlg = WdeDefaultState.use_def_dlg;
    WdeCurrentState.grid_x = WdeDefaultState.grid_x;
    WdeCurrentState.grid_y = WdeDefaultState.grid_y;
    WdeCurrentState.ignore_inc = WdeDefaultState.ignore_inc;
    WdeCurrentState.inc_path = WdeStrDup( WdeDefaultState.inc_path );
}

int WdeGetOption( WdeOptReq req )
{
    int ret;

    switch( req ) {
    case WdeOptIsWResFmt:
        ret = WdeCurrentState.is_wres_fmt;
        break;

    case WdeOptUseDefDlg:
        ret = WdeCurrentState.use_def_dlg;
        break;

    case WdeOptReqGridX:
        ret = WdeCurrentState.grid_x;
        break;

    case WdeOptReqGridY:
        ret = WdeCurrentState.grid_y;
        break;

    case WdeOptIgnoreInc:
        ret = WdeCurrentState.ignore_inc;
        break;

    case WdeOptIsScreenMax:
        ret = WdeCurrentState.is_screen_maximized;
        break;

    case WdeOptIsCntlsTBarVisible:
        ret = WdeCurrentState.is_ctoolbar_visible;
        break;

    case WdeOptIsRibbonVisible:
        ret = WdeCurrentState.is_ribbon_visible;
        break;

    case WdeOptUse3DEffects:
        ret = WdeCurrentState.use_3d_effects;
        break;

    default:
        ret = WDE_BAD_OPT_REQ;
        break;
    }

    return( ret );
}

char *WdeGetIncPathOption( void )
{
    return( WdeCurrentState.inc_path );
}

void WdeSetIncPathOption( char *path )
{
    if( WdeCurrentState.inc_path != NULL ) {
        WRMemFree( WdeCurrentState.inc_path );
    }
    WdeCurrentState.inc_path = path;
}

void WdeGetScreenPosOption( RECT *pos )
{
    *pos = WdeCurrentState.screen_pos;
}

void WdeSetScreenPosOption( RECT *pos )
{
    WdeCurrentState.screen_pos = *pos;
}

void WdeGetCntlTBarPosOption( RECT *pos )
{
    *pos = WdeCurrentState.control_toolbar_pos;
}

void WdeSetCntlTBarPosOption( RECT *pos )
{
    WdeCurrentState.control_toolbar_pos = *pos;
}

int WdeSetOption( WdeOptReq req, int val )
{
    int old;

    switch( req ) {
    case WdeOptIsWResFmt:
        old = WdeCurrentState.is_wres_fmt;
        WdeCurrentState.is_wres_fmt = val != 0;
        break;

    case WdeOptUseDefDlg:
        old = WdeCurrentState.use_def_dlg;
        WdeCurrentState.use_def_dlg = val != 0;
        break;

    case WdeOptReqGridX:
        old = WdeCurrentState.grid_x;
        WdeCurrentState.grid_x = val;
        break;

    case WdeOptReqGridY:
        old = WdeCurrentState.grid_y;
        WdeCurrentState.grid_y = val;
        break;

    case WdeOptIgnoreInc:
        old = WdeCurrentState.ignore_inc;
        WdeCurrentState.ignore_inc = val != 0;
        break;

    case WdeOptIsScreenMax:
        old = WdeCurrentState.is_screen_maximized;
        WdeCurrentState.is_screen_maximized = val != 0;
        break;

    case WdeOptIsCntlsTBarVisible:
        old = WdeCurrentState.is_ctoolbar_visible;
        WdeCurrentState.is_ctoolbar_visible = val != 0;
        break;

    case WdeOptIsRibbonVisible:
        old = WdeCurrentState.is_ribbon_visible;
        WdeCurrentState.is_ribbon_visible = val != 0;
        break;

    case WdeOptUse3DEffects:
        old = WdeCurrentState.use_3d_effects;
        WdeCurrentState.use_3d_effects = val != 0;
        break;
    }

    return( old );
}

bool WdeDisplayOptions( void )
{
    HWND      dialog_owner;
    DLGPROC   dlgproc;
    HINSTANCE app_inst;
    INT_PTR   modified;

    WdeSetStatusText( NULL, " ", false );
    WdeSetStatusByID( WDE_DISPLAYOPTIONS, 0 );

    dialog_owner = WdeGetMainWindowHandle();
    app_inst = WdeGetAppInstance();
    dlgproc = MakeProcInstance_DLG( WdeOptionsDlgProc, app_inst );
    modified = JDialogBoxParam( app_inst, "WdeOptions", dialog_owner, dlgproc, (LPARAM)NULL );
    FreeProcInstance_DLG( dlgproc );

    if( modified == -1 ) {
        WdeWriteTrail( "WdeDisplayOptions: Dialog not created!" );
        return( FALSE );
    }

    WdeSetStatusReadyText();

    return( TRUE );
}

static void WdeSetOptInfo( HWND hDlg, WdeOptState *state )
{
    if( state == NULL ) {
        return;
    }

    if( state->is_wres_fmt ) {
        CheckDlgButton( hDlg, IDB_OPT_WRES, BST_CHECKED );
        CheckDlgButton( hDlg, IDB_OPT_MRES, BST_UNCHECKED );
    } else {
        CheckDlgButton( hDlg, IDB_OPT_WRES, BST_UNCHECKED );
        CheckDlgButton( hDlg, IDB_OPT_MRES, BST_CHECKED );
    }

#if 0
    if( state->use_def_dlg ) {
        CheckDlgButton( hDlg, IDB_OPT_DEFDEF, BST_CHECKED );
        CheckDlgButton( hDlg, IDB_OPT_GENDEF, BST_UNCHECKED );
    } else {
        CheckDlgButton( hDlg, IDB_OPT_DEFDEF, BST_UNCHECKED );
        CheckDlgButton( hDlg, IDB_OPT_GENDEF, BST_CHECKED );
    }
#endif

    WdeSetEditWithSINT32( (int_32)state->grid_x, 10, hDlg, IDB_OPT_HINC );
    WdeSetEditWithSINT32( (int_32)state->grid_y, 10, hDlg, IDB_OPT_VINC );

    CheckDlgButton( hDlg, IDB_OPT_IGNOREINC, ( state->ignore_inc ) ? BST_CHECKED : BST_UNCHECKED );

    if( WdeCurrentState.inc_path != NULL ) {
        WdeSetEditWithStr( state->inc_path, hDlg, IDB_OPT_INCPATH );
    } else {
        WdeSetEditWithStr( "", hDlg, IDB_OPT_INCPATH );
    }
}

static void WdeGetOptInfo( HWND hDlg )
{
    if( IsDlgButtonChecked( hDlg, IDB_OPT_WRES ) ) {
        WdeCurrentState.is_wres_fmt = TRUE;
    } else {
        WdeCurrentState.is_wres_fmt = FALSE;
    }

#if 0
    if( IsDlgButtonChecked( hDlg, IDB_OPT_DEFDEF ) ) {
        WdeCurrentState.use_def_dlg = TRUE;
    } else {
        WdeCurrentState.use_def_dlg = FALSE;
    }
#endif

    WdeCurrentState.grid_x = (int)WdeGetSINT32FromEdit( hDlg, IDB_OPT_HINC, NULL );
    WdeCurrentState.grid_y = (int)WdeGetSINT32FromEdit( hDlg, IDB_OPT_VINC, NULL );
    WdeCurrentState.ignore_inc = IsDlgButtonChecked( hDlg, IDB_OPT_IGNOREINC );

    if( WdeCurrentState.inc_path != NULL ) {
        WRMemFree( WdeCurrentState.inc_path );
    }

    WdeCurrentState.inc_path = WdeGetStrFromEdit( hDlg, IDB_OPT_INCPATH, NULL );

    if( WdeCurrentState.inc_path != NULL ) {
        if( WdeIsStrSpace( WdeCurrentState.inc_path ) ) {
            WRMemFree( WdeCurrentState.inc_path );
            WdeCurrentState.inc_path = NULL;
        }
    }
}

INT_PTR CALLBACK WdeOptionsDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    bool ret;

    /* touch unused vars to get rid of warning */
    _wde_touch( lParam );

    ret = false;

    switch( message ) {
    case WM_SYSCOLORCHANGE:
        WdeCtl3dColorChange();
        break;

    case WM_INITDIALOG:
        WdeOptWin = hDlg;
        WdeSetOptInfo( hDlg, &WdeCurrentState );
        ret = true;
        break;

    case WM_DESTROY:
        WdeOptWin = NULL;
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDB_HELP:
            WdeHelpRoutine();
            break;

        case IDOK:
            WdeGetOptInfo( hDlg );
            EndDialog( hDlg, TRUE );
            ret = true;
            break;

        case IDCANCEL:
            EndDialog( hDlg, FALSE );
            ret = true;
            break;

#if 0
        case IDB_OPT_SETDEFS:
            WdeSetOptInfo( hDlg, &WdeDefaultState );
            break;
#endif

        }
    }

    return( ret );
}
