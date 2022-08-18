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


#include "wglbl.h"
#include <io.h>
#include "wresall.h"
#include "wstring.h"
#include "winst.h"
#include "wmemf.h"
#include "wrename.h"
#include "wnewitem.h"
#include "wdel.h"
#include "wmsg.h"
#include "wedit.h"
#include "wstat.h"
#include "wribbon.h"
#include "whints.h"
#include "wopts.h"
#include "whndl.h"
#include "sysall.rh"
#include "wctl3d.h"
#include "wsvobj.h"
#include "wsetedit.h"
#include "wmain.h"
#include "wstr2rc.h"
#include "weditsym.h"
#include "wstrdup.h"
#include "wrdll.h"
#include "dllmain.h"

#include "wwinhelp.h"
#include "jdlg.h"
#include "watini.h"
#include "inipath.h"
#include "aboutdlg.h"
#include "ldstr.h"

#include "clibext.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define WTIMER            666
#define ABOUT_TIMER       WTIMER
#define CLOSE_TIMER       WTIMER
#define WSTR_MINTRACKX    545
#define WSTR_MINTRACKY    500

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/
WINEXPORT LRESULT CALLBACK WMainWndProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static bool         WInit( HINSTANCE );
static void         WFini( void );
static WStringInfo  *WStringGetEInfo( WStringHandle, bool );
static bool         WRegisterMainClass( HINSTANCE );
static bool         WCreateEditWindow( HINSTANCE, WStringEditInfo * );
static void         WHandleMemFlags( WStringEditInfo *einfo );
static void         WUpdateScreenPosOpt( HWND );
static bool         WCleanup( WStringEditInfo * );
static bool         WQuerySave( WStringEditInfo *, bool );
static bool         WQuerySaveRes( WStringEditInfo *, bool );
static bool         WQuerySaveSym( WStringEditInfo *, bool );
static bool         WHandleWM_CLOSE( WStringEditInfo *, bool );
static void         WHandleClear( WStringEditInfo * );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WStringEditInfo *WCurrEditInfo = NULL;
static char            WMainClass[] = "WStrMainClass";
static char            WMainMenuName[] = "WMainMenu";
static char            WMainSOMenuName[] = "WSOMenu";
static char            WProfileName[_MAX_PATH] = WATCOM_INI;
static char            WSectionName[] = "wstring";
static char            WItemClipbdFmt[] = "WSTRING_ITEM_CLIPFMT";

static int      ref_count = 0;
static HACCEL   AccelTable = NULL;

UINT            WClipbdFormat     = 0;
UINT            WItemClipbdFormat = 0;

extern int appWidth;
extern int appHeight;

#ifdef __NT__

BOOL WINAPI DllMain( HINSTANCE inst, DWORD dwReason, LPVOID lpReserved )
{
    int ret;

    /* unused parameters */ (void)lpReserved;

    ret = TRUE;

    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        ref_count = 0;
        WSetEditInstance( inst );
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        /* do nothing here */
        break;
    }

    return( ret );
}

#else

int WINAPI LibMain( HINSTANCE inst, WORD dataseg, WORD heapsize, LPSTR cmdline )
{
    /* unused parameters */ (void)dataseg; (void)heapsize; (void)cmdline;

    ref_count = 0;
    WSetEditInstance( inst );

    return( TRUE );
}

int WINAPI WEP( int parm )
{
    /* unused parameters */ (void)parm;

    return( TRUE );
}
#endif

void WRESEAPI WStringInit( void )
{
    HINSTANCE   inst;

    inst = WGetEditInstance();
    if( AccelTable == (HACCEL)NULL ) {
        AccelTable = LoadAccelerators( inst, "WStringAccelTable" );
    }
    if( ref_count == 0 ) {
        WRInit();
        SetInstance( inst );
        WInit( inst );
    }
    ref_count++;
}

void WRESEAPI WStringFini( void )
{
    ref_count--;
    if( ref_count == 0 ) {
        WFini();
        WRFini();
    }
}

