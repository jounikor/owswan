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
#include "wderes.h"
#include "wdeopts.h"
#include "wdestat.h"
#include "wdedefin.h"
#include "wdeactn.h"
#include "wdemain.h"
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
    DISPATCH_FN     *dispatcher;
    WNDPROC         win_proc;
    int             win_extra;
    char            *win_class;
    OBJPTR          object_handle;
    OBJ_ID          object_id;
    OBJPTR          control;
    WdeCustControl  *cust_info;
    UINT            cust_type;
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
WINEXPORT LRESULT   CALLBACK WdeCustomSuperClassProc( HWND, UINT, WPARAM, LPARAM );
WINEXPORT bool      CALLBACK WdeCustomDispatcher( ACTION_ID, OBJPTR, void *, void * );
WINEXPORT WORD      CALLBACK WdeIDToStr( WORD, LPSTR, WORD );
WINEXPORT DWORD     CALLBACK WdeStrToID( LPSTR );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static void     WdeChooseCustControlFromList( LIST *, WdeDialogBoxControl *, WdeCustControl **, UINT * );
static bool     WdeChooseCustControlType( WdeCustControl *, WdeDialogBoxControl *, WdeCustControl **, UINT *, uint_32 * );
static OBJPTR   WdeMakeCustom( OBJPTR, RECT *, OBJPTR, int );
static OBJPTR   WdeCustomCreater( OBJPTR, RECT *, OBJPTR, OBJ_ID, WdeDialogBoxControl *, WdeCustControl *, UINT );
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
static FARPROC                  WdeStr2ID;
static FARPROC                  WdeID2Str;
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
                                   WdeCustControl **rinfo, UINT *rtype )
{
    LIST            *ilist;
    WdeCustControl  *info;
    uint_32         min_hd;
    bool            found;
    char            temp[5];

    min_hd = 32;
    *rinfo = NULL;

    found = false;

    if( info_list != NULL ) {
        for( ilist = info_list; ilist != NULL; ilist = ListNext( ilist ) ) {
            info = (WdeCustControl *)ListElement( ilist );
            found = WdeChooseCustControlType( info, control, rinfo, rtype, &min_hd );
            if( found ) {
                break;
            }
        }
    }

    if( !found ) {
        ultoa( min_hd, temp, 10 );
        WdeWriteTrail( "WdeChooseCustControlFromList: "
                       "Selected custom with hamming distance:" );
        WdeWriteTrail( temp );
    }
}

