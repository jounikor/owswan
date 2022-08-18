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
* Description:  Routines to handle the operations on a CURRITEM.
*
****************************************************************************/


#include <wwindows.h>
#include <stdlib.h>
#include "global.h"
#include "fmedit.def"
#include "curritem.h"
#include "state.def"
#include "paint.def"
#include "memory.def"


#define pick_ACTS(o) \
    pick_ACTION_DELETE_OBJECT(o,pick) \
    pick_ACTION_DESTROY(o,pick) \
    pick_ACTION_VALIDATE_ACTION(o,pick) \
    pick_ACTION_GET_OBJPTR(o,pick) \
    pick_ACTION_SHOW_SEL_BOXES(o,pick)

/* Local Window callback functions prototypes */
WINEXPORT LRESULT CALLBACK CurrItemWndProc( HWND, UINT, WPARAM, LPARAM );

/* forward references */
#define pick(e,n,c) static bool CurrItem ## n ## c;
    pick_ACTS( CURRITEM )
#undef pick

static DISPATCH_ITEM CurrItemActions[] = {
    #define pick(e,n,c) {e, (DISPATCH_RTN *)CurrItem ## n},
    pick_ACTS( CURRITEM )
    #undef pick
};

#define MAX_ACTIONS (sizeof( CurrItemActions ) / sizeof( DISPATCH_ITEM ))

#ifdef __NT__
    #define MOVE_TO( hdc, x, y, lppoint ) MoveToEx( (hdc), (x), (y), (lppoint) );
#else
    #define MOVE_TO( hdc, x, y, lppoint ) MoveTo( (hdc), (x), (y) );
#endif

static bool CALLBACK CurrItemDispatch( ACTION_ID id, OBJPTR ci, void *p1, void *p2 )
/**********************************************************************************/
{
    /* dispatch the desired operation to the correct place */
    int i;

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( CurrItemActions[i].id == id ) {
            return( CurrItemActions[i].rtn( ci, p1, p2 ) );
        }
    }
    return( Forward( ((CURRITEM *)ci)->obj, id, p1, p2 ) );
}

static bool CurrItemValidateAction( CURRITEM *ci, ACTION_ID *idptr, void *p2 )
/****************************************************************************/
{
    /* check if the desired action is valid for and CURRITEM */
    int         i;

    ci = ci;           /* ref'd to avoid warning */
    p2 = p2;           /* ref'd to avoid warning */

    for( i = 0; i < MAX_ACTIONS; i++ ) {
        if( CurrItemActions[i].id == *idptr ) {
            return( true );
        }
    }
    return( Forward( ci->obj, VALIDATE_ACTION, idptr, p2 ) );
}

static bool CurrItemDestroy( CURRITEM *ci, bool *first, bool *p2 )
/****************************************************************/
{
    /* destroy the CURRITEM - the object was destroyed while it was current */
    OBJPTR      obj;

    p2 = p2;          /* ref'd to avoid warning */
    obj = ci->obj;
    DeleteCurrObject( (OBJPTR)ci );
    Destroy( obj, *first );
    return( true );
}

static bool CurrItemDeleteObject( CURRITEM *ci, OBJPTR p1, bool *p2 )
/********************************************************************/
{
    /* delete the CURRITEM but do not destroy the object */
    p1 = p1;          /* ref'd to avoid warning */
    p2 = p2;          /* ref'd to avoid warning */

    if( ci->hwnd != NULL ) {
        SendMessage( ci->hwnd, WM_KILLFOCUS, 0, 0 );
        DestroyWindow( ci->hwnd );
    }
    EdFree( ci );
    return( true );
}

static bool CurrItemShowSelBoxes( CURRITEM *ci, bool *show, void *p2 )
/********************************************************************/
{
    p2 = p2;    // unused

    if( *show != ci->show_sel_boxes ) {
        ci->show_sel_boxes = *show;
        if( ci->hwnd != NULL ) {
            InvalidateRect( ci->hwnd, NULL, TRUE );
        }
    }
    return( true );
}