WStringHandle WRESEAPI WRStringStartEdit( WStringInfo *info )
{
    bool            ok;
    WStringEditInfo *einfo;

    einfo = NULL;

    ok = (info != NULL && info->parent != NULL && info->inst != NULL);

    if( ok ) {
        if( appWidth == -1 ) {
            WInitEditDlg( WGetEditInstance(), info->parent );
        }
        ok = ((einfo = WAllocStringEInfo()) != NULL);
    }

    if( ok ) {
        einfo->info = info;
        einfo->tbl = WMakeStringTableFromInfo( info );
        ok = (einfo->tbl != NULL);
    }

    if( ok ) {
        if( einfo->info->file_name != NULL ) {
            einfo->file_name = WStrDup( einfo->info->file_name );
            ok = (einfo->file_name != NULL);
            if( ok ) {
                einfo->file_type = WRIdentifyFile( einfo->file_name );
                ok = (einfo->file_type != WR_DONT_KNOW);
            }
        }
    }

    if( ok ) {
        ok = WResolveStringTable( einfo );
    }

    if( ok ) {
        ok = WCreateEditWindow( WGetEditInstance(), einfo );
    }

    if( ok ) {
        einfo->hndl = WRegisterEditSession( einfo );
        ok = (einfo->hndl != 0);
    }

    if( !ok ) {
        if( einfo != NULL ) {
            WFreeStringEInfo( einfo );
        }
        return( 0 );
    }

    return( einfo->hndl );
}

bool WRESEAPI WStringIsModified( WStringHandle hndl )
{
    WStringEditInfo *einfo;

    einfo = (WStringEditInfo *)WGetEditSessionInfo( hndl );

    return( einfo->info->modified );
}

void WRESEAPI WStringShowWindow( WStringHandle hndl, int show )
{
    WStringEditInfo *einfo;

    einfo = (WStringEditInfo *)WGetEditSessionInfo( hndl );

    if( einfo != NULL && einfo->win != (HWND)NULL ) {
        if( show ) {
            ShowWindow( einfo->win, SW_SHOWNA );
        } else {
            ShowWindow( einfo->win, SW_HIDE );
        }
    }
}

void WRESEAPI WStringBringToFront( WStringHandle hndl )
{
    WStringEditInfo *einfo;

    einfo = (WStringEditInfo *)WGetEditSessionInfo( hndl );

    if( einfo != NULL && einfo->win != (HWND)NULL ) {
        ShowWindow( einfo->win, SW_RESTORE );
        BringWindowToTop( einfo->win );
    }
}

bool WRESEAPI WStringIsDlgMsg( MSG *msg )
{
    return( WIsStringDialogMessage( msg, AccelTable ) );
}

WStringInfo *WRESEAPI WStringEndEdit( WStringHandle hndl )
{
    return( WStringGetEInfo( hndl, FALSE ) );
}

WStringInfo *WRESEAPI WStringGetEditInfo( WStringHandle hndl )
{
    return( WStringGetEInfo( hndl, TRUE ) );
}

int WRESEAPI WStringCloseSession( WStringHandle hndl, int force_exit )
{
    WStringEditInfo *einfo;

    einfo = (WStringEditInfo *)WGetEditSessionInfo( hndl );

    if( einfo != NULL && einfo->info != NULL ) {
        if( SendMessage( einfo->win, WM_CLOSE, (WPARAM)force_exit, 0 ) != 0 ) {
            return( FALSE );
        }
    }

    return( TRUE );
}

WStringInfo *WStringGetEInfo( WStringHandle hndl, bool keep )
{
    WStringEditInfo     *einfo;
    WStringInfo         *info;
    bool                ok;

    info = NULL;

    einfo = (WStringEditInfo *)WGetEditSessionInfo( hndl );

    ok = (einfo != NULL);

    if( ok ) {
        info = einfo->info;
        ok = (info != NULL);
    }

    if( ok ) {
        if( einfo->info->modified ) {
            WFreeStringNodes( info );
            info->tables = WMakeStringNodes( einfo->tbl );
        }
        if( !keep ) {
            WUnRegisterEditSession( hndl );
            WFreeStringEInfo( einfo );
        }
    }

    return( info );
}

