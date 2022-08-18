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
#include "wio.h"
#include "wresall.h"
#include "wmenu.h"
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
#include "wprev.h"
#include "wmen2rc.h"
#include "wsetedit.h"
#include "wmain.h"
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
#define WTIMER          666
#define ABOUT_TIMER     WTIMER
#define CLOSE_TIMER     WTIMER
#define WMENU_MINTRACKX 580
#define WMENU_MINTRACKY 600

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/
WINEXPORT LRESULT CALLBACK WMainWndProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static bool         WInit( HINSTANCE );
static void         WFini( void );
static WMenuInfo    *WMenuGetEInfo( WMenuHandle, bool );
static bool         WRegisterMainClass( HINSTANCE );
static bool         WCreateEditWindow( HINSTANCE, WMenuEditInfo * );
static void         WUpdateScreenPosOpt( HWND );
static void         WDisplayAboutBox( HINSTANCE, HWND, UINT );
static bool         WCleanup( WMenuEditInfo * );
static bool         WQuerySave( WMenuEditInfo *, bool );
static bool         WQuerySaveRes( WMenuEditInfo *, bool );
static bool         WQuerySaveSym( WMenuEditInfo *, bool );
static bool         WHandleWM_CLOSE( WMenuEditInfo *, bool );
static void         WHandleClear( WMenuEditInfo * );
static void         WToggleInsertBitmap( WMenuEditInfo *einfo );
static void         WToggleInsertSubitems( WMenuEditInfo *einfo );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WMenuEditInfo    *WCurrEditInfo = NULL;
static char             WMainClass[] = "WMenuMainClass";
static char             WMainMenuName[] = "WMainMenu";
static char             WMainSOMenuName[] = "WSOMenu";
static char             WProfileName[_MAX_PATH] = WATCOM_INI;
static char             WSectionName[] = "wmenu";
static char             WItemClipbdFmt[] = "WMENU_ITEM_CLIPFMT";

static int      ref_count = 0;
static HACCEL   AccelTable = NULL;
static HBITMAP  InsertBefore = NULL;
static HBITMAP  InsertAfter = NULL;
static HBITMAP  InsertSub = NULL;
static HBITMAP  InsertNoSub = NULL;

UINT            WClipbdFormat = 0;
UINT            WItemClipbdFormat = 0;

extern int  appWidth;
extern int  appHeight;

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

void WRESEAPI WMenuInit( void )
{
    HINSTANCE   inst;

    inst = WGetEditInstance();
    if( AccelTable == (HACCEL)NULL ) {
        AccelTable = LoadAccelerators( inst, "WMenuAccelTable");
    }
    if( InsertBefore == (HBITMAP)NULL ) {
        InsertBefore = LoadBitmap( inst, "InsertBefore");
    }
    if( InsertAfter == (HBITMAP)NULL ) {
        InsertAfter = LoadBitmap( inst, "InsertAfter");
    }
    if( InsertSub == (HBITMAP)NULL ) {
        InsertSub = LoadBitmap( inst, "InsertSub");
    }
    if( InsertNoSub == (HBITMAP)NULL ) {
        InsertNoSub = LoadBitmap( inst, "InsertNoSub");
    }
    if( ref_count == 0 ) {
        WRInit();
        SetInstance( inst );
        WInit( inst );
        WInitDummyMenuEntry();
    }
    ref_count++;
}

void WRESEAPI WMenuFini( void )
{
    ref_count--;
    if( ref_count == 0 ) {
        WFiniDummyMenuEntry();
        WFini();
        WRFini();
    }
}

WMenuHandle WRESEAPI WRMenuStartEdit( WMenuInfo *info )
{
    bool            ok;
    WMenuEditInfo   *einfo;

    einfo = NULL;

    ok = (info != NULL && info->parent != NULL && info->inst != NULL);

    if( ok ) {
        if( appWidth == -1 ) {
            WInitEditDlg( WGetEditInstance(), info->parent );
        }
        ok = ((einfo = WAllocMenuEInfo()) != NULL);
    }

    if( ok ) {
        einfo->info = info;
        einfo->menu = WMakeMenuFromInfo( info );
        ok = (einfo->menu != NULL);
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
        ok = WResolveMenuEntries( einfo );
    }

    if( ok ) {
        ok = WCreateEditWindow( WGetEditInstance(), einfo );
    }

    if ( ok ) {
        einfo->hndl = WRegisterEditSession( einfo );
        ok = (einfo->hndl != 0);
    }

    if( !ok ) {
        if( einfo != NULL ) {
            WFreeMenuEInfo( einfo );
        }
        return( 0 );
    }

    return( einfo->hndl );
}

