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
* Description:  Process fig/efig tags
*
****************************************************************************
*   :fig / :efig
*   Insert blank line before contents
*   Proportional font
*   Honor whitespace (as literal whitespace), including '\n' (as .br)
****************************************************************************/


#include "wipfc.hpp"
#include "fig.hpp"
#include "brcmd.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "figcap.hpp"
#include "p.hpp"
#include "page.hpp"


Lexer::Token Fig::parse( Lexer* lexer )
{
    Lexer::Token tok( parseAttributes( lexer ) );
    if( tok == Lexer::WHITESPACE && lexer->text()[0] == L'\n' )
        tok = _document->getNextToken(); //consume '\n' if just after tag end
    while( tok != Lexer::END && !( tok == Lexer::TAG && lexer->tagId() == Lexer::EUSERDOC)) {
        if( parseInline( lexer, tok ) ) {
            if( lexer->tagId() == Lexer::EFIG ) {
                break;
            } else if( lexer->tagId() == Lexer::FIGCAP ) {
                Figcap *figcap = new Figcap( _document, this, _document->dataName(),
                    _document->dataLine(), _document->dataCol() );
                appendChild( figcap );
                tok = figcap->parse( lexer );
            } else {
                parseCleanup( lexer, tok );
            }
        }
    }
    return tok;
}
/*****************************************************************************/
void Fig::buildText( Cell* cell )
{
    cell->addByte( Cell::TOGGLE_SPACING );  //toggle spacing
    cell->addByte( Cell::ESCAPE );          //esc
    cell->addByte( 0x03 );                  //size
    cell->addByte( 0x1A );                  //begin fig sequence
    cell->addByte( 0x01 );                  //left align
    cell->addByte( Cell::TOGGLE_SPACING );  //toggle spacing
    if( cell->textFull() ) {
        printError( ERR1_LARGEPAGE );
    }
}
/*****************************************************************************/
void EFig::buildText( Cell* cell )
{
    cell->addByte( Cell::ESCAPE );  //esc
    cell->addByte( 0x02 );          //size
    cell->addByte( 0x1B );          //end fig sequence
    if( cell->textFull() ) {
        printError( ERR1_LARGEPAGE );
    }
}