bool WInit( HINSTANCE inst )
{
    bool ok;

    ok = (inst != (HINSTANCE)NULL);

    if( ok ) {
        WCtl3DInit( inst );
        ok = JDialogInit();
    }

    if( ok ) {
        ok = WRegisterMainClass( inst );
    }

    if( ok ) {
        ok = WInitStatusLines( inst );
    }

    if( ok ) {
        WClipbdFormat = RegisterClipboardFormat( WR_CLIPBD_STRING );
        ok = (WClipbdFormat != 0);
    }

    if( ok ) {
        WItemClipbdFormat = RegisterClipboardFormat( WItemClipbdFmt );
        ok = (WItemClipbdFormat != 0);
    }

    if( ok ) {
        GetConfigFilePath( WProfileName, sizeof( WProfileName ) );
        strcat( WProfileName, "\\" WATCOM_INI );
        WInitOpts( WProfileName, WSectionName );
        WInitEditWindows( inst );
        ok = WInitRibbons( inst );
    }

    return( ok );
}

void WFini( void )
{
    HINSTANCE inst;

    inst = WGetEditInstance();

    WFiniStatusLines();
    WOptsShutdown();
    WShutdownRibbons();
    WShutdownToolBars();
    WFiniEditWindows();
    WCtl3DFini( inst );
    UnregisterClass( WMainClass, inst );
    JDialogFini();
}