bool WdeChooseCustControlType( WdeCustControl *info, WdeDialogBoxControl *control,
                               WdeCustControl **rinfo, UINT *rtype, uint_32 *min_hd )
{
    uint_16 type;
    bool    found;
    uint_32 s1;
    uint_32 s2;
    uint_32 new_min;

    /* if this class just has one type ASSUME (YIKES!!) that this is the
     * one we are looking for.
     */
    if( info->control_info.ms.wCtlTypes <= 1 ) {
        *rinfo = info;
        *rtype = 0;
        return( true );
    }

    found = false;

    for( type = 0; type < info->control_info.ms.wCtlTypes; type++ ) {
        if( info->ms_lib ) {
            s1 = info->control_info.ms.Type[type].dwStyle;
        } else {
            s1 = info->control_info.bor.Type[type].dwStyle;
        }
        s1 &= 0x0000ffff;
        s2 = GETCTL_STYLE( control ) & 0x0000ffff;
        if( s1 == s2 ) {
            found = true;
            break;
        }
        new_min = WdeHammingDistance( s1, s2 );
        if( new_min < *min_hd ) {
            *rinfo = info;
            *rtype = type;
            *min_hd = new_min;
        }
    }

    if( found ) {
        *rinfo = info;
        *rtype = type;
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

static bool WdeCheckForSmallRect( OBJPTR parent, WdeCustControl *cust_info,
                           UINT cust_type, RECT *obj_rect )
{
    int                 width;
    int                 height;
    WdeResizeRatio      r;

    if( parent == NULL || cust_info == NULL || obj_rect == NULL ) {
        return( false );
    }

    /* check if the objects size is greater than min allowed */
    if( obj_rect->right - obj_rect->left >= MIN_X ||
        obj_rect->bottom - obj_rect->top >= MIN_Y ) {
        return( true );
    }

    if( cust_info->ms_lib ) {
        width = cust_info->control_info.ms.Type[cust_type].wWidth;
        height = cust_info->control_info.ms.Type[cust_type].wHeight;
    } else {
        width = cust_info->control_info.bor.Type[cust_type].wWidth;
        height = cust_info->control_info.bor.Type[cust_type].wHeight;
    }

    if( !Forward( parent, GET_RESIZER, &r, NULL ) ) {
        return( false );
    }

    WdeMapCustomSize( &width, &height, &r );

    obj_rect->right = obj_rect->left + width;
    obj_rect->bottom = obj_rect->top  + height;

    return( true );
}

OBJPTR WdeMakeCustom( OBJPTR parent, RECT *obj_rect, OBJPTR handle, int which )
{
    DialogStyle         style;
    OBJPTR              ret;
    WdeCustControl      *cust_info;
    UINT                cust_type;
    char                *class_name;
    WdeDialogBoxControl *control;
    LIST                *info_list;

    info_list = NULL;
    cust_info = NULL;
    cust_type = 0;

    if( handle == NULL ) {
        WdeGetCurrentCustControl( which, &cust_info, &cust_type );

        if( cust_info == NULL ) {
            if( !WdeSetCurrentCustControl( which ) ) {
                WdeWriteTrail( "WdeMakeCustom: WdeSetCurrentCustControl failed!" );
                return( NULL );
            }
            WdeGetCurrentCustControl( which, &cust_info, &cust_type );
        }

        if( cust_info == NULL ) {
            WdeWriteTrail( "WdeMakeCustom: No custom controls!" );
            return( NULL );
        }

        if( cust_info->ms_lib ) {
            style = cust_info->control_info.ms.Type[cust_type].dwStyle;
            SETCTL_TEXT( WdeDefaultCustom,
                         ResStrToNameOrOrd( cust_info->control_info.ms.szTitle ) );
            SETCTL_CLASSID( WdeDefaultCustom,
                            WdeStrToControlClass( cust_info->control_info.ms.szClass ) );
        } else {
            style = cust_info->control_info.bor.Type[cust_type].dwStyle;
            SETCTL_TEXT( WdeDefaultCustom,
                         ResStrToNameOrOrd( cust_info->control_info.bor.szTitle ) );
            SETCTL_CLASSID( WdeDefaultCustom,
                            WdeStrToControlClass( cust_info->control_info.bor.szClass ) );
        }

        SETCTL_ID( WdeDefaultCustom, WdeGetNextControlID() );

        style &= ~WS_POPUP;
        style |= (WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP);

        SETCTL_STYLE( WdeDefaultCustom, style );

        control = WdeDefaultCustom;

    } else {
        control = (WdeDialogBoxControl *)handle;
        class_name = WdeControlClassToStr( GETCTL_CLASSID( control ) );
        if( class_name != NULL ) {
            WdeFindClassInAllCustLibs( class_name, &info_list );
            WRMemFree( class_name );
        }
        if( info_list == NULL ) {
            WdeWriteTrail( "WdeMakeCustom: There are no custom controls of this class!" );
            WdeSetStatusByID( 0, WDE_NOCUSTOMFORCLASS );
            return( NULL );
        }
        WdeChooseCustControlFromList( info_list, control, &cust_info, &cust_type );
        if( cust_info == NULL ) {
            WdeWriteTrail( "WdeMakeCustom: No custom control fits this class & style!" );
            WdeSetStatusByID( 0, WDE_CANTFINDCUSTOM );
            return( NULL );
        }
    }

    WdeCheckForSmallRect( parent, cust_info, cust_type, obj_rect );

    ret = WdeCustomCreater( parent, obj_rect, NULL, CUSTCNTL1_OBJ + which,
                            control, cust_info, cust_type );

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

bool WdeAddNewClassToList( char *class, char *new_name,
                           int win_extra, WNDPROC win_proc )
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
                         WdeCustControl *cust_info, UINT cust_type )
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

    if( cust_info->ms_lib ) {
        class = cust_info->control_info.ms.szClass;
    } else {
        class = cust_info->control_info.bor.szClass;
    }

    if( !WdeCustomRegisterClass( class, cust_info->lib->inst, &new->win_class,
                                 &new->win_extra, &new->win_proc ) ) {
        WdeWriteTrail( "WdeCustomCreate: WdeCustomRegisterClass failed!" );
        WRMemFree( new );
        return( NULL );
    }

    OBJ_DISPATCHER_SET( new, WdeCustomDispatch );
    new->object_id = id;
    new->cust_info = cust_info;
    new->cust_type = cust_type;

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

    WdeStr2ID = MakeProcInstance ( (FARPROC)WdeStrToID, WdeApplicationInstance );
    WdeID2Str = MakeProcInstance ( (FARPROC)WdeIDToStr, WdeApplicationInstance );

    return( true );
}

void WdeCustomFini( void )
{
    WdeFreeClassList();
    WdeFreeDialogBoxControl( &WdeDefaultCustom );
    FreeProcInstance_DISPATCHER( WdeCustomDispatch );
    FreeProcInstance( WdeStr2ID );
    FreeProcInstance( WdeID2Str );
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
    (*new)->cust_info = obj->cust_info;
    (*new)->cust_type = obj->cust_type;

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
    bool                        redraw;
    HWND                        dialog_owner;
    HGLOBAL                     ctl_style;
    uint_32                     ctl_size;
    WDECTLSTYLE                 *ctl_data;
    WdeCustStyleProc            proc;
    WdeDialogBoxControl         *info;
    char                        *text;
    int                         tlen;
    WdeDefineObjectInfo         o_info;

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

    if( obj->cust_info->ms_lib ) {
        ctl_size = sizeof( CTLSTYLE );
    } else {
        ctl_size = sizeof( WDECTLSTYLE );
    }

    ctl_style = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, ctl_size );
    if( ctl_style == NULL ) {
        WdeWriteTrail( "WdeCustomDefine: Could not GlobalAlloc ctl_style!" );
        return( false );
    }

    ctl_data = (WDECTLSTYLE *)GlobalLock( ctl_style );
    if( ctl_data == NULL ) {
        WdeWriteTrail( "WdeCustomDefine: Could not GlobalLock ctl_data!" );
        GlobalFree( ctl_style );
        return( false );
    }

    ctl_data->wX = GETCTL_SIZEX( info );
    ctl_data->wY = GETCTL_SIZEY( info );
    ctl_data->wCx = GETCTL_SIZEW( info );
    ctl_data->wCy = GETCTL_SIZEH( info );
    ctl_data->wId = GETCTL_ID( info );
    ctl_data->dwStyle = GETCTL_STYLE( info );

    if( (text = WdeResNameOrOrdinalToStr( GETCTL_TEXT( info ), 10 )) != NULL ) {
        tlen = strlen( text );
        if( tlen < CTLCLASS ) {
            strcpy( ctl_data->szTitle, text );
        } else {
            memcpy( ctl_data->szTitle, text, CTLCLASS );
            ctl_data->szTitle[CTLCLASS - 1] = '\0';
        }
        WRMemFree( text );
    } else {
         ctl_data->szTitle[0] = '\0';
    }

    strcpy( ctl_data->szClass, obj->cust_info->control_info.ms.szClass );

    if( !obj->cust_info->ms_lib ) {
        ctl_data->CtlDataSize = 0;
        memset( ctl_data->CtlData, 0, CTLDATALENGTH );
    }

    GlobalUnlock( ctl_style );

    proc = obj->cust_info->style_proc;

    redraw = ( (*proc)( dialog_owner, ctl_style, (LPFNSTRTOID)WdeStr2ID, (LPFNIDTOSTR)WdeID2Str ) != 0 );

    if( redraw ) {

        ctl_data = (WDECTLSTYLE *)GlobalLock( ctl_style );
        if( ctl_data == NULL ) {
            WdeWriteTrail( "WdeCustomDefine: Could not GlobalLock ctl_data!" );
            GlobalFree( ctl_style );
            return( false );
        }

        SETCTL_SIZEX( info, ctl_data->wX );
        SETCTL_SIZEY( info, ctl_data->wY );
        SETCTL_SIZEW( info, ctl_data->wCx );
        SETCTL_SIZEH( info, ctl_data->wCy );
        SETCTL_ID( info, ctl_data->wId );
        SETCTL_STYLE( info, ctl_data->dwStyle );

        WRMemFree( GETCTL_TEXT( info ) );
        WRMemFree( GETCTL_CLASSID( info ) );
        SETCTL_TEXT( info, ResStrToNameOrOrd( ctl_data->szTitle ) );
        SETCTL_CLASSID( info, WdeStrToControlClass( ctl_data->szClass ) );

        GlobalUnlock( ctl_style );

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

    GlobalFree( ctl_style );

    WdeSetStatusReadyText();

    return( true );
}

WORD CALLBACK WdeIDToStr( WORD id, LPSTR str, WORD len )
{
    char s[11];
    WORD slen;

    utoa( id, s, 10 );
    slen = strlen( s );
    if( slen > len - 1 ) {
        return( 0 );
    }
    strcpy( str, s );

    return( slen );
}

DWORD CALLBACK WdeStrToID( LPSTR str )
{
    uint_32 num;
    DWORD   ret;
    char    *ep;

    num = (uint_32)strtoul( str, &ep, 0 );
    if( *ep != '\0' ) {
        ret = MAKELONG( (UINT)0, (UINT)0 );
    } else {
        ret = MAKELONG( (UINT)1, (UINT)num );
    }

    return( ret );
}

LRESULT CALLBACK WdeCustomSuperClassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    WNDPROC             wnd_proc;
    uint_16             extra;
    LIST                *clist;
    WdeCustClassNode    *node;

    if( WdeProcessMouse( hWnd, message, wParam, lParam ) ) {
        return( FALSE );
    }

    extra = (uint_16)GetClassWord( hWnd, GCW_CBWNDEXTRA );
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
