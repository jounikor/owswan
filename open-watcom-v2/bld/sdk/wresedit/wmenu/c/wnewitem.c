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
#include "wmsg.h"
#include "ldstr.h"
#include "wsetedit.h"
#include "wedit.h"
#include "wstrdup.h"
#include "wnewitem.h"
#include "sysall.rh"

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define DEPTH_MULT      4
#define NULL_STRING     "Item"

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static WMenuEntry *WCreateNewMenuEntry( WMenuEditInfo *, bool, bool );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

bool WInsertNew( WMenuEditInfo *einfo )
{
    bool        ret;

    ret = FALSE;

    if( einfo != NULL && einfo->edit_dlg != (HWND)NULL ) {
        if( IsDlgButtonChecked( einfo->edit_dlg, IDM_MENUEDPOPUP ) ) {
            ret = WInsertNewMenuEntry( einfo, TRUE, FALSE );
        } else if( IsDlgButtonChecked( einfo->edit_dlg, IDM_MENUEDSEP ) ) {
            ret = WInsertNewMenuEntry( einfo, FALSE, TRUE );
        } else {
            ret = WInsertNewMenuEntry( einfo, FALSE, FALSE );
        }
    }

    return( ret );
}

bool WInsertNewMenuEntry( WMenuEditInfo *einfo, bool popup, bool sep )
{
    WMenuEntry  *new;
    bool        ok;

    new = NULL;

    ok = (einfo != NULL && einfo->edit_dlg != NULL);

    if( ok ) {
        new = WCreateNewMenuEntry( einfo, popup, sep );
        ok = (new != NULL);
    }

    if( ok ) {
        ok = WInsertMenuEntry( einfo, new, FALSE );
    }

    if( !ok ) {
        if( new != NULL ) {
            WFreeMenuEntry( new );
        }
    }

    return( ok );
}

bool WInsertMenuEntry( WMenuEditInfo *einfo, WMenuEntry *new, bool reset_lbox )
{
    HWND        lbox;
    WMenuEntry  *parent;
    WMenuEntry  *entry;
    LRESULT     pos;
    int         new_kids;
    bool        ok;
    bool        is_popup;
    bool        insert_before;
    bool        insert_subitems;

    entry = NULL;
    parent = NULL;
    is_popup = FALSE;

    ok = (einfo != NULL && einfo->edit_dlg != NULL && new != NULL);

    if( ok ) {
        insert_before = einfo->insert_before;
        insert_subitems = insert_before || einfo->insert_subitems;
        new_kids = WCountMenuChildren( new->child );
        if( new_kids == 0 ) {
            reset_lbox = FALSE;
        }
        lbox = GetDlgItem( einfo->edit_dlg, IDM_MENUEDLIST );
        ok = (lbox != NULL);
    }

    if( ok ) {
        pos = SendMessage( lbox, LB_GETCURSEL, 0, 0 );
        if( insert_before ) {
            if( pos == 0 ) {
                pos = LB_ERR;
            } else if( pos != LB_ERR ) {
                pos = pos - 1;
            }
        }
        if( pos != LB_ERR ) {
            entry = (WMenuEntry *)SendMessage( lbox, LB_GETITEMDATA, (WPARAM)pos, 0 );
        }
    }

    if( ok ) {
        if( entry != NULL ) {
            pos += 1;
            if( entry->item->IsPopup ) {
                insert_subitems = TRUE;
            }
            if( !insert_subitems ) {
                pos += WCountMenuChildren( entry->child );
            }
            parent = entry->parent;
            is_popup = (entry->item->IsPopup && insert_subitems);
        } else {
            pos = pos + 1;
        }
        ok = WInsertEntryIntoMenu( einfo, entry, parent, new, is_popup );
    }

    if( ok ) {
        einfo->info->modified = true;
        if( reset_lbox ) {
            ok = WInitEditWindowListBox( einfo );
        } else {
            ok = WAddEditWinLBoxEntry( lbox, new, pos );
        }
    }

    if( ok ) {
        ok = (SendMessage( lbox, LB_SETCURSEL, (WPARAM)pos, 0 ) != LB_ERR);
        if( ok ) {
            einfo->current_entry = NULL;
            einfo->current_pos = LB_ERR;
            WHandleSelChange( einfo );
        }
    }

    if( ok ) {
        SetFocus( GetDlgItem( einfo->edit_dlg, IDM_MENUEDTEXT ) );
        SendDlgItemMessage( einfo->edit_dlg, IDM_MENUEDTEXT, EM_SETSEL,
                            GET_EM_SETSEL_MPS( 0, -1 ) );
    }

    return( ok );
}

