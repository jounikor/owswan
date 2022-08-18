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
#include <ctype.h>
#include "wribbon.h"
#include "wmain.h"
#include "wmsg.h"
#include "wstat.h"
#include "wnewitem.h"
#include "wdel.h"
#include "wvk2str.h"
#include "wedit.h"
#include "wctl3d.h"
#include "wsetedit.h"
#include "wclip.h"
#include "sysall.rh"
#include "ldstr.h"
#include "jdlg.h"
#include "wclbproc.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define WEDIT_PAD 4

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/
WINEXPORT INT_PTR CALLBACK WAcccelEditDlgProc( HWND, UINT, WPARAM, LPARAM );
WINEXPORT INT_PTR CALLBACK WTestDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static bool WInitEditWindow( WAccelEditInfo * );
static void WExpandEditWindowItem( HWND, int, RECT * );

static bool WSetEditWindowKey( HWND, uint_16, uint_16 );
static bool WSetEditWindowID( HWND, char *, uint_16 );
static bool WSetEditWindowFlags( HWND, uint_16 );

static bool WGetEditWindowKey( HWND, uint_16 *, uint_16 *, bool * );
static bool WGetEditWindowID( HWND dlg, char **symbol, uint_16 *id, WRHashTable *symbol_table, bool combo_change );
static bool WGetEditWindowFlags( HWND, uint_16 * );

/****************************************************************************/
/* external variables                                                       */
/****************************************************************************/
extern UINT             WClipbdFormat;
extern UINT             WItemClipbdFormat;
extern WAccelEntry      DefaultEntry = { FALSE, { ACCEL_ASCII, 'A', 101 }, NULL, NULL, NULL };

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static DLGPROC  WAccelEditWinProc = NULL;
static HBRUSH   WEditWinBrush     = NULL;
static COLORREF WEditWinColor     = 0;

int appWidth = -1;
int appHeight = -1;

void WInitEditWindows( HINSTANCE inst )
{
    WEditWinColor = GetSysColor( COLOR_BTNFACE );
    WEditWinBrush = CreateSolidBrush( WEditWinColor );
    WAccelEditWinProc = MakeProcInstance_DLG( WAcccelEditDlgProc, inst );
}

void WFiniEditWindows( void )
{
    if( WEditWinBrush != NULL ) {
        DeleteObject( WEditWinBrush );
    }
    FreeProcInstance_DLG( WAccelEditWinProc );
}


bool WCreateAccelEditWindow( WAccelEditInfo *einfo, HINSTANCE inst )
{
    int tabstop;

    einfo->edit_dlg = JCreateDialogParam( inst, "WAccelEditDLG", einfo->win,
                                          WAccelEditWinProc, (LPARAM)einfo );

    if( einfo->edit_dlg == (HWND)NULL ) {
        return( false );
    }

    tabstop = 85;
    SendDlgItemMessage( einfo->edit_dlg, IDM_ACCEDLIST, LB_SETTABSTOPS,
                        (WPARAM)1, (LPARAM)&tabstop );

    SetWindowPos( einfo->edit_dlg, (HWND)NULL, 0, WGetRibbonHeight(), 0, 0,
                  SWP_NOSIZE | SWP_NOZORDER );

    return( WInitEditWindow( einfo ) );
}

bool WResizeAccelEditWindow( WAccelEditInfo *einfo, RECT *prect )
{
    int   width, height, ribbon_depth;
    HWND  win;
    RECT  crect;

    if( einfo == NULL || einfo->edit_dlg == NULL || prect == NULL ) {
        return( false );
    }

    if( einfo->show_ribbon ) {
        ribbon_depth = WGetRibbonHeight();
    } else {
        ribbon_depth = 0;
    }

    width = prect->right - prect->left;
    height = prect->bottom - prect->top - ribbon_depth - WGetStatusDepth();

    /* change the size of the divider */
    win = GetDlgItem( einfo->edit_dlg, IDM_ACCEDBLACKLINE );
    GetWindowRect( win, &crect );
    SetWindowPos( win, (HWND)NULL, 0, 0, width, crect.bottom - crect.top,
                  SWP_NOMOVE | SWP_NOZORDER );

    /* change the size of the resource name edit field */
    WExpandEditWindowItem( einfo->edit_dlg, IDM_ACCEDRNAME, prect );

    /* change the size of the listbox */
    WExpandEditWindowItem( einfo->edit_dlg, IDM_ACCEDLIST, prect );

    SetWindowPos( einfo->edit_dlg, (HWND)NULL, 0, ribbon_depth,
                  width, height, SWP_NOZORDER );

    return( true );
}

