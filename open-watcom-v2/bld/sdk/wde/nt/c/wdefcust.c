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


#include "wdeglbl.h"
#include "rcstr.grh"
#include "wderesin.h"
#include "wdeopts.h"
#include "wdestat.h"
#include "wdedefin.h"
#include "wdeactn.h"
#include "wdemain.h"
#include "wderes.h"
#include "wdesdup.h"
#include "wdecust.h"
#include "wdelist.h"
#include "wdedebug.h"
#include "wde_wres.h"
#include "wdefutil.h"
#include "wdedefsz.h"
#include "wdefcust.h"
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
    WNDPROC     win_proc;
    int         win_extra;
    char        *win_class;
    OBJPTR      object_handle;
    OBJ_ID      object_id;
    OBJPTR      control;
    WdeCustLib  *cust_lib;
    UINT        cust_index;
} WdeCustomObject;

typedef struct {
    char    *class;
    char    *new_name;
    int     win_extra;
    WNDPROC win_proc;
} WdeCustClassNode;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT LRESULT    CALLBACK WdeCustomSuperClassProc( HWND, UINT, WPARAM, LPARAM );
WINEXPORT bool       CALLBACK WdeCustomDispatcher( ACTION_ID, OBJPTR, void *, void * );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static void     WdeChooseCustControlFromList( LIST *, WdeDialogBoxControl *, WdeCustLib **, UINT * );
static bool     WdeChooseCustControlType( LPCCINFO, WdeDialogBoxControl *, WdeCustLib **, UINT *, uint_32 * );
static OBJPTR   WdeMakeCustom( OBJPTR, RECT *, OBJPTR, int );
static OBJPTR   WdeCustomCreater( OBJPTR, RECT *, OBJPTR, OBJ_ID, WdeDialogBoxControl *, WdeCustLib *, UINT );
static bool     WdeAddNewClassToList( char *, char *, int, WNDPROC );
static LIST     *WdeFindClassInList( char * );
static bool     WdeCustomRegisterClass( char *, HINSTANCE, char **, int *, WNDPROC * );
static void     WdeFreeClassList( void );
static void     WdeFreeClassNode( WdeCustClassNode * );

#define pick(e,n,c) static bool WdeCustom ## n ## c;
    pick_ACTS( WdeCustomObject )
#undef pick

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static HINSTANCE                WdeApplicationInstance;
static DISPATCH_FN              *WdeCustomDispatch;
static WdeDialogBoxControl      *WdeDefaultCustom = NULL;
static LIST                     *WdeCustClassList = NULL;
static char                     WdeClassName[MAX_NAME];

static DISPATCH_ITEM WdeCustomActions[] = {
    #define pick(e,n,c) {e, (DISPATCH_RTN *)WdeCustom ## n},
    pick_ACTS( WdeCustomObject )
    #undef pick
};

#define MAX_ACTIONS     (sizeof( WdeCustomActions ) / sizeof( DISPATCH_ITEM ))

void WdeChooseCustControlFromList( LIST *info_list, WdeDialogBoxControl *control,
                                   WdeCustLib **lib, UINT *index )
{
    LIST        *ilist;
    LPCCINFO    lpcci;
    uint_32     min_hd;
    bool        found;
    char        temp[5];

    min_hd = 32;
    *lib = NULL;
    *index = 0;

    found = false;

    if( info_list != NULL ) {
        for( ilist = info_list; ilist != NULL; ilist = ListNext( ilist ) ) {
            lpcci = (LPCCINFO)ListElement( ilist );
            found = WdeChooseCustControlType( lpcci, control, lib, index, &min_hd );
            if( found ) {
                break;
            }
        }
    }

    if( !found && min_hd != 0 ) {
        ultoa( min_hd, temp, 10 );
        WdeWriteTrail( "WdeChooseCustControlFromList: "
                       "Selected custom with hamming distance:" );
        WdeWriteTrail( temp );
    }
}

