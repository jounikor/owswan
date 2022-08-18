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
* Description:  Main Form Editor module.
*
****************************************************************************/


#define INCLUDE_COMMDLG_H
#include <wwindows.h>
#include <string.h>
#include <stdlib.h>
#include "global.h"
#include "dllmain.h"

/* imports */

#include "fmedit.def"
#include "fmdlgs.rh"
#include "state.def"
#include "paint.def"
#include "mouse.def"
#include "keybd.def"
#include "object.def"
#include "cursor.def"
#include "grid.def"
#include "fmerror.def"
#include "scroll.def"
#include "curritem.def"
#include "eatom.def"
#include "oitem.def"
#include "align.def"
#include "space.def"
#include "clip.def"
#include "wclbproc.h"


void FMEDITAPI CloseFormEdit( HWND wnd )
/**************************************/
{
    /* close the editing window */
    if( InitState( wnd ) ) {
        DestroyMainObject();
        FreeState();
    }
}

void FMEDITAPI CloseFormEditID( STATE_HDL hdl )
/*********************************************/
{
    /* close the editing window given a state handle */
    if( InitStateFormID( hdl ) ) {
        DestroyMainObject();
        FreeState();
    }
}

void FMEDITAPI ResetFormEdit( HWND wnd )
/**************************************/
{
    /* close the editing window */
    if( InitState( wnd ) ) {
        ResetCurrObject( false );
        DestroyMainObject();
        CreateMainObject();
        SetCurrObject( GetMainObject() );
        InvalidateRect( wnd, NULL, TRUE );
        InitScroll( GetScrollConfig(), wnd );
    }
}

STATE_HDL FMEDITAPI InitFormEdit( CREATE_TABLE objtable )
/*******************************************************/
{
    NewState();
    CreateCurrObject();
    InitializeObjects( objtable );
    return( GetCurrFormID() );
}

void FMEDITAPI SetFormEditWnd( STATE_HDL st, HWND wnd, int bitmap, SCR_CONFIG scroll )
/************************************************************************************/
{
    InitStateFormID( st );
    SetStateWnd( wnd );
    if( wnd != NULL ) {
        InitEditMenu( wnd, bitmap );
        SetCurrObject( GetMainObject() );
        ShowWindow( wnd, SW_SHOW );
        InitScroll( scroll, wnd );
    }
}


void FMEDITAPI OpenFormEdit( HWND wnd, CREATE_TABLE objtable, int bitmap, SCR_CONFIG scroll )
/*******************************************************************************************/
{
    /* Saves instance handle and creates main window */
    NewState();
    SetStateWnd( wnd );
    CreateCurrObject();
    SetStateCursor( GetState() );
    InitializeObjects( objtable );
    SetCurrObject( GetMainObject() );
    InitEditMenu( wnd, bitmap );
    ShowWindow( wnd, SW_SHOW );
    InitScroll( scroll, wnd );
}


static void OffsetPoint( POINT *point )
/*************************************/
{
    /* offset point according to scrolling info */
    POINT offset;

    GetOffset( &offset );
    point->x += offset.x;
    point->y += offset.y;
}

static void CutObjects( void )
/****************************/
{
    /* Cut the current objects */
    OBJPTR      currobj;
    OBJPTR      saveobj;
    OBJPTR      nextobj;
    OBJPTR      appobj;

    FMNewClipboard();
    for( currobj = GetEditCurrObject(); currobj != NULL; currobj = nextobj ) {
        nextobj = GetNextEditCurrObject( currobj );
        appobj = GetObjptr( currobj );
        if( appobj != GetMainObject() ) {
            if( !FMClipObjExists( appobj ) ) {
                saveobj = NULL;
                CutObject( appobj, &saveobj );
                DeleteCurrObject( currobj );
                FMAddClipboard( appobj, appobj );
            }
        }
    }
}

static void CopyObjects( void )
/*****************************/
{
    /* Copy the current objects */
    OBJPTR      currobj;
    OBJPTR      copyobj;
    OBJPTR      appobj;

    FMNewClipboard();
    for( currobj = GetEditCurrObject(); currobj != NULL; currobj = GetNextEditCurrObject( currobj ) ) {
        appobj = GetObjptr( currobj );
        if( appobj != GetMainObject() ) {
            if( !FMClipObjExists( appobj ) ) {
                copyobj = NULL;
                if( CopyObject( appobj, &copyobj, NULL ) ) {
                    FMAddClipboard( appobj, copyobj );
                }
            }
        }
    }
}