OBJPTR CurrItemCreate( OBJPTR parent, RECT *loc, OBJPTR obj )
/**************************************************************/
{
    /* Create a CURRITEM object */
    CURRITEM *new;

    parent = parent;         /* ref'd to avoid warning */
    new = EdAlloc( sizeof( CURRITEM ) );
    OBJ_DISPATCHER_SET( new, CurrItemDispatch );
    new->obj = obj;
    GetOffset( &new->offset );
    new->rect = *loc;
    new->show_sel_boxes = true;
    new->fmstate = GetCurrFormID();
    if( IsMarkValid( obj ) ) {
        new->hwnd = CreateWindow( "CurrItemClass",
                                  NULL,
                                  WS_VISIBLE | WS_CHILD | WS_DISABLED,
                                  loc->left - new->offset.x - SQUAREWIDTH / 2,
                                  loc->top - new->offset.y - SQUAREWIDTH / 2,
                                  loc->right - loc->left + SQUAREWIDTH,
                                  loc->bottom - loc->top + SQUAREWIDTH,
                                  GetAppWnd(),
                                  NULL,
                                  GetInst(),
                                  NULL );
        BringWindowToTop( new->hwnd );
        SET_WNDINFO( new->hwnd, (LONG_PTR)new );
    } else {
        new->hwnd = NULL;
    }
    return( (OBJPTR)new );
}

static bool CurrItemGetObjptr( CURRITEM *ci, OBJPTR *newobj, void *p2 )
/**********************************************************************/
{
    /* return the objptr of the object associated with this curritem */
    p2 = p2;    /* ref'd to avoid warning */

    if( newobj != NULL ) {
        *newobj = ci->obj;
    }
    return( true );
}

static void DrawSquare( HDC hdc, POINT point )
/********************************************/
{
    /* draws one of the small black squares used for sizing */
    RECT    rect;

    rect.left = point.x;
    rect.top = point.y;
    rect.right = point.x + SQUAREWIDTH / 2;
    rect.bottom = point.y + SQUAREWIDTH / 2;
    FillRect( hdc, &rect, (HBRUSH)GetStockObject( BLACK_BRUSH ) );
}

static void OutlineTopLeft( LPRECT currect, HDC hdc )
/***************************************************/
{
    /* Outline box at top left of current rectangle */
    RECT work;

    work = *currect;
    work.right--;
    work.bottom--;

    /* top left */
    MOVE_TO( hdc, work.left, work.top + SQUAREWIDTH / 2, NULL );
    LineTo( hdc, work.left - SQUAREWIDTH / 2, work.top + SQUAREWIDTH / 2 );
    LineTo( hdc, work.left - SQUAREWIDTH / 2, work.top - SQUAREWIDTH / 2 );
    LineTo( hdc, work.left + SQUAREWIDTH / 2, work.top - SQUAREWIDTH / 2 );
    LineTo( hdc, work.left + SQUAREWIDTH / 2, work.top );
}

static void OutlineBottomLeft( LPRECT currect, HDC hdc )
/******************************************************/
{
    /* Outline box at bottom left of current rectangle */
    RECT work;

    work = *currect;
    work.right--;
    work.bottom--;

    /* bottom left */
    MOVE_TO( hdc, work.left, work.bottom - SQUAREWIDTH / 2, NULL );
    LineTo( hdc, work.left - SQUAREWIDTH / 2, work.bottom - SQUAREWIDTH / 2 );
    LineTo( hdc, work.left - SQUAREWIDTH / 2, work.bottom + SQUAREWIDTH / 2 );
    LineTo( hdc, work.left + SQUAREWIDTH / 2, work.bottom + SQUAREWIDTH / 2 );
    LineTo( hdc, work.left + SQUAREWIDTH / 2, work.bottom );
}

