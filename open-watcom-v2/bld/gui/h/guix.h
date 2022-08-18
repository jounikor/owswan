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


/* use 0-9 for os-specific menu constants */

#define GUI_MENU_CHANGE_FONT            0
#define GUI_MENU_FIX_TOOLBAR            1

/* use all from 10 for MDI-specific menu constants */

#define GUI_MENU_MDI_FIRST_SEPARATOR    10
#define GUI_MENU_MDI_CASCADE            11
#define GUI_MENU_MDI_TILE_HORZ          12
#define GUI_MENU_MDI_TILE_VERT          13
#define GUI_MENU_MDI_ARRANGE_ICONS      14
#define GUI_MENU_MDI_SECOND_SEPARATOR   15
#define GUI_MENU_MDI_MORE_WINDOWS       16
#define GUI_MENU_MDI_FIRST_WINDOW       17

#define GUI_MENU_ID(x)                  (GUI_LAST_MENU_ID + x)
#define GUI_MENU_IDX(x)                 (x - GUI_LAST_MENU_ID)

#define GUI_CHANGE_FONT                 GUI_MENU_ID( GUI_MENU_CHANGE_FONT )
#define GUI_FIX_TOOLBAR                 GUI_MENU_ID( GUI_MENU_FIX_TOOLBAR )

#define GUI_MDI_FIRST_SEPARATOR         GUI_MENU_ID( GUI_MENU_MDI_FIRST_SEPARATOR )
#define GUI_MDI_CASCADE                 GUI_MENU_ID( GUI_MENU_MDI_CASCADE )
#define GUI_MDI_TILE_HORZ               GUI_MENU_ID( GUI_MENU_MDI_TILE_HORZ )
#define GUI_MDI_TILE_VERT               GUI_MENU_ID( GUI_MENU_MDI_TILE_VERT )
#define GUI_MDI_ARRANGE_ICONS           GUI_MENU_ID( GUI_MENU_MDI_ARRANGE_ICONS )
#define GUI_MDI_SECOND_SEPARATOR        GUI_MENU_ID( GUI_MENU_MDI_SECOND_SEPARATOR )
#define GUI_MDI_MORE_WINDOWS            GUI_MENU_ID( GUI_MENU_MDI_MORE_WINDOWS )

#define MAX_NUM_MDI_WINDOWS             9
#define GUI_MDI_FIRST_WINDOW            GUI_MENU_ID( GUI_MENU_MDI_FIRST_WINDOW )
#define GUI_MDI_LAST_WINDOW             GUI_MENU_ID( GUI_MENU_MDI_FIRST_WINDOW + MAX_NUM_MDI_WINDOWS - 1 )

#define GUI_MDI_MENU_FIRST              GUI_MDI_FIRST_SEPARATOR
#define GUI_MDI_MENU_LAST               GUI_MDI_FIRST_WINDOW + MAX_NUM_MDI_WINDOWS - 1

#define IS_MDIWIN(x)                    (x >= GUI_MDI_FIRST_WINDOW && x <= GUI_MDI_LAST_WINDOW)
#define IS_MDIMENU(x)                   (x >= GUI_MDI_MENU_FIRST && x <= GUI_MDI_MENU_LAST)

/* Initialization Functions */

extern bool         GUIXWndInit( unsigned );


extern bool         GUIXInitHotSpots( int num, gui_resource *hot );
extern void         GUIXCleanupHotSpots( void );

/* Window Functions */
extern bool         GUIXCreateWindow( gui_window *wnd, gui_create_info *, gui_window *parent_wnd );
extern void         GUIXSetupWnd( gui_window *wnd );

/* Control Functions */
extern bool         GUIXCreateDialog( gui_create_info *dlg_info, gui_window *wnd, int,
                        gui_control_info *controls_info, bool sys, res_name_or_id dlg_id );

/* Administration functions */

extern void         GUIFreeWindowMemory( gui_window *wnd, bool from_parent, bool dialog );

extern bool         GUIXCreateFloatingPopup( gui_window *wnd, const gui_point *location,
                        const gui_menu_items *menus, gui_mouse_track track, gui_ctl_id *curr_id );
extern bool         GUIXCreateToolBar( gui_window *wnd, bool fixed, gui_ord height,
                        const gui_toolbar_items *toolinfo, bool excl, gui_colour_set *plain,
                        gui_colour_set *standout, const gui_rect *float_pos );
extern bool         GUIXCreateToolBarWithTips( gui_window *wnd, bool fixed, gui_ord height,
                        const gui_toolbar_items *toolinfo, bool excl, gui_colour_set *plain,
                        gui_colour_set *standout, const gui_rect *float_pos, bool use_tips );

extern bool         GUIXCloseToolBar( gui_window *wnd );

extern void         GUIXDrawText( gui_window *wnd, const char *text, size_t length, const gui_coord *pos,
                        gui_attr attr, gui_ord extentx, bool draw_extent );
extern void         GUIXDrawTextRGB( gui_window *wnd, const char *text, size_t length, const gui_coord *pos,
                        gui_rgb fore, gui_rgb back, gui_ord extentx, bool draw_extent );
extern void         GUISetKeyState( void );
extern gui_window   *GUIXGetRootWindow( void );