static void WExpandEditWindowItem( HWND hDlg, int id, RECT *prect )
{
    HWND    win;
    RECT    crect, t;

    /* expand the child window */
    win = GetDlgItem( hDlg, id );
    GetWindowRect( win, &crect );
    MapWindowPoints( (HWND)NULL, hDlg, (POINT *)&crect, 2 );
    t.left = 0;
    t.top = 0;
    t.right = 0;
    t.bottom = WEDIT_PAD;
    MapDialogRect( hDlg, &t );
    SetWindowPos( win, (HWND)NULL, 0, 0, prect->right - crect.left - t.bottom,
                  crect.bottom - crect.top, SWP_NOMOVE | SWP_NOZORDER );
    InvalidateRect( win, NULL, TRUE );
}

void WResetEditWindow( WAccelEditInfo *einfo )
{
    if( einfo != NULL ) {
        WSetEditWithStr( GetDlgItem( einfo->edit_dlg, IDM_ACCEDKEY ), "" );
        WSetEditWithStr( GetDlgItem( einfo->edit_dlg, IDM_ACCEDCMDID ), "" );
        WSetEditWithStr( GetDlgItem( einfo->edit_dlg, IDM_ACCEDCMDNUM ), "" );
        CheckDlgButton( einfo->edit_dlg, IDM_ACCEDVIRT, BST_UNCHECKED );
        CheckDlgButton( einfo->edit_dlg, IDM_ACCEDASCII, BST_CHECKED );
        CheckDlgButton( einfo->edit_dlg, IDM_ACCEDCNTL, BST_UNCHECKED );
        CheckDlgButton( einfo->edit_dlg, IDM_ACCEDSHFT, BST_UNCHECKED );
        CheckDlgButton( einfo->edit_dlg, IDM_ACCEDALT, BST_UNCHECKED );
        CheckDlgButton( einfo->edit_dlg, IDM_ACCEDFLASH, BST_UNCHECKED );
    }
}

bool WSetEditWindowKeyEntry( WAccelEditInfo *einfo, WAccelEntry *entry )
{
    bool    ok;
    uint_16 key, flags, id;

    ok = (einfo != NULL && einfo->edit_dlg != NULL && entry != NULL);

    if( ok ) {
        if( entry->is32bit ) {
            key = entry->u.entry32.Ascii;
            flags = entry->u.entry32.Flags;
            id = entry->u.entry32.Id;
        } else {
            key = entry->u.entry.Ascii;
            flags = entry->u.entry.Flags;
            id = (uint_16)entry->u.entry.Id;
        }
        ok = WSetEditWindowKey( einfo->edit_dlg, key, flags );
        if( !ok ) {
            WSetStatusByID( einfo->wsb, 0, W_INVALIDACCEL );
            memcpy( entry, &DefaultEntry, sizeof( WAccelEntry ) );
            key = entry->u.entry.Ascii;
            flags = entry->u.entry.Flags;
            id = (uint_16)entry->u.entry.Id;
            ok = WSetEditWindowKey( einfo->edit_dlg, key, flags );
        }
    }

    if( ok ) {
        ok = WSetEditWindowID( einfo->edit_dlg, entry->symbol, id );
    }

    if( ok ) {
        ok = WSetEditWindowFlags( einfo->edit_dlg, flags );
    }

    return( ok );
}