static void OutlineBottomRight( LPRECT currect, HDC hdc )
/*******************************************************/
{
    /* Outline box at bottom right of current rectangle */
    RECT work;

    work = *currect;
    work.right--;
    work.bottom--;

    /* bottom right */
    MOVE_TO( hdc, work.right - SQUAREWIDTH / 2, work.bottom, NULL );
    LineTo( hdc, work.right - SQUAREWIDTH / 2, work.bottom + SQUAREWIDTH / 2 );
    LineTo( hdc, work.right + SQUAREWIDTH / 2, work.bottom + SQUAREWIDTH / 2 );
    LineTo( hdc, work.right + SQUAREWIDTH / 2, work.bottom - SQUAREWIDTH / 2 );
    LineTo( hdc, work.right, work.bottom - SQUAREWIDTH / 2 );
}

static void OutlineTopRight( LPRECT currect, HDC hdc )
/****************************************************/
{
    /* Outline box at top right of current rectangle */
    RECT work;

    work = *currect;
    work.right--;
    work.bottom--;

    /* top right */
    MOVE_TO( hdc, work.right, work.top + SQUAREWIDTH / 2, NULL );
    LineTo( hdc, work.right + SQUAREWIDTH / 2, work.top + SQUAREWIDTH / 2 );
    LineTo( hdc, work.right + SQUAREWIDTH / 2, work.top - SQUAREWIDTH / 2 );
    LineTo( hdc, work.right - SQUAREWIDTH / 2, work.top - SQUAREWIDTH / 2 );
    LineTo( hdc, work.right - SQUAREWIDTH / 2, work.top );
}

static void OutlineTopMiddle( LPRECT currect, HDC hdc )
/*****************************************************/
{
    /* Outline box at top middle of current rectangle */
    RECT work;
    RECT rect;

    work = *currect;
    work.right--;
    work.bottom--;

    rect.left = (work.right + work.left) / 2 - SQUAREWIDTH / 2; /* top middle */
    rect.right = (work.right + work.left) / 2 + SQUAREWIDTH / 2;
    rect.top = work.top - SQUAREWIDTH / 2;
    rect.bottom = work.top;
    MOVE_TO( hdc, rect.left, rect.bottom, NULL );
    LineTo( hdc, rect.left, rect.top );
    LineTo( hdc, rect.right, rect.top );
    LineTo( hdc, rect.right, rect.bottom );
}

static void OutlineBottomMiddle( LPRECT currect, HDC hdc )
/********************************************************/
{
    /* Outline box at bottom middle of current rectangle */
    RECT work;
    RECT rect;

    work = *currect;
    work.right--;
    work.bottom--;

    rect.left = (work.right + work.left) / 2 - SQUAREWIDTH / 2;  /* bottom middle */
    rect.right = (work.right + work.left) / 2 + SQUAREWIDTH / 2;
    rect.top = work.bottom;
    rect.bottom = work.bottom + SQUAREWIDTH / 2;
    MOVE_TO( hdc, rect.left, rect.top, NULL );
    LineTo( hdc, rect.left, rect.bottom );
    LineTo( hdc, rect.right, rect.bottom );
    LineTo( hdc, rect.right, rect.top );
}

static void OutlineLeftMiddle( LPRECT currect, HDC hdc )
/******************************************************/
{
    /* Outline box at left middle of current rectangle */
    RECT work;
    RECT rect;

    work = *currect;
    work.right--;
    work.bottom--;

    rect.left = work.left - SQUAREWIDTH / 2;  /* left middle */
    rect.right = work.left;
    rect.top = (work.bottom + work.top) / 2 - SQUAREWIDTH / 2;
    rect.bottom = (work.bottom + work.top) / 2 + SQUAREWIDTH / 2;
    MOVE_TO( hdc, rect.right, rect.top, NULL );
    LineTo( hdc, rect.left, rect.top );
    LineTo( hdc, rect.left, rect.bottom );
    LineTo( hdc, rect.right, rect.bottom );
}