bool WdeChooseCustControlType( LPCCINFO lpcci, WdeDialogBoxControl *control,
                               WdeCustLib **lib, UINT *index, uint_32 *min_hd )
{
    bool    found;
    uint_32 s1;
    uint_32 s2;
    uint_32 new_min;

    found = false;

    s1 = lpcci->flStyleDefault;
    s1 &= 0x0000ffff;

    s2 = GETCTL_STYLE( control ) & 0x0000ffff;

    if( s1 == s2 ) {
        found = WdeFindLibIndexFromInfo( lpcci, lib, index );
    } else {
        new_min = WdeHammingDistance( s1, s2 );
        if( new_min < *min_hd ) {
            WdeFindLibIndexFromInfo( lpcci, lib, index );
            *min_hd = new_min;
        }
    }

    return( found );
}

OBJPTR CALLBACK WdeCustomCreate1( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    return( WdeMakeCustom( parent, obj_rect, handle, 0 ) );
}

OBJPTR CALLBACK WdeCustomCreate2( OBJPTR parent, RECT *obj_rect, OBJPTR handle )
{
    return( WdeMakeCustom( parent, obj_rect, handle, 1 ) );
}

static bool WdeCheckForSmallRect( OBJPTR parent, WdeCustLib *cust_lib,
                           UINT cust_index, RECT *obj_rect )
{
    int                 width;
    int                 height;
    WdeResizeRatio      r;

    if( parent == NULL || cust_lib == NULL || obj_rect == NULL ) {
        return( false );
    }

    /* check if the objects size is greater than min allowed */
    if( obj_rect->right - obj_rect->left >= MIN_X ||
        obj_rect->bottom - obj_rect->top >= MIN_Y ) {
        return( true );
    }

    width = cust_lib->lpcci[cust_index].cxDefault;
    height = cust_lib->lpcci[cust_index].cyDefault;

    if( !Forward( parent, GET_RESIZER, &r, NULL ) ) {
        return( false );
    }

    WdeMapCustomSize( &width, &height, &r );

    obj_rect->right = obj_rect->left + width;
    obj_rect->bottom = obj_rect->top + height;

    return( true );
}

OBJPTR WdeMakeCustom( OBJPTR parent, RECT *obj_rect, OBJPTR handle, int which )
{
    DialogStyle         style;
    OBJPTR              ret;
    WdeCustLib          *cust_lib;
    UINT                cust_index;
    char                *class_name;
    WdeDialogBoxControl *control;
    LIST                *info_list;

    info_list = NULL;
    cust_lib = NULL;
    cust_index = 0;

    if( handle == NULL ) {
        WdeGetCurrentCustControl( which, &cust_lib, &cust_index );

        if( cust_lib == NULL ) {
            if( !WdeSetCurrentCustControl( which ) ) {
                WdeWriteTrail( "WdeMakeCustom: WdeSetCurrentCustControl failed!" );
                return( NULL );
            }
            WdeGetCurrentCustControl( which, &cust_lib, &cust_index );
        }

        if( cust_lib == NULL ) {
            WdeWriteTrail( "WdeMakeCustom: No custom controls!" );
            return( NULL );
        }

        style = cust_lib->lpcci[cust_index].flStyleDefault;
        style &= ~WS_POPUP;
        style |= (WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP);
        SETCTL_STYLE( WdeDefaultCustom, style );
        SETCTL_TEXT( WdeDefaultCustom,
                     ResStrToNameOrOrd( cust_lib->lpcci[cust_index].szTextDefault ) );
        SETCTL_CLASSID( WdeDefaultCustom,
                        WdeStrToControlClass( cust_lib->lpcci[cust_index].szClass ) );

        SETCTL_ID( WdeDefaultCustom, WdeGetNextControlID() );

        control = WdeDefaultCustom;
    } else {
        control = (WdeDialogBoxControl *)handle;
        class_name = WdeControlClassToStr( GETCTL_CLASSID( control ) );
        if( class_name != NULL ) {
            WdeFindClassInAllCustLibs( class_name, &info_list );
            WRMemFree( class_name );
        }
        if( info_list == NULL ) {
            WdeSetStatusByID( 0, WDE_NOCUSTOMFORCLASS );
            return( NULL );
        }
        WdeChooseCustControlFromList( info_list, control, &cust_lib, &cust_index );
        if( cust_lib == NULL ) {
            WdeWriteTrail( "WdeMakeCustom: No custom control fits this class & style!" );
            WdeSetStatusByID( 0, WDE_CANTFINDCUSTOM );
            return( NULL );
        }
    }

    WdeCheckForSmallRect( parent, cust_lib, cust_index, obj_rect );

    ret = WdeCustomCreater( parent, obj_rect, NULL, CUSTCNTL1_OBJ + which,
                            control, cust_lib, cust_index );

    if( handle == NULL ) {
        WRMemFree( GETCTL_TEXT( WdeDefaultCustom ) );
        WRMemFree( GETCTL_CLASSID( WdeDefaultCustom ) );
    }

    SETCTL_STYLE( WdeDefaultCustom, 0 );
    SETCTL_TEXT( WdeDefaultCustom, NULL );
    SETCTL_CLASSID( WdeDefaultCustom, NULL );

    return( ret );
}

