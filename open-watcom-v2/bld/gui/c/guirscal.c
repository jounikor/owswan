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
* Description:  Get rounded coordinate scaling factors.
*
****************************************************************************/


#include "guiwind.h"
#include "guiscale.h"


void GUIAPI GUIGetRoundScale( gui_coord * scale )
{
    gui_rect screen;

    GUIGetScreen( &screen );

    // The magnitude of screen size is wildly different in text vs. GUI mode.
    // Typical scaling could be 1000,1000 and the screen can be larger than that.
    if( scale->x < screen.width || scale->y < screen.height ) {
        scale->x = screen.width;
        scale->y = screen.height;
    } else {
        scale->x = ( scale->x / screen.width ) * screen.width;
        scale->y = ( scale->y / screen.height ) * screen.height;
    }
}
