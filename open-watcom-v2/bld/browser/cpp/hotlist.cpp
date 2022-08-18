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


#include <limits.h>     // NYI -- only for scroll bandage
#include <whotspot.hpp>
#include <wkeydefs.hpp>

#include "hotspots.h"
#include "hotlist.h"
#include "wbrdefs.h"

HotControlList::HotControlList( WWindow * prt, const WRect & r, WStyle stl )
                : WWindow( prt, r, NULL, stl )
                , HotSpotList( this, false )
//--------------------------------------------------------------------------
{
}

bool HotControlList::losingFocus( WWindow * )
//-------------------------------------------
{
    if( _selectedAttr != WPaintAttrMenuActive ) {
        _selectedAttr = WPaintAttrMenuActive;
        _win->invalidateRow( _selected - _topIndex );
    }
    return( false );
}

bool HotControlList::gettingFocus( WWindow * )
//--------------------------------------------
{
    if( _selectedAttr != WPaintAttrIcon ) {
        _selectedAttr = WPaintAttrIcon;
        _win->invalidateRow( _selected - _topIndex );
    }
    return( false );
}

HotWindowList::HotWindowList( const char * text, bool inf, WStyle stl )
                : WBRWindow( text, stl )
                , HotSpotList( this, inf )
//---------------------------------------------------------------------------
{
}

void HotWindowList::resized( WOrdinal w, WOrdinal h )
//---------------------------------------------------
{
    reset();
    WWindow::resized( w, h );
}

// HotSpotList has semantics for derived classes which are infinite
// or non-infinite.
//
// Non-infinite derived classes have a count() member
// function which is always valid, and they never return NULL from
// getString.
//
// Infinite lists have a member full() which returns true only when
// the list has been filled completely -- at this point, count() is
// valid.  If a string outside the range is requested, NULL is returned
//

HotSpotList::HotSpotList( WWindow * win, bool infinite )
                : _win( win )
                , _topIndex( 0 )
                , _selected( 0 )
                , _selectedAttr( WPaintAttrIcon )
                , _leftDown( false )
                , _hotPressIdx( -1 )
                , _inHotZone( false )
                , _infinite( infinite )
                , _width( 0 )
                , _changedClient( NULL )
                , _changed( NULL )
                , _dblClickClient( NULL )
                , _dblClick( NULL )
                , _hotPressClient( NULL )
                , _hotPress( NULL )
//--------------------------------------------------------------
{
}

bool HotSpotList::HLPaint()
//-------------------------
{
    int             maxRows = _win->numDirtyRows() + _topIndex + _win->firstDirtyRow();
    int             numElem = _infinite ? 0 : count();
    int             offset;
    int             hotSpot;
    WPoint          hotSize;
    int             maxExtent( _width );

    for( long i = _topIndex + _win->firstDirtyRow(); i < maxRows; i += 1 ) {
        const char * str;
        int          extent;

        if( !_infinite && i >= numElem ) break;
        str = getString( i );
        if( str == NULL ) break;

        if( i == _hotPressIdx && _inHotZone ) {
            hotSpot = getHotSpot( i, true );
        } else {
            hotSpot = getHotSpot( i, false );
        }

        GlobalHotSpots->hotSpotSize( hotSpot, hotSize );
        offset = getHotOffset( i ) + hotSize.x();

        _win->drawHotSpot( hotSpot, i - _topIndex, getHotOffset( i ) );

        extent = _win->getTextExtentX( str ) + offset;

        maxExtent = maxInt( maxExtent, extent );

        if( i == _selected ) {
            _win->drawTextExtent( i - _topIndex, offset, str, _selectedAttr, maxExtent );
        } else {
            _win->drawTextExtent( i - _topIndex, offset, str, WPaintAttrBackground, maxExtent );
        }
    }

    if( maxExtent != _width ) {
        _width = maxExtent;
        _win->setScrollRange( WScrollBarHorizontal, _width );
    }

    return true;
}