BOOL CALLBACK FMEditWndProc( HWND wnd, UINT message, WPARAM wparam, LPARAM lparam )
/*********************************************************************************/
{
    /* processes messages */
    DLGPROC        dlgproc;
    HANDLE         inst;
    POINT          point;
    POINT          offset;
    bool           frommenu;

    if( !InitState( wnd ) ) {
        return( FALSE );
    }
    inst = GetInst();
    switch( message ) {
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case IDM_DELETEOBJECT:
            frommenu = true;
            ExecuteCurrObject( DESTROY, &frommenu, NULL );
            SetCurrObject( GetMainObject() );
            break;
        case IDM_CUTOBJECT:
            CutObjects();
            SetCurrObject( GetMainObject() );
            break;
        case IDM_PASTEOBJECT:
            if( FMPasteValid() ) {
                SetCapture( wnd );
                SetState( PASTE_PENDING );
            }
            break;
        case IDM_ESCAPE:
            switch( GetState() ) {
            case PASTE_PENDING:
                ReleaseCapture();
                SetDefState();
                break;
            case MOVING:
                AbortMoveOperation();
                SetState( ACTION_ABORTED );
                break;
            case SIZING:
                AbortResize();
                SetState( ACTION_ABORTED );
                break;
            }
            break;
        case IDM_COPYOBJECT:
            CopyObjects();
            break;
        case IDM_GRID:
            dlgproc = MakeProcInstance_DLG( FMGridDlgProc, inst );
            DialogBox( inst, "GridBox", wnd, dlgproc );
            FreeProcInstance_DLG( dlgproc );
            InheritState( wnd );
            break;
        case IDM_FMLEFT:
        case IDM_FMHCENTRE:
        case IDM_FMRIGHT:
        case IDM_FMTOP:
        case IDM_FMVCENTRE:
        case IDM_FMBOTTOM:
            Align( wparam );
            break;
        case IDM_SPACE_HORZ:
        case IDM_SPACE_VERT:
            Space( wparam );
            break;
        default:
            return( FALSE );
        }
        break;
#if 0
    case WM_ERASEBKGND:
        /* do nothing */
        break;
#endif

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        MAKE_POINT( point, lparam );
        GetOffset( &offset );
        point.x += offset.x;
        point.y += offset.y;
        ProcessButtonDown( point, LOWORD( wparam ), NULL );
        break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        MAKE_POINT( point, lparam );
        OffsetPoint( &point );
        ProcessButtonUp( point );
        break;
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        MAKE_POINT( point, lparam );
        OffsetPoint( &point );
        ProcessDBLCLK( point );
        break;
    case WM_MOUSEMOVE:
        SetStateCursor( GetState() );
        MAKE_POINT( point, lparam );
        OffsetPoint( &point );
        ProcessMouseMove( point );
        break;
    case WM_VSCROLL:
        VerticalScroll( wparam, lparam, wnd );
        break;
    case WM_HSCROLL:
        HorizontalScroll( wparam, lparam, wnd );
        break;
    case WM_KEYDOWN:
        return( ProcessKeyDown( wparam ) );
        break;
    case WM_KEYUP:
        return( ProcessKeyUp( wparam ) );
    case WM_PAINT:
        DoPainting();
        break;
    case WM_SIZE:
        ScrollResize( wnd, lparam );
        break;
    case WM_SETFOCUS:
        UpdateWindow( wnd );
        break;
    default:
        return( FALSE );
    }
    return( TRUE );
}

#ifdef __NT__

BOOL WINAPI DllMain( HINSTANCE inst, DWORD dwReason, LPVOID lpReserved )
{
    /* Initializes window data and registers window class */
    lpReserved = lpReserved;     /* avoid warning */

    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        SetInst( inst );
        InitClipboard();
        InitCursors();
        InitEAtom();
        InitCurrItem();
        InitOItem();
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        /* do nothing here */
        break;
    }

    return ( 1 );
}

#else

int WINAPI LibMain( HINSTANCE inst, WORD dataseg, WORD heapsize, LPSTR cmdline )
/******************************************************************************/
{
    /* Initializes window data and registers window class */
    dataseg = dataseg;              /* ref'd to avoid warnings */
    heapsize = heapsize;            /* ref'd to avoid warnings */
    cmdline = cmdline;              /* ref'd to avoid warnings */

    SetInst( inst );
    InitClipboard();
    InitCursors();
    InitEAtom();
    InitCurrItem();
    InitOItem();
    return( TRUE );
}

int WINAPI WEP( int parm )
/************************/
{
    /* terminate the DLL */
    parm = parm;
    return( 1 );
}

#endif

bool FMEDITAPI ObjectPress( OBJPTR obj, POINT *pt, WORD wparam, HWND wnd )
/************************************************************************/
{
    /* The application is telling us that the object obj got a button down
     * on it.
     */
    if( InitState( wnd ) ) {
        ProcessButtonDown( *pt, wparam & MK_SHIFT, obj );
        return( true );
    }
    return( false );
}