bool WGetEditWindowKeyEntry( WAccelEditInfo *einfo, WAccelEntry *entry, bool check_mod )
{
    bool        ok;
    bool        force_ascii;
    uint_16     key, flags, id;
    char        *symbol;

    symbol = NULL;
    flags = 0;
    force_ascii = false;

    ok = (einfo != NULL && einfo->edit_dlg != NULL && entry != NULL);

    if( ok ) {
        ok = WGetEditWindowFlags( einfo->edit_dlg, &flags );
    }

    if( ok ) {
        ok = WGetEditWindowKey( einfo->edit_dlg, &key, &flags, &force_ascii );
        if( !ok ) {
            WSetStatusByID( einfo->wsb, 0, W_INVALIDACCELKEY );
        }
    }

    if( ok ) {
        ok = WGetEditWindowID( einfo->edit_dlg, &symbol, &id,
                               einfo->info->symbol_table, einfo->combo_change );
    }

    if( ok ) {
        if( force_ascii ) {
            flags &= ~(ACCEL_VIRTKEY | ACCEL_SHIFT | ACCEL_CONTROL | ACCEL_ALT);
        }
    }

    /* check if anything was actually modified */
    if( ok ) {
        // make sure the symbol info did not change
        ok = (entry->symbol == NULL && symbol != NULL) ||
             (entry->symbol != NULL && symbol == NULL);
        if( !ok ) {
            ok = (symbol != NULL && stricmp( entry->symbol, symbol ));
            if( !ok ) {
                if( entry->is32bit ) {
                    ok = (entry->u.entry32.Ascii != key || entry->u.entry32.Flags != flags ||
                          entry->u.entry32.Id != id);
                } else {
                    ok = (entry->u.entry.Ascii != key || entry->u.entry.Flags != (uint_8)flags ||
                          entry->u.entry.Id != id);
                }
            }
        }
        if( check_mod ) {
            return( ok );
        }
    }

    if( ok ) {
        if( entry->is32bit ) {
            entry->u.entry32.Ascii = key;
            entry->u.entry32.Flags = flags;
            entry->u.entry32.Id = id;
        } else {
            entry->u.entry.Ascii = key;
            entry->u.entry.Flags = (uint_8)flags;
            entry->u.entry.Id = id;
        }
        if( entry->symbol != NULL ) {
            WRMemFree( entry->symbol );
        }
        entry->symbol = symbol;
    } else {
        if( symbol != NULL ) {
            WRMemFree( symbol );
        }
    }

    return( ok );
}

static bool WSetEditWindowKey( HWND dlg, uint_16 key, uint_16 flags )
{
    char        *text;
    HWND        edit;
    bool        ok;

    ok = (dlg != (HWND)NULL);

    if( ok ) {
        text = WGetKeyText( key, flags );
        ok = (text != NULL);
    }

    if( ok ) {
        edit = GetDlgItem( dlg, IDM_ACCEDKEY );
        ok = WSetEditWithStr( edit, text );
    }

    return( ok );
}

bool WGetEditWindowKey( HWND dlg, uint_16 *key, uint_16 *flags, bool *force_ascii )
{
    bool    ok;
    char    *text;

    text = NULL;

    ok = (dlg != (HWND)NULL && key != NULL && flags != NULL && force_ascii != NULL);

    if( ok ) {
        text = WGetStrFromEdit( GetDlgItem( dlg, IDM_ACCEDKEY ), NULL );
        ok = (text != NULL);
    }

    if( ok ) {
        ok = WGetKeyFromText( text, key, flags, force_ascii );
    }

    if( text != NULL ) {
        WRMemFree( text );
    }

    return( ok );
}

bool WSetEditWindowID( HWND dlg, char *symbol, uint_16 id )
{
    bool ok;

    ok = (dlg != (HWND)NULL);

    if( ok ) {
        if( symbol != NULL ) {
            ok = WSetEditWithStr( GetDlgItem( dlg, IDM_ACCEDCMDID ), symbol );
        } else {
            ok = WSetEditWithSINT32( GetDlgItem( dlg, IDM_ACCEDCMDID ), (int_32)id, 10 );
        }
    }

    if( ok ) {
        ok = WSetEditWithSINT32( GetDlgItem( dlg, IDM_ACCEDCMDNUM ), (int_32)id, 10 );
    }

    return( ok );
}

