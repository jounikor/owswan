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
#include "wdeoinfo.h"
#include "wdedefsz.h"
#include "wdedebug.h"
#include "wde.rh"
#include "wdesdup.h"
#include "wdecctl.h"
#include "wdeftvw.h"
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
} WdeTViewObject;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT bool    CALLBACK WdeTViewDispatcher( ACTION_ID, OBJPTR, void *, void * );
WINEXPORT LRESULT CALLBACK WdeTViewSuperClassProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static OBJPTR   WdeMakeTView( OBJPTR, RECT *, OBJPTR, DialogStyle, char *, OBJ_ID );
static OBJPTR   WdeTVCreate( OBJPTR, RECT *, OBJPTR, OBJ_ID, WdeDialogBoxControl * );
static void     WdeTViewSetDefineInfo( WdeDefineObjectInfo *, HWND );
static void     WdeTViewGetDefineInfo( WdeDefineObjectInfo *, HWND );
static bool     WdeTViewDefineHook( HWND, UINT, WPARAM, LPARAM, DialogStyle );

#define pick(e,n,c) static bool WdeTView ## n ## c;
    pick_ACTS( WdeTViewObject )
#undef pick

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static HINSTANCE                WdeApplicationInstance;
static DISPATCH_FN              *WdeTViewDispatch;
static WdeDialogBoxControl      *WdeDefaultTView = NULL;
static int                      WdeTViewWndExtra;
static WNDPROC                  WdeOriginalTViewProc;
//static WNDPROC                WdeTViewProc;

#define WWC_TREEVIEW     WC_TREEVIEW

static DISPATCH_ITEM WdeTViewActions[] = {
    #define pick(e,n,c) {e, (DISPATCH_RTN *)WdeTView ## n},
    pick_ACTS( WdeTViewObject )
    #undef pick
};

#define MAX_ACTIONS     (sizeof( WdeTViewActions ) / sizeof( DISPATCH_ITEM ))

OBJPTR CALLBACK WdeTViewCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    if( handle == NULL ) {
        return( WdeMakeTView( parent, obj_rect, handle, 0, "", TVIEW_OBJ ) );
    } else {
        return( WdeTVCreate( parent, obj_rect, NULL, TVIEW_OBJ, (WdeDialogBoxControl *)handle ) );
    }
}

OBJPTR WdeMakeTView( OBJPTR parent, RECT *obj_rect, OBJPTR handle, DialogStyle style, char *text, OBJ_ID id )
{
    OBJPTR new;

    style |= WS_BORDER | WS_VISIBLE | WS_TABSTOP | WS_CHILD;

    SETCTL_STYLE( WdeDefaultTView, style );
    SETCTL_TEXT( WdeDefaultTView, ResStrToNameOrOrd( text ) );
    SETCTL_ID( WdeDefaultTView, WdeGetNextControlID() );

    WdeChangeSizeToDefIfSmallRect( parent, id, obj_rect );

    new = WdeTVCreate( parent, obj_rect, handle, id, WdeDefaultTView );

    WRMemFree( GETCTL_TEXT( WdeDefaultTView ) );
    SETCTL_TEXT( WdeDefaultTView, NULL );

    return( new );
}

