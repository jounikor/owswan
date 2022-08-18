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
* Description:  Process pbutton tag
*   :pbutton
*       id=[a-zA-z][a-zA-z0-9]*
*       res=[0-9]+
*       text=''
*
****************************************************************************/

#ifndef PBUTTON_INCLUDED
#define PBUTTON_INCLUDED

#include "ctrltag.hpp"

class Controls;

class PButton : public CtrlTag {
public:
    PButton( Document* d, Element* p, const std::wstring* f, unsigned int r, unsigned int c ) :
        CtrlTag( d, p, f, r, c ), _res( 0 ) { };
    ~PButton() { };
    Lexer::Token parse( Lexer* lexer );
    void buildText( Cell* cell ) { (void)cell; };
    void build( Controls* ctrls );
private:
    PButton( const PButton& rhs );              //no copy
    PButton& operator=( const PButton& rhs );   //no assignment

    std::wstring        _id;
    std::wstring        _text;      //button text
    unsigned int        _res;
};

#endif //PBUTTON_INCLUDED