bool HotSpotList::HLMouseMove( int x, int y )
//-------------------------------------------
{
    if( _leftDown ) {
        int         row = _win->getRow( WPoint( x, y ) );
        int         oldSel = _selected;
        WPoint      hotSize;
        int         hotOffset;

        if( row < 0 ) row = 0;
        if( row > _win->getRows() - 1 ) {
            row = _win->getRows() - 1;
        }

        row += _topIndex;

        if( _hotPressIdx >= 0 ) {

            GlobalHotSpots->hotSpotSize( getHotSpot( _hotPressIdx, false ), hotSize );
            hotOffset = getHotOffset( _hotPressIdx );

            if( row == _hotPressIdx
                && x >= hotOffset
                && x <= hotOffset + hotSize.x() ) {

                if( !_inHotZone ) {
                    _inHotZone = true;
                    _win->invalidateRow( _hotPressIdx - _topIndex );
                }
            } else {
                if( _inHotZone ) {
                    _inHotZone = false;
                    _win->invalidateRow( _hotPressIdx - _topIndex );
                }
            }
        } else {
            if( full() && row >= count() ) {
                row = count() - 1;
            }

            if( _selected != row ) {
                _selected = row;

                _win->invalidateRow( oldSel - _topIndex );
                _win->invalidateRow( _selected - _topIndex );
                scrollToSelected();
            }
        }

        return true;
    } else {
        return false;
    }
}

bool HotSpotList::HLLeftBttnDn( int x, int y )
//--------------------------------------------
{
    int     hotOffset;
    WPoint  hotSize;
    int     row;

    row = _win->getRow( WPoint( x, y ) ) + _topIndex;

    if( row < 0 )
        row = 0;

    if( full() && row >= count() )
        row = count() - 1;

    if( row < 0 ) {     // count == 0
        return false;
    }

    GlobalHotSpots->hotSpotSize( getHotSpot( row, false ), hotSize );
    hotOffset = getHotOffset( row );

    if( x > hotOffset && x < hotOffset + hotSize.x() ) {
        _hotPressIdx = row;
    }

    _leftDown = true;

    HLMouseMove( x, y );

    return true;
}

bool HotSpotList::HLLeftBttnUp( int x, int y )
//--------------------------------------------
{
    int oldSel;

    HLMouseMove( x, y );
    _leftDown = false;

    if( _inHotZone && _win->getRow( WPoint( x, y ) ) + _topIndex == _hotPressIdx ) {
        // have to set false before calling invalidateRow().
        _inHotZone = false;

        if( _selected != _hotPressIdx ) {
            oldSel = _selected;
            _selected = _hotPressIdx;
            _win->invalidateRow( oldSel - _topIndex );
        }

        if( _hotPressClient && _hotPress )
            (_hotPressClient->*_hotPress)( _win );

        _win->invalidateRow( _selected - _topIndex );
    }

    _inHotZone = false;
    _hotPressIdx = -1;

    changed();

    return true;
}

bool HotSpotList::HLLeftBttnDbl( int x, int y )
//---------------------------------------------
{
    long      row = _win->getRow( WPoint( x, y ) ) + _topIndex;

    if( _selected == row ) {
        if( _dblClickClient && _dblClick ) {
            (_dblClickClient->*_dblClick)( _win );
        }
    }
    _win->invalidateRow( _selected - _topIndex );

    return true;
}

void HotSpotList::onChanged( WObject* obj, cbw changed )
//------------------------------------------------------
{
    _changedClient = obj;
    _changed = changed;
}

void HotSpotList::onDblClick( WObject* obj, cbw dblClick )
//--------------------------------------------------------
{
    _dblClickClient = obj;
    _dblClick = dblClick;
}

void HotSpotList::onHotPress( WObject* obj, cbw hotPress )
//--------------------------------------------------------
{
    _hotPressClient = obj;
    _hotPress = hotPress;
}

int HotSpotList::selected()
//-------------------------
{
    return _selected;
}

void HotSpotList::select( int index )
//-----------------------------------
{
    int curr_selected = _selected;
    _selected = index;
    _win->invalidateRow( curr_selected - _topIndex );
    _win->invalidateRow( _selected - _topIndex );
    scrollToSelected();
    changed();
}

void HotSpotList::resetScrollRange()
//----------------------------------
{
    // NYI -- fix scrolling gear!

    int scrollr;
    WPoint avg;
    WPoint max;
    int nRows;

    if( _infinite ) {
        nRows = _win->getRows();
        _win->setScrollTextRange( WScrollBarVertical, nRows * 2 );
        _win->setScrollTextPos( WScrollBarVertical, nRows / 2 );
    } else {
        _win->textMetrics( avg, max );
        if( (max.y() == 0) || (count() <= INT_MAX / max.y()) ) {
            scrollr = count();
        } else {
            scrollr = INT_MAX / max.y();
        }

        if( scrollr <= _win->getRows() ) {
            _topIndex = 0;
        }
        _win->setScrollTextPos( WScrollBarVertical, _topIndex );
        _win->setScrollTextRange( WScrollBarVertical, scrollr );

    }
}

