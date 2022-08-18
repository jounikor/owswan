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
#include "wdefstat.h"
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
} WdeStaticObject;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT LRESULT CALLBACK WdeStaticSuperClassProc( HWND, UINT, WPARAM, LPARAM );
WINEXPORT bool    CALLBACK WdeStaticDispatcher( ACTION_ID, OBJPTR, void *, void * );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static OBJPTR   WdeMakeStatic( OBJPTR, RECT *, OBJPTR, DialogStyle, char *, OBJ_ID );
static OBJPTR   WdeStatCreate( OBJPTR, RECT *, OBJPTR, OBJ_ID, WdeDialogBoxControl * );
static void     WdeStaticSetDefineInfo( WdeDefineObjectInfo *, HWND );
static void     WdeStaticGetDefineInfo( WdeDefineObjectInfo *, HWND );

#define pick(e,n,c) static bool WdeStatic ## n ## c;
    pick_ACTS( WdeStaticObject )
#undef pick

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static HINSTANCE                WdeApplicationInstance;
static DISPATCH_FN              *WdeStaticDispatch;
static WdeDialogBoxControl      *WdeDefaultStatic = NULL;
static int                      WdeStaticWndExtra;
static WNDPROC                  WdeOriginalStaticProc;
//static WNDPROC                WdeStaticProc;

static DISPATCH_ITEM WdeStaticActions[] = {
    #define pick(e,n,c) {e, (DISPATCH_RTN *)WdeStatic ## n},
    pick_ACTS( WdeStaticObject )
    #undef pick
};

#define MAX_ACTIONS     (sizeof( WdeStaticActions ) / sizeof( DISPATCH_ITEM ))

OBJPTR CALLBACK WdeFrameCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    if( handle == NULL ) {
        return( WdeMakeStatic( parent, obj_rect, handle,
                               SS_BLACKFRAME, "", FRAME_OBJ ) );
    } else {
        return( WdeStatCreate( parent, obj_rect, NULL, FRAME_OBJ,
                               (WdeDialogBoxControl *)handle ) );
    }
}

OBJPTR CALLBACK WdeTextCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    if( handle == NULL ) {
        return( WdeMakeStatic( parent, obj_rect, handle,
                               SS_LEFT, "Text", TEXT_OBJ ) );
    } else {
        return( WdeStatCreate( parent, obj_rect, NULL, TEXT_OBJ,
                               (WdeDialogBoxControl *)handle ) );
    }
}

OBJPTR CALLBACK WdeIconCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    if( handle == NULL ) {
        return( WdeMakeStatic( parent, obj_rect, handle,
                               SS_ICON, "DefaultIcon", ICON_OBJ ) );
    } else {
        return( WdeStatCreate( parent, obj_rect, NULL,
                               ICON_OBJ, (WdeDialogBoxControl *)handle ) );
    }
}

OBJPTR WdeMakeStatic( OBJPTR parent, RECT *obj_rect, OBJPTR handle,
                      DialogStyle style, char *text, OBJ_ID id )
{
    OBJPTR new;

    style |= WS_VISIBLE | WS_CHILD;
    SETCTL_STYLE( WdeDefaultStatic, style );
    SETCTL_TEXT( WdeDefaultStatic, ResStrToNameOrOrd( text ) );
    SETCTL_ID( WdeDefaultStatic, WdeGetNextControlID() );

    WdeChangeSizeToDefIfSmallRect( parent, id, obj_rect );

    new = WdeStatCreate( parent, obj_rect, handle, id, WdeDefaultStatic );

    WRMemFree( GETCTL_TEXT( WdeDefaultStatic ) );
    SETCTL_TEXT( WdeDefaultStatic, NULL );

    return( new );
}

