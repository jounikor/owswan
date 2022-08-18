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


#include "wtoolbar.hpp"
#include "wbaritem.hpp"
#include "wwindow.hpp"


WEXPORT WToolBar::WToolBar( bool fixed )
    : _parent( NULL )
    , _changedClient( NULL )
    , _changed( NULL )
    , _rect( 0, 0, 0, 0 )
/****************************/
{
    if( fixed ) {
        _flags = WToolFlagsFixed;
    }
}


WEXPORT WToolBar::WToolBar( bool fixed, bool use_tips )
    : _parent( NULL )
    , _changedClient( NULL )
    , _changed( NULL )
    , _rect( 0, 0, 0, 0 )
/****************************/
{
    _flags = 0;
    if( fixed ) {
        _flags |= WToolFlagsFixed;
    }
    if( use_tips ) {
        _flags |= WToolFlagsUseTips;
    }
}


WEXPORT WToolBar::WToolBar( WRect r )
    : _parent( NULL )
    , _changedClient( NULL )
    , _changed( NULL )
    , _rect( r )
/****************************/
{
    _flags = WToolFlagsUseRect;
}


WEXPORT WToolBar::~WToolBar() {
/*****************************/

    detach();
    while( _children.count() ) {
        delete _children[_children.count() - 1];
    }
    if( _toolBarItems != NULL ) {
        delete[] _toolBarItems;
    }
}


void WEXPORT WToolBar::addTool( WToolBarItem *tool ) {
/****************************************************/

    _children.add( tool );
    tool->setParent( this );
}


void WEXPORT WToolBar::removeTool( WToolBarItem *tool ) {
/*******************************************************/

    _children.removeSame( tool );
}


void WToolBar::attach( WWindow *win ) {
/*************************************/

    bool rc;
    gui_toolbar_items toolinfo;
    int num_tools = _children.count();

    _toolBarItems = new gui_toolbar_struct[num_tools];
    toolinfo.toolbar = _toolBarItems;
    toolinfo.num_items = 0;
    if( _toolBarItems != NULL ) {
        toolinfo.num_items = num_tools;
        for( int i = 0; i < num_tools; ++i ) {
            _toolBarItems[i] = *(*(WToolBarItem *)_children[i]).toolBarInfo();
        }
        bool fixed = (_flags & WToolFlagsFixed) != 0;
        gui_rect trect;
        if( _flags & WToolFlagsUseRect ) {
            trect.x = _rect.x();
            trect.y = _rect.y();
            trect.width = _rect.w();
            trect.height = _rect.h();
            rc = GUICreateFloatToolBar( win->handle(), fixed, 0, &toolinfo, false, NULL, NULL, &trect );
        } else if( _flags & WToolFlagsUseTips ) {
            rc = GUICreateToolBarWithTips( win->handle(), fixed, 0, &toolinfo, false, NULL, NULL );
        } else {
            rc = GUICreateToolBar( win->handle(), fixed, 0, &toolinfo, false, NULL, NULL );
        }
        if( rc ) {
            _parent = win;
        } else {
            delete[] _toolBarItems;
            _toolBarItems = NULL;
        }
    }
}


void WToolBar::detach() {
/***********************/

    if( _parent ) {
        // Check the handle in case WWindow::close() is called.
        // Calling WWindow::close() destroys the window before
        // the destructor for the tool bar is invoked.
        if( _parent->handle() != NULL ) {
            GUICloseToolBar( _parent->handle() );
            _parent = NULL;
        }
    }
}


void WEXPORT WToolBar::onChanged( WObject *client, cbtb tb ) {
/************************************************************/

    _changedClient = client;
    _changed = tb;
}


bool WEXPORT WToolBar::changed( WToolBarState state ) {
/*****************************************************/

    if( _changedClient && _changed ) {
        return( (_changedClient->*_changed)( this, state ) );
    }
    return( false );
}
