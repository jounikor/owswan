/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2009-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Process i2 tags
*
*   :i2
*       refid=[a-zA-z][a-zA-z0-9]*
*       global
*       sortkey='key-text'.index-text
*   Must follow :i1
*
****************************************************************************/

#ifndef I2_INCLUDED
#define I2_INCLUDED

#include <cstdio>
#include <memory>
#include "element.hpp"
#include "index.hpp"

class GlobalDictionaryWord; //forward reference

class I2 : public Element {
public:
    I2( Document* d, Element* p, const std::wstring* f, unsigned int r, unsigned int c ) :
        Element( d, p, f, r, c ), _index( new IndexItem( IndexItem::SECONDARY ) ),
        _parentId( 0 ), _parentRes( 0 ) { }
    ~I2() { };
    Lexer::Token parse( Lexer* lexer );
    void buildIndex();
    void buildText( Cell* cell ) { (void)cell; };
    void setRes( word r ) { _parentRes = r; };
    void setIdOrName( GlobalDictionaryWord* w ) { _parentId = w; };
    bool isGlobal() const { return _index->isGlobal(); };
    dword write( OutFile* out ) { return _index->write( out ); };
private:
    I2( const I2& rhs );                //no copy
    I2& operator=( const I2& rhs );     //no assignment
    Lexer::Token parseAttributes( Lexer* lexer );

    std::auto_ptr< IndexItem >  _index;
    std::wstring                _refid;
    GlobalDictionaryWord*       _parentId;
    word                        _parentRes;
};
#endif //I2_INCLUDED