bool WRegisterMainClass( HINSTANCE inst )
{
    WNDCLASS wc;

    /* fill in the window class structure for the main window */
    wc.style = CS_DBLCLKS | CS_GLOBALCLASS;
    wc.lpfnWndProc = WMainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof( LONG_PTR );
    wc.hInstance = inst;
    wc.hIcon = LoadIcon( inst, "APPLICON" );
    wc.hCursor = LoadCursor( (HINSTANCE)NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszMenuName = WMainMenuName;
    wc.lpszClassName = WMainClass;

    return( RegisterClass( &wc ) != 0 );
}

char *WCreateEditTitle( WStringEditInfo *einfo )
{
    char        *title;
    char        *fname;
    char        *text;
    int         offset;
    size_t      len;

    title = NULL;
    fname = NULL;

    if( einfo == NULL ) {
        return( NULL );
    }

    if( einfo->file_name == NULL ) {
        fname = einfo->info->file_name;
    } else {
        fname = einfo->file_name;
    }

    text = AllocRCString( W_STRINGAPPTITLE );

    if( fname == NULL || text == NULL ) {
        return( NULL );
    }

    offset = WRFindFnOffset( fname );
    fname = &fname[offset];
    len = strlen( fname ) + strlen( text ) + 6;
    title = (char *)WRMemAlloc( len );
    if( title != NULL ) {
        strcpy( title, text );
        strcat( title, " - [" );
        strcat( title, fname );
        strcat( title, "]" );
    }

    if( text != NULL ) {
        FreeRCString( text );
    }

    return( title );
}

void WSetEditTitle( WStringEditInfo *einfo )
{
    char        *title;
    bool        is_rc;

    title = WCreateEditTitle( einfo );
    is_rc = false;

    if( title == NULL ) {
        title = AllocRCString( W_STRINGAPPTITLE );
        is_rc = true;
    }

    if( title != NULL ) {
        SendMessage( einfo->win, WM_SETTEXT, 0, (LPARAM)(LPCSTR)title );
        if( is_rc ) {
            FreeRCString( title );
        } else {
            WRMemFree( title );
        }
    }
}

bool WCreateEditWindow( HINSTANCE inst, WStringEditInfo *einfo )
{
    int         x, y, width, height;
    char        *title;
    HMENU       hmenu1;
    HMENU       hmenu2;
    bool        is_rc;
    RECT        rect;

    if( einfo == NULL ) {
        return( false );
    }

    x = CW_USEDEFAULT;
    y = CW_USEDEFAULT;
    width = appWidth;
    height = appHeight;

    if( einfo->info->stand_alone ) {
        WGetScreenPosOption( &rect );
        if( !IsRectEmpty( &rect ) ) {
            x = rect.left;
            y = rect.top;
            width = appWidth;
            if( width < rect.right - rect.left )
                width = rect.right - rect.left;
            height = appHeight;
            if( height < rect.bottom - rect.top ) {
                height = rect.bottom - rect.top;
            }
        }
    }

    is_rc = false;
    title = WCreateEditTitle( einfo );
    if( title == NULL ) {
        title = AllocRCString( W_STRINGAPPTITLE );
        is_rc = true;
    }

    hmenu1 = (HMENU)NULL;
    if( einfo->info->stand_alone ) {
        hmenu1 = LoadMenu( inst, WMainSOMenuName );
    }

    einfo->win = CreateWindow( WMainClass, title, WS_OVERLAPPEDWINDOW,
                               x, y, width, height, einfo->info->parent,
                               hmenu1, inst, einfo );

    if( title != NULL ) {
        if( is_rc ) {
            FreeRCString( title );
        } else {
            WRMemFree( title );
        }
    }

    if( einfo->win == (HWND)NULL ) {
        return( false );
    }

    if( !WCreateRibbon( einfo ) ) {
        return( false );
    }

    einfo->wsb = WCreateStatusLine( einfo->win, inst );
    if( einfo->wsb == NULL ) {
        return( false );
    }

    if( !WCreateStringEditWindow( einfo, inst ) ) {
        return( false );
    }

    hmenu2 = GetMenu( einfo->win );
    if( hmenu2 != (HMENU)NULL ) {
        EnableMenuItem( hmenu2, IDM_STR_CUT, MF_GRAYED );
        EnableMenuItem( hmenu2, IDM_STR_COPY, MF_GRAYED );
    }

    if( WGetOption( WOptScreenMax ) ) {
        ShowWindow( einfo->win, SW_SHOWMAXIMIZED );
    } else {
        ShowWindow( einfo->win, SW_SHOWNORMAL );
    }
    UpdateWindow( einfo->win );

    WResizeWindows( einfo );

    SetFocus( einfo->edit_dlg );

    return( true );
}

WStringEditInfo *WGetCurrentEditInfo( void )
{
    return( WCurrEditInfo );
}

void WSetCurrentEditInfo( WStringEditInfo *einfo )
{
    WCurrEditInfo = einfo;
}

HMENU WGetMenuHandle( WStringEditInfo *einfo )
{
    if( einfo == NULL ) {
        einfo = WGetCurrentEditInfo();
    }

    if( einfo != NULL && einfo->win != NULL ) {
        return( GetMenu( einfo->win ) );
    }

    return( NULL );
}

static void handleSymbols( WStringEditInfo *einfo )
{
    if( !WEditSymbols( einfo->win, &einfo->info->symbol_table,
                       WGetEditInstance(), WStrHelpRoutine ) ) {
        return;
    }

    WResolveStringTableSymIDs( einfo );

    WHandleSelChange( einfo );
}

static void handleLoadSymbols( WStringEditInfo *einfo )
{
    char        *file;
    LRESULT     pos;

    file = WLoadSymbols( &einfo->info->symbol_table,
                         einfo->info->symbol_file,
                         einfo->win, TRUE );
    if( file == NULL ) {
        return;
    }

    if( einfo->info->symbol_file != NULL ) {
        WRMemFree( einfo->info->symbol_file );
    }
    einfo->info->symbol_file = file;

    pos = einfo->current_pos;
    if( pos == LB_ERR ) {
        pos = 0;
    }

    // lookup the id associated with the symbol for all entries
    WResolveStringTableSymIDs( einfo );

    // look for the symbol matching the id for all entries
    WResolveStringTable( einfo );

    WInitEditWindowListBox( einfo );

    WRAddSymbolsToComboBox( einfo->info->symbol_table, einfo->edit_dlg, IDM_STREDCMDID, WR_HASHENTRY_ALL );

    SendDlgItemMessage( einfo->edit_dlg, IDM_STREDLIST, LB_SETCURSEL, (WPARAM)pos, 0 );

    einfo->info->modified = true;

    WDoHandleSelChange( einfo, FALSE, TRUE );
}

WINEXPORT LRESULT CALLBACK WMainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    HMENU           hmenu;
#if 0
    HWND            win;
#endif
    LRESULT         ret;
    bool            pass_to_def;
    WStringEditInfo *einfo;
    WORD            wp;
    MINMAXINFO      *minmax;
    about_info      ai;

    pass_to_def = true;
    ret = FALSE;
    einfo = (WStringEditInfo *)GET_WNDLONGPTR( hWnd, 0 );
    WSetCurrentEditInfo( einfo );

    switch( message ) {
    case WM_ACTIVATE:
        if( GET_WM_ACTIVATE_FACTIVE( wParam, lParam ) &&
            !GET_WM_ACTIVATE_FMINIMIZED( wParam, lParam ) &&
            einfo != NULL && einfo->edit_dlg != (HWND)NULL ) {
            SetFocus( einfo->edit_dlg );
            pass_to_def = false;
        }
        break;

    case WM_INITMENU:
        if( wParam == (WPARAM)GetMenu( hWnd ) ) {
            // set the cut and copy menu items
            if( SendDlgItemMessage( einfo->edit_dlg, IDM_STREDLIST, LB_GETCURSEL, 0, 0 ) != LB_ERR ) {
                EnableMenuItem( (HMENU)wParam, IDM_STR_CUT, MF_ENABLED );
                EnableMenuItem( (HMENU)wParam, IDM_STR_COPY, MF_ENABLED );
                EnableMenuItem( (HMENU)wParam, IDM_STR_MEM_FLAGS, MF_ENABLED );
            } else {
                EnableMenuItem( (HMENU)wParam, IDM_STR_CUT, MF_GRAYED );
                EnableMenuItem( (HMENU)wParam, IDM_STR_COPY, MF_GRAYED );
                EnableMenuItem( (HMENU)wParam, IDM_STR_MEM_FLAGS, MF_GRAYED );
            }
            // set the paste menu item
            if( OpenClipboard( hWnd ) ) {
                if( //IsClipboardFormatAvailable( WClipbdFormat ) ||
                    IsClipboardFormatAvailable( CF_TEXT ) ) {
                    EnableMenuItem( (HMENU)wParam, IDM_STR_PASTE, MF_ENABLED );
                } else {
                    EnableMenuItem( (HMENU)wParam, IDM_STR_PASTE, MF_GRAYED );
                }
                CloseClipboard();
            }
        }
        break;

    case WM_CREATE:
        einfo = ((CREATESTRUCT *)lParam)->lpCreateParams;
        SET_WNDLONGPTR( hWnd, 0, (LONG_PTR)einfo );
        break;

    case WM_MENUSELECT:
        if( einfo != NULL ) {
            hmenu = WGetMenuHandle( einfo );
            WHandleMenuSelect( einfo->wsb, hmenu, wParam, lParam );
        }
        break;

    case WM_GETMINMAXINFO:
        minmax = (MINMAXINFO *)lParam;
        minmax->ptMinTrackSize.x = appWidth;
        minmax->ptMinTrackSize.y = appHeight;
        break;

    case WM_MOVE:
        if( einfo != NULL ) {
            if( IsZoomed( hWnd ) ) {
                WSetOption( WOptScreenMax, TRUE );
            } else if( !IsIconic( hWnd ) ) {
                WUpdateScreenPosOpt( hWnd );
                WSetOption( WOptScreenMax, FALSE );
            }
        }
        break;

    case WM_SIZE:
        if( einfo != NULL ) {
            if( wParam == SIZE_MAXIMIZED ) {
                WSetOption( WOptScreenMax, TRUE );
            } else if( wParam != SIZE_MINIMIZED ) {
                WUpdateScreenPosOpt( hWnd );
                WSetOption( WOptScreenMax, FALSE );
            }
            WResizeWindows( einfo );
        }
        break;

    case WM_COMMAND:
        wp = LOWORD(wParam);
        switch( wp ) {
        case IDM_STR_CLEAR:
            WHandleClear( einfo );
            pass_to_def = false;
            break;

        case IDM_STR_UPDATE:
            SendMessage( einfo->info->parent, STRING_PLEASE_SAVEME, 0,
                         (LPARAM)einfo->hndl );
            pass_to_def = false;
            break;

        case IDM_STR_OPEN:
            pass_to_def = false;
            if( einfo->info->modified ) {
                ret = WQuerySave( einfo, FALSE );
                if( !ret ) {
                    break;
                }
            }
            ret = SendMessage( einfo->info->parent, STRING_PLEASE_OPENME, 0,
                               (LPARAM)einfo->hndl );
            ret = FALSE;
            break;

        case IDM_STR_SAVE:
            WSaveObject( einfo, FALSE, FALSE );
            pass_to_def = false;
            break;

        case IDM_STR_SAVEAS:
            WSaveObject( einfo, TRUE, FALSE );
            pass_to_def = false;
            break;

        case IDM_STR_SAVEINTO:
            WSaveObject( einfo, TRUE, TRUE );
            pass_to_def = false;
            break;

        case IDM_STR_EXIT:
            /* clean up before we exit */
            PostMessage( einfo->win, WM_CLOSE, 0, 0 );
            break;

        case IDM_STR_PASTE:
            WPasteStringItem( einfo );
            pass_to_def = false;
            break;

        case IDM_STR_CUT:
        case IDM_STR_COPY:
            WClipStringItem( einfo, wp == IDM_STR_CUT );
            pass_to_def = false;
            break;

        case IDM_STR_DELETE:
            WDeleteStringEntry( einfo );
            pass_to_def = false;
            break;

        case IDM_STR_NEWITEM:
            WInsertStringEntry( einfo );
            pass_to_def = false;
            break;

        case IDM_STR_SYMBOLS:
            handleSymbols( einfo );
            pass_to_def = false;
            break;

        case IDM_STR_LOAD_SYMBOLS:
            handleLoadSymbols( einfo );
            pass_to_def = false;
            break;

        case IDM_STR_SHOWRIBBON:
            hmenu = WGetMenuHandle( einfo );
            WShowRibbon( einfo, hmenu );
            pass_to_def = false;
            break;

        case IDM_STR_MEM_FLAGS:
            WHandleMemFlags( einfo );
            pass_to_def = false;
            break;

        case IDM_HELP:
            WStrHelpRoutine();
            pass_to_def = false;
            break;

        case IDM_HELP_SEARCH:
            WStrHelpSearchRoutine();
            pass_to_def = false;
            break;

        case IDM_HELP_ON_HELP:
            WStrHelpOnHelpRoutine();
            pass_to_def = false;
            break;

        case IDM_STR_ABOUT:
            ai.owner = hWnd;
            ai.inst = WGetEditInstance();
            ai.name = AllocRCString( W_ABOUT_NAME );
            ai.version = AllocRCString( W_ABOUT_VERSION );
            ai.title = AllocRCString( W_ABOUT_TITLE );
            DoAbout( &ai );
            FreeRCString( ai.name );
            FreeRCString( ai.version );
            FreeRCString( ai.title );
            pass_to_def = false;
            break;
        }
        break;

    case WM_DESTROY:
        WWinHelp( hWnd, "resstr.hlp", HELP_QUIT, 0 );
        WCleanup( einfo );
        break;

    case WM_CLOSE:
        ret = TRUE;
        pass_to_def = WHandleWM_CLOSE( einfo, wParam != 0 );
        wParam = 0;
        break;
    }

    if( pass_to_def ) {
        ret = DefWindowProc( hWnd, message, wParam, lParam );
    }

    return( ret );
}

