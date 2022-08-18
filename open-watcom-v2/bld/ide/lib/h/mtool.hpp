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


#ifndef mtool_class
#define mtool_class

#include "wstring.hpp"
#include "wpicklst.hpp"
#include "wtokfile.hpp"
#include "wstrmap.hpp"

WCLASS MFamily;
WCLASS MSwitch;
WCLASS MTool : public WObject
{
    Declare( MTool )
    public:
        MTool( const char* name, const char* tag );
        MTool( WTokenFile& fil, WString& tok );
        ~MTool();
        int compare( const WObject* o ) const { return( _tag.compare( &((MTool*)o)->_tag ) ); }

        WString& tag() { return( _tag ); };
        void name( WString& s ) { s = _name; }
        const WString& help() { return( _help ); }
        MSwitch* findSwitch( WString& switchtag, long fixed_version=0 );
        WString *displayText( MSwitch *sw, WString& text, bool first=true );
#if CUR_CFG_VERSION > 4
        WString* findSwitchByText( WString& id, WString& text, int kludge=0 );
#endif
        bool hasSwitches( bool setable );
        void addSwitches( WVList& list, const char* mask, bool setable );
        void addFamilies( WVList& list );
    private:
        WString         _tag;
        WString         _name;
        WString         _help;
        WPickList       _families;
        WVList          _incTools;
#if CUR_CFG_VERSION > 4
        WStringMap      _switchesTexts; //<WString>
        WStringMap      _switchesIds;   //<WString>
#endif
};

#endif