OBJPTR WdeTVCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle, OBJ_ID id, WdeDialogBoxControl *info )
{
    WdeTViewObject *new;

    WdeDebugCreate( "TView", parent, obj_rect, handle );

    if( parent == NULL ) {
        WdeWriteTrail( "WdeTViewCreate: TView has no parent!" );
        return( NULL );
    }

    new = (WdeTViewObject *)WRMemAlloc( sizeof( WdeTViewObject ) );
    if( new == NULL ) {
        WdeWriteTrail( "WdeTViewCreate: Object malloc failed" );
        return( NULL );
    }

    OBJ_DISPATCHER_SET( new, WdeTViewDispatch );
    new->object_id = id;
    if( handle == NULL ) {
        new->object_handle = (OBJPTR)new;
    } else {
        new->object_handle = handle;
    }

    new->control = Create( CONTROL_OBJ, parent, obj_rect, new->object_handle );

    if( new->control == NULL ) {
        WdeWriteTrail( "WdeTViewCreate: CONTROL_OBJ not created!" );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, SET_OBJECT_INFO, info, NULL ) ) {
        WdeWriteTrail( "WdeTViewCreate: SET_OBJECT_INFO failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, CREATE_WINDOW, NULL, NULL ) ) {
        WdeWriteTrail( "WdeTViewCreate: CREATE_WINDOW failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    return( (OBJPTR)new );
}

bool CALLBACK WdeTViewDispatcher( ACTION_ID act, OBJPTR obj, void *p1, void *p2 )
{
    int     i;

    WdeDebugDispatch( "TView", act, obj, p1, p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeTViewActions[i].id == act ) {
            return( WdeTViewActions[i].rtn( obj, p1, p2 ) );
        }
    }

    return( Forward( ((WdeTViewObject *)obj)->control, act, p1, p2 ) );
}

bool WdeTViewInit( bool first )
{
    WNDCLASS    wc;

    WdeApplicationInstance = WdeGetAppInstance();
    GetClassInfo( (HINSTANCE)NULL, WWC_TREEVIEW, &wc );
    WdeOriginalTViewProc = wc.lpfnWndProc;
    WdeTViewWndExtra = wc.cbWndExtra;

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
        wc.lpszClassName = "wdeedit";
        wc.cbWndExtra += sizeof( OBJPTR );
        //wc.lpfnWndProc = WdeTViewSuperClassProc;
        if( !RegisterClass( &wc ) ) {
            WdeWriteTrail( "WdeTViewInit: RegisterClass failed." );
        }
#endif
    }

    WdeDefaultTView = WdeAllocDialogBoxControl();
    if( WdeDefaultTView == NULL ) {
        WdeWriteTrail( "WdeTViewInit: Alloc of control failed!" );
        return( false );
    }

    /* set up the default control structure */
    SETCTL_STYLE( WdeDefaultTView, WS_BORDER | WS_VISIBLE | WS_TABSTOP | WS_GROUP );
    SETCTL_ID( WdeDefaultTView, 0 );
    SETCTL_EXTRABYTES( WdeDefaultTView, 0 );
    SETCTL_SIZEX( WdeDefaultTView, 0 );
    SETCTL_SIZEY( WdeDefaultTView, 0 );
    SETCTL_SIZEW( WdeDefaultTView, 0 );
    SETCTL_SIZEH( WdeDefaultTView, 0 );
    SETCTL_TEXT( WdeDefaultTView, NULL );
    SETCTL_CLASSID( WdeDefaultTView, WdeStrToControlClass( WWC_TREEVIEW ) );

    WdeTViewDispatch = MakeProcInstance_DISPATCHER( WdeTViewDispatcher, WdeGetAppInstance() );
    return( true );
}

void WdeTViewFini( void )
{
    WdeFreeDialogBoxControl( &WdeDefaultTView );
    FreeProcInstance_DISPATCHER( WdeTViewDispatch );
}

bool WdeTViewDestroy( WdeTViewObject *obj, bool *flag, bool *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    if( !Forward( obj->control, DESTROY, flag, NULL ) ) {
        WdeWriteTrail( "WdeTViewDestroy: Control DESTROY failed" );
        return( false );
    }

    WRMemFree( obj );

    return( true );
}

bool WdeTViewValidateAction( WdeTViewObject *obj, ACTION_ID *act, void *p2 )
{
    int     i;

    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeTViewActions[i].id == *act ) {
            return( true );
        }
    }

    return( ValidateAction( obj->control, *act, p2 ) );
}

bool WdeTViewCopyObject( WdeTViewObject *obj, WdeTViewObject **new, OBJPTR handle )
{
    if( new == NULL ) {
        WdeWriteTrail( "WdeTViewCopyObject: Invalid new object!" );
        return( false );
    }

    *new = (WdeTViewObject *)WRMemAlloc( sizeof( WdeTViewObject ) );

    if( *new == NULL ) {
        WdeWriteTrail( "WdeTViewCopyObject: Object malloc failed" );
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
        WdeWriteTrail( "WdeTViewCopyObject: Control not created!" );
        WRMemFree( *new );
        return( false );
    }

    return( true );
}

bool WdeTViewIdentify( WdeTViewObject *obj, OBJ_ID *id, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    *id = obj->object_id;

    return( true );
}

bool WdeTViewGetWndProc( WdeTViewObject *obj, WNDPROC *proc, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *proc = WdeTViewSuperClassProc;

    return( true );
}

