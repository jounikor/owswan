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
#include "wdelist.h"
#include "wdedebug.h"
#include "wdefdiag.h"
#include "wdefont.h"
#include "wdeedit.h"
#include "wdefutil.h"
#include "wdestyle.h"
#include "wdefordr.h"

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define TAG_FACENAME   "Helv"
#define TAG_POINTSIZE  8
#define TAG_WIDTH      32
#define TAG_HEIGHT     17

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/* Local Window callback functions prototypes */
WINEXPORT LRESULT CALLBACK WdeTagProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/

/****************************************************************************/
/* external variables                                                       */
/****************************************************************************/

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static int        WdeTagExtra           = 0;
static WNDPROC    WdeOriginalButtonProc = NULL;
static HINSTANCE  WdeAppInst            = NULL;
static HFONT      WdeTagFont            = NULL;
static char       WdeTagClass[]         = "wdetag";

static void WdeSetTagState( WdeOrderedEntry *oe )
{
    LRESULT     result;
    bool        pressed;

    if( oe != NULL && oe->tag != (HWND)NULL ) {
        if( oe->mode == WdeSetOrder ) {
            result = SendMessage( oe->tag, BM_GETSTATE, 0, 0 );
            pressed = ((result & BST_PUSHED) != 0);
            if( pressed && !oe->pos_set ) {
                SendMessage( oe->tag, BM_SETSTATE, FALSE, 0 );
            } else if( !pressed && oe->pos_set ) {
                SendMessage( oe->tag, BM_SETSTATE, TRUE, 0 );
            }
        } else {
            SendMessage( oe->tag, BM_SETSTATE, FALSE, 0 );
        }
    }
}

static void WdeSetTagText( WdeOrderedEntry *oe )
{
    char        str[10];
    int         len;

    if( oe != NULL && oe->tag != (HWND)NULL ) {
        itoa( oe->pos, str, 10 );
        len = strlen( str );
        switch( oe->mode ) {
        case WdeSetTabs:
            if( oe->tab_set ) {
                strcat( str, ":T" );
            }
            break;
        case WdeSetGroups:
            if( oe->grp_set ) {
                strcat( str, ":G" );
            }
            break;
        }
        SendMessage( oe->tag, WM_SETTEXT, 0, (LPARAM)(LPCSTR)str );
    }
}

static void WdeSetTagOrder( WdeSetOrderStruct *o, bool reorder )
{
    if( o->new_oe ) {
        ListRemoveElt( &o->lists->newlist, (OBJPTR)o->new_oe );
        WRMemFree( o->new_oe );
        o->new_oe = NULL;
        o->old_oe->present = TRUE;
        o->old_oe->pos_set = FALSE;
    } else {
        o->new_oe = (WdeOrderedEntry *)WRMemAlloc( sizeof( WdeOrderedEntry ) );
        if( o->new_oe != NULL ) {
            o->old_oe->pos_set = TRUE;
            memcpy( o->new_oe, o->old_oe, sizeof( WdeOrderedEntry ) );
            o->old_oe->present = FALSE;
            o->old_oe->pos = 0;
            WdeInsertObject( &o->lists->newlist, (OBJPTR)o->new_oe );
        }
    }

    if( reorder ) {
        WdeReorderTags( o->lists, FALSE );
    }
}

static void WdeOrderPrevTags( WdeSetOrderStruct *o )
{
    LIST                *olist;
    WdeOrderedEntry     *oentry;
    WdeSetOrderStruct   *os;

    if( o == NULL || o->lists == NULL ) {
        return;
    }

    for( olist = o->lists->oldlist; olist != NULL; olist = ListNext( olist ) ) {
        oentry = (WdeOrderedEntry *)ListElement( olist );
        os = WdeGetTagInfo( oentry->tag );
        if( os == o ) {
            break;
        }
        if( !oentry->pos_set ) {
            WdeSetTagOrder( os, FALSE );
        }
    }
}

static void WdeTagDblClicked( WdeSetOrderStruct *o )
{
    LIST                *olist;
    WdeOrderedEntry     *oentry;
    WdeSetOrderStruct   *os;

    if( o == NULL || o->lists == NULL ) {
        return;
    }

    if( o->old_oe->mode != WdeSetOrder ) {
        return;
    }

    if( o->lists->newlist ) {
        WdeFreeOrderedList( o->lists->newlist );
        o->lists->newlist = NULL;
    }

    for( olist = o->lists->oldlist; olist != NULL; olist = ListNext( olist ) ) {
        oentry = (WdeOrderedEntry *)ListElement( olist );
        oentry->present = TRUE;
        oentry->pos_set = FALSE;
        os = WdeGetTagInfo( oentry->tag );
        if( os != NULL ) {
            os->new_oe = NULL;
        }
    }

    WdeTagPressed( o );

    WdeReorderTags( o->lists, TRUE );
}

