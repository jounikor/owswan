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
* Description:  Main debugger window.
*
****************************************************************************/


extern wnd_posn        WndPosition[];
extern gui_rect        WndMainRect;

extern bool ScanStatus( void );
extern char *GetWndFont( a_window wnd );
extern void WndFontHook( a_window wnd );
extern gui_coord *WndMainClientSize( void );
extern void WndMainResized( void );
extern void WndResizeHook( a_window wnd );
extern void InitFont( void );
extern void FiniFont( void );
extern void ProcFont( void );
extern void ConfigFont( void );
extern void FontChange( void );
extern void ProcDisplay( void );
extern void ConfigDisp( void );
extern wnd_class_wv ReqWndName( void );
