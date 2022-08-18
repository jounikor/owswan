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


#ifndef __WI163264_INCLUDED__
#define __WI163264_INCLUDED__

#if defined( __WINDOWS__ )

#ifndef APIENTRY
#define APIENTRY            FAR PASCAL
#endif
#ifndef __FAR
#define __FAR               __far
#endif

#define WINMAINENTRY        PASCAL

#if defined( _M_I86 )

#define INT_PTR             int
#define UINT_PTR            unsigned int

#else   /* __WINDOWS_386__ */

#define INT_PTR             short
#define UINT_PTR            unsigned short

#endif

#define LONG_PTR            LONG
#define ULONG_PTR           DWORD

#define BST_UNCHECKED       0
#define BST_CHECKED         1
#define BST_PUSHED          4

#define GET_HINSTANCE(hwnd)                     (HINSTANCE)GetWindowWord( hwnd, GWW_HINSTANCE )
#define GET_WNDPROC(hwnd)                       GetWindowLong( hwnd, GWL_WNDPROC )
#define SET_WNDPROC(hwnd, proc)                 SetWindowLong( hwnd, GWL_WNDPROC, proc )
#define GET_WNDLONGPTR(hwnd, offs)              GetWindowLong( hwnd, offs )
#define SET_WNDLONGPTR(hwnd, offs, data)        SetWindowLong( hwnd, offs, data )
#define GET_WNDINFO(hwnd)                       GetWindowLong( hwnd, 0 )
#define SET_WNDINFO(hwnd, data)                 SetWindowLong( hwnd, 0, data )
#define SET_HICON( hw, ic ) (HICON)             SetClassWord( hw, GCW_HICON, (WORD)ic )
#define SET_HBRBACKGROUND( hw, br )             (HBRUSH)SetClassWord( hw, GCW_HBRBACKGROUND, (WORD)br )
#define SET_CLASSCURSOR(hwnd, cur)              (WORD)SetClassWord( hwnd, GCW_HCURSOR, (WORD)cur )
#define GET_DLGDATA(hwnd)                       GetWindowLong( hwnd, DWL_USER )
#define SET_DLGDATA(hwnd, data)                 SetWindowLong( hwnd, DWL_USER, (LONG)data )
#define GET_DLGRESULT(hwnd)                     GetWindowLong( hwnd, DWL_MSGRESULT )
#define SET_DLGRESULT(hwnd, data)               SetWindowLong( hwnd, DWL_MSGRESULT, (LONG)data )

#define GET_ID(hwnd)                            (WORD)GetWindowWord( hwnd, GWW_ID )
#define GET_CBWNDEXTRA(hwnd)                    (WORD)GetClassWord( hwnd, GCW_CBWNDEXTRA )
#define GET_CLASS_STYLE(hwnd)                   GetClassWord( hwnd, GCW_STYLE )

#define GET_EM_SETSEL_MPS(iStart, iEnd)         0, MAKELONG(iStart, iEnd)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(lp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)LOWORD(lp)
#define GET_WM_COMMAND_MPS(id, hwnd, cmd)       (UINT)(id), MAKELONG(hwnd, cmd)
#define GET_WM_ACTIVATE_FACTIVE(wp, lp)         (wp)
#define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)      (BOOL)HIWORD(lp)
#define GET_WM_ACTIVATE_HWND(wp, lp)            (HWND)LOWORD(lp)
#define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp) (BOOL)(wp)
#define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp) (HWND)HIWORD(lp)
#define GET_WM_MDIACTIVATE_HWNDDEACTIVATE(wp, lp) (HWND)LOWORD(lp)
#define GET_WM_VSCROLL_POS( wp, lp )            LOWORD(lp)
#define GET_WM_VSCROLL_HWND( wp, lp )           (HWND)HIWORD(lp)
#define GET_WM_VSCROLL_CODE( wp, lp )           (wp)
#define GET_WM_HSCROLL_POS( wp, lp )            LOWORD(lp)
#define GET_WM_HSCROLL_HWND( wp, lp )           (HWND)HIWORD(lp)
#define GET_WM_HSCROLL_CODE( wp, lp )           (wp)
#define MAKE_POINT( p, pnt )                    p = MAKEPOINT( pnt )
#define GET_WM_MENUSELECT_HMENU(wp,lp)          (HMENU)HIWORD(lp)
#define GET_WM_MENUSELECT_ITEM(wp,lp)           (wp)
#define GET_WM_MENUSELECT_FLAGS(wp,lp)          LOWORD(lp)
#define GET_WM_PARENTNOTIFY_EVENT(wp,lp)        LOWORD(wp)
#define GET_WM_PARENTNOTIFY_ID(wp,lp)           HIWORD(lp)
#define GET_WM_PARENTNOTIFY_HANDLE(wp,lp)       LOWORD(lp)
#define GET_WM_PARENTNOTIFY_POINT(wp,lp)        (lp)

#define WC_BUTTON                               "Button"
#define WC_COMBOBOX                             "ComboBox"
#define WC_EDIT                                 "Edit"
#define WC_LISTBOX                              "ListBox"
#define WC_SCROLLBAR                            "ScrollBar"
#define WC_STATIC                               "Static"

