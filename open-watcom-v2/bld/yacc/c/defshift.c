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
* Description:  Mark default shifts.
*
****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "yacc.h"
#include "yaccins.h"
#include "alloc.h"

unsigned keyword_id_low;
unsigned keyword_id_high;
unsigned nStates;
unsigned nActions;

static bool okToConsider( a_sym *sym )
{
    if( sym->pro != NULL )
        return( false );
    if( sym->token < keyword_id_low )
        return( false );
    if( sym->token > keyword_id_high )
        return( false );
    return( true );
}

static void doState( a_state *state, unsigned *state_freq, bool *all_used, unsigned range_size )
{
    unsigned        i;
    unsigned        max;
    action_n        max_sidx;
    a_shift_action  *saction;
    a_sym           *shift_sym;
    index_n         sidx;

    if( IsDead( state ) ) {
        return;
    }
    if( IsAmbiguous( state ) ) {
        return;
    }
    if( state->redun->pro != NULL ) {
        // has a reduction
        return;
    }
    memset( state_freq, 0, nstate * sizeof( *state_freq ) );
    memset( all_used, 0, range_size * sizeof( bool ) );
    /* find shift state frequencies */
    for( saction = state->trans; (shift_sym = saction->sym) != NULL; ++saction ) {
        if( ! okToConsider( shift_sym ) )
            continue;
        all_used[shift_sym->token - keyword_id_low] = true;
        ++state_freq[saction->state->sidx];
    }
    /* verify entire range of tokens shift somewhere */
    for( i = 0; i < range_size; ++i ) {
        if( !all_used[i] ) {
            return;
        }
    }
    /* find which state had the highest frequency */
    max_sidx = 0;
    max = state_freq[0];
    for( sidx = 1; sidx < nstate; ++sidx ) {
        unsigned test = state_freq[sidx];
        if( test > max ) {
            max = test;
            max_sidx = sidx;
        }
    }
    if( max == 0 ) {
        return;
    }
    ++nStates;
    /* mark tokens used to shift to highest frequency state */
    for( saction = state->trans; (shift_sym = saction->sym) != NULL; ++saction ) {
        if( ! okToConsider( shift_sym ) )
            continue;
        if( saction->state->sidx == max_sidx ) {
            saction->is_default = true;
            ++nActions;
        }
    }
}

void MarkDefaultShifts( void )
/****************************/
{
    unsigned range_size;
    unsigned *state_freq;
    bool *all_used;
    int i;

    state_freq = MALLOC( nstate, unsigned );
    range_size = ( keyword_id_high - keyword_id_low ) + 1;
    all_used = MALLOC( range_size, bool );
    for( i = 0; i < nstate; ++i ) {
        doState( statetab[i], state_freq, all_used, range_size );
    }
    FREE( state_freq );
    FREE( all_used );
    dumpstatistic( "default shift optimization states", nStates );
    dumpstatistic( "default shift optimization actions", nActions );
}
