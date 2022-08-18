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


#include <stdio.h>
#include <errno.h>
#include <conio.h>
#include <string.h>
#include <process.h>
#include "uidef.h"
#include "uimouse.h"
#include "uinlm.h"
#include "uicurshk.h"


int     ScreenHandle;       // used by uikeyb.c and uicurs.c as well

static MONITOR ui_data = {
    25,
    80,
    M_CGA,
    NULL,
    NULL,
    NULL,
    NULL,
    4,
    1
};

bool UIAPI uiset80col( void )
/***************************/
{
    return( true );
}

static bool initmonitor( void )
/*****************************/
{
    WORD  height;
    WORD  width;

    ScreenHandle = CreateScreen( uigetscreenname(), DONT_CHECK_CTRL_CHARS | AUTO_DESTROY_SCREEN );
    if( ScreenHandle == -1 ) {
        return( false );
    }
    SetCurrentScreen( ScreenHandle );

    SetPositionOfInputCursor( 0, 0 );
    HideInputCursor();

    DisplayScreen( ScreenHandle );

    if( UIData == NULL ) {
        UIData = &ui_data;
    }

    GetSizeOfScreen( &height, &width );

    UIData->height = height;
    UIData->width  = width;


    /* IsColorMonitor doesn't seem to be working for NetWare 3.11 */
    /* so we'll just assume a colour monitor for now. */

    if( IsColorMonitor() ) {
        UIData->colour = M_CGA;
    } else {
        UIData->colour = M_MONO;
    }

    return( true );
}


bool intern initbios( void )
/**************************/
{
    bool        initialized = false;
    size_t      size;
    size_t      i;

    if( initmonitor() ) {

        size = UIData->width * UIData->height;
        UIData->screen.origin = (LP_PIXEL)uimalloc( size * sizeof( PIXEL ) );
        for( i = 0; i < size; ++i ) {
            UIData->screen.origin[i].ch = ' ';
            UIData->screen.origin[i].attr = 7;
        } /* end for */

        UIData->screen.increment = UIData->width;

        uiinitcursor();
        initkeyboard();

        /* No mouse support in NetWare! */

        UIData->mouse_acc_delay = 0;
        UIData->mouse_rpt_delay = 0;
        UIData->mouse_clk_delay = 0;
        UIData->mouse_speed     = 0;

        /* A 500 millisecond tick delay is pretty reasonable. */

        UIData->tick_delay = uiclockdelay( 500 /* ms */ );

        initialized = true;
    }

    return( initialized );

} /* end initbios */

static void finimonitor( void )
/********************/
{
    DestroyScreen( ScreenHandle );
} /* end finimonitor */

void intern finibios( void )
/**************************/
{

    uifinicursor();
    finikeyboard();
    finimonitor();
    uifree( (void *)UIData->screen.origin );

}

/* Update the physical screen with the contents of the virtual copy. */

void intern physupdate( SAREA *area )
/***********************************/
{
    int i;

    for( i = area->row; i < area->row + area->height; ++i ) {
        CopyToScreenMemory( 1, area->width, (void *)( UIData->screen.origin + i * UIData->width + area->col ), area->col, i );
    } /* end for */

} /* end physupdate */