OBJPTR WdeStatCreate( OBJPTR parent, RECT *obj_rect, OBJPTR handle,
                      OBJ_ID id, WdeDialogBoxControl *info )
{
    WdeStaticObject *new;

    WdeDebugCreate( "Static", parent, obj_rect, handle );

    if( parent == NULL ) {
        WdeWriteTrail( "WdeStaticCreate: Static has no parent!" );
        return( NULL );
    }

    new = (WdeStaticObject *)WRMemAlloc( sizeof( WdeStaticObject ) );
    if( new == NULL ) {
        WdeWriteTrail( "WdeStaticCreate: Object malloc failed" );
        return( NULL );
    }

    OBJ_DISPATCHER_SET( new, WdeStaticDispatch );
    new->object_id = id;
    if( handle == NULL ) {
        new->object_handle = (OBJPTR)new;
    } else {
        new->object_handle = handle;
    }

    new->control = Create( CONTROL_OBJ, parent, obj_rect, new->object_handle );

    if( new->control == NULL ) {
        WdeWriteTrail( "WdeStaticCreate: CONTROL_OBJ not created!" );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, SET_OBJECT_INFO, info, NULL ) ) {
        WdeWriteTrail( "WdeStaticCreate: SET_OBJECT_INFO failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, CREATE_WINDOW, NULL, NULL ) ) {
        WdeWriteTrail( "WdeStaticCreate: CREATE_WINDOW failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    return( (OBJPTR)new );
}

bool CALLBACK WdeStaticDispatcher( ACTION_ID act, OBJPTR obj, void *p1, void *p2 )
{
    int     i;

    WdeDebugDispatch( "Static", act, obj, p1, p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeStaticActions[i].id == act ) {
            return( WdeStaticActions[i].rtn( obj, p1, p2 ) );
        }
    }

    return( Forward( ((WdeStaticObject *)obj)->control, act, p1, p2 ) );
}

bool WdeStaticInit( bool first )
{
    WNDCLASS    wc;

    WdeApplicationInstance = WdeGetAppInstance();
    GetClassInfo( (HINSTANCE)NULL, "STATIC", &wc );
    WdeOriginalStaticProc = wc.lpfnWndProc;
    WdeStaticWndExtra = wc.cbWndExtra;

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
        wc.lpszClassName = "wdestatic";
        wc.cbWndExtra  += sizeof( OBJPTR );
        //wc.lpfnWndProc = WdeStaticSuperClassProc;
        if( !RegisterClass( &wc ) ) {
            WdeWriteTrail( "WdeStaticInit: RegisterClass failed." );
        }
#endif
    }

    WdeDefaultStatic = WdeAllocDialogBoxControl();
    if( WdeDefaultStatic == NULL ) {
        WdeWriteTrail( "WdeStaticInit: Alloc of control failed!" );
        return( false );
    }

    /* set up the default control structure */
    SETCTL_STYLE( WdeDefaultStatic, 0 );
    SETCTL_ID( WdeDefaultStatic, 0 );
    SETCTL_EXTRABYTES( WdeDefaultStatic, 0 );
    SETCTL_SIZEX( WdeDefaultStatic, 0 );
    SETCTL_SIZEY( WdeDefaultStatic, 0 );
    SETCTL_SIZEW( WdeDefaultStatic, 0 );
    SETCTL_SIZEH( WdeDefaultStatic, 0 );
    SETCTL_TEXT( WdeDefaultStatic, NULL );
    SETCTL_CLASSID( WdeDefaultStatic, ResNumToControlClass( CLASS_STATIC ) );

    WdeStaticDispatch = MakeProcInstance_DISPATCHER( WdeStaticDispatcher, WdeGetAppInstance() );
    return( true );
}

void WdeStaticFini( void )
{
    WdeFreeDialogBoxControl( &WdeDefaultStatic );
    FreeProcInstance_DISPATCHER( WdeStaticDispatch );
}

bool WdeStaticDestroy( WdeStaticObject *obj, bool *flag, bool *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    if( !Forward( obj->control, DESTROY, flag, NULL ) ) {
        WdeWriteTrail( "WdeStaticDestroy: Control DESTROY failed" );
        return( false );
    }

    WRMemFree( obj );

    return( true );
}

bool WdeStaticValidateAction( WdeStaticObject *obj, ACTION_ID *act, void *p2 )
{
    int     i;

    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeStaticActions[i].id == *act ) {
            return( true );
        }
    }

    return( ValidateAction( obj->control, *act, p2 ) );
}