bool WGetEditWindowID( HWND dlg, char **symbol, uint_16 *id,
                       WRHashTable *symbol_table, bool combo_change )
{
    int_32      val;
    char        *ep;
    WRHashValue hv;
    WRHashEntry *new_entry;
    bool        dup;

    if( dlg == (HWND)NULL ) {
        return( false );
    }

    if( combo_change ) {
        *symbol = WGetStrFromComboLBox( GetDlgItem( dlg, IDM_ACCEDCMDID ), CB_ERR );
    } else {
        *symbol = WGetStrFromEdit( GetDlgItem( dlg, IDM_ACCEDCMDID ), NULL );
    }

    if( *symbol == NULL ) {
        return( false );
    }

    if( **symbol == '\0' ) {
        *symbol = WGetStrFromEdit( GetDlgItem( dlg, IDM_ACCEDCMDNUM ), NULL );
    }

    if( *symbol == NULL ) {
        return( false );
    }

    strupr( *symbol );

    // check if the string has a numeric representation
    val = (int_32)strtol( *symbol, &ep, 0 );
    if( *ep != '\0' ) {
        // the string did not have a numeric representation
        // so let's look it up in the hash table
        if( WRLookupName( symbol_table, *symbol, &hv ) ) {
            *id = (uint_16)hv;
        } else {
            dup = false;
            new_entry = WRAddDefHashEntry( symbol_table, *symbol, &dup );
            if( new_entry != NULL ) {
                *id = (uint_16)new_entry->value;
                if( !dup ) {
                    SendDlgItemMessage( dlg, IDM_ACCEDCMDID, CB_ADDSTRING,
                                        0, (LPARAM)(LPCSTR)new_entry->name );
                    SendDlgItemMessage( dlg, IDM_ACCEDCMDID, CB_SETITEMDATA,
                                        0, (LPARAM)new_entry );
                }
            } else {
                *id = 0;
                WRMemFree( *symbol );
                *symbol = NULL;
                return( false );
            }
        }
    } else {
        // the string did have a numeric representation
        *id = (uint_16)val;
        WRMemFree( *symbol );
        *symbol = NULL;
    }

    return( true );
}

bool WSetEditWindowFlags( HWND dlg, uint_16 flags )
{
    bool ok, is_virt;

    ok = (dlg != (HWND)NULL);

    if( ok ) {
        is_virt = ((flags & ACCEL_VIRTKEY) != 0);
        WSetVirtKey( dlg, is_virt );
        if( is_virt ) {
            if( flags & ACCEL_CONTROL ) {
                CheckDlgButton( dlg, IDM_ACCEDCNTL, BST_CHECKED );
            } else {
                CheckDlgButton( dlg, IDM_ACCEDCNTL, BST_UNCHECKED );
            }

            if( flags & ACCEL_SHIFT ) {
                CheckDlgButton( dlg, IDM_ACCEDSHFT, BST_CHECKED );
            } else {
                CheckDlgButton( dlg, IDM_ACCEDSHFT, BST_UNCHECKED );
            }
        }

        if( flags & ACCEL_ALT ) {
            CheckDlgButton( dlg, IDM_ACCEDALT, BST_CHECKED );
        } else {
            CheckDlgButton( dlg, IDM_ACCEDALT, BST_UNCHECKED );
        }

        if( flags & ACCEL_NOINVERT ) {
            CheckDlgButton( dlg, IDM_ACCEDFLASH, BST_UNCHECKED );
        } else {
            CheckDlgButton( dlg, IDM_ACCEDFLASH, BST_CHECKED );
        }
    }

    return( ok );
}

bool WGetEditWindowFlags( HWND dlg, uint_16 *flags )
{
    bool ok, is_virt;

    ok = (dlg != (HWND)NULL);
    is_virt = false;
    if( ok ) {
        if( IsDlgButtonChecked( dlg, IDM_ACCEDVIRT ) ) {
            *flags |= ACCEL_VIRTKEY;
            is_virt = true;
        }

        if( !IsDlgButtonChecked( dlg, IDM_ACCEDFLASH ) ) {
            *flags |= ACCEL_NOINVERT;
        }

        if( IsDlgButtonChecked( dlg, IDM_ACCEDALT ) ) {
            *flags |= ACCEL_ALT;
        }

        if( is_virt ) {
            if( IsDlgButtonChecked( dlg, IDM_ACCEDCNTL ) ) {
                *flags |= ACCEL_CONTROL;
            }

            if( IsDlgButtonChecked( dlg, IDM_ACCEDSHFT ) ) {
                *flags |= ACCEL_SHIFT;
            }
        }
    }

    return( ok );
}