bool WRESEAPI WMenuIsModified( WMenuHandle hndl )
{
    WMenuEditInfo *einfo;

    einfo = (WMenuEditInfo *)WGetEditSessionInfo( hndl );

    return( einfo->info->modified );
}

void WRESEAPI WMenuShowWindow( WMenuHandle hndl, int show )
{
    WMenuEditInfo *einfo;

    einfo = (WMenuEditInfo *)WGetEditSessionInfo( hndl );

    if( einfo != NULL && einfo->win != (HWND)NULL ) {
        if( show ) {
            ShowWindow( einfo->win, SW_SHOWNA );
        } else {
            ShowWindow( einfo->win, SW_HIDE );
        }
    }
}

void WRESEAPI WMenuBringToFront( WMenuHandle hndl )
{
    WMenuEditInfo *einfo;

    einfo = (WMenuEditInfo *)WGetEditSessionInfo( hndl );

    if( einfo != NULL && einfo->win != (HWND)NULL ) {
        ShowWindow( einfo->win, SW_RESTORE );
        BringWindowToTop( einfo->win );
    }
}

bool WRESEAPI WMenuIsDlgMsg( MSG *msg )
{
    return( WIsMenuDialogMessage( msg, AccelTable ) );
}

WMenuInfo *WRESEAPI WMenuEndEdit( WMenuHandle hndl )
{
    return( WMenuGetEInfo( hndl, FALSE ) );
}

WMenuInfo *WRESEAPI WMenuGetEditInfo( WMenuHandle hndl )
{
    return( WMenuGetEInfo( hndl, TRUE ) );
}

int WRESEAPI WMenuCloseSession( WMenuHandle hndl, int force_exit )
{
    WMenuEditInfo *einfo;

    einfo = (WMenuEditInfo *)WGetEditSessionInfo( hndl );

    if( einfo != NULL && einfo->info != NULL ) {
        if( SendMessage( einfo->win, WM_CLOSE, (WPARAM)force_exit, 0 ) != 0 ) {
            return( FALSE );
        }
    }

    return( TRUE );
}