bool WdeTViewGetWindowClass( WdeTViewObject *obj, char **class, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *class = WWC_TREEVIEW;

    return( true );
}

bool WdeTViewDefine( WdeTViewObject *obj, POINT *pnt, void *p2 )
{
    WdeDefineObjectInfo  o_info;

    /* touch unused vars to get rid of warning */
    _wde_touch( pnt );
    _wde_touch( p2 );

    o_info.obj = obj->object_handle;
    o_info.obj_id = obj->object_id;
    o_info.mask = WS_VISIBLE | WS_DISABLED | WS_TABSTOP | WS_GROUP | WS_BORDER;
    o_info.set_func = (WdeSetProc)WdeTViewSetDefineInfo;
    o_info.get_func = (WdeGetProc)WdeTViewGetDefineInfo;
    o_info.hook_func = WdeTViewDefineHook;
    o_info.win = NULL;

    return( WdeControlDefine( &o_info ) );
}

void WdeTViewSetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
#ifdef __NT__XX
    DialogStyle mask;

    // set the tree view options
    mask = GETCTL_STYLE( o_info->info.c.info ) & 0x000000ff;
    if( mask & TVS_HASBUTTONS ) {
        CheckDlgButton( hDlg, IDB_TVS_HASBUTTONS, BST_CHECKED );
    }
    if( mask & TVS_HASLINES ) {
        CheckDlgButton( hDlg, IDB_TVS_HASLINES, BST_CHECKED );
    }
    if( mask & TVS_LINESATROOT ) {
        CheckDlgButton( hDlg, IDB_TVS_LINESATROOT, BST_CHECKED );
    }
    if( mask & TVS_EDITLABELS ) {
        CheckDlgButton( hDlg, IDB_TVS_EDITLABELS, BST_CHECKED );
    }
    if( mask & TVS_DISABLEDRAGDROP ) {
        CheckDlgButton( hDlg, IDB_TVS_DISABLEDRAGDROP, BST_CHECKED );
    }
    if( mask & TVS_SHOWSELALWAYS ) {
        CheckDlgButton( hDlg, IDB_TVS_SHOWSELALWAYS, BST_CHECKED );
    }

    // set the extended style controls only
    WdeEXSetDefineInfo( o_info, hDlg );
#else
    _wde_touch( o_info );
    _wde_touch( hDlg );
#endif
}

void WdeTViewGetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
#ifdef __NT__XX
    DialogStyle mask = 0;

    // get the tree view control settings
    if( IsDlgButtonChecked( hDlg, IDB_TVS_HASBUTTONS ) ) {
        mask |= TVS_HASBUTTONS;
    }
    if( IsDlgButtonChecked( hDlg, IDB_TVS_HASLINES ) ) {
        mask |= TVS_HASLINES;
    }
    if( IsDlgButtonChecked( hDlg, IDB_TVS_LINESATROOT ) ) {
        mask |= TVS_LINESATROOT;
    }
    if( IsDlgButtonChecked( hDlg, IDB_TVS_EDITLABELS ) ) {
        mask |= TVS_EDITLABELS;
    }
    if( IsDlgButtonChecked( hDlg, IDB_TVS_DISABLEDRAGDROP ) ) {
        mask |= TVS_DISABLEDRAGDROP;
    }
    if( IsDlgButtonChecked( hDlg, IDB_TVS_SHOWSELALWAYS ) ) {
        mask |= TVS_SHOWSELALWAYS;
    }

    SETCTL_STYLE( o_info->info.c.info,
                  (GETCTL_STYLE( o_info->info.c.info ) & 0xffff0000) | mask );

    // get the extended control settings
    WdeEXGetDefineInfo( o_info, hDlg );
#else
    _wde_touch( o_info );
    _wde_touch( hDlg );
#endif
}

bool WdeTViewDefineHook( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, DialogStyle mask )
{
    bool processed;

    /* touch unused vars to get rid of warning */
    _wde_touch( hDlg );
    _wde_touch( message );
    _wde_touch( wParam );
    _wde_touch( lParam );
    _wde_touch( mask );

    processed = false;

    return( processed );
}

LRESULT CALLBACK WdeTViewSuperClassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if( !WdeProcessMouse( hWnd, message, wParam, lParam ) ) {
        return( CallWindowProc( WdeOriginalTViewProc, hWnd, message, wParam, lParam ) );
    }
    return( FALSE );
}