bool WAddMenuEntriesToLBox( HWND lbox, WMenuEntry *entry, LRESULT *pos )
{
    bool        ok;

    ok = ((lbox != (HWND)NULL) && pos != NULL);

    for( ; entry != NULL && ok; entry = entry->next ) {
        ok = WAddEditWinLBoxEntry( lbox, entry, *pos );
        *pos += 1;
        if( ok && entry->item->IsPopup ) {
            ok = WAddMenuEntriesToLBox( lbox, entry->child, pos );
        }
    }

    return( ok );
}

bool WAddEditWinLBoxEntry( HWND lbox, WMenuEntry *entry, LRESULT pos )
{
    bool        ok;
    char        *text;
    char        *lbtext;
    char        *lbtext1;
    int         depth;
    size_t      tlen;

    lbtext = NULL;

    ok = (lbox != (HWND)NULL && entry != NULL);

    if( ok ) {
        depth = WGetMenuEntryDepth( entry );
        ok = (depth != -1);
    }

    if ( ok ) {
        if( entry->item->Item.Normal.ItemFlags & MENU_SEPARATOR ) {
            text = "SEPARATOR";
            tlen = 14;
        } else {
            text = WGETMENUITEMTEXT( entry->item );
            if( text == NULL ) {
                text = NULL_STRING;
            }
            tlen = strlen( text ) + 1;
        }
        lbtext = (char *)WRMemAlloc( depth * DEPTH_MULT + tlen + 14 );
        ok = (lbtext != NULL);
    }

    if( ok ) {
        memset( lbtext, ' ', depth * DEPTH_MULT );
        lbtext[depth * DEPTH_MULT] = '\0';
        if( entry->item->Item.Normal.ItemFlags & MENU_POPUP ) {
            strcat( lbtext, "POPUP    \"" );
        } else if( entry->item->Item.Normal.ItemFlags & MENU_SEPARATOR ) {
            strcat( lbtext, "MENUITEM    " );
        } else {
            strcat( lbtext, "MENUITEM    \"" );
        }
        strcat( lbtext, text );
        if( (entry->item->Item.Normal.ItemFlags & MENU_SEPARATOR) == 0 ) {
            strcat( lbtext, "\"" );
        }
        lbtext1 = WConvertStringFrom( lbtext, "\t\x8\"\\", "ta\"\\" );
        ok = WInsertLBoxWithStr( lbox, pos, lbtext, entry );
    }

    if( lbtext1 != NULL ) {
        WRMemFree( lbtext1 );
    }

    if( lbtext != NULL ) {
        WRMemFree( lbtext );
    }

    return( ok );
}

WMenuEntry *WCreateNewMenuEntry( WMenuEditInfo *einfo, bool popup, bool sep )
{
    WMenuEntry  *new;
    char        *text;
    char        *symbol;
    MenuFlags   flags;
    uint_16     id;

    text = NULL;
    symbol = NULL;
    flags = 0;

    WGetEditWindowText( einfo->edit_dlg, &text );
    WGetEditWindowID( einfo->edit_dlg, &symbol, &id, einfo->info->symbol_table, FALSE );
    if( id == 0 ) {
        id = DEFAULT_MENU_ID;
    }
    WGetEditWindowFlags( einfo->edit_dlg, &flags );
    flags &= ~(MENU_POPUP | MENU_SEPARATOR);

    new = (WMenuEntry *)WRMemAlloc( sizeof( WMenuEntry ) );

    if( new != NULL ) {
        memset( new, 0, sizeof( WMenuEntry ) );
        new->is32bit = einfo->menu->is32bit;
        new->item = ResNewMenuItem();
        if( popup ) {
            new->item->IsPopup = TRUE;
            new->item->Item.Popup.ItemFlags = flags | MENU_POPUP;
            if( text != NULL ) {
                new->item->Item.Popup.ItemText = WStrDup( text );
            } else {
                new->item->Item.Popup.ItemText = AllocRCString( W_MENUPOPUP );
            }
        } else {
            if( sep ) {
                new->item->Item.Normal.ItemFlags = MENU_SEPARATOR;
            } else {
                new->item->Item.Normal.ItemID = id;
                new->item->Item.Normal.ItemFlags = flags;
                if( text != NULL ) {
                    new->item->Item.Normal.ItemText = WStrDup( text );
                } else {
                    new->item->Item.Normal.ItemText = AllocRCString( W_MENUITEM );
                }
            }
        }
    }

    if( !popup && !sep ) {
        new->symbol = symbol;
    } else {
        if( symbol != NULL ) {
            WRMemFree( symbol );
        }
    }

    if( text != NULL ) {
        WRMemFree( text );
    }

    return( new );
}
