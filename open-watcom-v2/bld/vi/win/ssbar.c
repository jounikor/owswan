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
* Description:  Status bar.
*
****************************************************************************/


#include "vi.h"
#include "statwnd.h"
#include "ssbar.rh"
#include "utils.h"
#include "subclass.h"
#include "wstatus.h"
#include <assert.h>


/* Local Windows CALLBACK function prototypes */
WINEXPORT LRESULT CALLBACK StaticSubclassProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
WINEXPORT INT_PTR CALLBACK SSDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

#define NARRAY( a )             (sizeof( a ) / sizeof( a[0] ))

#define DEFAULT_STATUSSTRING    "Line:$5L$[Col:$3C$[Mode: $M$[$|$T$[$H"
#define DEFAULT_STATUSSECTIONS  { 78, 137, 250, 317 }

enum buttonType {
    BUTTON_CONTENT,
    BUTTON_ALIGNMENT,
};

HWND            hSSbar;

static bool     haveCapture = false;
static int      curItemID = -1;
static HWND     mod_hwnd;

static char *findBlockString( int block )
{
    char    *mod;
    int     i;

    i = 0;
    mod = EditVars.StatusString;
    while( i != block ) {
        while( *mod != '\0' && *mod != '$' ) {
            mod++;
        }
        assert( *mod != '\0' );
        mod++;
        if( *mod == '[' ) {
            i++;
        }
        mod++;
    }
    return( mod );
}

static void totalRedraw( void )
{
    StatusWndSetSeparatorsWithArray( EditVars.StatusSections, EditVars.NumStatusSections );
    UpdateStatusWindow();
    InvalidateRect( status_window_id, NULL, TRUE );
    UpdateWindow( status_window_id );
}

static void destroyBlock( int i, char *start )
{
    char    new_ss[MAX_STR];

    if( EditVars.NumStatusSections == 1 ) {
        // unfortunately wstatus.c can't handle this right now
        // (it would be nice)
        MyBeep();
        return;
    }

    if( i != EditVars.NumStatusSections ) {
        memmove( EditVars.StatusSections + i, EditVars.StatusSections + i + 1, (EditVars.NumStatusSections - 1 - i) * sizeof( section_size ) );
    }
    EditVars.NumStatusSections--;
    EditVars.StatusSections = _MemReallocArray( EditVars.StatusSections, section_size, EditVars.NumStatusSections );

    strncpy( new_ss, EditVars.StatusString, start - EditVars.StatusString );
    new_ss[start - EditVars.StatusString] = '\0';
    while( start[0] != '\0' && !(start[0] == '$' && start[1] == '[') ) {
        start++;
    }
    if( start[0] == '$' ) {
        start += 2;
        strcat( new_ss, start );
    }
    ReplaceString( &EditVars.StatusString, new_ss );

    totalRedraw();
}

static void splitBlock( int i, char *start )
{
    char    new_ss[MAX_STR];
    int     diff;
    RECT    rect;

    if( EditVars.NumStatusSections == MAX_SECTIONS ) {
        MyBeep();
        return;
    }

    if( i == EditVars.NumStatusSections ) {
        GetWindowRect( status_window_id, &rect );
        diff = rect.right - EditVars.StatusSections[i - 1];
    } else if( i == 0 ) {
        diff = EditVars.StatusSections[1];
    } else {
        diff = EditVars.StatusSections[i] - EditVars.StatusSections[i - 1];
    }

    if( diff < BOUNDARY_WIDTH * 4 ) {
        MyBeep();
        return;
    }
    EditVars.NumStatusSections++;
    EditVars.StatusSections = _MemReallocArray( EditVars.StatusSections, section_size, EditVars.NumStatusSections );
    memmove( EditVars.StatusSections + i + 1, EditVars.StatusSections + i, (EditVars.NumStatusSections - 1 - i) * sizeof( section_size ) );
    if( i > 0 ) {
        EditVars.StatusSections[i] = EditVars.StatusSections[i - 1] + (diff / 2);
    } else {
        EditVars.StatusSections[i] /= 2;
    }

    while( start[0] && !(start[0] == '$' && start[1] == '[') ) {
        start++;
    }
    strncpy( new_ss, EditVars.StatusString, start - EditVars.StatusString );
    new_ss[start - EditVars.StatusString] = '\0';
    strcat( new_ss, "$[ " );
    strcat( new_ss, start );
    ReplaceString( &EditVars.StatusString, new_ss );

    totalRedraw();
}

