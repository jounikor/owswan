/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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


#include "wmdiwndw.hpp"
#include "wwindow.hpp"
#include "wmdichld.hpp"

#include "wmenu.hpp"
#include "wmenuitm.hpp"
#include "wpopmenu.hpp"
#include "wmetrics.hpp"


WMdiInitializer::WMdiInitializer() {
/**********************************/

    static bool MDIInitialized = false;

    if( !MDIInitialized ) {
        GUIMDIInitMenuOnly();
        MDIInitialized = true;
    }
}


WEXPORT WMdiWindow::WMdiWindow( const char *text, WStyle style )
    : WWindow( text, style ) {
/****************************/

    getClientRect( _initDefRect );
    _initDefRect.x( 0 );
    _initDefRect.y( 0 );
    _initDefRect.w( ( _initDefRect.w() * 3 ) / 4 );
    _initDefRect.h( ( _initDefRect.h() * 2 ) / 3 );
    _defRect = _initDefRect;
}


WPopupMenu * WEXPORT WMdiWindow::getMdiPopup() {
/**********************************************/

    WPopupMenu *mdipopup = new WPopupMenu( "&Window" );
    mdipopup->insertItem( new WMenuItem( "&Cascade", this, (cbm)&WMdiWindow::cascadeChildren ), 0 );
    mdipopup->setMdiPopup();
    return( mdipopup );
}


void WEXPORT WMdiWindow::cascadeChildren( WMenuItem * ) {
/*******************************************************/

    GUICascadeWindows();
}


WRect WEXPORT WMdiWindow::defaultRectangle() {
/********************************************/

    WRect r( _defRect );
    _defRect.x( _defRect.x() + WSystemMetrics::captionSize() );
    _defRect.y( _defRect.y() + WSystemMetrics::captionSize() );
    if( (_defRect.x() > _initDefRect.w()/3) ||
        (_defRect.y() > _initDefRect.h()/3) ) {
        _defRect = _initDefRect;
    }
    return( r );
}


#ifdef __WATCOMC__
// Complain about defining trivial destructor inside class
#pragma disable_message( 656 )
#endif

WEXPORT WMdiWindow::~WMdiWindow() {
/*********************************/
}

