/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Include file for aui library sample application.
*
****************************************************************************/


#include <stdlib.h>
#include <string.h>
#ifndef __UNIX__
#include <process.h>
#endif
#include "aui.h"
#include "guidlg.h"
#include "rcdefs.h"
#include "dlgbutn.h"


#define ArraySize( x )          (sizeof( x ) / sizeof( *(x) ))

enum {
    MENU_FIRST          = 2400,
    MENU_SEARCH,
    MENU_NEXT,
    MENU_PREV,
    MENU_QUIT,
    MENU_COMMAND,
    MENU_PICK,
    MENU_STATUS,
    MENU_MATCH,
    MENU_SCRAMBLE_MENUS,
    MENU_BUG,
    MENU_OPTIONS,
    MENU_PASSWORD,

    MENU_SECOND         = 2500,
    MENU_OPEN1,
    MENU_OPEN1A,
    MENU_OPEN2,
    MENU_OPEN3,
    MENU_OPEN4,
    MENU_OPEN5,
    MENU_GET_FILE,
    MENU_TOOLS,
    MENU_OPEN4B,
    MENU_OPEN6,
    MENU_OPEN7,
    MENU_MORE,
    MENU_OPEN8,


    MENU_THIRD          = 2600,
    MENU_POPUP          = 2700,

    MENU_W1_ALIGN       = 500,
    MENU_W1_UNALIGN,
    MENU_W1_SAY,
    MENU_W1_NEWWORD,
    MENU_W1_MORE,

    MENU_W2_SAY         = 600,
    MENU_W2_TOP,
    MENU_W2_BOTTOM,
    MENU_W2_TITLE,
    MENU_W2_OPEN1,

    MENU_W3_POPUP       = 700,
    MENU_W3_BUG

};

enum {
    APP_COLOR_PLAIN = WND_FIRST_UNUSED,
    APP_COLOR_TABSTOP,
    APP_COLOR_SELECTED,
    APP_COLOR_HOTSPOT,
    APP_COLOR_CENSORED,
    APP_COLOR_BASEBALL,
    APP_NUMBER_OF_COLOURS
};

typedef enum {
    EV_UPDATE_NONE      = 0x0000,
    EV_UPDATE_1         = 0x0001,
    EV_UPDATE_2         = 0x0002,
    EV_UPDATE_ALL       = 0xFFFF,
} wnd_update_flags;

enum {
    CLASS_W1,
    CLASS_W2
};

extern wnd_update_flags WndUpdateFlags;

extern int RandNum( int );

#define MAX_WORD 14
#define WORD_SIZE 50

extern char     *Word[WORD_SIZE];

extern WNDOPEN W1Open;
extern WNDOPEN W2Open;
extern WNDOPEN W3Open;
extern WNDOPEN W4Open;
extern WNDOPEN W5Open;
extern WNDOPEN W6Open;
extern WNDOPEN W7Open;
extern WNDOPEN W8Open;

extern void     Password( const char *title, char *buff, size_t buff_len );
extern void     DlgCmd( void );