static void buildNewItem( char *start, int id )
{
    char    new_ss[MAX_STR];
    char    *sz_content[] = { "$T", "$D", "Mode: $M", "Line:$5L", "Col:$3C", "$H" };
    char    *sz_alignment[] = { "$<", "$|", "$>" };
    char    *new_item = "";
    int     type = 0;

    if( id >= SSB_FIRST_CONTENT && id <= SSB_LAST_CONTENT ) {
        new_item = sz_content[id - SSB_FIRST_CONTENT];
        type = BUTTON_CONTENT;
    } else if( id >= SSB_FIRST_ALIGNMENT && id <= SSB_LAST_ALIGNMENT ) {
        new_item = sz_alignment[id - SSB_FIRST_ALIGNMENT];
        type = BUTTON_ALIGNMENT;
    } else {
        assert( 0 );
    }

    strncpy( new_ss, EditVars.StatusString, start - EditVars.StatusString );
    new_ss[start - EditVars.StatusString] = '\0';
    strcat( new_ss, new_item );
    if( type == BUTTON_CONTENT ) {
        // only copy alignments, if any
        while( start[0] != '\0' && !(start[0] == '$' && start[1] == '[') ) {
            if( start[0] == '$' && (start[1] == '<' || start[1] == '|' || start[1] == '>') ) {
                strncat( new_ss, start, 2 );
                start++;
            }
            start++;
        }
    } else {
        // only copy contents, if any
        while( start[0] != '\0' && !(start[0] == '$' && start[1] == '[') ) {
            if( start[0] == '$' && (start[1] == '<' || start[1] == '|' || start[1] == '>') ) {
                start += 2;
                continue;
            }
            // by no means the most efficient way, but hardly matters
            strncat( new_ss, start, 1 );
            start++;
        }
    }
    strcat( new_ss, start );
    ReplaceString( &EditVars.StatusString, new_ss );

    totalRedraw();
}

static void buildDefaults( void )
{
    section_size    def_sections[] = DEFAULT_STATUSSECTIONS;

    ReplaceString( &EditVars.StatusString, DEFAULT_STATUSSTRING );

    EditVars.NumStatusSections = NARRAY( def_sections );
    EditVars.StatusSections = MemRealloc( EditVars.StatusSections, sizeof( def_sections ) );
    memcpy( EditVars.StatusSections, def_sections, sizeof( def_sections ) );

    totalRedraw();
}

static void buildNewStatusString( int block, int id )
{
    char    *mod;

    mod = findBlockString( block );

    switch( id ) {
    case SSB_CMD_SPLIT:
        splitBlock( block, mod );
        break;
    case SSB_CMD_DESTROY:
        destroyBlock( block, mod );
        break;
    case SSB_CMD_DEFAULTS:
        buildDefaults();
        break;
    default:
        buildNewItem( mod, id );
    }
}

static void sendNewItem( int x, int id )
{
    int     i;

    if( BAD_ID( mod_hwnd ) ) {
        return;
    }
    assert( mod_hwnd == status_window_id );

    for( i = 0; i < EditVars.NumStatusSections; ++i ) {
        if( EditVars.StatusSections[i] >= x ) {
            break;
        }
    }

    assert( curItemID != -1 );
    buildNewStatusString( i, id );
}

static long processLButtonUp( HWND hwnd, LPARAM lparam )
{
    POINT   m_pt;
    if( haveCapture ) {
        MAKE_POINT( m_pt, lparam );
        ClientToScreen( hwnd, &m_pt );
        ScreenToClient( status_window_id, &m_pt );
        sendNewItem( m_pt.x, curItemID );
        CursorOp( COP_ARROW );
        DrawRectangleUpDown( hwnd, DRAW_UP );
        ReleaseCapture();
        haveCapture = false;
        curItemID = -1;
    }
    return( 0L );
}

static long processLButtonDown( HWND hwnd )
{
    DrawRectangleUpDown( hwnd, DRAW_DOWN );
    CursorOp( COP_DROPSS );
    SetCapture( hwnd );
    haveCapture = true;
    curItemID = GetDlgCtrlID( hwnd );
    mod_hwnd = NO_WINDOW;
    return( 0L );
}

