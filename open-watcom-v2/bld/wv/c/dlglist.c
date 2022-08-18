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


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "guidlg.h"
#include "dlglist.h"
#include "strutil.h"
#include "dlgfile.h"


typedef struct dlg_list {
    void (*clear)(void);
    void (*add)(const char *,unsigned);
    char_ring *(*next)(char_ring *);
    char *(*name)(char_ring *);
    char *title;
} dlg_list;

static void SelectListLast( gui_window *gui )
{
    int         size;

    size = GUIGetListSize( gui, CTL_LIST_LIST );
    if( size != 0 ) {
        GUISetCurrSelect( gui, CTL_LIST_LIST, size - 1 );
    }
}

static void AddText( gui_window *gui, char *add )
{
    int         size;
    int         i;
    char        *text;
    bool        dup;

    size = GUIGetListSize( gui, CTL_LIST_LIST );
    dup = false;
    for( i = 0; i < size; ++i ) {
        text = GUIGetListItem( gui, CTL_LIST_LIST, i );
        if( text != NULL ) {
            dup = ( strcmp( add, text ) == 0 );
            GUIMemFree( text );
            if( dup ) {
                break;
            }
        }
    }
    if( !dup ) {
        GUIAddText( gui, CTL_LIST_LIST, add );
    }
}

static bool SourceGUIEventProc( gui_window *gui, gui_event gui_ev, void *param )
{
    gui_ctl_id  id;
    void        *curr;
    int         i;
    int         size;
    char        *text;
    dlg_list    *dlg;

    dlg = GUIGetExtra( gui );
    switch( gui_ev ) {
    case GUI_DESTROY:
        WndFree( dlg->title );
        return( true );
    case GUI_INIT_DIALOG:
        GUISetWindowText( gui, dlg->title );
        GUIClearList( gui, CTL_LIST_LIST );
        for( curr = dlg->next( NULL ); curr != NULL; curr = dlg->next( curr ) ) {
            AddText( gui, dlg->name( curr ) );
        }
        GUISetFocus( gui, CTL_LIST_EDIT );
        return( true );
    case GUI_CONTROL_CLICKED:
        GUI_GETID( param, id );
        switch( id ) {
        case CTL_LIST_LIST:
            GUIDlgBuffGetText( gui, CTL_LIST_LIST, TxtBuff, TXT_LEN );
            GUISetText( gui, CTL_LIST_EDIT, TxtBuff );
            return( true );
        case CTL_LIST_DELETE:
            i = -1;
            if( GUIGetCurrSelect( gui, CTL_LIST_LIST, &i ) ) {
                GUIDeleteItem( gui, CTL_LIST_LIST, i );
            }
            size = GUIGetListSize( gui, CTL_LIST_LIST );
            if( i < size ) {
                GUISetCurrSelect( gui, CTL_LIST_LIST, i );
            } else {
                SelectListLast( gui );
            }
            GUISetFocus( gui, CTL_LIST_LIST );
            GUISetText( gui, CTL_LIST_EDIT, NULL );
            return( true );
        case CTL_LIST_ADD:
        case CTL_LIST_OK:
            GUIDlgBuffGetText( gui, CTL_LIST_EDIT, TxtBuff, TXT_LEN );
            if( TxtBuff[0] != NULLCHAR )
                AddText( gui, TxtBuff );
            SelectListLast( gui );
            GUIClearText( gui, CTL_LIST_EDIT );
            GUISetFocus( gui, CTL_LIST_EDIT );
            if( id == CTL_LIST_ADD )
                return( true );
            dlg->clear();
            size = GUIGetListSize( gui, CTL_LIST_LIST );
            for( i = 0; i < size; ++i ) {
                text = GUIGetListItem( gui, CTL_LIST_LIST, i );
                if( text != NULL ) {
                    dlg->add( text, strlen( text ) );
                    GUIMemFree( text );
                }
            }
            /* fall through */
        case CTL_LIST_CANCEL:
            GUICloseDialog( gui );
            return( true );
        case CTL_LIST_BROWSE:
            GUIDlgBuffGetText( gui, CTL_LIST_EDIT, TxtBuff, TXT_LEN );
            if( !AllBrowse( TxtBuff ) )
                return( true );
            GUISetText( gui, CTL_LIST_EDIT, TxtBuff );
            GUISetFocus( gui, CTL_LIST_EDIT );
            return( true );
        }
        break;
    default:
        break;
    }
    return( false );
}

void DlgList( const char *title, void (*clear)(void), void (*add)(const char *,unsigned),
                           char_ring *(*next)(char_ring *), char *(*name)(char_ring *) )
{
    dlg_list dlg;
    dlg.clear = clear;
    dlg.add = add;
    dlg.next = next;
    dlg.name = name;
    dlg.title = DupStr( title );
    ResDlgOpen( SourceGUIEventProc, &dlg, DIALOG_LIST );
}
