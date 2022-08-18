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
* Description:  Process xmp/exmp tags
*
*   :xmp / :exmp
*   Honors white space (as in <pre>), monospaced, indented 2 spaces left
*   Cannot nest
*
****************************************************************************/

#ifndef XMP_INCLUDED
#define XMP_INCLUDED

#include "tag.hpp"

class Xmp : public Tag {
public:
    Xmp( Document* d, Element *p, const std::wstring* f, unsigned int r,
        unsigned int c ) : Tag( d, p, f, r, c, Tag::SPACES ) { };
    ~Xmp() { };
    Lexer::Token parse( Lexer* lexer );
    void buildText( Cell* cell );
private:
    Xmp( const Xmp& rhs );              //no copy
    Xmp& operator=( const Xmp& rhs );   //no assignment
};

class EXmp : public Tag {
public:
    EXmp( Document* d, Element *p, const std::wstring* f, unsigned int r,
        unsigned int c ) : Tag( d, p, f, r, c ) { };
    ~EXmp() { };
    void buildText( Cell* cell );
private:
    EXmp( const EXmp& rhs );            //no copy
    EXmp& operator=( const EXmp& rhs ); //no assignment
};

#endif //XMP_INCLUDED
