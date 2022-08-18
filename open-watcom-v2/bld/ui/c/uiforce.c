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


#include "uidef.h"
#include "uiforce.h"

#define FORCED_Q_SIZE 5

typedef struct {
    int         head;
    int         tail;
    ui_event    ui_ev[FORCED_Q_SIZE];
} Queue;

static Queue Forced_Q = { 0, 0 };

static bool Q_ismember( Queue *q, ui_event ui_ev )
/************************************************/
{
    int i;

    for( i = q->head; i != q->tail; i = (i + 1) % FORCED_Q_SIZE ) {
        if( q->ui_ev[i] == ui_ev ) {
            return( true );
        }
    }
    return( false );
}

#define _queue_empty( q ) (q).head = (q).tail = 0
#define _queue_is_empty( q ) ((q).head == (q).tail)
#define _queue_is_full( q ) ( ((q).tail + 1) % FORCED_Q_SIZE == (q).head )
#define _queue_add( q, ev )     (q).ui_ev[(q).tail] = (ev); \
                                (q).tail = ((q).tail + 1) % FORCED_Q_SIZE
#define _queue_remove( q, evp ) *(evp) = (q).ui_ev[(q).head]; \
                                (q).head = ((q).head + 1) % FORCED_Q_SIZE
#define _queue_is_member( q, ev ) Q_ismember( &(q), ev )


void intern forceevinit( void )
/*****************************/
{
    // initialize critical section
}

void intern forceevfini( void )
/*****************************/
{
    // finalize critical section
}

bool UIAPI uiforceevadd( ui_event ui_ev )
/***************************************/
{
    bool ret;

    // enter critical section
    if( !_queue_is_full( Forced_Q ) ) {
        _queue_add( Forced_Q, ui_ev );
        ret = true;
    } else {
        ret = false;
    }
    // exit critical section
    return( ret );
}

void UIAPI uiforceevflush( void )
/********************************/
/* The EV_KILL_UI event can't be flushed */
{
    bool    ev_kill;

    // enter critical section
    ev_kill = _queue_is_member( Forced_Q, EV_KILL_UI );
    _queue_empty( Forced_Q );
    if( ev_kill ) {
        _queue_add( Forced_Q, EV_KILL_UI );
    }
    // exit critical section
}

ui_event intern forcedevent( void )
/*********************************/
/* This function should ONLY be called from the uieventsource function */
{
    ui_event    ui_ev;

    // enter critical section
    if( _queue_is_empty( Forced_Q ) ) {
        ui_ev = EV_NO_EVENT;
    } else {
        _queue_remove( Forced_Q, &ui_ev );
    }
    // exit critical section

    return( ui_ev );
}
