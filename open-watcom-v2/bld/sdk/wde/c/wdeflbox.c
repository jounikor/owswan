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


#include "wdeglbl.h"
#include "wderesin.h"
#include "wdeobjid.h"
#include "wdefutil.h"
#include "wde_wres.h"
#include "wdemain.h"
#include "wdedebug.h"
#include "wdedefsz.h"
#include "wdeoinfo.h"
#include "wde.rh"
#include "wdecctl.h"
#include "wdeflbox.h"
#include "wdedispa.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

#define pick_ACTS(o) \
    pick_ACTION_DESTROY(o,pick) \
    pick_ACTION_COPY(o,pick) \
    pick_ACTION_VALIDATE_ACTION(o,pick) \
    pick_ACTION_IDENTIFY(o,pick) \
    pick_ACTION_GET_WINDOW_CLASS(o,pick) \
    pick_ACTION_DEFINE(o,pick) \
    pick_ACTION_GET_WND_PROC(o,pick)

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct {
    DISPATCH_FN *dispatcher;
    OBJPTR      object_handle;
    OBJ_ID      object_id;
    OBJPTR      control;
} WdeLBoxObject;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT bool    CALLBACK WdeLBoxDispatcher( ACTION_ID, OBJPTR, void *, void * );
WINEXPORT LRESULT CALLBACK WdeLBoxSuperClassProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static OBJPTR   WdeMakeLBox( OBJPTR, RECT *, OBJPTR, DialogStyle, char *, OBJ_ID );
static OBJPTR   WdeLBCreate( OBJPTR, RECT *, OBJPTR, OBJ_ID, WdeDialogBoxControl * );
static void     WdeLBoxSetDefineInfo( WdeDefineObjectInfo *, HWND );
static void     WdeLBoxGetDefineInfo( WdeDefineObjectInfo *, HWND );
static bool     WdeLBoxDefineHook( HWND, UINT, WPARAM, LPARAM, DialogStyle );

#define pick(e,n,c) static bool WdeLBox ## n ## c;
    pick_ACTS( WdeLBoxObject )
#undef pick

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static HINSTANCE                WdeApplicationInstance;
static DISPATCH_FN              *WdeLBoxDispatch;
static WdeDialogBoxControl      *WdeDefaultLBox = NULL;
static int                      WdeLBoxWndExtra;
static WNDPROC                  WdeOriginalLBoxProc;
//static WNDPROC                WdeLBoxProc;

static DISPATCH_ITEM WdeLBoxActions[] = {
    #define pick(e,n,c) {e, (DISPATCH_RTN *)WdeLBox ## n},
    pick_ACTS( WdeLBoxObject )
    #undef pick
};

#define MAX_ACTIONS      (sizeof( WdeLBoxActions ) / sizeof( DISPATCH_ITEM ))

OBJPTR CALLBACK WdeLBoxCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    if( handle == NULL ) {
        return( WdeMakeLBox( parent, obj_rect, handle, LBS_STANDARD, "", LISTBOX_OBJ ) );
    } else {
        return( WdeLBCreate( parent, obj_rect, NULL, LISTBOX_OBJ, (WdeDialogBoxControl *)handle ) );
    }
}

OBJPTR WdeMakeLBox( OBJPTR parent, RECT *obj_rect, OBJPTR handle,
                    DialogStyle style, char *text, OBJ_ID id )
{
    OBJPTR new;

    style |= WS_VISIBLE | WS_TABSTOP | WS_CHILD;
    SETCTL_STYLE( WdeDefaultLBox, style );
    SETCTL_TEXT( WdeDefaultLBox, ResStrToNameOrOrd( text ) );
    SETCTL_ID( WdeDefaultLBox, WdeGetNextControlID() );

    WdeChangeSizeToDefIfSmallRect( parent, id, obj_rect );

    new = WdeLBCreate( parent, obj_rect, handle, id, WdeDefaultLBox );

    WRMemFree( GETCTL_TEXT( WdeDefaultLBox ) );
    SETCTL_TEXT( WdeDefaultLBox, NULL );

    return( new );
}

