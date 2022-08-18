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


#include <wpshbttn.hpp>
#include <wmsgdlg.hpp>

#include "brwhelp.h"
#include "gtclsopt.h"
#include "gtinhopt.h"
#include "gtlnopt.h"
#include "optmgr.h"
#include "screendv.h"
#include "util.h"
#include "wbrwin.h"

const char * const Desc[ 6 ] = {
    "Public, Non-Virtual",
    "Protected, Non-Virtual",
    "Private, Non-Virtual",
    "Public, Virtual",
    "Protected, Virtual",
    "Private, Virtual",
};

GTInheritOption::GTInheritOption( const GTClassOpts & opt )
            : InheritStyleDlg( this )
            , WDialog( frame().r, frame().t )
            , _hasFocus( 0 )
//----------------------------------------------------
{
    _options = new GTClassOpts( opt );
    _rects = new WRect[ 6 ];
}

GTInheritOption::~GTInheritOption()
//---------------------------------
{
    delete _options;
    delete[] _rects;
}

void GTInheritOption::initialize()
//--------------------------------
{
    setSystemFont( false );
    rescale();
    move( frame().r );
    centre();

    _okButton          = new WDefPushButton( this, _okButtonR.r, _okButtonR.t );
    _cancelButton      = new WPushButton( this, _cancelButtonR.r, _cancelButtonR.t );
    _modifyButton      = new WPushButton( this, _modifyButtonR.r, _modifyButtonR.t );
    _helpButton        = new WPushButton( this, _helpButtonR.r, _helpButtonR.t );

    _okButton->show();
    _cancelButton->show();
    _modifyButton->show();
    _helpButton->show();

    _okButton->onClick(     this, (cbw) GTInheritOption::okButton );
    _cancelButton->onClick( this, (cbw) GTInheritOption::cancelButton );
    _modifyButton->onClick( this, (cbw) GTInheritOption::modifyButton );
    _helpButton->onClick(   this, (cbw) GTInheritOption::helpButton );

    _okButton->setFocus();

    _rects[ 0 ] = _nvPublicR.r;
    _rects[ 1 ] = _nvProtectedR.r;
    _rects[ 2 ] = _nvPrivateR.r;

    _rects[ 3 ] = _vPublicR.r;
    _rects[ 4 ] = _vProtectedR.r;
    _rects[ 5 ] = _vPrivateR.r;

    show();
}

void GTInheritOption::okButton( WWindow * )
//-----------------------------------------
{
    WBRWinBase::optManager()->setClassOpts( _options );
    quit( true );
}

void GTInheritOption::cancelButton( WWindow * )
//---------------------------------------------
{
    WBRWinBase::optManager()->setClassOpts( NULL );
    quit( false );
}

void GTInheritOption::modifyButton( WWindow * )
//---------------------------------------------
{
    PaintInfo * p;
    if( _hasFocus < 3 ) {
        p = &_options->value( _hasFocus + 1, false );
    } else {
        p = &_options->value( _hasFocus - 2, true );
    }
    GTLineOption mod( Desc[ _hasFocus ], this, p, true );
    mod.process( this );
}

void GTInheritOption::helpButton( WWindow * )
//-------------------------------------------
{
    WBRWinBase::helpInfo()->sysHelpId( BRH_INHERITANCE_OPTIONS );
}

bool GTInheritOption::contextHelp( bool is_active_win )
//-----------------------------------------------------
{
    if( is_active_win ) {
        WBRWinBase::helpInfo()->sysHelpId( BRH_INHERITANCE_OPTIONS );
    }
    return( true );
}

int GTInheritOption::inRect( int x, int y )
//-----------------------------------------
{
    int inr = -1;
    int i;

    for( i = 0; i < 6; i += 1 ) {
        WRect & r( _rects[ i ] );

        if( r.contains( x, y ) ) {
            inr = i;
            break;
        }
    }

    return inr;
}

bool GTInheritOption::leftBttnDn( int x, int y, WMouseKeyFlags )
//--------------------------------------------------------------
{
    int focusTo = inRect( x, y );

    if( focusTo >= 0 && focusTo != _hasFocus ) {
        int prev_focus = _hasFocus;
        _hasFocus = focusTo;
        invalidateRect( _rects[ prev_focus ] );
        invalidateRect( _rects[ _hasFocus ] );
    }
    return true;
}