void WdeFreeClassList( void )
{
    LIST             *clist;
    WdeCustClassNode *node;

    for( clist = WdeCustClassList; clist != NULL; clist = ListNext( clist ) ) {
        node = (WdeCustClassNode *)ListElement( clist );
        WdeFreeClassNode( node );
    }
}

void WdeFreeClassNode( WdeCustClassNode *node )
{
    if( node != NULL ) {
        if( node->class != NULL ) {
            WRMemFree( node->class );
        }
        if( node->new_name != NULL ) {
            WRMemFree( node->new_name );
        }
        WRMemFree( node );
    }
}

bool WdeAddNewClassToList( char *class, char *new_name, int win_extra, WNDPROC win_proc )
{
    WdeCustClassNode *node;
    char             *str;

    node = (WdeCustClassNode *)WRMemAlloc( sizeof( WdeCustClassNode ) );
    if( node == NULL ) {
        WdeWriteTrail( "WdeAddNewClassToList: node alloc failed!" );
        return( false );
    }

    str = WdeStrDup( class );
    if( str == NULL ) {
        WdeWriteTrail( "WdeAddNewClassToList: class strdup failed!" );
        WRMemFree( node );
        return( false );
    }
    node->class = str;

    str = WdeStrDup( new_name );
    if( str == NULL ) {
        WdeWriteTrail( "WdeAddNewClassToList: new_name alloc failed!" );
        WRMemFree( node->class );
        WRMemFree( node );
        return( false );
    }
    node->new_name = str;

    node->win_extra = win_extra;
    node->win_proc = win_proc;

    WdeInsertObject( &WdeCustClassList, (OBJPTR)node );

    return( true );
}

LIST *WdeFindClassInList( char *class )
{
    LIST             *clist;
    WdeCustClassNode *node;

    for( clist = WdeCustClassList; clist != NULL; clist = ListNext( clist ) ) {
        node = (WdeCustClassNode *)ListElement( clist );
        if( stricmp( node->class, class ) == 0 ) {
            break;
        }
    }

    return( clist );
}

bool WdeCustomRegisterClass( char *class, HINSTANCE inst, char **new_name,
                             int *win_extra, WNDPROC *win_proc )
{
    WdeCustClassNode    *node;
    WNDCLASS            wc;
    LIST                *clist;

    if( (clist = WdeFindClassInList( class )) != NULL ) {
        node = (WdeCustClassNode *)ListElement( clist );
        *win_extra = node->win_extra;
        *win_proc = node->win_proc;
        *new_name = WdeStrDup( node->new_name );
        if( *new_name == NULL ) {
            WdeWriteTrail( "WdeCustomRegisterClass: new_name alloc failed!" );
            return( false );
        }
        return( true );
    }

    if( !GetClassInfo( inst, class, &wc ) ) {
        WdeWriteTrail( "WdeCustomRegisterClass: GetClassInfo failed!" );
        return( false );
    }

    *new_name = (char *)WRMemAlloc( strlen( class ) + 5 );
    if( *new_name == NULL ) {
        WdeWriteTrail( "WdeCustomRegisterClass: new_name alloc failed!" );
        return( false );
    }
    strcpy( *new_name, "WDE_" );
    strcat( *new_name, class );

    *win_extra = wc.cbWndExtra;
    *win_proc = wc.lpfnWndProc;

    if( wc.style & CS_GLOBALCLASS ) {
        wc.style ^= CS_GLOBALCLASS;
    }
    if( wc.style & CS_PARENTDC ) {
        wc.style ^= CS_PARENTDC;
    }
    wc.style |= (CS_HREDRAW | CS_VREDRAW);

    wc.hInstance = WdeApplicationInstance;
    wc.lpszClassName = *new_name;
    wc.cbWndExtra += sizeof( LONG_PTR );
    //wc.lpfnWndProc = WdeCustomSuperClassProc;

    if( !RegisterClass( &wc ) ) {
        WdeWriteTrail( "WdeCustomRegisterClass: RegisterClass failed!" );
        // subclass controls instead of superclassing them makes this
        // much less fatal
        //WRMemFree( *new_name );
        //return( false );
    }

    if( !WdeAddNewClassToList( class, *new_name, *win_extra, *win_proc ) ) {
        WdeWriteTrail( "WdeCustomRegisterClass: AddNewClass failed!" );
        WRMemFree( *new_name );
        return( false );
    }

    return( true );
}