OBJPTR WdeLBCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle, OBJ_ID id, WdeDialogBoxControl *info )
{
    WdeLBoxObject *new;

    WdeDebugCreate( "LBox", parent, obj_rect, handle );

    if( parent == NULL ) {
        WdeWriteTrail( "WdeLBoxCreate: LBox has no parent!" );
        return( NULL );
    }

    new = (WdeLBoxObject *)WRMemAlloc( sizeof( WdeLBoxObject ) );
    if( new == NULL ) {
        WdeWriteTrail( "WdeLBoxCreate: Object malloc failed" );
        return( NULL );
    }

    OBJ_DISPATCHER_SET( new, WdeLBoxDispatch );

    new->object_id = id;

    if( handle == NULL ) {
        new->object_handle = (OBJPTR)new;
    } else {
        new->object_handle = handle;
    }

    new->control = Create( CONTROL_OBJ, parent, obj_rect, new->object_handle );

    if( new->control == NULL ) {
        WdeWriteTrail( "WdeLBoxCreate: CONTROL_OBJ not created!" );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, SET_OBJECT_INFO, info, NULL ) ) {
        WdeWriteTrail( "WdeLBoxCreate: SET_OBJECT_INFO failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, CREATE_WINDOW, NULL, NULL ) ) {
        WdeWriteTrail( "WdeLBoxCreate: CREATE_WINDOW failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    return( (OBJPTR)new );
}

bool CALLBACK WdeLBoxDispatcher( ACTION_ID act, OBJPTR obj, void *p1, void *p2 )
{
    int     i;

    WdeDebugDispatch( "LBox", act, obj, p1, p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeLBoxActions[i].id == act ) {
            return( WdeLBoxActions[i].rtn( obj, p1, p2 ) );
        }
    }

    return( Forward( ((WdeLBoxObject *)obj)->control, act, p1, p2 ) );
}

bool WdeLBoxInit( bool first )
{
    WNDCLASS    wc;

    WdeApplicationInstance = WdeGetAppInstance();
    GetClassInfo( (HINSTANCE)NULL, "LISTBOX", &wc );
    WdeOriginalLBoxProc = wc.lpfnWndProc;
    WdeLBoxWndExtra = wc.cbWndExtra;

    if( first ) {
#if 0
        if( wc.style & CS_GLOBALCLASS ) {
            wc.style ^= CS_GLOBALCLASS;
        }
        if( wc.style & CS_PARENTDC ) {
            wc.style ^= CS_PARENTDC;
        }
        wc.style |= CS_HREDRAW | CS_VREDRAW;
        wc.hInstance = WdeApplicationInstance;
        wc.lpszClassName = "wdelistbox";
        wc.cbWndExtra += sizeof( OBJPTR );
        //wc.lpfnWndProc = WdeLBoxSuperClassProc;
        if( !RegisterClass( &wc ) ) {
            WdeWriteTrail( "WdeLBoxInit: RegisterClass failed." );
        }
#endif
    }

    WdeDefaultLBox = WdeAllocDialogBoxControl();
    if( WdeDefaultLBox == NULL ) {
        WdeWriteTrail( "WdeLBoxInit: Alloc of control failed!" );
        return( false );
    }

    /* set up the default control structure */
    SETCTL_STYLE( WdeDefaultLBox, 0 );
    SETCTL_ID( WdeDefaultLBox, 0 );
    SETCTL_EXTRABYTES( WdeDefaultLBox, 0 );
    SETCTL_SIZEX( WdeDefaultLBox, 0 );
    SETCTL_SIZEY( WdeDefaultLBox, 0 );
    SETCTL_SIZEW( WdeDefaultLBox, 0 );
    SETCTL_SIZEH( WdeDefaultLBox, 0 );
    SETCTL_TEXT( WdeDefaultLBox, NULL );
    SETCTL_CLASSID( WdeDefaultLBox, ResNumToControlClass( CLASS_LISTBOX ) );

    WdeLBoxDispatch = MakeProcInstance_DISPATCHER( WdeLBoxDispatcher, WdeGetAppInstance() );

    return( true );
}

void WdeLBoxFini( void )
{
    WdeFreeDialogBoxControl( &WdeDefaultLBox );
    FreeProcInstance_DISPATCHER( WdeLBoxDispatch );
}

bool WdeLBoxDestroy( WdeLBoxObject *obj, bool *flag, bool *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    if( !Forward( obj->control, DESTROY, flag, NULL ) ) {
        WdeWriteTrail( "WdeLBoxDestroy: Control DESTROY failed" );
        return( false );
    }

    WRMemFree( obj );

    return( true );
}

bool WdeLBoxValidateAction( WdeLBoxObject *obj, ACTION_ID *act, void *p2 )
{
    int     i;

    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeLBoxActions[i].id == *act ) {
            return( true );
        }
    }

    return( ValidateAction( obj->control, *act, p2 ) );
}

