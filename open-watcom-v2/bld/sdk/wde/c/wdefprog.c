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
#include "wdefprog.h"
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
} WdeProgObject;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT bool    CALLBACK WdeProgDispatcher( ACTION_ID, OBJPTR, void *, void * );
WINEXPORT LRESULT CALLBACK WdeProgSuperClassProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static OBJPTR   WdeMakeProg( OBJPTR, RECT *, OBJPTR, DialogStyle, char *, OBJ_ID );
static OBJPTR   WdeProgressCreate( OBJPTR, RECT *, OBJPTR, OBJ_ID, WdeDialogBoxControl * );
static void     WdeProgSetDefineInfo( WdeDefineObjectInfo *, HWND );
static void     WdeProgGetDefineInfo( WdeDefineObjectInfo *, HWND );
static bool     WdeProgDefineHook( HWND, UINT, WPARAM, LPARAM, DialogStyle );

#define pick(e,n,c) static bool WdeProg ## n ## c;
    pick_ACTS( WdeProgObject )
#undef pick

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static HINSTANCE                WdeApplicationInstance;
static DISPATCH_FN              *WdeProgDispatch;
static WdeDialogBoxControl      *WdeDefaultProg = NULL;
static int                      WdeProgWndExtra;
static WNDPROC                  WdeOriginalProgProc;
//static WNDPROC                WdeProgProc;

#define WPROGRESS_CLASS  PROGRESS_CLASS

static DISPATCH_ITEM WdeProgActions[] = {
    #define pick(e,n,c) {e, (DISPATCH_RTN *)WdeProg ## n},
    pick_ACTS( WdeProgObject )
    #undef pick
};

#define MAX_ACTIONS      (sizeof( WdeProgActions ) / sizeof( DISPATCH_ITEM ))

OBJPTR CALLBACK WdeProgCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    if( handle == NULL ) {
        return( WdeMakeProg( parent, obj_rect, handle, 0, "", PROGRESS_OBJ ) );
    } else {
        return( WdeProgressCreate( parent, obj_rect, NULL, PROGRESS_OBJ, (WdeDialogBoxControl *)handle ) );
    }
}

OBJPTR WdeMakeProg( OBJPTR parent, RECT *obj_rect, OBJPTR handle, DialogStyle style, char *text, OBJ_ID id )
{
    OBJPTR new;

    style |= WS_BORDER | WS_VISIBLE | WS_TABSTOP | WS_CHILD;

    SETCTL_STYLE( WdeDefaultProg, style );
    SETCTL_TEXT( WdeDefaultProg, ResStrToNameOrOrd( text ) );
    SETCTL_ID( WdeDefaultProg, WdeGetNextControlID() );

    WdeChangeSizeToDefIfSmallRect( parent, id, obj_rect );

    new = WdeProgressCreate( parent, obj_rect, handle, id, WdeDefaultProg );

    WRMemFree( GETCTL_TEXT( WdeDefaultProg ) );
    SETCTL_TEXT( WdeDefaultProg, NULL );

    return( new );
}

OBJPTR WdeProgressCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle,
                          OBJ_ID id, WdeDialogBoxControl *info )
{
    WdeProgObject *new;

    WdeDebugCreate( "Prog", parent, obj_rect, handle );

    if( parent == NULL ) {
        WdeWriteTrail( "WdeProgCreate: Prog has no parent!" );
        return( NULL );
    }

    new = (WdeProgObject *)WRMemAlloc( sizeof( WdeProgObject ) );
    if( new == NULL ) {
        WdeWriteTrail( "WdeProgCreate: Object malloc failed" );
        return( NULL );
    }

    OBJ_DISPATCHER_SET( new, WdeProgDispatch );
    new->object_id = id;
    if( handle == NULL ) {
        new->object_handle = (OBJPTR)new;
    } else {
        new->object_handle = handle;
    }

    new->control = Create( CONTROL_OBJ, parent, obj_rect, new->object_handle );

    if( new->control == NULL ) {
        WdeWriteTrail( "WdeProgCreate: CONTROL_OBJ not created!" );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, SET_OBJECT_INFO, info, NULL ) ) {
        WdeWriteTrail( "WdeProgCreate: SET_OBJECT_INFO failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, CREATE_WINDOW, NULL, NULL ) ) {
        WdeWriteTrail( "WdeProgCreate: CREATE_WINDOW failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    return( (OBJPTR)new );
}

bool CALLBACK WdeProgDispatcher( ACTION_ID act, OBJPTR obj, void *p1, void *p2 )
{
    int     i;

    WdeDebugDispatch( "Prog", act, obj, p1, p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeProgActions[i].id == act ) {
            return( WdeProgActions[i].rtn( obj, p1, p2 ) );
        }
    }

    return( Forward( ((WdeProgObject *)obj)->control, act, p1, p2 ) );
}

bool WdeProgInit( bool first )
{
    WNDCLASS    wc;

    WdeApplicationInstance = WdeGetAppInstance();
    GetClassInfo( (HINSTANCE)NULL, WPROGRESS_CLASS, &wc );
    WdeOriginalProgProc = wc.lpfnWndProc;
    WdeProgWndExtra = wc.cbWndExtra;

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
        //wc.lpfnWndProc = WdeProgSuperClassProc;
        if( !RegisterClass( &wc ) ) {
            WdeWriteTrail( "WdeProgInit: RegisterClass failed." );
        }
#endif
    }

    WdeDefaultProg = WdeAllocDialogBoxControl();
    if( !WdeDefaultProg ) {
        WdeWriteTrail( "WdeProgInit: Alloc of control failed!" );
        return( false );
    }

    /* set up the default control structure */
    SETCTL_STYLE( WdeDefaultProg, WS_BORDER | WS_VISIBLE | WS_TABSTOP | WS_GROUP );
    SETCTL_ID( WdeDefaultProg, 0 );
    SETCTL_EXTRABYTES( WdeDefaultProg, 0 );
    SETCTL_SIZEX( WdeDefaultProg, 0 );
    SETCTL_SIZEY( WdeDefaultProg, 0 );
    SETCTL_SIZEW( WdeDefaultProg, 0 );
    SETCTL_SIZEH( WdeDefaultProg, 0 );
    SETCTL_TEXT( WdeDefaultProg, NULL );
    SETCTL_CLASSID( WdeDefaultProg, WdeStrToControlClass( WPROGRESS_CLASS ) );

    WdeProgDispatch = MakeProcInstance_DISPATCHER( WdeProgDispatcher, WdeGetAppInstance() );
    return( true );
}

