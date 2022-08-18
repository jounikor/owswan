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


#include "wcombox.hpp"
#include "wwindow.hpp"


//      Define Combo Box
//      ================

WEXPORT WComboBox::WComboBox( WWindow* parent, const WRect& r,
                              const char* text, WStyle style )
        : WCombo( parent, GUI_COMBOBOX, r, text, style ) {
/********************************************************/

}

WEXPORT WComboBox::WComboBox( WStatDialog* parent,
                              WControlId id, WStyle style )
        : WCombo( parent, id, style ) {
/*************************************/

}


//      Define Edit Combo Box
//      =====================

WEXPORT WEditComboBox::WEditComboBox( WWindow* parent, const WRect& r,
                                      const char* text, WStyle style )
        : WCombo( parent, GUI_EDIT_COMBOBOX, r, text, style ) {
/*************************************************************/

}

WEXPORT WEditComboBox::WEditComboBox( WStatDialog* parent, WControlId id,
                                      WStyle style )
        : WCombo( parent, id, style ) {
/*************************************/

}


//      Define General Combo Box
//      ========================

WEXPORT WCombo::WCombo( WWindow* parent, gui_control_class control_class,
                        const WRect& r, const char* text, WStyle style )
        : WControl( parent, control_class, r, text, style )
        , _changedClient( NULL )
        , _changed( NULL )
        , _dblClickClient( NULL )
        , _dblClick( NULL ) {
/***************************/

}

WEXPORT WCombo::WCombo( WStatDialog *parent, WControlId id, WStyle style )
        : WControl( parent, id, style )
        , _changedClient( NULL )
        , _changed( NULL )
        , _dblClickClient( NULL )
        , _dblClick( NULL ) {
/***************************/

}


bool WCombo::gettingFocus( WWindow* ) {
/*************************************/

    if( count() > 0  && selected() < 0 ) {
        select( 0 );
    }
    return( false );
}


void WEXPORT WCombo::onChanged( WObject* client, cbw changed ) {
/**************************************************************/

    _changedClient = client;
    _changed = changed;
}


void WEXPORT WCombo::onDblClick( WObject* client, cbw click ) {
/*************************************************************/

    _dblClickClient = client;
    _dblClick = click;
}


bool WCombo::processMsg( gui_event gui_ev )
/*****************************************/
{
    switch( gui_ev ) {
    case GUI_CONTROL_CLICKED:
        if( _changedClient && _changed ) {
            (_changedClient->*_changed)( this );
            return( true );
        }
        break;
    case GUI_CONTROL_DCLICKED:
        if( _dblClickClient && _dblClick ) {
            (_dblClickClient->*_dblClick)( this );
            return( true );
        }
        break;
    }
    return( false );
}


void WEXPORT WCombo::getString( int index, WString& str ) {
/*********************************************************/

    char        *list_item;

    list_item = GUIGetListItem( parent()->handle(), controlId(), index );
    WString s( list_item );
    GUIMemFree( list_item );
    str = s;
}


int WEXPORT WCombo::insertString( const char *s, int index ) {
/************************************************************/

    int         newIndex;

    if( index < 0 ) {
        GUIAddText( parent()->handle(), controlId(), s );
        newIndex = count() - 1;
    } else {
        GUIInsertText( parent()->handle(), controlId(), index, s );
        newIndex = index;
    }
    if( count() == 1 ) {
        select( 0 );
    }
    return( newIndex );
}


void WEXPORT WCombo::deleteString( int index ) {
/**********************************************/

    GUIDeleteItem( parent()->handle(), controlId(), index );
}


void WEXPORT WCombo::reset() {
/****************************/

    GUIClearList( parent()->handle(), controlId() );
}


int WEXPORT WCombo::count() {
/***************************/

    return( GUIGetListSize( parent()->handle(), controlId() ) );
}


int WEXPORT WCombo::selected()
/****************************/
{
    int     choice = -1;

    GUIGetCurrSelect( parent()->handle(), controlId(), &choice );
    return( choice );
}


void WEXPORT WCombo::select( int index ) {
/****************************************/

    GUISetCurrSelect( parent()->handle(), controlId(), index );
    if( _changedClient && _changed ) {
        (_changedClient->*_changed)( this );
    }
}


void WEXPORT WCombo::setTagPtr( int index, void *tagPtr ) {
/*********************************************************/

    GUISetListItemData( parent()->handle(), controlId(), index, tagPtr );
}


void * WEXPORT WCombo::tagPtr( int index ) {
/******************************************/

    return( GUIGetListItemData( parent()->handle(), controlId(), index ) );
}

#ifdef __WATCOMC__
// Complain about defining trivial destructor inside class
#pragma disable_message( 656 )
#pragma disable_message( 657 )
#endif

WEXPORT WComboBox::~WComboBox() {
/*******************************/
}

WEXPORT WEditComboBox::~WEditComboBox() {
/***************************************/
}

WEXPORT WCombo::~WCombo() {
/*************************/
}