bool WdeLBoxCopyObject( WdeLBoxObject *obj, WdeLBoxObject **new, OBJPTR handle )
{
    if( new == NULL ) {
        WdeWriteTrail( "WdeLBoxCopyObject: Invalid new object!" );
        return( false );
    }

    *new = (WdeLBoxObject *)WRMemAlloc( sizeof( WdeLBoxObject ) );

    if( *new == NULL ) {
        WdeWriteTrail( "WdeLBoxCopyObject: Object malloc failed" );
        return( false );
    }

    OBJ_DISPATCHER_COPY( *new, obj );
    (*new)->object_id = obj->object_id;

    if( handle == NULL ) {
        (*new)->object_handle = (OBJPTR)*new;
    } else {
        (*new)->object_handle = handle;
    }

    if( !CopyObject( obj->control, &(*new)->control, (*new)->object_handle ) ) {
        WdeWriteTrail( "WdeLBoxCopyObject: Control not created!" );
        WRMemFree( *new );
        return( false );
    }

    return( true );
}

bool WdeLBoxIdentify( WdeLBoxObject *obj, OBJ_ID *id, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    *id = obj->object_id;

    return( true );
}

bool WdeLBoxGetWndProc( WdeLBoxObject *obj, WNDPROC *proc, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *proc = WdeLBoxSuperClassProc;

    return( true );
}

bool WdeLBoxGetWindowClass( WdeLBoxObject *obj, char **class, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *class = "listbox";

    return( true );
}

bool WdeLBoxDefine( WdeLBoxObject *obj, POINT *pnt, void *p2 )
{
    WdeDefineObjectInfo  o_info;

    /* touch unused vars to get rid of warning */
    _wde_touch( pnt );
    _wde_touch( p2 );

    o_info.obj = obj->object_handle;
    o_info.obj_id = obj->object_id;
    o_info.mask = WS_VISIBLE | WS_DISABLED | WS_TABSTOP | WS_GROUP |
                  WS_VSCROLL | WS_HSCROLL  | WS_BORDER;
    o_info.set_func = (WdeSetProc)WdeLBoxSetDefineInfo;
    o_info.get_func = (WdeGetProc)WdeLBoxGetDefineInfo;
    o_info.hook_func = WdeLBoxDefineHook;
    o_info.win = NULL;

    return( WdeControlDefine( &o_info ) );
}

void WdeLBoxSetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
    OBJ_ID      id;
    DialogStyle mask;

    id = o_info->obj_id;

    mask = GETCTL_STYLE( o_info->info.c.info ) & 0x0000ffff;

    if( mask & LBS_NOTIFY ) {
        CheckDlgButton( hDlg, IDB_LBS_NOTIFY, BST_CHECKED );
    }

    if( mask & LBS_SORT ) {
        CheckDlgButton( hDlg, IDB_LBS_SORT, BST_CHECKED );
    }

    if( mask & LBS_NOREDRAW ) {
        CheckDlgButton( hDlg, IDB_LBS_NOREDRAW, BST_CHECKED );
    }

    if( mask & LBS_MULTIPLESEL ) {
        CheckDlgButton( hDlg, IDB_LBS_MULTIPLESEL, BST_CHECKED );
    }

    if( mask & LBS_USETABSTOPS ) {
        CheckDlgButton( hDlg, IDB_LBS_USETABSTOPS, BST_CHECKED );
    }

    if( mask & LBS_NOINTEGRALHEIGHT ) {
        CheckDlgButton( hDlg, IDB_LBS_NOINTEGRALHEIGHT, BST_CHECKED );
    }

    if( mask & LBS_MULTICOLUMN ) {
        CheckDlgButton( hDlg, IDB_LBS_MULTICOLUMN, BST_CHECKED );
    }

    if( mask & LBS_WANTKEYBOARDINPUT ) {
        CheckDlgButton( hDlg, IDB_LBS_WANTKEYBOARDINPUT, BST_CHECKED );
    }

    if( mask & LBS_EXTENDEDSEL ) {
        CheckDlgButton( hDlg, IDB_LBS_EXTENDEDSEL, BST_CHECKED );
    }

    if( mask & LBS_DISABLENOSCROLL ) {
        CheckDlgButton( hDlg, IDB_LBS_DISABLENOSCROLL, BST_CHECKED );
    }

    if( mask & LBS_STANDARD ) {
        CheckDlgButton( hDlg, IDB_LBS_STANDARD, BST_CHECKED );
    }

    CheckDlgButton( hDlg, IDB_LBS_HASSTRINGS, BST_CHECKED );
    EnableWindow( GetDlgItem( hDlg, IDB_LBS_HASSTRINGS ), FALSE );

    if( mask & LBS_OWNERDRAWFIXED ) {
        CheckDlgButton( hDlg, IDB_LBS_OWNERDRAWFIXED, BST_CHECKED );
        EnableWindow( GetDlgItem( hDlg, IDB_LBS_HASSTRINGS ), TRUE );
        if( !(mask & LBS_HASSTRINGS) ) {
            CheckDlgButton( hDlg, IDB_LBS_HASSTRINGS, BST_UNCHECKED );
        }
    }

    if( mask & LBS_OWNERDRAWVARIABLE ) {
        CheckDlgButton( hDlg, IDB_LBS_OWNERDRAWVARIABLE, BST_CHECKED );
        EnableWindow ( GetDlgItem(hDlg, IDB_LBS_HASSTRINGS), TRUE );
        if ( !(mask & LBS_HASSTRINGS) ) {
            CheckDlgButton ( hDlg, IDB_LBS_HASSTRINGS, BST_UNCHECKED );
        }
    }