void WdeProgFini( void )
{
    WdeFreeDialogBoxControl( &WdeDefaultProg );
    FreeProcInstance_DISPATCHER( WdeProgDispatch );
}

bool WdeProgDestroy( WdeProgObject *obj, bool *flag, bool *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    if( !Forward( obj->control, DESTROY, flag, NULL ) ) {
        WdeWriteTrail( "WdeProgDestroy: Control DESTROY failed" );
        return( false );
    }

    WRMemFree( obj );

    return( true );
}

bool WdeProgValidateAction( WdeProgObject *obj, ACTION_ID *act, void *p2 )
{
    int     i;

    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeProgActions[i].id == *act ) {
            return( true );
        }
    }

    return( ValidateAction( obj->control, *act, p2 ) );
}

bool WdeProgCopyObject( WdeProgObject *obj, WdeProgObject **new, OBJPTR handle )
{
    if( new == NULL ) {
        WdeWriteTrail( "WdeProgCopyObject: Invalid new object!" );
        return( false );
    }

    *new = (WdeProgObject *)WRMemAlloc( sizeof( WdeProgObject ) );

    if( *new == NULL ) {
        WdeWriteTrail( "WdeProgCopyObject: Object malloc failed" );
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
        WdeWriteTrail( "WdeProgCopyObject: Control not created!" );
        WRMemFree( *new );
        return( false );
    }

    return( true );
}

bool WdeProgIdentify( WdeProgObject *obj, OBJ_ID *id, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    *id = obj->object_id;

    return( true );
}

bool WdeProgGetWndProc( WdeProgObject *obj, WNDPROC *proc, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *proc = WdeProgSuperClassProc;

    return( true );
}

bool WdeProgGetWindowClass( WdeProgObject *obj, char **class, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *class = WPROGRESS_CLASS;

    return( true );
}

bool WdeProgDefine( WdeProgObject *obj, POINT *pnt, void *p2 )
{
    WdeDefineObjectInfo  o_info;

    /* touch unused vars to get rid of warning */
    _wde_touch( pnt );
    _wde_touch( p2 );

    o_info.obj = obj->object_handle;
    o_info.obj_id = obj->object_id;
    o_info.mask = WS_VISIBLE | WS_DISABLED | WS_TABSTOP | WS_GROUP | WS_BORDER;
    o_info.set_func = (WdeSetProc)WdeProgSetDefineInfo;
    o_info.get_func = (WdeGetProc)WdeProgGetDefineInfo;
    o_info.hook_func = WdeProgDefineHook;
    o_info.win = NULL;

    return( WdeControlDefine( &o_info ) );
}

void WdeProgSetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
    // there are no styles for the progress bar control
    // set the extended style controls only
    WdeEXSetDefineInfo( o_info, hDlg );
}

void WdeProgGetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
    // clear the style flags, since there are none for a progress bar
    SETCTL_STYLE( o_info->info.c.info,
                  GETCTL_STYLE( o_info->info.c.info ) & 0xffff0000 );

    // get the extended control settings
    WdeEXGetDefineInfo ( o_info, hDlg );
}

bool WdeProgDefineHook( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, DialogStyle mask )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( hDlg );
    _wde_touch( message );
    _wde_touch( wParam );
    _wde_touch( lParam );
    _wde_touch( mask );

    return( false );
}

LRESULT CALLBACK WdeProgSuperClassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if( !WdeProcessMouse( hWnd, message, wParam, lParam ) ) {
        return( CallWindowProc( WdeOriginalProgProc, hWnd, message, wParam, lParam ) );
    }
    return( FALSE );
}