bool WdeStaticCopyObject( WdeStaticObject *obj, WdeStaticObject **new, OBJPTR handle )
{
    if( new == NULL ) {
        WdeWriteTrail( "WdeStaticCopyObject: Invalid new object!" );
        return( false );
    }

    *new = (WdeStaticObject *)WRMemAlloc( sizeof( WdeStaticObject ) );

    if( *new == NULL ) {
        WdeWriteTrail( "WdeStaticCopyObject: Object malloc failed" );
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
        WdeWriteTrail( "WdeStaticCopyObject: Control not created!" );
        WRMemFree( *new );
        return( false );
    }

    return( true );
}

bool WdeStaticIdentify( WdeStaticObject *obj, OBJ_ID *id, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    *id = obj->object_id;

    return( true );
}

bool WdeStaticGetWndProc( WdeStaticObject *obj, WNDPROC *proc, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *proc = WdeStaticSuperClassProc;

    return( true );
}

bool WdeStaticGetWindowClass( WdeStaticObject *obj, char **class, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *class = "static";

    return( true );
}

bool WdeStaticDefine( WdeStaticObject *obj, POINT *pnt, void *p2 )
{
    WdeDefineObjectInfo  o_info;

    /* touch unused vars to get rid of warning */
    _wde_touch( pnt );
    _wde_touch( p2 );

    o_info.obj = obj->object_handle;
    o_info.obj_id = obj->object_id;
    o_info.mask = WS_VISIBLE | WS_DISABLED | WS_TABSTOP | WS_GROUP;
    o_info.set_func = (WdeSetProc)WdeStaticSetDefineInfo;
    o_info.get_func = (WdeGetProc)WdeStaticGetDefineInfo;
    o_info.hook_func = NULL;
    o_info.win = NULL;

    return( WdeControlDefine( &o_info ) );
}

void WdeStaticSetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
    OBJ_ID      id;
    DialogStyle mask;

    id = o_info->obj_id;

    mask = GETCTL_STYLE( o_info->info.c.info ) & (0x0000ffff ^ SS_NOPREFIX);

    if( id  == FRAME_OBJ ) {
#if __NT__XX
        mask &= ~SS_SUNKEN;
#endif
        if( mask == SS_BLACKRECT ) {
            CheckDlgButton( hDlg, IDB_SS_BLACKRECT, BST_CHECKED );
        } else if( mask == SS_GRAYRECT ) {
            CheckDlgButton( hDlg, IDB_SS_GRAYRECT, BST_CHECKED );
        } else if( mask == SS_WHITERECT ) {
            CheckDlgButton( hDlg, IDB_SS_WHITERECT, BST_CHECKED );
        } else if( mask == SS_BLACKFRAME ) {
            CheckDlgButton( hDlg, IDB_SS_BLACKFRAME, BST_CHECKED );
        } else if( mask == SS_GRAYFRAME ) {
            CheckDlgButton( hDlg, IDB_SS_GRAYFRAME, BST_CHECKED );
        } else if( mask == SS_WHITEFRAME ) {
            CheckDlgButton( hDlg, IDB_SS_WHITEFRAME, BST_CHECKED );
#if __NT__XX
        } else if( mask == SS_ETCHEDFRAME ) {
            CheckDlgButton( hDlg, IDB_SS_ETCHEDFRAME, BST_CHECKED );
        } else if( mask == SS_ETCHEDHORZ ) {
            CheckDlgButton( hDlg, IDB_SS_ETCHEDHORZ, BST_CHECKED );
        } else if( mask == SS_ETCHEDVERT ) {
            CheckDlgButton( hDlg, IDB_SS_ETCHEDVERT, BST_CHECKED );
#endif
        } else {
            WdeWriteTrail( "WdeStaticSetDefineInfo: Bad Frame mask!" );
        }

#if __NT__XX
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_SUNKEN ) {
            CheckDlgButton( hDlg, IDB_SS_SUNKEN, BST_CHECKED );
        }