bool WSetEditWinResName( WAccelEditInfo *einfo )
{
    if( einfo != NULL && einfo->edit_dlg != NULL && einfo->info->res_name != NULL ) {
        return( WSetEditWithWResID( GetDlgItem( einfo->edit_dlg, IDM_ACCEDRNAME ),
                                    einfo->info->res_name ) );
    }

    return( true );
}

void WSetVirtKey( HWND hDlg, bool is_virt )
{
    CheckDlgButton( hDlg, IDM_ACCEDVIRT, ( is_virt ) ? BST_CHECKED : BST_UNCHECKED );
    CheckDlgButton( hDlg, IDM_ACCEDASCII, ( !is_virt ) ? BST_CHECKED : BST_UNCHECKED );

    EnableWindow( GetDlgItem( hDlg, IDM_ACCEDSHFT ), is_virt );
    EnableWindow( GetDlgItem( hDlg, IDM_ACCEDCNTL ), is_virt );
    EnableWindow( GetDlgItem( hDlg, IDM_ACCEDALT ), is_virt );

    if( !is_virt ) {
        CheckDlgButton( hDlg, IDM_ACCEDSHFT, BST_UNCHECKED );
        CheckDlgButton( hDlg, IDM_ACCEDCNTL, BST_UNCHECKED );
        CheckDlgButton( hDlg, IDM_ACCEDALT, BST_UNCHECKED );
    }
}

bool WInitEditWindowListBox( WAccelEditInfo *einfo )
{
    bool        ok;
    HWND        lbox;
    WAccelEntry *entry;

    ok = (einfo != NULL && einfo->edit_dlg != NULL && einfo->tbl != NULL);

    if( ok ) {
        lbox = GetDlgItem( einfo->edit_dlg, IDM_ACCEDLIST );
        SendMessage( lbox, WM_SETREDRAW, FALSE, 0 );
        SendMessage( lbox, LB_RESETCONTENT, 0, 0 );
        for( entry = einfo->tbl->first_entry; entry != NULL; entry = entry->next ) {
            ok = WAddEditWinLBoxEntry( einfo, entry, LB_ERR );
            if( !ok ) {
                 break;
            }
        }
        SendMessage( lbox, WM_SETREDRAW, TRUE, 0 );
    }

    return( ok );
}

static bool WInitEditWindow( WAccelEditInfo *einfo )
{
    HWND    lbox;
    bool    ok;

    ok = (einfo != NULL && einfo->edit_dlg != NULL);

    if( ok ) {
        ok = WSetEditWinResName( einfo );
    }

    if( ok ) {
        ok = WInitEditWindowListBox( einfo );
    }

    if( ok ) {
        if( einfo->tbl->first_entry != NULL ) {
            ok = WSetEditWindowKeyEntry( einfo, einfo->tbl->first_entry );
            if( ok ) {
                lbox = GetDlgItem( einfo->edit_dlg, IDM_ACCEDLIST );
                ok = (SendMessage( lbox, LB_SETCURSEL, 0, 0 ) != LB_ERR);
                einfo->current_entry = einfo->tbl->first_entry;
                einfo->current_pos = 0;
            }
        }
    }

    return( ok );
}

bool WPasteAccelItem( WAccelEditInfo *einfo )
{
    WAccelEntry entry;
    void        *data;
    uint_32     dsize;
    bool        ok;

    data = NULL;
    entry.symbol = NULL;

    ok = (einfo != NULL);

    if( ok ) {
        ok = WGetClipData( einfo->win, WItemClipbdFormat, &data, &dsize );
    }

    if( ok ) {
        ok = WMakeEntryFromClipData( &entry, data, dsize );
    }

    if( ok ) {
        WResolveEntrySymbol( &entry, einfo->info->symbol_table );
        ok = WSetEditWindowKeyEntry( einfo, &entry );
    }

    if( ok ) {
        ok = WInsertAccelEntry( einfo );
    }

    if( entry.symbol != NULL ) {
        WRMemFree( entry.symbol );
    }

    if( data != NULL ) {
        WRMemFree( data );
    }

    return( ok );
}