OBJPTR WdeCustomCreater( OBJPTR parent, RECT *obj_rect, OBJPTR handle,
                         OBJ_ID id, WdeDialogBoxControl *info,
                         WdeCustLib *cust_lib, UINT cust_index )
{
    WdeCustomObject *new;
    char            *class;

    WdeDebugCreate( "Custom", parent, obj_rect, handle );

    WRMemValidate( parent );

    if( parent == NULL ) {
        WdeWriteTrail( "WdeCustomCreate: Custom has no parent!" );
        return( NULL );
    }

    new = (WdeCustomObject *)WRMemAlloc( sizeof( WdeCustomObject ) );
    if( new == NULL ) {
        WdeWriteTrail( "WdeCustomCreate: Object malloc failed" );
        return( NULL );
    }

    class = cust_lib->lpcci[cust_index].szClass;

    if( !WdeCustomRegisterClass( class, cust_lib->inst, &new->win_class,
                                 &new->win_extra, &new->win_proc ) ) {
        WdeWriteTrail( "WdeCustomCreate: WdeCustomRegisterClass failed!" );
        WRMemFree( new );
        return( NULL );
    }

    OBJ_DISPATCHER_SET( new, WdeCustomDispatch );
    new->object_id = id;
    new->cust_lib = cust_lib;
    new->cust_index = cust_index;

    if( handle == NULL ) {
        new->object_handle = (OBJPTR)new;
    } else {
        new->object_handle = handle;
    }

    new->control = Create( CONTROL_OBJ, parent, obj_rect, new->object_handle );

    if( new->control == NULL ) {
        WdeWriteTrail( "WdeCustomCreate: CONTROL_OBJ not created!" );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, SET_OBJECT_INFO, info, NULL ) ) {
        WdeWriteTrail( "WdeCustomCreate: SET_OBJECT_INFO failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    if( !Forward( new->object_handle, CREATE_WINDOW, NULL, NULL ) ) {
        WdeWriteTrail( "WdeCustomCreate: CREATE_WINDOW failed!" );
        Destroy( new->control, false );
        WRMemFree( new );
        return( NULL );
    }

    return( (OBJPTR)new );
}

bool CALLBACK WdeCustomDispatcher( ACTION_ID act, OBJPTR obj, void *p1, void *p2 )
{
    int     i;

    WdeDebugDispatch( "Custom", act, obj, p1, p2 );

    WRMemChkRange( obj, sizeof( WdeCustomObject ) );

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeCustomActions[i].id == act ) {
            return( WdeCustomActions[i].rtn( obj, p1, p2 ) );
        }
    }

    return( Forward( ((WdeCustomObject *)obj)->control, act, p1, p2 ) );
}