#endif

    } else if( id == TEXT_OBJ ) {

#if __NT__XX
        mask &= ~SS_NOTIFY;
        mask &= ~SS_SUNKEN;
#endif

        if( mask == SS_LEFT ) {
            CheckDlgButton( hDlg, IDB_SS_LEFT, BST_CHECKED );
        } else if( mask == SS_CENTER ) {
            CheckDlgButton( hDlg, IDB_SS_CENTER, BST_CHECKED );
        } else if( mask == SS_RIGHT ) {
            CheckDlgButton( hDlg, IDB_SS_RIGHT, BST_CHECKED );
        } else if( mask == SS_SIMPLE ) {
            CheckDlgButton( hDlg, IDB_SS_SIMPLE, BST_CHECKED );
        } else if( mask == SS_LEFTNOWORDWRAP ) {
            CheckDlgButton( hDlg, IDB_SS_LEFTNOWORDWRAP, BST_CHECKED );
        } else {
            WdeWriteTrail( "WdeStaticSetDefineInfo: Bad Text mask!" );
        }

        if( GETCTL_STYLE( o_info->info.c.info ) & SS_NOPREFIX ) {
            CheckDlgButton( hDlg, IDB_SS_NOPREFIX, BST_CHECKED );
        }
#if __NT__XX
        EnableWindow( GetDlgItem( hDlg, IDB_SS_NOTIFY ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_SUNKEN ), TRUE );
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_NOTIFY ) {
            CheckDlgButton( hDlg, IDB_SS_NOTIFY, BST_CHECKED );
        }
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_SUNKEN ) {
            CheckDlgButton( hDlg, IDB_SS_SUNKEN, BST_CHECKED );
        }
#else
        EnableWindow( GetDlgItem( hDlg, IDB_SS_NOTIFY ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_SUNKEN ), FALSE );
#endif

    } else if( id == ICON_OBJ ) {

#if __NT__XX
        EnableWindow( GetDlgItem( hDlg, IDB_SS_ICON ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_BITMAP ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_ENHMETAFILE ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_CENTERIMAGE ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_RIGHTJUST ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_REALSIZEIMAGE ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_SUNKEN ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_NOTIFY ), TRUE );

        mask = GETCTL_STYLE( o_info->info.c.info ) & 0x0000000f;
        if( mask == SS_ICON ) {
            CheckDlgButton( hDlg, IDB_SS_ICON, BST_CHECKED );
        } else if( mask == SS_BITMAP ) {
            CheckDlgButton( hDlg, IDB_SS_BITMAP, BST_CHECKED );
        } else if( mask == SS_ENHMETAFILE ) {
            CheckDlgButton( hDlg, IDB_SS_ENHMETAFILE, BST_CHECKED );
        } else {
            WdeWriteTrail( "WdeStaticSetDefineInfo: Bad Image mask!" );
        }
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_CENTERIMAGE ) {
            CheckDlgButton( hDlg, IDB_SS_CENTERIMAGE, BST_CHECKED );
        }
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_RIGHTJUST ) {
            CheckDlgButton( hDlg, IDB_SS_RIGHTJUST, BST_CHECKED );
        }
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_REALSIZEIMAGE ) {
            CheckDlgButton( hDlg, IDB_SS_REALSIZEIMAGE, BST_CHECKED );
        }
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_SUNKEN ) {
            CheckDlgButton( hDlg, IDB_SS_SUNKEN, BST_CHECKED );
        }
        if( GETCTL_STYLE( o_info->info.c.info ) & SS_NOTIFY ) {
            CheckDlgButton( hDlg, IDB_SS_NOTIFY, BST_CHECKED );
        }
#else
        // only leave the ICON radio button on
        EnableWindow( GetDlgItem( hDlg, IDB_SS_ICON ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_BITMAP ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_ENHMETAFILE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_CENTERIMAGE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_RIGHTJUST ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_REALSIZEIMAGE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_SUNKEN ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDB_SS_NOTIFY ), FALSE );

        if( mask != SS_ICON ) {
            WdeWriteTrail( "WdeStaticSetDefineInfo: Bad Icon mask!" );
        }
#endif
    } else {
        WdeWriteTrail( "WdeStaticSetDefineInfo: Bad OBJ_ID!" );
    }

    WdeEXSetDefineInfo( o_info, hDlg );
}

