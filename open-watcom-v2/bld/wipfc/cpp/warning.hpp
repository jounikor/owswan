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
* Description:  Process warning/ewarning tags
*
*   :warning / :ewarning
*       text='' (in place of data from nls file)
*   Contents follows text
*
****************************************************************************/

#ifndef WARNING_INCLUDED
#define WARNING_INCLUDED

#include "tag.hpp"

class Warning : public Tag {
public:
    Warning( Document* d, Element *p, const std::wstring* f, unsigned int r, unsigned int c ) :
        Tag( d, p, f, r, c ) { };
    ~Warning() { };
    Lexer::Token parse( Lexer* lexer );
    void linearize( Page* page ) { linearizeChildren( page ); };
    void buildText( Cell* cell ) { (void)cell; };
private:
    Warning( const Warning& rhs );              //no copy
    Warning& operator=( const Warning& rhs );   //no assignment
    static void prepBufferName( std::wstring* buffer, const std::wstring& fname );
};

class EWarning : public Tag {
public:
    EWarning( Document* d, Element *p, const std::wstring* f, unsigned int r, unsigned int c ) :
        Tag( d, p, f, r, c ) { };
    ~EWarning() { };
    void buildText( Cell* cell );
private:
    EWarning( const EWarning& rhs );            //no copy
    EWarning& operator=( const EWarning& rhs ); //no assignment
};

#endif //WARNING_INCLUDED