bool WdeCustomInit( bool first )
{
    _wde_touch( first );
    WdeApplicationInstance = WdeGetAppInstance();

    WdeDefaultCustom = WdeAllocDialogBoxControl();
    if( WdeDefaultCustom == NULL ) {
        WdeWriteTrail( "WdeCustomInit: Alloc of control failed!" );
        return( false );
    }

    /* set up the default control structure */
    SETCTL_STYLE( WdeDefaultCustom, 0 );
    SETCTL_ID( WdeDefaultCustom, 0 );
    SETCTL_EXTRABYTES( WdeDefaultCustom, 0 );
    SETCTL_SIZEX( WdeDefaultCustom, 0 );
    SETCTL_SIZEY( WdeDefaultCustom, 0 );
    SETCTL_SIZEW( WdeDefaultCustom, 0 );
    SETCTL_SIZEH( WdeDefaultCustom, 0 );
    SETCTL_TEXT( WdeDefaultCustom, NULL );
    SETCTL_CLASSID( WdeDefaultCustom, NULL );

    WdeCustomDispatch = MakeProcInstance_DISPATCHER( WdeCustomDispatcher, WdeGetAppInstance() );

    return( true );
}

void WdeCustomFini( void )
{
    WdeFreeClassList();
    WdeFreeDialogBoxControl( &WdeDefaultCustom );
    FreeProcInstance_DISPATCHER( WdeCustomDispatch );
}

bool WdeCustomDestroy( WdeCustomObject *obj, bool *flag, bool *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    if( !Forward( obj->control, DESTROY, flag, NULL ) ) {
        WdeWriteTrail( "WdeCustomDestroy: Control DESTROY failed" );
        return( false );
    }

    if( obj->win_class != NULL ) {
        WRMemFree( obj->win_class );
    }

    WRMemFree( obj );

    return( true );
}

bool WdeCustomValidateAction( WdeCustomObject *obj, ACTION_ID *act, void *p2 )
{
    int     i;

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( WdeCustomActions[i].id == *act ) {
            return( true );
        }
    }

    return( ValidateAction( obj->control, *act, p2 ) );
}

bool WdeCustomCopyObject( WdeCustomObject *obj, WdeCustomObject **new, OBJPTR handle )
{
    if( new == NULL ) {
        WdeWriteTrail( "WdeCustomCopyObject: Invalid new object!" );
        return( false );
    }

    *new = (WdeCustomObject *)WRMemAlloc( sizeof( WdeCustomObject ) );

    if( *new == NULL ) {
        WdeWriteTrail( "WdeCustomCopyObject: Object malloc failed" );
        return( false );
    }

    OBJ_DISPATCHER_COPY( *new, obj );
    (*new)->win_proc = obj->win_proc;
    (*new)->win_extra = obj->win_extra;
    (*new)->object_id = obj->object_id;
    (*new)->cust_lib = obj->cust_lib;
    (*new)->cust_index = obj->cust_index;

    (*new)->win_class = WdeStrDup( obj->win_class );
    if( (*new)->win_class == NULL ) {
        WdeWriteTrail( "WdeCustomCopyObject: Class alloc failed!" );
        WRMemFree( *new );
        return( false );
    }

    if( handle == NULL ) {
        (*new)->object_handle = (OBJPTR)*new;
    } else {
        (*new)->object_handle = handle;
    }

    if( !CopyObject( obj->control, &(*new)->control, (*new)->object_handle ) ) {
        WdeWriteTrail( "WdeCustomCopyObject: Control not created!" );
        WRMemFree( *new );
        return( false );
    }

    return( true );
}

bool WdeCustomIdentify( WdeCustomObject *obj, OBJ_ID *id, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( p2 );

    *id = obj->object_id;

    return( true );
}

bool WdeCustomGetWndProc( WdeCustomObject *obj, WNDPROC *proc, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *proc = WdeCustomSuperClassProc;

    return( true );
}

bool WdeCustomGetWindowClass( WdeCustomObject *obj, char **class, void *p2 )
{
    /* touch unused vars to get rid of warning */
    _wde_touch( obj );
    _wde_touch( p2 );

    *class = obj->win_class;

    return( true );
}