void WdeFreeOrderedList( LIST *list )
{
    LIST                *olist;
    WdeOrderedEntry     *oe;

    for( olist = list; olist != NULL; olist = ListNext( olist ) ) {
        oe = (WdeOrderedEntry *)ListElement( olist );
        if( oe != NULL ) {
            WRMemFree( oe );
        }
    }

    ListFree( list );
}

LIST *WdeCopyOrderedList( LIST *src )
{
    LIST            *dest;

    dest = NULL;

    if( !WdeListConcat( &dest, src, sizeof( WdeOrderedEntry ) ) ) {
        WdeFreeOrderedList( dest );
        dest = NULL;
    }

    return( dest );
}

LIST *WdeFindOrderedEntry( LIST *list, OBJPTR obj )
{
    WdeOrderedEntry *oentry;
    LIST            *olist;

    for( olist = list; olist != NULL; olist = ListNext( olist ) ) {
        oentry = (WdeOrderedEntry *)ListElement( olist );
        if( oentry->obj == obj ) {
            return( olist );
        }
    }

    return( NULL );
}

bool WdeAddOrderedEntry( LIST **list, OBJPTR obj )
{
    WdeOrderedEntry *oentry;
    LIST            *olist;

    if( list == NULL ) {
        return( false );
    }

    if( (olist = WdeFindOrderedEntry( *list, obj )) != NULL ) {
        oentry = (WdeOrderedEntry *)ListElement ( olist );
        oentry->present = TRUE;
        return( true );
    }

    oentry = (WdeOrderedEntry *)WRMemAlloc( sizeof( WdeOrderedEntry ) );
    if( oentry != NULL ) {
        memset( oentry, 0, sizeof( WdeOrderedEntry ) );
        oentry->obj = obj;
        oentry->present = TRUE;
        WdeInsertObject( list, (OBJPTR)oentry );
    }

    return( oentry != NULL );
}

bool WdeRemoveOrderedEntry( LIST *list, OBJPTR obj )
{
    WdeOrderedEntry *oentry;
    LIST            *olist;

    if( (olist = WdeFindOrderedEntry( list, obj )) != NULL ) {
        oentry = (WdeOrderedEntry *)ListElement( olist );
        oentry->present = FALSE;
        return( true );
    }

    return( false );
}

bool WdeCleanOrderedList( LIST **list )
{
    WdeOrderedEntry *oentry;
    LIST            *tlist;
    LIST            *olist;

    if( list == NULL ) {
        return( false );
    }

    tlist = WdeListCopy( *list );

    for( olist = tlist; olist != NULL; olist = ListNext( olist ) ) {
        oentry = (WdeOrderedEntry *)ListElement( olist );
        if( !oentry->present ) {
            ListRemoveElt( list, (OBJPTR)oentry );
            WRMemFree( oentry );
        }
    }

    if( tlist != NULL ) {
        ListFree( tlist );
    }

    return( true );
}

bool WdeGetNextChild( LIST **list, OBJPTR *obj, bool up )
{
    WdeOrderedEntry *oentry;
    LIST            *o;

    WdeCleanOrderedList( list );

    if( list != NULL && *list != NULL && obj != NULL && *obj != NULL &&
        (o = WdeFindOrderedEntry( *list, *obj )) != NULL ) {
        if( up ) {
            o = ListNext( o );
            if( o == NULL ) {
                o = *list;
            }
        } else {
            o = ListPrev( o );
            if( o == NULL ) {
                WdeListLastElt( *list, &o );
            }
        }
        oentry = (WdeOrderedEntry *)ListElement( o );
        *obj = oentry->obj;
        return( true );
    }

    return( false );
}

void WdeFiniOrderStuff( void )
{
    if( WdeTagFont != (HFONT)NULL ) {
        DeleteObject( WdeTagFont );
    }
}

bool WdeRegisterTagClass( HINSTANCE inst )
{
    WNDCLASS  wc;

    WdeAppInst = inst;

    WdeTagFont = WdeGetFont( TAG_FACENAME, TAG_POINTSIZE, FW_BOLD );

    GetClassInfo( (HINSTANCE)NULL, "BUTTON", &wc );

    wc.style &= ~CS_GLOBALCLASS;
    wc.style |= CS_HREDRAW | CS_VREDRAW;

    wc.hInstance = inst;
    wc.lpszClassName = WdeTagClass;

    WdeTagExtra = wc.cbWndExtra;
    wc.cbWndExtra += sizeof( LONG_PTR );

    WdeOriginalButtonProc = wc.lpfnWndProc;
    wc.lpfnWndProc = WdeTagProc;

    return( RegisterClass( &wc ) != (ATOM)0 );
}

void WdeDestroyTag ( HWND tag )
{
    if( tag != (HWND)NULL && IsWindow( tag ) ) {
        DestroyWindow( tag );
    }
}