#else /* __NT__ */

#define __FAR
#define WINMAINENTRY        WINAPI

#define GET_WNDLONGPTR(hwnd, offs)              GetWindowLongPtr( hwnd, offs )
#define SET_WNDLONGPTR(hwnd, offs, data)        SetWindowLongPtr( hwnd, offs, data )
#define GET_WNDINFO(hwnd)                       GetWindowLongPtr( hwnd, 0 )
#define SET_WNDINFO(hwnd, data)                 SetWindowLongPtr( hwnd, 0, data )
#define GET_HINSTANCE(hwnd)                     (HINSTANCE)GetWindowLongPtr( hwnd, GWLP_HINSTANCE )
#define GET_WNDPROC(hwnd)                       GetWindowLongPtr( hwnd, GWLP_WNDPROC )
#define SET_WNDPROC(hwnd, proc)                 SetWindowLongPtr( hwnd, GWLP_WNDPROC, proc )
#define GET_DLGDATA(hwnd)                       GetWindowLongPtr( hwnd, DWLP_USER )
#define SET_DLGDATA(hwnd, data)                 SetWindowLongPtr( hwnd, DWLP_USER, (LONG_PTR)data )
#define GET_DLGRESULT(hwnd)                     GetWindowLongPtr( hwnd, DWLP_MSGRESULT )
#define SET_DLGRESULT(hwnd, data)               SetWindowLongPtr( hwnd, DWLP_MSGRESULT, (LONG_PTR)data )
#define SET_HICON( hw, ic )                     (HICON)SetClassLongPtr( hw, GCLP_HICON, (LONG_PTR)ic )
#define SET_HBRBACKGROUND( hw, br )             (HBRUSH)SetClassLongPtr( hw, GCLP_HBRBACKGROUND, (LONG_PTR)br )
#define SET_CLASSCURSOR(hwnd, cur)              (HCURSOR)SetClassLongPtr( hwnd, GCLP_HCURSOR, (LONG_PTR)cur )

#define GET_ID( hwnd )                          (DWORD)GetWindowLong( hwnd, GWL_ID )
#define GET_CBWNDEXTRA( hwnd )                  (DWORD)GetClassLong( hwnd, GCL_CBWNDEXTRA )
#define GET_CLASS_STYLE(hwnd)                   GetClassLong( hwnd, GCL_STYLE )

#define GET_EM_SETSEL_MPS(iStart, iEnd)         (UINT)(iStart), (LONG)(iEnd)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_MPS(id, hwnd, cmd)       (UINT)MAKELONG(id, cmd), (LONG)(hwnd)
#define GET_WM_ACTIVATE_FACTIVE(wp, lp)         LOWORD(wp)
#define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)      (BOOL)HIWORD(wp)
#define GET_WM_ACTIVATE_HWND(wp, lp)            (HWND)(lp)
#define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp) ((HWND)lp == hwnd)
#define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp) (HWND)(wp)
#define GET_WM_MDIACTIVATE_HWNDDEACTIVATE(wp, lp) (HWND)(lp)
#define GET_WM_VSCROLL_POS( wp, lp )            HIWORD(wp)
#define GET_WM_VSCROLL_HWND( wp, lp )           (HWND)(lp)
#define GET_WM_VSCROLL_CODE( wp, lp )           LOWORD(wp)
#define GET_WM_HSCROLL_POS( wp, lp )            HIWORD(wp)
#define GET_WM_HSCROLL_HWND( wp, lp )           (HWND)(lp)
#define GET_WM_HSCROLL_CODE( wp, lp )           LOWORD(wp)
#define MAKE_POINT( p, pnt )                    (p).x = (signed short)LOWORD( pnt ), (p).y = (signed short)HIWORD( pnt )
#define GET_WM_MENUSELECT_HMENU(wp,lp)          (HMENU)(lp)
#define GET_WM_MENUSELECT_ITEM(wp,lp)           LOWORD(wp)
#define GET_WM_MENUSELECT_FLAGS(wp,lp)          HIWORD(wp)
#define GET_WM_PARENTNOTIFY_EVENT(wp,lp)        LOWORD(wp)
#define GET_WM_PARENTNOTIFY_ID(wp,lp)           HIWORD(wp)
#define GET_WM_PARENTNOTIFY_HANDLE(wp,lp)       (lp)
#define GET_WM_PARENTNOTIFY_POINT(wp,lp)        (lp)

#endif  /* __NT__ */

#define GET_WNDSTYLE(hwnd)                      (DWORD)GetWindowLong( hwnd, GWL_STYLE )
#define SET_WNDSTYLE(hwnd, data)                SetWindowLong( hwnd, GWL_STYLE, (DWORD)data )
#define GET_WNDEXSTYLE(hwnd)                    (DWORD)GetWindowLong( hwnd, GWL_EXSTYLE )
#define SET_WNDEXSTYLE(hwnd, data)              SetWindowLong( hwnd, GWL_EXSTYLE, (DWORD)data )

#define MENU_CLOSED(wp,lp)                      ((GET_WM_MENUSELECT_FLAGS(wp,lp)==(WORD)-1)&&(GET_WM_MENUSELECT_HMENU(wp,lp)==(HMENU)NULL))

#endif