bool WClipAccelItem( WAccelEditInfo *einfo, bool cut )
{
    HWND        lbox;
    LRESULT     pos;
    WAccelEntry *entry;
    void        *data;
    uint_32     dsize;
    bool        ok;

    data = NULL;
    ok = (einfo != NULL);

    if( ok ) {
        lbox = GetDlgItem( einfo->edit_dlg, IDM_ACCEDLIST );
        ok = (lbox != (HWND)NULL);
    }

    if( ok ) {
        pos = SendMessage( lbox, LB_GETCURSEL, 0, 0 );
        ok = (pos != LB_ERR);
    }

    if( ok ) {
        entry = (WAccelEntry *)SendMessage( lbox, LB_GETITEMDATA, (WPARAM)pos, 0 );
        ok = (entry != NULL);
    }

    if( ok ) {
        ok = WMakeEntryClipData( entry, &data, &dsize );
    }

    if( ok ) {
        ok = WCopyClipData( einfo->win, WItemClipbdFormat, data, dsize );
    }

    if( ok ) {
        if( cut ) {
            ok = WDeleteAccelEntry( einfo );
        }
    }

    if( data != NULL ) {
        WRMemFree( data );
    }

    return( ok );
}

static bool WQueryChangeEntry( WAccelEditInfo *einfo )
{
    int         ret;
    UINT        style;
    char        *title;
    char        *text;

    style = MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION;
    title = WCreateEditTitle( einfo );
    text = AllocRCString( W_CHANGEMODIFIEDMENUITEM );

    ret = MessageBox( einfo->edit_dlg, text, title, style );

    if( text != NULL ) {
        FreeRCString( text );
    }
    if( title != NULL ) {
        WRMemFree( title );
    }

    if( ret == IDYES ) {
        return( true );
    }

    return( false );
}

void WDoHandleSelChange( WAccelEditInfo *einfo, bool change, bool reset )
{
    HWND        lbox;
    LRESULT     pos;
    WAccelEntry *entry;
    bool        mod;

    if( einfo == NULL ) {
        return;
    }

    lbox = GetDlgItem( einfo->edit_dlg, IDM_ACCEDLIST );
    if( lbox == (HWND)NULL ) {
        return;
    }

    pos = SendMessage( lbox, LB_GETCURSEL, 0, 0 );
    if( pos != LB_ERR ) {
        entry = (WAccelEntry *)SendMessage( lbox, LB_GETITEMDATA, (WPARAM)pos, 0 );
    } else {
        entry = NULL;
    }

    if( einfo->current_entry != NULL && !reset ) {
        mod = WGetEditWindowKeyEntry( einfo, einfo->current_entry, TRUE );
        if( mod && einfo->current_pos != LB_ERR ) {
            if( change || WQueryChangeEntry( einfo ) ) {
                WGetEditWindowKeyEntry( einfo, einfo->current_entry, FALSE );
                einfo->info->modified = true;
                SendMessage( lbox, LB_DELETESTRING, (WPARAM)einfo->current_pos, 0 );
                WAddEditWinLBoxEntry( einfo, einfo->current_entry, einfo->current_pos );
            }
        }
    }

    if( entry != NULL ) {
        if( change ) {
            uint_16 id;
            if( entry->is32bit ) {
                id = entry->u.entry32.Id;
            } else {
                id = (uint_16)entry->u.entry.Id;
            }
            WSetEditWindowID( einfo->edit_dlg, entry->symbol, id );
        } else {
            WSetEditWindowKeyEntry( einfo, entry );
        }
    }

    einfo->current_entry = entry;
    einfo->current_pos = pos;
    if ( pos != LB_ERR ) {
        SendMessage( lbox, LB_SETCURSEL, (WPARAM)pos, 0 );
    }
}

void WHandleChange( WAccelEditInfo *einfo )
{
    WDoHandleSelChange( einfo, TRUE, FALSE );
}

void WHandleSelChange( WAccelEditInfo *einfo )
{
    WDoHandleSelChange( einfo, FALSE, FALSE );
}