void HotSpotList::reset()
//-----------------------
{
    resetScrollRange();
    _win->invalidate();
}

void HotSpotList::scrollToSelected()
//----------------------------------
{
    int nRows = _win->getRows();

    // make sure that _selected is within the range of
    // acceptable rows

    if( _selected < 0 || (full() && _selected >= count()) ) {
        _selected = 0;
    }

    if( _selected < _topIndex ) {
        performScroll( _selected, true );
    }
    if( _selected > _topIndex + nRows - 1 ) {
        performScroll( _selected - nRows + 1, true );
    }
}

void HotSpotList::changed()
//-------------------------
{
    if( _changedClient && _changed ) {
        (_changedClient->*_changed)( _win );
    }
}

void HotSpotList::performScroll( long pos, bool absolute )
//--------------------------------------------------------
{
    long    diff;
    int     nRows = _win->getRows();

    if( absolute ) {
        diff = pos - _topIndex;
    } else {
        diff = pos;
    }

    if( _topIndex + diff < 0 ) {        // it's important this appears twice
        diff = -1 * _topIndex;
    }
    if( _infinite ) {
        if( getString( _topIndex + diff ) == NULL ) {
            diff = count() - nRows - _topIndex;
        }
        if( full() && _topIndex + diff > count() - nRows ) {
            diff = count() - nRows - _topIndex;
        }
    } else {
        if( _topIndex + diff >= count() ) {
            diff = count() - nRows - _topIndex;
        }
        if( _topIndex + diff > count() - nRows ) {
            diff = count() - nRows - _topIndex;
        }
    }

    if( _topIndex + diff < 0 ) {        // it's important this appears twice
        diff = -1 * _topIndex;
    }

    _topIndex += diff;

    if( _infinite ) {
        _win->scrollWindow( WScrollBarVertical, diff );
    } else {
        _win->setScrollTextPos( WScrollBarVertical, _topIndex );
    }
}

bool HotSpotList::HLScrollNotify( WScrollNotification sn, int diff )
//------------------------------------------------------------------
{
    switch( sn ) {
        case WScrollUp:
            performScroll( -1 );
            return true;

        case WScrollPageUp:
            performScroll( -1 * _win->getRows() + 1 );
            return true;

        case WScrollDown:
            performScroll( 1 );
            return true;

        case WScrollPageDown:
            performScroll( _win->getRows() - 1 );
            return true;

        case WScrollVertical:
            performScroll( diff );
            return true;
    }

    return false;
}

bool HotSpotList::HLKeyDown(  WKeyCode key, WKeyState state )
//-----------------------------------------------------------
{
    int  nRows = _win->getRows();
    long oldSel = _selected;

    if( state == WKeyStateNone ) {
        switch( key ) {
        case WKeyPageup:
            _selected -= nRows - 1;
            if( _selected < 0 ) _selected = 0;

            if( oldSel != _selected ) {
                _win->invalidateRow( oldSel - _topIndex );
                _win->invalidateRow( _selected - _topIndex );
            }
            scrollToSelected();
            changed();
            return true;

        case WKeyPagedown:
            _selected += nRows - 1;
            if( full() && _selected >= count() ) {
                _selected = count() - 1;
            }

            if( oldSel != _selected ) {
                _win->invalidateRow( oldSel - _topIndex );
                _win->invalidateRow( _selected - _topIndex );
            }
            scrollToSelected();
            changed();
            return true;

        case WKeyUp:
            _selected -= 1;
            if( _selected < 0 ) _selected = 0;

            scrollToSelected();
            changed();

            if( oldSel != _selected ) {
                _win->invalidateRow( oldSel - _topIndex );
                _win->invalidateRow( _selected - _topIndex );
            }
            return true;

        case WKeyDown:
            _selected += 1;
            if( full() && _selected >= count() ) {
                _selected = count() - 1;
            }

            scrollToSelected();
            changed();

            if( oldSel != _selected ) {
                _win->invalidateRow( oldSel - _topIndex );
                _win->invalidateRow( _selected - _topIndex );
            }
            return true;

        case WKeyEnter:
            if( _dblClickClient && _dblClick ) {
                (_dblClickClient->*_dblClick)( _win );
                return true;
            }
            break;

        case WKeySpace:
            if( _hotPressClient && _hotPress ) {
                (_hotPressClient->*_hotPress)( _win );
                return true;
            }
            break;

        default:
            return false;
        }
    }

    return false;
}

// Complain about defining trivial destructor inside class
#pragma disable_message( 657 )

HotSpotList::~HotSpotList()
//-------------------------
{
}