HWND WdeCreateTag( HWND parent, WdeSetOrderStruct *o )
{
    HWND        tag;
    RECT        rect;

    if( o == NULL || o->res_info == NULL || parent == (HWND)NULL ) {
        return( (HWND)NULL );
    }

    GetWindowRect( parent, &rect );
    MapWindowPoints( (HWND)NULL, o->res_info->forms_win, (POINT *)&rect, 2 );

    tag = CreateWindow( WdeTagClass, NULL, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        rect.left, rect.top, TAG_WIDTH, TAG_HEIGHT,
                        o->res_info->forms_win, (HMENU)NULL, WdeAppInst, o );

    if( tag != NULL ) {
        if( WdeTagFont != (HFONT)NULL ) {
            SendMessage( tag, WM_SETFONT, (WPARAM)WdeTagFont, FALSE );
        }
        WdeSetTagText( o->old_oe );
    }

    return( tag );
}

void WdeReorderTags( WdeSetOrderLists *ol, bool force_redraw )
{
    int             pos;
    LIST            *olist;
    WdeOrderedEntry *oentry;

    pos = 1;

    for( olist = ol->newlist; olist != NULL; olist = ListNext( olist ) ) {
        oentry = (WdeOrderedEntry *)ListElement( olist );
        if( oentry->present ) {
            if( force_redraw || oentry->pos != pos ) {
                oentry->pos = pos;
                WdeSetTagText( oentry );
            }
            WdeSetTagState( oentry );
            pos++;
        }
    }

    for( olist = ol->oldlist; olist != NULL; olist = ListNext( olist ) ) {
        oentry = (WdeOrderedEntry *)ListElement ( olist );
        if( oentry->present ) {
            if( force_redraw || oentry->pos != pos ) {
                oentry->pos = pos;
                WdeSetTagText( oentry );
            }
            WdeSetTagState( oentry );
            pos++;
        }
    }
}

void WdeTagPressed( WdeSetOrderStruct *o )
{
    OBJPTR      parent;

    if( o != NULL ) {
        switch( o->old_oe->mode ) {
        case WdeSetOrder:
            if( GetKeyState( VK_SHIFT ) < 0 ) {
                WdeOrderPrevTags( o );
            }
            WdeSetTagOrder( o, TRUE );
            break;
        case WdeSetTabs:
            o->old_oe->tab_set = !o->old_oe->tab_set;
            if( o->new_oe != NULL ) {
                o->new_oe->tab_set = o->old_oe->tab_set;
                WdeSetTagText( o->new_oe );
            } else {
                WdeSetTagText( o->old_oe );
            }
            break;
        case WdeSetGroups:
            o->old_oe->grp_set = !o->old_oe->grp_set;
            if( o->new_oe != NULL ) {
                o->new_oe->grp_set = o->old_oe->grp_set;
                WdeSetTagText( o->new_oe );
            } else {
                WdeSetTagText( o->old_oe );
            }
            break;
        case WdeSelect:
        default:
            WdeWriteTrail( "WdeTagPressed: Bad tag mode!" );
            return;
        }
        parent = NULL;
        if( GetObjectParent( o->old_oe->obj, &parent ) && parent != NULL ) {
            WdeDialogModified( parent );
        }
    }
}

WdeSetOrderStruct *WdeGetTagInfo( HWND tag )
{
    if( tag != (HWND)NULL && IsWindow( tag ) ) {
        return( (WdeSetOrderStruct *)GET_WNDLONGPTR( tag, WdeTagExtra ) );
    }
    return( NULL );
}

LRESULT CALLBACK WdeTagProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    WdeSetOrderStruct   *o;
    bool                pass_to_def;
    LRESULT             ret;

    pass_to_def = true;
    ret = FALSE;
    o = (WdeSetOrderStruct *)GET_WNDLONGPTR( hWnd, WdeTagExtra );

    switch( message ) {
    case WM_CREATE:
        o = (WdeSetOrderStruct *)((CREATESTRUCT *)lParam)->lpCreateParams;
        o->old_oe->tag = hWnd;
        SET_WNDLONGPTR( hWnd, WdeTagExtra, (LONG_PTR)o );
        break;

    case WM_ERASEBKGND:
        pass_to_def = false;
        ret = TRUE;
        break;

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WdeTagDblClicked( o );
        pass_to_def = false;
        ret = TRUE;
        break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if( o != NULL ) {
            Notify( o->old_oe->obj, PRIMARY_OBJECT, NULL );
        }
        pass_to_def = false;
        ret = TRUE;
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        if( o != NULL ) {
            Notify( o->old_oe->obj, PRIMARY_OBJECT, NULL );
        }
        pass_to_def = false;
        ret = TRUE;
        break;

    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONUP:
    case WM_NCRBUTTONUP:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCMOUSEMOVE:
    case WM_MOUSEMOVE:
        pass_to_def = false;
        ret = TRUE;
        break;
    }

    if( pass_to_def ) {
        ret = CallWindowProc( WdeOriginalButtonProc, hWnd, message, wParam, lParam );
    }

    return( ret );
}