WINEXPORT INT_PTR CALLBACK WAcccelEditDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    WAccelEditInfo      *einfo;
    HWND                win;
    RECT                r;
    POINT               p;
    BOOL                ret;
    WORD                wp;
    WORD                cmd;

    ret = FALSE;
    einfo = (WAccelEditInfo *)GET_DLGDATA( hDlg );

    switch( message ) {
    case WM_INITDIALOG:
        einfo = (WAccelEditInfo *)lParam;
        einfo->edit_dlg = hDlg;
        SET_DLGDATA( hDlg, einfo );
        WRAddSymbolsToComboBox( einfo->info->symbol_table, hDlg,
                                IDM_ACCEDCMDID, WR_HASHENTRY_ALL );
        ret = TRUE;
        break;

    case WM_SYSCOLORCHANGE:
        WCtl3dColorChange();
        break;

#if 0
#ifdef __NT__
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
#else
    case WM_CTLCOLOR:
#endif
        return( WCtl3dCtlColorEx( message, wParam, lParam ) );
#endif

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
        MAKE_POINT( p, lParam );
        win = GetDlgItem( hDlg, IDM_ACCEDRNAME );
        GetClientRect( win, &r );
        MapWindowPoints( win, hDlg, (POINT *)&r, 2 );
        if( PtInRect( &r, p ) ) {
            WHandleRename( einfo );
        }
        ret = TRUE;
        break;

#if 0
    case WM_PARENTNOTIFY:
        cmd = GET_WM_PARENTNOTIFY_EVENT( wParam, lParam );
        switch( cmd ) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            MAKE_POINT( p, lParam );
            win = GetDlgItem( hDlg, IDM_ACCEDLIST );
            GetWindowRect( win, &r );
            MapWindowPoints( HWND_DESKTOP, hDlg, (POINT *)&r, 2 );
            if( PtInRect( &r, p ) ) {
                WHandleSelChange( einfo );
            }
            break;
        }
        ret = TRUE;
        break;
#endif

    case WM_COMMAND:
        wp = LOWORD( wParam );
        cmd = GET_WM_COMMAND_CMD( wParam, lParam );
        switch( wp ) {
        case IDM_ACCEDINSERT:
            WInsertAccelEntry( einfo );
            break;
        case IDM_ACCEDCHANGE:
            WHandleChange( einfo );
            break;
        case IDM_ACCEDRESET:
            WDoHandleSelChange( einfo, FALSE, TRUE );
            break;
        case IDM_ACCEDSETACC:
            if( einfo->tbl != NULL && einfo->tbl->num != 0 ) {
                WHandleSelChange( einfo );
            } else {
                WInsertAccelEntry( einfo );
            }
            break;
#if 0
        case IDM_ACCEDCMDID:
            if( cmd == CBN_SELCHANGE ) {
                einfo->combo_change = TRUE;
                WHandleSelChange( einfo );
                einfo->combo_change = FALSE;
            }
            break;
#endif
        case IDM_ACCEDLIST:
            if( cmd == LBN_SELCHANGE ) {
                WHandleSelChange( einfo );
            }
            break;
        case IDM_ACCEDVIRT:
        case IDM_ACCEDASCII:
            if( cmd == BN_CLICKED ) {
                WSetVirtKey( hDlg, wp == IDM_ACCEDVIRT );
            }
            break;
        }
        break;
    }

    return( ret );
}

WINEXPORT INT_PTR CALLBACK WTestDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    RECT        r;

    /* unused parameters */ (void)wParam; (void)lParam;

    if( message == WM_INITDIALOG ) {
        GetWindowRect( hDlg, &r );
        appWidth = r.right - r.left + 10;
        appHeight = r.bottom - r.top + 85;
        DestroyWindow( hDlg );
        return( TRUE );
    }

    return( FALSE );
}

void WInitEditDlg( HINSTANCE inst, HWND parent )
{
    DLGPROC     dlgproc;

    dlgproc = MakeProcInstance_DLG( WTestDlgProc, inst );
    JCreateDialog( inst, "WAccelEditDLG", parent, dlgproc );
    FreeProcInstance_DLG( dlgproc );
}