bool WQuerySave( WStringEditInfo *einfo, bool force_exit )
{
    return( WQuerySaveRes( einfo, force_exit ) &&
            WQuerySaveSym( einfo, force_exit ) );
}

bool WQuerySaveRes( WStringEditInfo *einfo, bool force_exit )
{
    int         msg_ret;
    bool        ok;
    UINT        style;
    char        *title;
    char        *text;

    ok = true;

    if( einfo != NULL && einfo->info->modified ) {
        msg_ret = IDYES;
        if( einfo->info->stand_alone ) {
            if( force_exit ) {
                style = MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION;
            } else {
                style = MB_YESNOCANCEL | MB_APPLMODAL | MB_ICONEXCLAMATION;
            }
            title = WCreateEditTitle( einfo );
            text = AllocRCString( W_UPDATEMODIFIEDSTRING );
            msg_ret = MessageBox( einfo->edit_dlg, text, title, style );
            if( text != NULL ) {
                FreeRCString( text );
            }
            if( title != NULL ) {
                WRMemFree( title );
            }
        }
        if( msg_ret == IDYES ) {
            if( einfo->info->stand_alone ) {
                ok = WSaveObject( einfo, FALSE, FALSE );
            } else {
                SendMessage( einfo->info->parent, STRING_PLEASE_SAVEME, 0,
                             (LPARAM)einfo->hndl );
            }
        } else if( msg_ret == IDCANCEL ) {
            ok = false;
        }
    }

    return( ok );
}

