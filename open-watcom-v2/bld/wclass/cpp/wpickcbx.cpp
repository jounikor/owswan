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


#include "wpickcbx.hpp"

#define _model ((WPickList*)model())


WEXPORT WPickCBox::WPickCBox( WPickList& plist, cbs callback, WWindow* win, const WRect& r, char* text, WStyle style )
    : WComboBox( win, r, text, (style & ~CStyleSorted) )
    , WView( &plist )
    , _nameCallback( callback )
    , _sorted( (style & CStyleSorted) != 0 ) {
/********************************************/

    fillBox();
}


void WEXPORT WPickCBox::name( int index, WString& str ) {
/*******************************************************/

    if( _nameCallback != NULL ) {
        (((*_model)[index])->*_nameCallback)( str );
    } else {
        str = "";
    }
}


void WEXPORT WPickCBox::fillBox() {
/*********************************/
    setUpdates( false );
    if( _sorted ) {
        _model->sort();
    }
    WString maxStr;
    int icount = _model->count();
    for( int i=0; i<icount; i++ ) {
        WString n;
        name( i, n );
        int index = insertString( n );
        setTagPtr( index, (*_model)[i] );
        if( n.size() > maxStr.size() ) {
            maxStr = n;
        }
    }
    setUpdates( true );
}


void* WEXPORT WPickCBox::selectedTagPtr() {
/*****************************************/

    int index = selected();
    if( index >= 0 ) {
        return( tagPtr( index ) );
    }
    return( NULL );
}


void WEXPORT WPickCBox::updateView() {
/************************************/

    if( _model != NULL ) {
        int cur = selected();
        reset();
        fillBox();
        select( cur );
    }
}


void WEXPORT WPickCBox::modelGone() {
/***********************************/

    reset();
}