#if __NT__XX
    EnableWindow( GetDlgItem( hDlg, IDB_LBS_NOSEL ), TRUE );
    if( mask & LBS_NOSEL ) {
        CheckDlgButton( hDlg, IDB_LBS_NOSEL, BST_CHECKED );
    }
#else
    EnableWindow( GetDlgItem( hDlg, IDB_LBS_NOSEL ), FALSE );
#endif

#if __NT__XX
    // do the extended style stuff
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_STATICEDGE ), TRUE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_NOPARENTNOTIFY ), TRUE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_LEFT ), TRUE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_RIGHT ), TRUE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_LTRREADING ), TRUE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_RTLREADING ), TRUE );

    mask = GETCTL_EXSTYLE( o_info->info.c.info );
    if( mask & WS_EX_STATICEDGE ) {
        CheckDlgButton( hDlg, IDB_WS_EX_STATICEDGE, BST_CHECKED );
    }
    if( mask & WS_EX_NOPARENTNOTIFY ) {
        CheckDlgButton( hDlg, IDB_WS_EX_NOPARENTNOTIFY, BST_CHECKED );
    }
    if( mask & WS_EX_RIGHT ) {
        CheckDlgButton( hDlg, IDB_WS_EX_RIGHT, BST_CHECKED );
    } else {
        CheckDlgButton( hDlg, IDB_WS_EX_LEFT, BST_CHECKED );
    }
    if( mask & WS_EX_RTLREADING ) {
        CheckDlgButton( hDlg, IDB_WS_EX_RTLREADING, BST_CHECKED );
    } else {
        CheckDlgButton( hDlg, IDB_WS_EX_LTRREADING, BST_CHECKED );
    }
#else
    // disable the extended styles
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_STATICEDGE ), FALSE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_NOPARENTNOTIFY ), FALSE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_LEFT ), FALSE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_RIGHT ), FALSE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_LTRREADING ), FALSE );
    EnableWindow( GetDlgItem( hDlg, IDB_WS_EX_RTLREADING ), FALSE );
#endif
}

void WdeLBoxGetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
    OBJ_ID      id;
    DialogStyle mask;

    id = o_info->obj_id;

    mask = 0;

    if( IsDlgButtonChecked( hDlg, IDB_LBS_NOTIFY ) ) {
        mask |= LBS_NOTIFY;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_SORT ) ) {
        mask |= LBS_SORT;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_NOREDRAW ) ) {
        mask |= LBS_NOREDRAW;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_MULTIPLESEL ) ) {
        mask |= LBS_MULTIPLESEL;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_HASSTRINGS ) ) {
        mask |= LBS_HASSTRINGS;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_USETABSTOPS ) ) {
        mask |= LBS_USETABSTOPS;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_NOINTEGRALHEIGHT ) ) {
        mask |= LBS_NOINTEGRALHEIGHT;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_MULTICOLUMN ) ) {
        mask |= LBS_MULTICOLUMN;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_WANTKEYBOARDINPUT ) ) {
        mask |= LBS_WANTKEYBOARDINPUT;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_EXTENDEDSEL ) ) {
        mask |= LBS_EXTENDEDSEL;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_DISABLENOSCROLL ) ) {
        mask |= LBS_DISABLENOSCROLL;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_OWNERDRAWFIXED ) ) {
        mask |= LBS_OWNERDRAWFIXED;
    }

    if( IsDlgButtonChecked( hDlg, IDB_LBS_OWNERDRAWVARIABLE ) ) {
        mask |= LBS_OWNERDRAWVARIABLE;
    }

