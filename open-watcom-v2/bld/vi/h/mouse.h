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


#ifndef _MOUSE_INCLUDED
#define _MOUSE_INCLUDED

/* these are DOS constants, but we need some consistancy */
#define MOUSE_LEFT_BUTTON_DOWN          0x01
#define MOUSE_RIGHT_BUTTON_DOWN         0x02
#define MOUSE_MIDDLE_BUTTON_DOWN        0x04
#define MOUSE_ANY_BUTTON_DOWN           0x07

/* mouse events */
typedef enum vi_mouse_event {
    VI_MOUSE_NONE = -1,
    VI_MOUSE_PRESS,
    VI_MOUSE_RELEASE,
    VI_MOUSE_DCLICK,
    VI_MOUSE_HOLD,
    VI_MOUSE_DRAG,
    VI_MOUSE_REPEAT,
    VI_MOUSE_PRESS_R,
    VI_MOUSE_RELEASE_R,
    VI_MOUSE_DCLICK_R,
    VI_MOUSE_HOLD_R,
    VI_MOUSE_DRAG_R,
    VI_MOUSE_REPEAT_R,
    VI_MOUSE_PRESS_M,
    VI_MOUSE_RELEASE_M,
    VI_MOUSE_DCLICK_M,
    VI_MOUSE_HOLD_M,
    VI_MOUSE_DRAG_M,
    VI_MOUSE_REPEAT_M,
    VI_MOUSE_MOVE
} vi_mouse_event;

typedef bool (*mouse_callback)( window_id, int, int );

typedef struct mouse_hook {
    struct mouse_hook   *next;
    mouse_callback      cb;
} mouse_hook;

/* hdlmouse.c */
extern vi_rc        HandleMouseEvent( void );

/* mouse.c */
extern vi_mouse_event GetMouseEvent( void );
extern void         RedrawMouse( windim row, windim col );
extern bool         DisplayMouse( bool flag );

/* mouseev.c */
extern window_id    GetMousePosInfo( windim *win_x, windim *win_y );
extern bool         TestMouseEvent( bool );
extern void         PushMouseEventHandler( mouse_callback cb );
extern void         PopMouseEventHandler( void );

/* system dependant, <sys>mouse.c */
extern void         SetMouseSpeed( int speed );
extern void         SetMousePosition( windim row, windim col );
extern void         ShowMouse( int on );
extern void         PollMouse( int *status, windim *row, windim *col );
extern void         InitMouse( void );
extern void         FiniMouse( void );

#endif
