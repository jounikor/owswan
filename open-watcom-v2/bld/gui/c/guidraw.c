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


#include "guiwind.h"

/*
 * GUIDrawText
 */

void GUIAPI GUIDrawText( gui_window *wnd, const char *text, size_t length,
                         gui_text_ord row, gui_ord indent, gui_attr attr )
{
    gui_text_metrics    metrics;
    gui_coord           pos;

    GUIGetTextMetrics( wnd, &metrics );
    pos.x = indent;
    pos.y = row * metrics.avg.y;
    GUIXDrawText( wnd, text, length, &pos, attr, GUI_NO_COLUMN, false );
}

void GUIAPI GUIDrawTextPos( gui_window *wnd, const char *text, size_t length,
                            const gui_coord *pos, gui_attr attr )
{
    GUIXDrawText( wnd, text, length, pos, attr, GUI_NO_COLUMN, false );
}

/*
 * GUIDrawTextExtent
 */

void GUIAPI GUIDrawTextExtent( gui_window *wnd, const char *text, size_t length,
                        gui_text_ord row, gui_ord indent, gui_attr attr,
                        gui_ord extentx )
{
    gui_text_metrics    metrics;
    gui_coord           pos;

    GUIGetTextMetrics( wnd, &metrics );
    pos.x = indent;
    pos.y = row * metrics.avg.y;
    GUIXDrawText( wnd, text, length, &pos, attr, extentx, true );
}

void GUIAPI GUIDrawTextExtentPos( gui_window *wnd, const char *text, size_t length,
                           const gui_coord *pos, gui_attr attr, gui_ord extentx )
{
    GUIXDrawText( wnd, text, length, pos, attr, extentx, true );
}

void GUIAPI GUIDrawTextRGB( gui_window *wnd, const char *text, size_t length,
                            gui_text_ord row, gui_ord indent,
                            gui_rgb fore, gui_rgb back )
{
    gui_text_metrics    metrics;
    gui_coord           pos;

    GUIGetTextMetrics( wnd, &metrics );
    pos.x = indent;
    pos.y = row * metrics.avg.y;
    GUIXDrawTextRGB( wnd, text, length, &pos, fore, back, GUI_NO_COLUMN, false );
}

void GUIAPI GUIDrawTextPosRGB( gui_window *wnd, const char *text, size_t length,
                               const gui_coord *pos, gui_rgb fore, gui_rgb back )
{
    GUIXDrawTextRGB( wnd, text, length, pos, fore, back, GUI_NO_COLUMN, false );
}

/*
 * GUIDrawTextExtent
 */

void GUIAPI GUIDrawTextExtentRGB( gui_window *wnd, const char *text, size_t length,
                           gui_text_ord row, gui_ord indent,
                           gui_rgb fore, gui_rgb back, gui_ord extentx )
{
    gui_text_metrics    metrics;
    gui_coord           pos;

    GUIGetTextMetrics( wnd, &metrics );
    pos.x = indent;
    pos.y = row * metrics.avg.y;
    GUIXDrawTextRGB( wnd, text, length, &pos, fore, back, extentx, true );
}

void GUIAPI GUIDrawTextExtentPosRGB( gui_window *wnd, const char *text, size_t length,
                              const gui_coord *pos, gui_rgb fore, gui_rgb back,
                              gui_ord extentx )
{
    GUIXDrawTextRGB( wnd, text, length, pos, fore, back, extentx, true );
}