bool WQuerySaveSym( WStringEditInfo *einfo, bool force_exit )
{
    int         ret;
    UINT        style;
    char        *title;
    char        *text;

    if( einfo == NULL || !einfo->info->stand_alone ) {
        return( true );
    }

    if( !WRIsHashTableDirty( einfo->info->symbol_table ) ) {
        return( true );
    }

    if( force_exit ) {
        style = MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION;
    } else {
        style = MB_YESNOCANCEL | MB_APPLMODAL | MB_ICONEXCLAMATION;
    }

    title = WCreateEditTitle( einfo );
    text = AllocRCString( W_UPDATEMODIFIEDSYM );
    ret = MessageBox( einfo->edit_dlg, text, title, style );
    if( text != NULL ) {
        FreeRCString( text );
    }
    if( title != NULL ) {
        WRMemFree( title );
    }

    if( ret == IDYES ) {
        if( einfo->info->symbol_file == NULL ) {
            char        *fname;
            if( einfo->file_name == NULL ) {
                fname = einfo->info->file_name;
            } else {
                fname = einfo->file_name;
            }
            einfo->info->symbol_file = WCreateSymFileName( fname );
        }
        return( WSaveSymbols( einfo, einfo->info->symbol_table,
                              &einfo->info->symbol_file, FALSE ) );
    } else if( ret == IDCANCEL ) {
        return( false );
    }

    return( true );
}