WMenuInfo *WMenuGetEInfo( WMenuHandle hndl, bool keep )
{
    WMenuEditInfo   *einfo;
    WMenuInfo       *info;
    bool            ok;

    info = NULL;

    einfo = (WMenuEditInfo *)WGetEditSessionInfo( hndl );

    ok = (einfo != NULL);

    if( ok ) {
        info = einfo->info;
        ok = (info != NULL);
    }

    if( ok ) {
        if( einfo->info->modified ) {
            if( info->data != NULL ) {
                WRMemFree( info->data );
                info->data = NULL;
            }
            info->data_size = 0;
            WMakeDataFromMenu( einfo->menu, &info->data, &info->data_size );
        }
        if( !keep ) {
            WUnRegisterEditSession( hndl );
            WFreeMenuEInfo( einfo );
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
        ok = WRegisterPrevClass( inst );
    }

    if( ok ) {
        ok = WInitStatusLines( inst );
    }

    if( ok ) {
        WClipbdFormat = RegisterClipboardFormat( WR_CLIPBD_MENU );
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
    WUnRegisterPrevClass( inst );

    if( InsertBefore != (HBITMAP)NULL ) {
        DeleteObject( InsertBefore );
        InsertBefore = (HBITMAP)NULL;
    }
    if( InsertAfter != (HBITMAP)NULL ) {
        DeleteObject( InsertAfter );
        InsertAfter = (HBITMAP)NULL;
    }
    if( InsertSub != (HBITMAP)NULL ) {
        DeleteObject( InsertSub );
        InsertSub = (HBITMAP)NULL;
    }
    if( InsertNoSub != (HBITMAP)NULL ) {
        DeleteObject( InsertNoSub );
        InsertNoSub = (HBITMAP)NULL;
    }
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

char *WCreateEditTitle( WMenuEditInfo *einfo )
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

    text = AllocRCString( W_MENUAPPTITLE );

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

void WSetEditTitle( WMenuEditInfo *einfo )
{
    char        *title;
    bool        is_rc;

    title = WCreateEditTitle( einfo );
    is_rc = false;

    if( title == NULL ) {
        title = AllocRCString( W_MENUAPPTITLE );
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

bool WCreateEditWindow( HINSTANCE inst, WMenuEditInfo *einfo )
{
    int         x, y, width, height;
    char        *title;
    HMENU       hmenu2;
    HMENU       hmenu1;
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
        title = AllocRCString( W_MENUAPPTITLE );
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

    einfo->insert_subitems = FALSE;
    einfo->insert_before = FALSE;
    einfo->first_preview_id = FIRST_PREVIEW_ID;

    hmenu2 = GetMenu( einfo->win );
    if( hmenu2 != (HMENU)NULL ) {
        EnableMenuItem( hmenu2, IDM_MENU_CUT, MF_GRAYED );
        EnableMenuItem( hmenu2, IDM_MENU_COPY, MF_GRAYED );
    }

    CheckMenuItem( hmenu2, IDM_MENU_INSERTSUBITEMS, MF_UNCHECKED );
    CheckMenuItem( hmenu2, IDM_MENU_INSERTAFTER, MF_CHECKED );

    if( !WCreateMenuEditWindow( einfo, inst ) ) {
        return( false );
    }

    if( WGetOption( WOptScreenMax ) ) {
        ShowWindow( einfo->win, SW_SHOWMAXIMIZED );
    } else {
        ShowWindow( einfo->win, SW_SHOWNORMAL );
    }
    UpdateWindow( einfo->win );

    WResizeWindows( einfo );

    SetFocus( einfo->edit_dlg );

    return( TRUE );
}

WMenuEditInfo *WGetCurrentEditInfo( void )
{
    return( WCurrEditInfo );
}

void WSetCurrentEditInfo( WMenuEditInfo *einfo )
{
    WCurrEditInfo = einfo;
}

HMENU WGetMenuHandle( WMenuEditInfo *einfo )
{
    if( einfo == NULL ) {
        einfo = WGetCurrentEditInfo();
    }

    if( einfo != NULL && einfo->win != NULL ) {
        return( GetMenu( einfo->win ) );
    }

    return( NULL );
}

static void handleSymbols( WMenuEditInfo *einfo )
{
    char        *text;

    if( !WEditSymbols( einfo->win, &einfo->info->symbol_table,
                       WGetEditInstance(), WMenuHelpRoutine ) ) {
        return;
    }

    WResolveMenuSymIDs( einfo );

    text = WGetStrFromEdit( GetDlgItem( einfo->edit_dlg, IDM_MENUEDID ), NULL );
    WRAddSymbolsToComboBox( einfo->info->symbol_table, einfo->edit_dlg,
                            IDM_MENUEDID, WR_HASHENTRY_ALL );
    if( text != NULL ) {
        WSetEditWithStr( GetDlgItem( einfo->edit_dlg, IDM_MENUEDID ), text );
        WRMemFree( text );
    }

    WHandleSelChange( einfo );
}

static void handleLoadSymbols( WMenuEditInfo *einfo )
{
    char        *file;

    file = WLoadSymbols( &einfo->info->symbol_table, einfo->info->symbol_file,
                         einfo->win, TRUE );
    if( file == NULL ) {
        return;
    }

    if( einfo->info->symbol_file != NULL ) {
        WRMemFree( einfo->info->symbol_file );
    }
    einfo->info->symbol_file = file;

    // lookup the id associated with the symbol for all entries
    WResolveMenuSymIDs( einfo );

    // look for the symbol matching the id for all entries
    WResolveMenuEntries( einfo );

    WRAddSymbolsToComboBox( einfo->info->symbol_table, einfo->edit_dlg,
                            IDM_MENUEDID, WR_HASHENTRY_ALL );

    einfo->info->modified = true;

    WDoHandleSelChange( einfo, FALSE, TRUE );
}

WINEXPORT LRESULT CALLBACK WMainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    HMENU               hmenu;
#if 0
    HWND                win;
#endif
    LRESULT             ret;
    bool                pass_to_def;
    WMenuEditInfo       *einfo;
    WORD                wp;
    MINMAXINFO          *minmax;
    about_info          ai;

    pass_to_def = true;
    ret = FALSE;
    einfo = (WMenuEditInfo *)GET_WNDLONGPTR( hWnd, 0 );
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
            if( SendDlgItemMessage( einfo->edit_dlg, IDM_MENUEDLIST, LB_GETCURSEL, 0, 0 ) != LB_ERR ) {
                EnableMenuItem( (HMENU)wParam, IDM_MENU_CUT, MF_ENABLED );
                EnableMenuItem( (HMENU)wParam, IDM_MENU_COPY, MF_ENABLED );
            } else {
                EnableMenuItem( (HMENU)wParam, IDM_MENU_CUT, MF_GRAYED );
                EnableMenuItem( (HMENU)wParam, IDM_MENU_COPY, MF_GRAYED );
            }
            // set the paste menu item
            if( OpenClipboard( hWnd ) ) {
                if( //IsClipboardFormatAvailable( WClipbdFormat ) ||
                    IsClipboardFormatAvailable( WItemClipbdFormat ) ) {
                    EnableMenuItem( (HMENU)wParam, IDM_MENU_PASTE, MF_ENABLED );
                } else {
                    EnableMenuItem( (HMENU)wParam, IDM_MENU_PASTE, MF_GRAYED );
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
            WMovePrevWindow( einfo );
        }
        break;

    case WM_SETFOCUS:
        if( einfo != NULL && einfo->preview_window != (HWND)NULL ) {
            SendMessage( einfo->preview_window, WM_NCACTIVATE, (WPARAM)TRUE, (LPARAM)NULL );
        }
        break;

    case WM_SHOWWINDOW:
        if( wParam ) {
            ShowWindow( einfo->preview_window, SW_SHOWNA );
        } else {
            ShowWindow( einfo->preview_window, SW_HIDE );
        }
        break;

    case WM_SIZE:
        if( einfo != NULL ) {
            if( wParam == SIZE_MAXIMIZED ) {
                WSetOption( WOptScreenMax, TRUE );
            } else if ( wParam != SIZE_MINIMIZED ) {
                WUpdateScreenPosOpt( hWnd );
                WSetOption( WOptScreenMax, FALSE );
            }
            if( einfo->preview_window != (HWND)NULL ) {
                if( wParam == SIZE_MINIMIZED ) {
                    ShowWindow( einfo->preview_window, SW_HIDE );
                } else {
                    ShowWindow( einfo->preview_window, SW_SHOWNA );
                }
            }
            if( wParam != SIZE_MINIMIZED ) {
                WResizeWindows( einfo );
            }
        }
        break;

#if 0
    case WM_ACTIVATE:
        if( GET_WM_ACTIVATE_FACTIVE( wParam, lParam ) != WA_INACTIVE ) {
            win = GET_WM_ACTIVATE_HWND( wParam, lParam );
            einfo = (WMenuEditInfo *)GET_WNDLONGPTR( win, 0 );
            WSetCurrentEditInfo( einfo );
        } else {
            WSetCurrentEditInfo( NULL );
        }
        break;
#endif

    case WM_COMMAND:
        wp = LOWORD( wParam );
        switch( wp ) {
        case IDM_MENU_CLEAR:
            WHandleClear( einfo );
            pass_to_def = false;
            break;

        case IDM_MENU_UPDATE:
            SendMessage( einfo->info->parent, MENU_PLEASE_SAVEME, 0,
                         (LPARAM)einfo->hndl );
            pass_to_def = false;
            break;

        case IDM_MENU_OPEN:
            pass_to_def = false;
            if( einfo->info->modified ) {
                ret = WQuerySave( einfo, FALSE );
                if( !ret ) {
                    break;
                }
            }
            ret = SendMessage( einfo->info->parent, MENU_PLEASE_OPENME, 0,
                               (LPARAM)einfo->hndl );
            ret = FALSE;
            break;

        case IDM_MENU_SAVE:
            WSaveObject( einfo, FALSE, FALSE );
            pass_to_def = false;
            break;

        case IDM_MENU_SAVEAS:
            WSaveObject( einfo, TRUE, FALSE );
            pass_to_def = false;
            break;

        case IDM_MENU_SAVEINTO:
            WSaveObject( einfo, TRUE, TRUE );
            pass_to_def = false;
            break;

        case IDM_MENU_EXIT:
            /* clean up before we exit */
            PostMessage( einfo->win, WM_CLOSE, 0, 0 );
            break;

        case IDM_MENU_PASTE:
            WPasteMenuItem( einfo );
            pass_to_def = false;
            break;

        case IDM_MENU_CUT:
        case IDM_MENU_COPY:
            WClipMenuItem( einfo, wp == IDM_MENU_CUT );
            pass_to_def = false;
            break;

        case IDM_MENU_DELETE:
            WDeleteMenuEntry( einfo );
            pass_to_def = false;
            break;

        case IDM_MENU_INSERTSUBITEMS:
            WToggleInsertSubitems( einfo );
            break;

        case IDM_MENU_INSERTBEFORE:
            if( !einfo->insert_before ) {
                WToggleInsertBitmap( einfo );
            }
            break;

        case IDM_MENU_INSERTAFTER:
            if( einfo->insert_before ) {
                WToggleInsertBitmap( einfo );
            }
            break;

        case IDM_MENU_INSERTTOGGLE:
            WToggleInsertBitmap( einfo );
            break;

#if 0
        case IDM_MENU_NEWITEM:
            WInsertNewMenuEntry( einfo, FALSE, FALSE );
            pass_to_def = false;
            break;

        case IDM_MENU_NEWPOPUP:
            WInsertNewMenuEntry( einfo, TRUE, FALSE );
            pass_to_def = false;
            break;

        case IDM_MENU_NEWSEPARATOR:
            WInsertNewMenuEntry( einfo, FALSE, TRUE );
            pass_to_def = false;
            break;
#else
        case IDM_MENU_NEWITEM:
            WInsertNew( einfo );
            pass_to_def = false;
            break;
#endif

        case IDM_MENU_SYMBOLS:
            handleSymbols( einfo );
            pass_to_def = false;
            break;

        case IDM_MENU_LOAD_SYMBOLS:
            handleLoadSymbols( einfo );
            pass_to_def = false;
            break;

        case IDM_MENU_SHOWRIBBON:
            hmenu = WGetMenuHandle( einfo );
            WShowRibbon( einfo, hmenu );
            pass_to_def = false;
            break;

        case IDM_MENU_MEM_FLAGS:
            WSetStatusByID( einfo->wsb, W_CHANGEMENUMEMFLAGS, 0 );
            einfo->info->modified |= WChangeMemFlags( einfo->win, &einfo->info->MemFlags,
                                                      einfo->info->res_name,
                                                      WGetEditInstance(),
                                                      WMenuHelpRoutine );
            WSetStatusReadyText( einfo->wsb );
            pass_to_def = false;
            break;

        case IDM_MENU_RENAME:
            WHandleRename( einfo );
            pass_to_def = false;
            break;

        case IDM_HELP:
            WMenuHelpRoutine();
            pass_to_def = false;
            break;

        case IDM_HELP_SEARCH:
            WMenuHelpSearchRoutine();
            pass_to_def = false;
            break;

        case IDM_HELP_ON_HELP:
            WMenuHelpOnHelpRoutine();
            pass_to_def = false;
            break;

        case IDM_MENU_ABOUT:
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
        WWinHelp( hWnd, "resmnu.hlp", HELP_QUIT, 0 );
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

void WToggleInsertSubitems( WMenuEditInfo *einfo )
{
    HMENU       hmenu;

    einfo->insert_subitems = !einfo->insert_subitems;

    if( einfo->insert_subitems ) {
        ChangeToolButtonBitmap( einfo->ribbon->tbar, IDM_MENU_INSERTSUBITEMS, InsertSub );
    } else {
        ChangeToolButtonBitmap( einfo->ribbon->tbar, IDM_MENU_INSERTSUBITEMS, InsertNoSub );
    }

    hmenu = WGetMenuHandle( einfo );
    if( hmenu == (HMENU)NULL ) {
        return;
    }

    if( einfo->insert_subitems ) {
        CheckMenuItem( hmenu, IDM_MENU_INSERTSUBITEMS, MF_CHECKED );
    } else {
        CheckMenuItem( hmenu, IDM_MENU_INSERTSUBITEMS, MF_UNCHECKED );
    }
}

void WToggleInsertBitmap( WMenuEditInfo *einfo )
{
    HMENU       hmenu;

    einfo->insert_before = !einfo->insert_before;

    if( einfo->insert_before ) {
        ChangeToolButtonBitmap( einfo->ribbon->tbar, IDM_MENU_INSERTTOGGLE, InsertBefore );
    } else {
        ChangeToolButtonBitmap( einfo->ribbon->tbar, IDM_MENU_INSERTTOGGLE, InsertAfter );
    }

    hmenu = WGetMenuHandle( einfo );
    if( hmenu == (HMENU)NULL ) {
        return;
    }

    if( einfo->insert_before ) {
        CheckMenuItem( hmenu, IDM_MENU_INSERTBEFORE, MF_CHECKED );
        CheckMenuItem( hmenu, IDM_MENU_INSERTAFTER, MF_UNCHECKED );
    } else {
        CheckMenuItem( hmenu, IDM_MENU_INSERTBEFORE, MF_UNCHECKED );
        CheckMenuItem( hmenu, IDM_MENU_INSERTAFTER, MF_CHECKED );
    }
}

bool WQuerySave( WMenuEditInfo *einfo, bool force_exit )
{
    return( WQuerySaveRes( einfo, force_exit ) && WQuerySaveSym( einfo, force_exit ) );
}

bool WQuerySaveRes( WMenuEditInfo *einfo, bool force_exit )
{
    int         ret;
    UINT        style;
    char        *title;
    char        *text;

    if( einfo != NULL && einfo->info->modified ) {
        ret = IDYES;
        if( einfo->info->stand_alone ) {
            if( force_exit ) {
                style = MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION;
            } else {
                style = MB_YESNOCANCEL | MB_APPLMODAL | MB_ICONEXCLAMATION;
            }
            title = WCreateEditTitle( einfo );
            text = AllocRCString( W_UPDATEMODIFIEDMENU );
            ret = MessageBox( einfo->edit_dlg, text, title, style );
            if( text != NULL ) {
                FreeRCString( text );
            }
            if( title != NULL ) {
                WRMemFree( title );
            }
        }
        if( ret == IDYES ) {
            if( einfo->info->stand_alone ) {
                return( WSaveObject( einfo, FALSE, FALSE ) );
            } else {
                SendMessage( einfo->info->parent, MENU_PLEASE_SAVEME, 0,
                             (LPARAM)einfo->hndl );
            }
        } else if( ret == IDCANCEL ) {
            return( FALSE );
        }
    }

    return( TRUE );
}

bool WQuerySaveSym( WMenuEditInfo *einfo, bool force_exit )
{
    int         ret;
    UINT        style;
    char        *title;
    char        *text;

    if( einfo == NULL || !einfo->info->stand_alone ) {
        return( TRUE );
    }

    if( !WRIsHashTableDirty( einfo->info->symbol_table ) ) {
        return( TRUE );
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
        return( FALSE );
    }

    return( TRUE );
}

bool WHandleWM_CLOSE( WMenuEditInfo *einfo, bool force_exit )
{
    bool        ret;

    ret = TRUE;

    if( einfo != NULL ) {
        if( einfo->info->modified || WRIsHashTableDirty( einfo->info->symbol_table ) ) {
            ret = WQuerySave( einfo, force_exit );
        }
        if( ret ) {
            SendMessage( einfo->info->parent, MENU_I_HAVE_CLOSED, 0, (LPARAM)einfo->hndl );
            WUnRegisterEditSession( WGetEditSessionHandle( einfo ) );
        }
    }

    return( ret );
}

void WHandleRename( WMenuEditInfo *einfo )
{
    if( einfo != NULL ) {
        WSetStatusByID( einfo->wsb, W_RENAMINGMENU, 0 );
        einfo->info->modified |= WRenameResource( einfo->win, &einfo->info->res_name,
                                                  WMenuHelpRoutine );
        WSetEditWinResName( einfo );
        WSetStatusReadyText( einfo->wsb );
    }
}

static bool WQueryClearRes( WMenuEditInfo *einfo )
{
    int         ret;
    UINT        style;
    char        *title;
    char        *text;

    if( einfo != NULL ) {
        style = MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION;
        text = AllocRCString( W_MENUCLEARWARNING );
        title = AllocRCString( W_MENUCLEARTITLE );
        ret = MessageBox( einfo->edit_dlg, text, title, style );
        if( text != NULL ) {
            FreeRCString( text );
        }
        if( title != NULL ) {
            FreeRCString( title );
        }
        if( ret == IDYES ) {
            return( TRUE );
        }
    }

    return( FALSE );
}

void WHandleClear( WMenuEditInfo *einfo )
{
    if( einfo->menu != NULL && einfo->menu->first_entry != NULL ) {
        if( WQueryClearRes( einfo ) ) {
            WSetEditWindowText( einfo->edit_dlg, 0, NULL );
            WSetEditWindowID( einfo->edit_dlg, 0, TRUE, NULL );
            WResetEditWindowFlags( einfo->edit_dlg );
            SendDlgItemMessage( einfo->edit_dlg, IDM_MENUEDLIST, LB_RESETCONTENT, 0, 0 );
            WFreeMenuEntries( einfo->menu->first_entry );
            einfo->menu->first_entry = NULL;
            einfo->current_entry = NULL;
            einfo->current_pos = LB_ERR;
            einfo->first_preview_id = FIRST_PREVIEW_ID;
            WResetPrevWindowMenu( einfo );
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
            WSetStatusByID( einfo->wsb, W_MENUCLEARMSG, 0 );
        }
    }
}

void WUpdateScreenPosOpt( HWND win )
{
    RECT rect;

    GetWindowRect( win, &rect );

    WSetScreenPosOption( &rect );
}

void WResizeWindows( WMenuEditInfo *einfo )
{
    RECT rect;

    if( einfo == NULL ) {
        einfo = WGetCurrentEditInfo();
    }

    if( einfo != NULL && einfo->win != NULL ) {
        GetClientRect( einfo->win, &rect );
        WResizeMenuEditWindow( einfo, &rect );
        WResizeStatusWindows( einfo->wsb, &rect );
        WResizeRibbon( einfo, &rect );
    }
}

bool WCleanup( WMenuEditInfo *einfo )
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
        WFreeMenuEInfo( einfo );
        if( owner != (HWND)NULL ) {
            BringWindowToTop( owner );
        }
    }

    return( ok );
}

WINEXPORT void CALLBACK WMenuHelpRoutine( void )
{
    WMenuEditInfo       *einfo;

    einfo = WGetCurrentEditInfo();
    if( einfo != NULL ) {
        if( !WHtmlHelp( einfo->win, "resmnu.chm", HELP_CONTENTS, 0 ) ) {
            WWinHelp( einfo->win, "resmnu.hlp", HELP_CONTENTS, 0 );
        }
    }
}

WINEXPORT void CALLBACK WMenuHelpSearchRoutine( void )
{
    WMenuEditInfo       *einfo;

    einfo = WGetCurrentEditInfo();
    if( einfo != NULL ) {
        if( !WHtmlHelp( einfo->win, "resmnu.chm", HELP_PARTIALKEY, (HELP_DATA)(LPCSTR)"" ) ) {
            WWinHelp( einfo->win, "resmnu.hlp", HELP_PARTIALKEY, (HELP_DATA)(LPCSTR)"" );
        }
    }
}

WINEXPORT void CALLBACK WMenuHelpOnHelpRoutine( void )
{
    WMenuEditInfo       *einfo;

    einfo = WGetCurrentEditInfo();
    if( einfo != NULL ) {
        WWinHelp( einfo->win, "winhelp.hlp", HELP_HELPONHELP, 0 );
    }
}
