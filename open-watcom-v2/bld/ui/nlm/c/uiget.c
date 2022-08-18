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


#include <time.h>
#include <process.h>
#include "uidef.h"
#include "uiforce.h"
#include "nw_lib.h"


#define MILLISEC_PER_TICK   10L

// be very careful about setting this true
static bool EnterForever = false;

MOUSETIME UIAPI uiclock( void )
/*****************************
 * this routine get time in platform dependant units,
 * used for mouse & timer delays
 */
{
    /* NetWare uses a clock tick of .01 seconds. */
    return( clock() );
}

unsigned UIAPI uiclockdelay( unsigned milli )
/*******************************************
 * this routine converts milli-seconds into platform
 * dependant units - used to set mouse & timer delays
 */
{
    /* NetWare uses a clock tick of .01 seconds. */
    return( ( milli + MILLISEC_PER_TICK / 2 ) / MILLISEC_PER_TICK );
}

void UIAPI uiflush( void )
/************************/
{
    uiflushevent();
    flushkey();
}

void UIAPI uiforceinfloop( void )
/********************************/
/* This is a dangerous function. This may be called from any thread. */
{
    EnterForever = true;
    uiwakethread();
    while( kbdisblocked() ) {
        ThreadSwitch();
    }
}

static void foreverloop( void )
/*****************************/
/* This function intentionally goes into an infinite loop. It's purpose */
/* is to allow the UI thread of a Netware application to unblock off the */
/* keyboard to allow the NLM to call exit */
{
    for( ;; ) {
        ThreadSwitch();
    }
}

ui_event UIAPI uieventsource( bool update )
/*****************************************/
{
    static int      ReturnIdle = 1;
    ui_event        ui_ev;
    MOUSETIME       start;

    start = uiclock();
    for( ;; ) {
        ThreadSwitch();
        if( EnterForever )
            foreverloop();

        ui_ev = forcedevent();
        if( ui_ev > EV_NO_EVENT )
            break;

        /* There is no mouse support in NetWare. */
        //ev = mouseevent();
        //if( ui_ev > EV_NO_EVENT )
        //    break;

        ui_ev = keyboardevent();
        if( ui_ev > EV_NO_EVENT ) {
            //uihidemouse();
            break;
        }
        if( ReturnIdle ) {
            ReturnIdle--;
            return( EV_IDLE );
        } else {
            if( update )
                uirefresh();
            if( uiclock() - start >= UIData->tick_delay ) {
                return( EV_CLOCK_TICK );
            } else if( UIData->busy_wait ) {
                return( EV_SINK );
            }
        }
        waitforevent();
    }
    ReturnIdle = 1;
    return( ui_ev );
}


ui_event UIAPI uiget( void )
/**************************/
{
    return( uieventsource( true ) );
}