void WdeStaticGetDefineInfo( WdeDefineObjectInfo *o_info, HWND hDlg )
{
    OBJ_ID      id;
    DialogStyle mask;

    id = o_info->obj_id;
    mask = 0;

    if( id == FRAME_OBJ ) {

        if( IsDlgButtonChecked( hDlg, IDB_SS_BLACKRECT ) ) {
            mask = SS_BLACKRECT;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_GRAYRECT ) ) {
            mask = SS_GRAYRECT;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_WHITERECT ) ) {
            mask = SS_WHITERECT;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_BLACKFRAME ) ) {
            mask = SS_BLACKFRAME;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_GRAYFRAME ) ) {
            mask = SS_GRAYFRAME;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_WHITEFRAME ) ) {
            mask = SS_WHITEFRAME;
#if __NT__XX
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_ETCHEDFRAME ) ) {
            mask = SS_ETCHEDFRAME;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_ETCHEDHORZ ) ) {
            mask = SS_ETCHEDHORZ;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_ETCHEDVERT ) ) {
            mask = SS_ETCHEDVERT;
#endif
        } else {
            WdeWriteTrail( "WdeStaticGetDefineInfo: Bad Frame style!" );
            return;
        }

#if __NT__XX
        if( IsDlgButtonChecked( hDlg, IDB_SS_SUNKEN ) ) {
            mask |= SS_SUNKEN;
        }
#endif
    } else if( id == TEXT_OBJ ) {

        if( IsDlgButtonChecked( hDlg, IDB_SS_LEFT ) ) {
            mask = SS_LEFT;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_CENTER ) ) {
            mask = SS_CENTER;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_RIGHT ) ) {
            mask = SS_RIGHT;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_SIMPLE ) ) {
            mask = SS_SIMPLE;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_LEFTNOWORDWRAP ) ) {
            mask = SS_LEFTNOWORDWRAP;
        } else {
            WdeWriteTrail( "WdeStaticGetDefineInfo: Bad Text style!" );
            return;
        }

        if( IsDlgButtonChecked( hDlg, IDB_SS_NOPREFIX ) ) {
            mask |= SS_NOPREFIX;
        }
#if __NT__XX
        if( IsDlgButtonChecked( hDlg, IDB_SS_NOTIFY ) ) {
            mask |= SS_NOTIFY;
        }
        if( IsDlgButtonChecked( hDlg, IDB_SS_SUNKEN ) ) {
            mask |= SS_SUNKEN;
        }
#endif

    } else if( id == ICON_OBJ ) {

#if __NT__XX
        if( IsDlgButtonChecked( hDlg, IDB_SS_ICON ) ) {
            mask = SS_ICON;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_BITMAP ) ) {
            mask = SS_BITMAP;
        } else if( IsDlgButtonChecked( hDlg, IDB_SS_ENHMETAFILE ) ) {
            mask = SS_ENHMETAFILE;
        } else {
            WdeWriteTrail( "WdeStaticSetDefineInfo: Bad Image mask!" );
        }

        if( IsDlgButtonChecked( hDlg, IDB_SS_CENTERIMAGE ) ) {
            mask |= SS_CENTERIMAGE;
        }
        if( IsDlgButtonChecked( hDlg, IDB_SS_RIGHTJUST ) ) {
            mask |= SS_RIGHTJUST;
        }
        if( IsDlgButtonChecked( hDlg, IDB_SS_REALSIZEIMAGE ) ) {
            mask |= SS_REALSIZEIMAGE;
        }
        if( IsDlgButtonChecked( hDlg, IDB_SS_SUNKEN ) ) {
            mask |= SS_SUNKEN;
        }
        if( IsDlgButtonChecked( hDlg, IDB_SS_NOTIFY ) ) {
            mask |= SS_NOTIFY;
        }
#else
        mask = SS_ICON;
#endif

    } else {
        WdeWriteTrail( "WdeButtonGetDefineInfo: Bad OBJ_ID!" );
        return;
    }

    SETCTL_STYLE( o_info->info.c.info,
                  (GETCTL_STYLE( o_info->info.c.info ) & 0xffff0000) | mask );

    // set the extended mask - same for Frame, Icon (Image) and Text
    WdeEXSetDefineInfo( o_info, hDlg );
}

LRESULT CALLBACK WdeStaticSuperClassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if( !WdeProcessMouse( hWnd, message, wParam, lParam ) ) {
        return( CallWindowProc( WdeOriginalStaticProc, hWnd, message, wParam, lParam ) );
    }
    return( FALSE );
}
