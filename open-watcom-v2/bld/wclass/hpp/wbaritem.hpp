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


#ifndef wbaritem_class
#define wbaritem_class

#include "_windows.hpp"
#include "wobject.hpp"

WCLASS WToolBar;
WCLASS WToolBarItem;

typedef void (WObject::*cbtbi)( WToolBarItem *m );

WCLASS WToolBarItem : public WObject {
    public:
        WEXPORT WToolBarItem( const char *text, WResourceId tool_id, WObject *client,
                              cbtbi pick, const char *htext = NULL,
                              const char *tip = NULL );
        WEXPORT ~WToolBarItem();

        void WEXPORT setParent( WToolBar *parent ) { _parent = parent; }
        WToolBar * WEXPORT parent() { return( _parent ); }
        virtual void WEXPORT picked();
        gui_toolbar_struct * toolBarInfo( void ) { return( &_toolbar ); }
        void setTagPtr( void *tagPtr ) { _tagPtr = tagPtr; }
        void *tagPtr() { return( _tagPtr ); }

    private:
        WToolBar                *_parent;
        WObject                 *_client;
        void                    *_tagPtr;
        cbtbi                   _pick;
        gui_toolbar_struct      _toolbar;
};

#endif
