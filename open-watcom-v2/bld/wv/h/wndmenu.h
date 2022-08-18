/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Top level debugger menu.
*
****************************************************************************/


#include "wndmenu1.h"


extern gui_menu_items   WndMainMenuMacro;
extern wnd_info         *WndInfoTab[];

extern void             PlayDead( bool dead );
extern char             *GetMenuLabel( const gui_menu_items *menus, gui_ctl_id id, char *buff, bool strip_amp );
#ifdef DEADCODE
extern int              FindMenuLen( gui_menu_struct *child );
#endif
extern void             AccelMenuItem( gui_menu_struct *menu, bool is_main );
extern void             ProcAccel( void );
extern void             SetBrkMenuItems( void );
extern void             SetLogMenuItems( bool active );
extern void             SetTargMenuItems( void );
extern void             InitMenus( void );
extern void             FiniMenus( void );
extern void             WndMenuSetHotKey( gui_menu_struct *menu, bool is_main, const char *key );
extern gui_menu_struct  *AddMenuAccel( const char *key, const char *cmd, wnd_class_wv wndclass, bool *is_main );
extern bool             WndMainMenuProc( a_window wnd, gui_ctl_id id );