#if __NT__XX
    if( IsDlgButtonChecked( hDlg, IDB_LBS_NOSEL ) ) {
        mask |= LBS_NOSEL;
    }
#endif

    SETCTL_STYLE( o_info->info.c.info,
                  (GETCTL_STYLE( o_info->info.c.info ) & 0xffff0000) | mask );

#if __NT__XX
    // set the extended mask
    mask = 0;
    if( IsDlgButtonChecked( hDlg, IDB_WS_EX_STATICEDGE ) ) {
        mask |= WS_EX_STATICEDGE;
    }
    if( IsDlgButtonChecked( hDlg, IDB_WS_EX_NOPARENTNOTIFY ) ) {
        mask |= WS_EX_NOPARENTNOTIFY;
    }
    if( IsDlgButtonChecked( hDlg, IDB_WS_EX_RIGHT ) ) {
        mask |= WS_EX_RIGHT;
    }
    if( IsDlgButtonChecked( hDlg, IDB_WS_EX_RTLREADING ) ) {
        mask |= WS_EX_RTLREADING;
    }
    SETCTL_EXSTYLE( o_info->info.c.info, mask );
#endif
}

bool WdeLBoxDefineHook ( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, DialogStyle mask )
{
    bool processed;

    /* touch unused vars to get rid of warning */
    _wde_touch( mask );
    _wde_touch( lParam );

    processed = false;

    if( message == WM_COMMAND && GET_WM_COMMAND_CMD( wParam, lParam ) == BN_CLICKED ) {
        switch( LOWORD( wParam ) ) {
        case IDB_LBS_NOTIFY:
        case IDB_LBS_SORT:
        case IDB_WS_VSCROLL:
        case IDB_WS_BORDER:
            if( IsDlgButtonChecked( hDlg, IDB_LBS_NOTIFY ) &&
                IsDlgButtonChecked( hDlg, IDB_LBS_SORT ) &&
                IsDlgButtonChecked( hDlg, IDB_WS_VSCROLL ) &&
                IsDlgButtonChecked( hDlg, IDB_WS_BORDER ) ) {
                CheckDlgButton( hDlg, IDB_LBS_STANDARD, BST_CHECKED );
            } else {
                CheckDlgButton( hDlg, IDB_LBS_STANDARD, BST_UNCHECKED );
            }
            processed = true;
            break;

        case IDB_LBS_STANDARD:
            if( IsDlgButtonChecked( hDlg, IDB_LBS_STANDARD ) ) {
                CheckDlgButton( hDlg, IDB_LBS_NOTIFY, BST_CHECKED );
                CheckDlgButton( hDlg, IDB_LBS_SORT, BST_CHECKED );
                CheckDlgButton( hDlg, IDB_WS_VSCROLL, BST_CHECKED );
                CheckDlgButton( hDlg, IDB_WS_BORDER, BST_CHECKED );
            } else {
                CheckDlgButton( hDlg, IDB_LBS_NOTIFY, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDB_LBS_SORT, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDB_WS_VSCROLL, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDB_WS_BORDER, BST_UNCHECKED );
            }
            processed = true;
            break;

        case IDB_LBS_OWNERDRAWFIXED:
        case IDB_LBS_OWNERDRAWVARIABLE:
            if( IsDlgButtonChecked( hDlg, IDB_LBS_OWNERDRAWFIXED ) ||
                IsDlgButtonChecked( hDlg, IDB_LBS_OWNERDRAWVARIABLE ) ) {
                EnableWindow( GetDlgItem( hDlg, IDB_LBS_HASSTRINGS ), TRUE );
                CheckDlgButton( hDlg, IDB_LBS_OWNERDRAWFIXED, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDB_LBS_OWNERDRAWVARIABLE, BST_UNCHECKED );
                CheckDlgButton( hDlg, LOWORD( wParam ), BST_CHECKED );
            } else {
                CheckDlgButton( hDlg, IDB_LBS_HASSTRINGS, BST_CHECKED );
                EnableWindow( GetDlgItem( hDlg, IDB_LBS_HASSTRINGS ), FALSE );
            }
            processed = true;
            break;
        }
    }

    return( processed );
}

LRESULT CALLBACK WdeLBoxSuperClassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if( !WdeProcessMouse( hWnd, message, wParam, lParam ) ) {
        return( CallWindowProc( WdeOriginalLBoxProc, hWnd, message, wParam, lParam ) );
    }
    return( FALSE );
}
