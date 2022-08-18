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


#ifndef mtoolitem_class
#define mtoolitem_class

#include "wtokfile.hpp"

typedef char ToolType;
#define TOOL_UNDEFINED          0
#define TOOL_MENU               1
#define TOOL_ITEM_ACTION        2
#define TOOL_TARGET_ACTION      3
#define TOOL_PROJECT_ACTION     4

WCLASS MToolItem : public WObject
{
    Declare( MToolItem )
    public:
        MToolItem() {}
        MToolItem( WTokenFile& fil, WString& tok );
        ~MToolItem() {}
        int toolId() { return( _toolId ); }
        ToolType toolType() { return( _toolType ); }
        const WString& actionName() { return( _actionName ); }
        const WString& hint() { return( _hint ); }
        const WString& tip() { return( _tip ); }
    private:
        int             _toolId;
        ToolType        _toolType;
        WString         _actionName;
        WString         _hint;
        WString         _tip;
};

#endif