bool WdeCustomDefine( WdeCustomObject *obj, POINT *pnt, void *p2 )
{
    bool                redraw;
    HWND                dialog_owner;
    LPCCINFO            lpcci;
    CCSTYLE             ccs;
    WdeDefineObjectInfo o_info;
    WdeDialogBoxControl *info;
    char                *text;
    int                 tlen;
    LPFNCCSTYLE         proc;

    /* touch unused vars to get rid of warning */
    _wde_touch( pnt );
    _wde_touch( p2 );

    if( !Forward( (OBJPTR)obj, GET_WINDOW_HANDLE, &o_info.win, NULL ) ) {
        WdeWriteTrail( "WdeControlDefine: GET_WINDOW_HANDLE failed!" );
        return( false );
    }

    if( !Forward( obj->object_handle, GET_OBJECT_INFO,
                  &o_info.info.c.info, &o_info.symbol ) ) {
        WdeWriteTrail( "WdeCustomDefine: GET_OBJECT_INFO failed!" );
        return( false );
    }

    if( !WdeGetOption( WdeOptUseDefDlg ) ) {
        o_info.obj = obj->object_handle;
        o_info.obj_id = obj->object_id;
        o_info.mask = 0xffff;
        o_info.hook_func = WdeWinStylesHook;
        o_info.set_func = NULL;
        o_info.get_func = NULL;
        o_info.res_info = WdeGetCurrentRes();
        return( WdeGenericDefine( &o_info ) );
    }

    info = o_info.info.c.info;

    dialog_owner = WdeGetMainWindowHandle();

    WdeSetStatusText( NULL, "", false );
    WdeSetStatusByID( WDE_DEFININGCUSTOM, 0 );

    lpcci = &obj->cust_lib->lpcci[obj->cust_index];

    ccs.flStyle = GETCTL_STYLE( info );
    ccs.flExtStyle = 0L;
    ccs.lgid = 0;
    ccs.wReserved1 = 0;

    if( (text = WdeResNameOrOrdinalToStr( GETCTL_TEXT( info ), 10 )) != NULL ) {
        tlen = strlen( text );
        if( tlen < CCHCCTEXT ) {
            strcpy( ccs.szText, text );
        } else {
            memcpy( ccs.szText, text, CCHCCTEXT );
            ccs.szText[CCHCCTEXT - 1] = '\0';
        }
        WRMemFree( text );
    } else {
         ccs.szText[0] = '\0';
    }

    proc = lpcci->lpfnStyle;

    redraw = ( (*proc)( dialog_owner, &ccs ) != 0 );

    if( redraw ) {

        SETCTL_STYLE( info, (DialogStyle)ccs.flStyle );

        if( !Forward( obj->object_handle, DESTROY_WINDOW, NULL, NULL ) ) {
            WdeWriteTrail( "WdeCustomDefine: DESTROY_WINDOW failed!" );
            return( false );
        }

        if( !Forward( obj->object_handle, CREATE_WINDOW, NULL, NULL ) ) {
            WdeWriteTrail( "WdeCustomDefine: CREATE_WINDOW failed!" );
            return( false );
        }

        Notify( obj->object_handle, CURRENT_OBJECT, NULL );
    }

    WdeSetStatusReadyText();

    return( true );
}

LRESULT CALLBACK WdeCustomSuperClassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    WNDPROC             wnd_proc;
    int                 extra;
    LIST                *clist;
    WdeCustClassNode    *node;

    if( WdeProcessMouse( hWnd, message, wParam, lParam ) ) {
        return( FALSE );
    }

    extra = (int)GET_CBWNDEXTRA( hWnd );
    extra -= sizeof( LONG_PTR );
    wnd_proc = (WNDPROC)GET_WNDLONGPTR( hWnd, extra );
    if( wnd_proc == NULL ) {
        if( GetClassName( hWnd, WdeClassName, sizeof( WdeClassName ) ) == 0 ) {
            WdeWriteTrail( "WdeCustomSuperClassProc: GetClassName failed!" );
            return( FALSE );
        }
        clist = WdeFindClassInList( &WdeClassName[4] );
        if( clist == NULL ) {
            WdeWriteTrail( "WdeCustomSuperClassProc: FindClassInList failed!" );
            return( FALSE );
        }
        node = (WdeCustClassNode *)ListElement( clist );
        wnd_proc = node->win_proc;
        if( wnd_proc == NULL ) {
            WdeWriteTrail( "WdeCustomSuperClassProc: NULL wnd_proc!" );
            return( FALSE );
        }
        SET_WNDLONGPTR( hWnd, extra, (LONG_PTR)wnd_proc );
    }

    return( CallWindowProc( wnd_proc, hWnd, message, wParam, lParam ) );
}
