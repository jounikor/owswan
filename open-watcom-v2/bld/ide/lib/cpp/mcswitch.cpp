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


#include "mtool.hpp"
#include "mcswitch.hpp"
#include "wobjfile.hpp"
#include "mcstate.hpp"

Define( MCSwitch )

MCSwitch::MCSwitch( WTokenFile& fil, WString& tok )
    : MSwitch( fil, tok )
{
    fil.token( _on );
    bool state = false;
    for( int i=0; i<SWMODE_COUNT; i++ ) {
        if( !fil.eol() ) {
            state = ( fil.token( tok ) == "ON" );
        }
        _state[i] = state;
    }
}

#ifndef NOPERSIST
MCSwitch* WEXPORT MCSwitch::createSelf( WObjectFile& )
{
    return( new MCSwitch() );
}

void WEXPORT MCSwitch::readSelf( WObjectFile& p )
{
    MSwitch::readSelf( p );
    p.readObject( &_on );
    if( p.version() > 28 ) {
        for( int i=0; i<SWMODE_COUNT; i++ ) {
            p.readObject( &_state[i] );
        }
    } else {
        p.readObject( &_state[SWMODE_RELEASE] );
        _state[SWMODE_DEBUG] = _state[SWMODE_RELEASE];
    }
}

void WEXPORT MCSwitch::writeSelf( WObjectFile& p )
{
    MSwitch::writeSelf( p );
    p.writeObject( &_on );
    for( int i=0; i<SWMODE_COUNT; i++ ) {
        p.writeObject( _state[i] );
    }
}
#endif

void MCSwitch::addone( WString& str, bool state )
{
    if( state && _on.size() > 0 ) {
        str.concat( _on );
    }
}

void MCSwitch::getText( WString& str, MState* state )
{
    MCState* st = (MCState*)state;
    addone( str, st->state() );
}

void MCSwitch::getText( WString& str, WVList* states, SwMode mode )
{
    bool state = _state[mode];
    WVList found;
    findStates( states, found );
    int icount = found.count();
    for( int i=0; i<icount; i++ ) {
        MCState* st = (MCState*)found[i];
        if( st->mode() == mode ) {
            state = st->state();
        }
    }
    addone( str, state );
}