static long processMouseMove( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    RECT    rect;
    POINT   m_pt;

    msg = msg;
    wparam = wparam;

    if( !haveCapture ) {
        return( 0L );
    }

    // check we aren't on ourselves first
    MAKE_POINT( m_pt, lparam );
    ClientToScreen( hwnd, &m_pt );
    GetWindowRect( GetParent( hwnd ), &rect );
    if( PtInRect( &rect, m_pt ) ) {
        CursorOp( COP_DROPSS );
        mod_hwnd = NO_WINDOW;
        return( 0L );
    }

    // otherwise, figure out what we're over & set cursor based on that
    mod_hwnd = GetOwnedWindow( m_pt );
    if( mod_hwnd == status_window_id ) {
        CursorOp( COP_DROPSS );
    } else {
        mod_hwnd = NO_WINDOW;
        CursorOp( COP_NODROP );
    }
    return( 0L );
}

WINEXPORT LRESULT CALLBACK StaticSubclassProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch( msg ) {
    case WM_NCHITTEST:
        return( HTCLIENT );
    case WM_MOUSEMOVE:
        return( processMouseMove( hwnd, msg, wparam, lparam ) );
    case WM_LBUTTONDOWN:
        return( processLButtonDown( hwnd ) );
    case WM_LBUTTONUP:
        return( processLButtonUp( hwnd, lparam ) );
    }
    return( CallWindowProc( SubclassGenericFindOldProc( hwnd ), hwnd, msg, wparam, lparam ) );
}

static void addSubclasses( HWND hwnd )
{
    int     i;
    for( i = SSB_FIRST_CONTENT; i <= SSB_LAST_CONTENT; i++ ) {
        SubclassGenericAdd( GetDlgItem( hwnd, i ), StaticSubclassProc, InstanceHandle );
    }
    for( i = SSB_FIRST_ALIGNMENT; i <= SSB_LAST_ALIGNMENT; i++ ) {
        SubclassGenericAdd( GetDlgItem( hwnd, i ), StaticSubclassProc, InstanceHandle );
    }
    for( i = SSB_FIRST_COMMAND; i <= SSB_LAST_COMMAND; i++ ) {
        SubclassGenericAdd( GetDlgItem( hwnd, i ), StaticSubclassProc, InstanceHandle );
    }
}

static void removeSubclasses( HWND hwnd )
{
    int     i;
    for( i = SSB_FIRST_CONTENT; i <= SSB_LAST_CONTENT; i++ ) {
        SubclassGenericRemove( GetDlgItem( hwnd, i ) );
    }
    for( i = SSB_FIRST_ALIGNMENT; i <= SSB_LAST_ALIGNMENT; i++ ) {
        SubclassGenericRemove( GetDlgItem( hwnd, i ) );
    }
    for( i = SSB_FIRST_COMMAND; i <= SSB_LAST_COMMAND; i++ ) {
        SubclassGenericRemove( GetDlgItem( hwnd, i ) );
    }
}

/*
 * SSDlgProc - callback routine for status bar settings drag & drop dialog
 */
WINEXPORT INT_PTR CALLBACK SSDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    (void)lparam; (void)wparam;

    switch( msg ) {
    case WM_INITDIALOG:
        hSSbar = hwnd;
        MoveWindowTopRight( hwnd );
        addSubclasses( hwnd );
        return( TRUE );
    case WM_CLOSE:
        removeSubclasses( hwnd );
        hSSbar = NO_WINDOW;
        // update editflags (may have closed from system menu)
        EditFlags.SSbar = false;
        DestroyWindow( hwnd );
        break;
    }
    return( FALSE );

} /* SSDlgProc */

/*
 * RefreshSSbar - turn status settings bar on/off
 */
void RefreshSSbar( void )
{
    static DLGPROC      dlgproc = NULL;

    if( EditFlags.SSbar ) {
        if( !BAD_ID( hSSbar ) ) {
            return;
        }
        dlgproc = MakeProcInstance_DLG( SSDlgProc, InstanceHandle );
        hSSbar = CreateDialog( InstanceHandle, "SSBAR", root_window_id, dlgproc );
    } else {
        if( BAD_ID( hSSbar ) ) {
            return;
        }
        SendMessage( hSSbar, WM_CLOSE, 0, 0L );
        FreeProcInstance_DLG( dlgproc );
    }
    UpdateStatusWindow();

} /* RefreshSSbar */