bool WHandleWM_CLOSE( WStringEditInfo *einfo, bool force_exit )
{
    bool        ok;

    ok = true;

    if( einfo != NULL ) {
        if( einfo->info->modified || WRIsHashTableDirty( einfo->info->symbol_table ) ) {
            ok = WQuerySave( einfo, force_exit );
        }
        if( ok ) {
            SendMessage( einfo->info->parent, STRING_I_HAVE_CLOSED, 0, (LPARAM)einfo->hndl );
            WUnRegisterEditSession( WGetEditSessionHandle( einfo ) );
        }
    }

    return( ok );
}

void WHandleMemFlags( WStringEditInfo *einfo )
{
    char        *rtext;
    char        *ntext;
    WResID      *rname;

    ntext = AllocRCString( W_STRINGNAMES );
    if( einfo != NULL && einfo->current_block != NULL && ntext != NULL ) {
        WSetStatusByID( einfo->wsb, W_CHANGESTRINGMEMFLAGS, 0 );
        // alloc space for ntext and two 16-bit ints
        rtext = (char *)WRMemAlloc( strlen( ntext ) + 20 );
        if( rtext != NULL ) {
            sprintf( rtext, ntext, einfo->current_block->blocknum & 0xfff0,
                     (einfo->current_block->blocknum & 0xfff0) + 16 - 1 );
        }
        rname = WResIDFromStr( rtext );
        if( rname != NULL ) {
            einfo->info->modified |= WChangeMemFlags( einfo->win,
                                                      &einfo->current_block->MemFlags,
                                                      rname, WGetEditInstance(),
                                                      WStrHelpRoutine );
            WRMemFree( rname );
        }
        FreeRCString( ntext );
        WSetStatusReadyText( einfo->wsb );
    }
}