static void OutlineRightMiddle( LPRECT currect, HDC hdc )
/*******************************************************/
{
    /* Outline box at right middle of current rectangle */
    RECT work;
    RECT rect;

    work = *currect;
    work.right--;
    work.bottom--;

    rect.left = work.right;  /* right middle */
    rect.right = work.right + SQUAREWIDTH / 2;
    rect.top = (work.bottom + work.top) / 2 - SQUAREWIDTH / 2;
    rect.bottom = (work.bottom + work.top) / 2 + SQUAREWIDTH / 2;
    MOVE_TO( hdc, rect.left, rect.top, NULL );
    LineTo( hdc, rect.right, rect.top );
    LineTo( hdc, rect.right, rect.bottom );
    LineTo( hdc, rect.left, rect.bottom );
}

static void OutlineBoxes( LPRECT currect, HDC hdc )
/*************************************************/
{
    /* draws the outlined squares at the corners and sides of the current object */
    OutlineTopLeft( currect, hdc );
    OutlineBottomLeft( currect, hdc );
    OutlineBottomRight( currect, hdc );
    OutlineTopRight( currect, hdc );
    OutlineTopMiddle( currect, hdc );
    OutlineBottomMiddle( currect, hdc );
    OutlineLeftMiddle( currect, hdc );
    OutlineRightMiddle( currect, hdc );
}

static void MarkBoxes( LPRECT currect, HDC hdc, RESIZE_ID sizeid )
/****************************************************************/
{
    /* draws the black squares at the corners and sides of the current object */
    POINT   point;
    RECT    work;

    work = *currect;
    OffsetRect( &work, -(SQUAREWIDTH / 2), -(SQUAREWIDTH / 2) );

    if( (sizeid & R_LEFT) && (sizeid & R_TOP) ) {
        point.x = work.left;                           /* top left */
        point.y = work.top;
        DrawSquare( hdc, point );
        point.x += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.x = work.left;
        point.y += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineTopLeft( currect, hdc );
    }

    if( (sizeid & R_LEFT) && (sizeid & R_BOTTOM) ) {
        point.x = work.left;                            /* bottom left */
        point.y = work.bottom;
        DrawSquare( hdc, point );
        point.y += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.x += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineBottomLeft( currect, hdc );
    }

    if( (sizeid & R_RIGHT) && (sizeid & R_BOTTOM) ) {
        point.x = work.right;                               /* bottom right */
        point.y = work.bottom + SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.x += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.y -= SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineBottomRight( currect, hdc );
    }

    if( (sizeid & R_RIGHT) && (sizeid & R_TOP) ) {
        point.x = work.right + SQUAREWIDTH / 2;             /* top right */
        point.y = work.top + SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.y -= SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.x -= SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineTopRight( currect, hdc );
    }

    if( sizeid & R_TOP ) {
        point.x = (work.right + work.left) / 2;             /* top middle */
        point.y = work.top;
        DrawSquare( hdc, point );
        point.x += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineTopMiddle( currect, hdc );
    }

    if( sizeid & R_BOTTOM ) {
        point.x = (work.right + work.left) / 2 + SQUAREWIDTH / 2; /* bottom middle */
        point.y = work.bottom + SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.x -= SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineBottomMiddle( currect, hdc );
    }

    if( sizeid & R_LEFT ) {
        point.x = work.left;                                /* left middle */
        point.y = (work.bottom + work.top) / 2;
        DrawSquare( hdc, point );
        point.y += SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineLeftMiddle( currect, hdc );
    }

    if( sizeid & R_RIGHT ) {
        point.x = work.right + SQUAREWIDTH / 2;             /* right middle */
        point.y = (work.bottom + work.top) / 2 + SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
        point.y -= SQUAREWIDTH / 2;
        DrawSquare( hdc, point );
    } else {
        OutlineRightMiddle( currect, hdc );
    }
}