bool GTInheritOption::leftBttnDbl( int x, int y, WMouseKeyFlags )
//---------------------------------------------------------------
{
    if( _hasFocus == inRect( x, y ) ) {
        modifyButton( NULL );
    }
    return true;
}

#define COL_1   3
#define COL_2   3

bool GTInheritOption::keyDown( WKeyCode kc, WKeyState ks )
//--------------------------------------------------------
{
    int         prev_focus = _hasFocus;

    switch( kc ) {
    case WKeyUp:
        if( _hasFocus < COL_1 ) {
            _hasFocus = (_hasFocus - 1 + COL_1) % COL_1;
        } else {
            _hasFocus = COL_1 + (_hasFocus - COL_1 - 1 + COL_2) % COL_2;
        }
        invalidateRect( _rects[ prev_focus ] );
        invalidateRect( _rects[ _hasFocus ] );
        return true;
    case WKeyDown:
        if( _hasFocus < COL_1 ) {
            _hasFocus = (_hasFocus + 1) % COL_1;
        } else {
            _hasFocus = COL_1 + (_hasFocus - COL_1 + 1) % COL_2;
        }
        invalidateRect( _rects[ prev_focus ] );
        invalidateRect( _rects[ _hasFocus ] );
        return true;
    case WKeyLeft:
    case WKeyRight:
        if( _hasFocus < COL_1 ) {
            _hasFocus = COL_1;
        } else {
            _hasFocus = 0;
        }
        invalidateRect( _rects[ prev_focus ] );
        invalidateRect( _rects[ _hasFocus ] );
        return true;
    }
    return( WDialog::keyDown( kc, ks ) );
}

static void OutRect( const ControlRect & rect, ScreenDev & dev )
//--------------------------------------------------------------
{
    WPoint      p( rect.r.x(), rect.r.y() );

    dev.drawText( p, rect.t );
}

bool GTInheritOption::paint()
//---------------------------
{
    int         i;
    ScreenDev   dev;
    PaintInfo   black( ColorBlack, 1, LS_PEN_SOLID );
    PaintInfo   gray( ColorDarkGray, 1, LS_PEN_DOT );
    WRect       r;

    dev.open( this );

    dev.setPaintInfo( &black );
    dev.rectangle( _nonVirtualBoxR.r );
    dev.rectangle( _virtualBoxR.r );

    OutRect( _nonVirtualTextR, dev );
    OutRect( _nvPublicTextR, dev );
    OutRect( _nvProtectedTextR, dev );
    OutRect( _nvPrivateTextR, dev );
    OutRect( _virtualTextR, dev );
    OutRect( _vPublicTextR, dev );
    OutRect( _vProtectedTextR, dev );
    OutRect( _vPrivateTextR, dev );

    for( i = 1; i <= 3; i += 1 ) {
        WRect &     r( _rects[ i - 1 ] );
        PaintInfo   p( _options->value( i, false ) );

        dev.setPaintInfo( &p );
        dev.moveTo( r.x(), r.y() + r.h() / 2 );
        dev.lineTo( r.x() + r.w(), r.y() + r.h() / 2 );

        if( (i - 1) == _hasFocus ) {
            dev.setPaintInfo( &gray );
            dev.rectangle( _rects[ i - 1 ] );
        }
    }

    for( i = 1; i <= 3; i += 1 ) {
        WRect &     r( _rects[ i + 2 ] );
        PaintInfo   p( _options->value( i, true ) );

        dev.setPaintInfo( &p );
        dev.moveTo( r.x(), r.y() + r.h() / 2 );
        dev.lineTo( r.x() + r.w(), r.y() + r.h() / 2 );

        if( (i + 2) == _hasFocus ) {
            dev.setPaintInfo( &gray );
            dev.rectangle( _rects[ i + 2 ] );
        }
    }

    dev.close();

    return true;
}

void GTInheritOption::endEdit()
//-----------------------------
// line editor informs us it is dead
// so don't access it any more
{
}

void GTInheritOption::setInfo( PaintInfo * p )
//--------------------------------------------
// line editor wishes to be terminated (suicidal?)
// and may or may not have changed the PaintInfo
{
    if( p ) {
        invalidate();
    }
}