static bool WQueryClearRes( WStringEditInfo *einfo )
{
    int         ret;
    UINT        style;
    char        *title;
    char        *text;

    if( einfo != NULL ) {
        style = MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION;
        text = AllocRCString( W_STRINGCLEARWARNING );
        title = AllocRCString( W_STRINGCLEARTITLE );
        ret = MessageBox( einfo->edit_dlg, text, title, style );
        if( text != NULL ) {
            FreeRCString( text );
        }
        if( title != NULL ) {
            FreeRCString( title );
        }
        if( ret == IDYES ) {
            return( true );
        }
    }

    return( false );
}

void WHandleClear( WStringEditInfo *einfo )
{
    if( einfo->tbl != NULL && einfo->tbl->first_block != NULL ) {
        if( WQueryClearRes( einfo ) ) {
            WResetEditWindow( einfo );
            SendDlgItemMessage( einfo->edit_dlg, IDM_STREDLIST, LB_RESETCONTENT, 0, 0 );
            WFreeStringTableBlocks( einfo->tbl->first_block );
            einfo->tbl->first_block = NULL;
            einfo->current_block = NULL;
            einfo->current_string = 0;
            einfo->current_pos = LB_ERR;
            if( einfo->info->stand_alone ) {
                if( einfo->file_name != NULL ) {
                    WRMemFree( einfo->file_name );
                    einfo->file_name = NULL;
                    WSetEditTitle( einfo );
                }
                if( einfo->info->symbol_table != NULL ) {
                    WRFreeHashTable( einfo->info->symbol_table );
                    einfo->info->symbol_table = WRInitHashTable();
                }
            }
            einfo->info->modified = true;
            SetFocus( einfo->edit_dlg );
            WSetStatusByID( einfo->wsb, W_STRINGCLEARMSG, 0 );
        }
    }
}

void WUpdateScreenPosOpt( HWND win )
{
    RECT    rect;

    GetWindowRect( win, &rect );

    WSetScreenPosOption( &rect );
}

void WResizeWindows( WStringEditInfo *einfo )
{
    RECT    rect;

    if( einfo == NULL ) {
        einfo = WGetCurrentEditInfo();
    }

    if( einfo != NULL && einfo->win != NULL ) {
        GetClientRect( einfo->win, &rect );
        WResizeStringEditWindow( einfo, &rect );
        WResizeStatusWindows( einfo->wsb, &rect );
        WResizeRibbon( einfo, &rect );
    }
}

bool WCleanup( WStringEditInfo *einfo )
{
    HWND        owner;
    bool        ok;

    ok = (einfo != NULL);

    if( ok ) {
        owner = (HWND)NULL;
        if( !einfo->info->stand_alone ) {
            owner = GetWindow( einfo->win, GW_OWNER );
        }
        einfo->win = (HWND)NULL;
        WFreeStringEInfo( einfo );
        if( owner != (HWND)NULL ) {
            BringWindowToTop( owner );
        }
    }

    return( ok );
}

WINEXPORT void CALLBACK WStrHelpRoutine( void )
{
    WStringEditInfo     *einfo;

    einfo = WGetCurrentEditInfo();
    if( einfo != NULL ) {
        if( !WHtmlHelp( einfo->win, "resstr.chm", HELP_CONTENTS, 0 ) ) {
            WWinHelp( einfo->win, "resstr.hlp", HELP_CONTENTS, 0 );
        }
    }
}

WINEXPORT void CALLBACK WStrHelpSearchRoutine( void )
{
    WStringEditInfo     *einfo;

    einfo = WGetCurrentEditInfo();
    if( einfo != NULL ) {
        if( !WHtmlHelp( einfo->win, "resstr.chm", HELP_PARTIALKEY, (HELP_DATA)(LPCSTR)"" ) ) {
            WWinHelp( einfo->win, "resstr.hlp", HELP_PARTIALKEY, (HELP_DATA)(LPCSTR)"" );
        }
    }
}

WINEXPORT void CALLBACK WStrHelpOnHelpRoutine( void )
{
    WStringEditInfo     *einfo;

    einfo = WGetCurrentEditInfo();
    if( einfo != NULL ) {
        WWinHelp( einfo->win, "winhelp.hlp", HELP_HELPONHELP, 0 );
    }
}