LRESULT CALLBACK CurrItemWndProc( HWND wnd, UINT message, WPARAM wparam, LPARAM lparam )
/**************************************************************************************/
{
    /* processes messages */
    HDC             hdc;
    PAINTSTRUCT     ps;
    RESIZE_ID       sizeid;
    RECT            rect;
    CURRITEM        *ci;
    POINT           offset;

    switch( message ) {
    case WM_PAINT :
        ci = (CURRITEM *)GET_WNDINFO( wnd );
        if( !InitStateFormID( ci->fmstate ) ) {
            /* the correct state can't be found so just ignore */
            /* this WM_PAINT */
            BeginPaint( wnd, &ps );
            EndPaint( wnd, &ps );
            break;
        }
        Location( ci->obj, &rect );
        GetOffset( &offset );
        /* if the offset has changed move the window and leave the */
        /* WM_PAINT pending (it will be resent) */
        /* NOTE: the debuggin kernel will complain about no */
        /* BeginPaint/EndPaint on WM_PAINT but we will eventually */
        /* hit them */
        if( offset.x != ci->offset.x || offset.y != ci->offset.y ) {
            CopyRect( &ci->rect, &rect );
            ci->offset = offset;
            MoveWindow( wnd,
                        rect.left - offset.x - SQUAREWIDTH / 2,
                        rect.top - offset.y - SQUAREWIDTH / 2,
                        rect.right - rect.left + SQUAREWIDTH,
                        rect.bottom - rect.top + SQUAREWIDTH,
                        FALSE );
            break;
        }
        /* if the object has changed size move the window and leave the */
        /* WM_PAINT pending */
        if( !EqualRect( &rect, &ci->rect ) ) {
            CopyRect( &ci->rect, &rect );
            ci->offset = offset;
            MoveWindow( wnd,
                        rect.left - offset.x - SQUAREWIDTH / 2,
                        rect.top - offset.y - SQUAREWIDTH / 2,
                        rect.right - rect.left + SQUAREWIDTH,
                        rect.bottom - rect.top + SQUAREWIDTH,
                        TRUE );
             break;
        }
        BeginPaint( wnd, &ps );
        if( IsMarkValid( ci->obj ) && ci->show_sel_boxes ) {
            hdc = ps.hdc;
            switch( GetState() ) {
            case DORMANT:
            case OVERBOX:
            case EDITING:
            case PASTE_PENDING:
            case MOVE_PENDING:
            case ACTION_ABORTED:
                GetClientRect( wnd, &rect );
                InflateRect( &rect, -SQUAREWIDTH / 2, -SQUAREWIDTH / 2 );
                if( !IsRectEmpty( &rect ) ) {
                    if( (OBJPTR)ci == GetPrimaryObject() ) {
                        sizeid = R_ALL;
                        GetResizeInfo( (OBJPTR)ci, &sizeid );
                        MarkBoxes( &rect, hdc, sizeid );
                    } else {
                        OutlineBoxes( &rect, hdc );
                    }
                }
                break;
            }
        }
        EndPaint( wnd, &ps );
        break;
    case WM_DESTROY:
        ci = (CURRITEM *)GET_WNDINFO( wnd );
        ci->hwnd = NULL;
        break;
    default :
        return( DefWindowProc( wnd, message, wparam, lparam ) );
    }
    return( 0L );
}

void InitCurrItem( void )
/***********************/
{
    /* Initialization for CURRITEM objects - register the CurrItemClass */
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = CurrItemWndProc;       /* Function to retrieve messages for*/
                                            /* windows of this class.           */
    wc.cbClsExtra = 0;                      /* No per-class extra data.         */
    wc.cbWndExtra = sizeof( LONG_PTR );     /* Extra data for each window       */
                                            /* This stores the OBJPTR           */
                                            /* associated with each curritem    */
    wc.hInstance = GetInst();               /* Application that owns the class. */
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;                /* NULL background-invisible window */
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "CurrItemClass";     /* Name used in call to CreateWindow*/
    RegisterClass( &wc );
}
