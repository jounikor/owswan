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
* Description:  Process ul/eul tags
*
*   :ul / :eul
*       compact (else blank line between items)
*       verycompact (no extra lines at all; a wipfc extension)
*   If nested, indent 4 spaces
****************************************************************************/


#include "wipfc.hpp"
#include "ul.hpp"
#include "brcmd.hpp"
#include "dl.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "lm.hpp"
#include "lp.hpp"
#include "ol.hpp"
#include "p.hpp"
#include "parml.hpp"
#include "punct.hpp"
#include "sl.hpp"
#include "whtspc.hpp"
#include "word.hpp"

Lexer::Token Ul::parse( Lexer* lexer )
{
    Lexer::Token tok( parseAttributes( lexer ) );
    unsigned int itemCount( 0 );
    bool needLine( true );
    while( tok != Lexer::END && !( tok == Lexer::TAG && lexer->tagId() == Lexer::EUSERDOC ) ) {
        if( parseInline( lexer, tok ) ) {
            switch( lexer->tagId() ) {
            case Lexer::DL:
                {
                    Dl *dl = new Dl( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(), _nestLevel + 1,
                        _indent == 1 ? 4 : _indent + 3 );
                    appendChild( dl );
                    tok = dl->parse( lexer );
                    needLine = true;
                }
                break;
            case Lexer::OL:
                {
                    Ol *ol = new Ol( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(),
                        0, _indent == 1 ? 4 : _indent + 3 );
                    appendChild( ol );
                    tok = ol->parse( lexer );
                    needLine = true;
                }
                break;
            case Lexer::LI:
                {
                    UlLi *ulli = new UlLi( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(),
                        itemCount++, _nestLevel, _indent, _veryCompact ||
                        ( _compact && !needLine ) );
                    appendChild( ulli );
                    tok = ulli->parse( lexer );
                    needLine = false;
                }
                break;
            case Lexer::LP:
                {
                    Lp *lp = new Lp( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(), _indent + 2 );
                    appendChild( lp );
                    tok = lp->parse( lexer );
                }
                break;
            case Lexer::PARML:
                {
                    Parml *parml = new Parml( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(), _nestLevel + 1,
                        _indent == 1 ? 4 : _indent + 3 );
                    appendChild( parml );
                    tok = parml->parse( lexer );
                    needLine = true;
                }
                break;
            case Lexer::SL:
                {
                    Sl *sl = new Sl( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(),
                        0, _indent == 1 ? 4 : _indent + 3 );
                    appendChild( sl );
                    tok = sl->parse( lexer );
                    needLine = true;
                }
                break;
            case Lexer::UL:
                {
                    Ul *ul = new Ul( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol(),
                        _nestLevel + 1, _indent == 1 ? 4 : _indent + 3 );
                    appendChild( ul );
                    tok = ul->parse( lexer );
                    needLine = true;
                }
                break;
            case Lexer::EUL:
                {
                    EUl *eul = new EUl( _document, this, _document->dataName(),
                        _document->dataLine(), _document->dataCol() );
                    appendChild( eul );
                    tok = eul->parse( lexer );
                    if( !_nestLevel ) {
                        appendChild( new BrCmd( _document, this, _document->dataName(),
                            _document->dataLine(), _document->dataCol() ) );
                    }
                    return tok;
                }
            default:
                _document->printError( ERR1_NOENDLIST );
                return tok;
            }
        }
    }
    return tok;
}
/***************************************************************************/
Lexer::Token Ul::parseAttributes( Lexer* lexer )
{
    Lexer::Token tok;

    (void)lexer;

    while( (tok = _document->getNextToken()) != Lexer::TAGEND ) {
        if( tok == Lexer::ATTRIBUTE ) {
            _document->printError( ERR1_ATTRNOTDEF );
        } else if( tok == Lexer::FLAG ) {
            if( lexer->text() == L"compact" ) {
                _compact = true;
            } else if( lexer->text() == L"verycompact" ) {
                _veryCompact = true;
            } else {
                _document->printError( ERR1_ATTRNOTDEF );
            }
        } else if( tok == Lexer::ERROR_TAG ) {
            throw FatalError( ERR_SYNTAX );
        } else if( tok == Lexer::END ) {
            throw FatalError( ERR_EOF );
        } else {
            _document->printError( ERR1_TAGSYNTAX );
        }
    }
    return _document->getNextToken();    //consume TAGEND
}
/***************************************************************************/
void EUl::buildText( Cell* cell )
{
    cell->addByte( Cell::ESCAPE );  //esc
    cell->addByte( 0x03 );          //size
    cell->addByte( 0x02 );          //set left margin
    cell->addByte( 1 );
    if( cell->textFull() ) {
        printError( ERR1_LARGEPAGE );
    }
}
/***************************************************************************/
Lexer::Token UlLi::parse( Lexer* lexer )
{
    Lexer::Token tok( parseAttributes( lexer ) );
    appendChild( new Lm( _document, this, _document->dataName(), _document->dataLine(),
        _document->dataCol(), _indent ) );
    if( _compact ) {
        appendChild( new BrCmd( _document, this, _document->dataName(), _document->dataLine(),
            _document->dataCol() ) );
    } else {
        appendChild( new P( _document, this, _document->dataName(), _document->dataLine(),
            _document->dataCol() ) );
    }
    if( _document->ulBullet( _nestLevel ).size() == 1 ) {
        appendChild( new LiteralWhiteSpace( _document, this, _document->dataName(),
            _document->dataLine(), _document->dataCol(), true ) );
    }
    appendChild( new TextWord( _document, this, _document->dataName(),
        _document->dataLine(), _document->dataCol(), _document->ulBullet( _nestLevel ),
        _document->ulBullet( _nestLevel ).size() == 1 ) );
    appendChild( new Lm( _document, this, _document->dataName(), _document->dataLine(),
        _document->dataCol(), _indent + 2 ) );
    while( tok != Lexer::END && !( tok == Lexer::TAG && lexer->tagId() == Lexer::EUSERDOC ) ) {
        if( parseInline( lexer, tok ) ) {
            if( lexer->tagId() == Lexer::LI ) {
                break;
            } else if( lexer->tagId() == Lexer::LP ) {
                P *p = new P( _document, this, _document->dataName(),
                    _document->dataLine(), _document->dataCol() );
                appendChild( p );
                tok = p->parse( lexer );
            } else if( parseBlock( lexer, tok ) ) {
                break;
            }
        }
    }
    return tok;
}

