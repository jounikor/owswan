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


#ifndef wcontrol_class
#define wcontrol_class

#include "wwindow.hpp"
#include "wrect.hpp"


WCLASS WStatDialog;

WCLASS WControl : public WWindow {

    friend class WStatDialog;

    public:
        WEXPORT WControl( WWindow* parent, gui_control_class control_class,
                          const WRect& r, const char *text, WStyle wstyle );
        WEXPORT WControl( WStatDialog* parent, WControlId id, WStyle wstyle );
        WEXPORT ~WControl();

        WControlId controlId() { return( _id ); }
        WStyle   style() { return( _style ); }

        virtual bool processMsg( gui_event ) { return( false ); }
        virtual bool WEXPORT processMsg( gui_event gui_ev, void *parm ) {
            return( WWindow::processMsg( gui_ev, parm ) );
        };
        virtual void WEXPORT autosize();
        virtual void WEXPORT getText( char* buff, size_t len );
        virtual void WEXPORT getText( WString& str );
        virtual size_t WEXPORT getTextLength( void );
        virtual void WEXPORT setText( const char *text );
        virtual void WEXPORT setUpdates( bool start_update=true );
        virtual void WEXPORT show( WWindowState state=WWinStateShow );
        virtual bool WEXPORT isVisible() { return( GUIIsControlVisible( parent()->handle(), controlId() ) ); }
        virtual bool WEXPORT isEnabled();
        virtual void WEXPORT enable( bool state );
        virtual void WEXPORT getRectangle( WRect& r, bool absolute=false );
        virtual bool WEXPORT isHidden( void );
        virtual bool WEXPORT setFocus( void );
        virtual int WEXPORT getTextExtentX( const char *text, size_t len ) {
            return( GUIGetControlExtentX( parent()->handle(), controlId(), text, len ) );
        }
        virtual int WEXPORT getTextExtentX( const char *text ) {
            return( GUIGetControlExtentX( parent()->handle(), controlId(), text, strlen( text ) ) );
        }
        virtual int WEXPORT getTextExtentY( const char *text ) {
            return( GUIGetControlExtentY( parent()->handle(), controlId(), text ) );
        }
        virtual void WEXPORT textMetrics( WPoint &, WPoint & );

    protected:
        virtual gui_control_class controlClass() = 0;

    private:
        WControlId          _id;
        WStyle              _style;
};

#endif
