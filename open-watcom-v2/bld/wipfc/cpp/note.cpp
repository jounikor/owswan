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
* Description:  Process note tag
*
*   :note
*       text=[a-zA-z0-9]* (in place of default from nls file: "Note: ")
*   Blank line before contents, contents follows 'Note: ' on line
*   Contents ends with next block-level tag
*   If child of list, aligns with list item
*
****************************************************************************/


#include "wipfc.hpp"
#include "note.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "ipfbuff.hpp"
#include "p.hpp"
#include "page.hpp"
#include "util.hpp"
#include "whtspc.hpp"

Lexer::Token Note::parse( Lexer* lexer )
{
    std::wstring temp;
    std::wstring* fname( new std::wstring() );
    prepBufferName( fname, *( _document->dataName() ) );
    fname = _document->addFileName( fname );
    Lexer::Token tok;

    while( (tok = _document->getNextToken()) != Lexer::TAGEND ) {
        if( tok == Lexer::ATTRIBUTE ) {
            std::wstring key;
            std::wstring value;
            splitAttribute( lexer->text(), key, value );
            if( key == L"text" ) {
                temp = L":hp2.";
                temp += value;
                temp += L":ehp2.";
            } else {
                _document->printError( ERR1_ATTRNOTDEF );
            }
        } else if( tok == Lexer::FLAG ) {
            _document->printError( ERR1_ATTRNOTDEF );
        } else if( tok == Lexer::ERROR_TAG ) {
            throw FatalError( ERR_SYNTAX );
        } else if( tok == Lexer::END ) {
            throw FatalError( ERR_EOF );
        } else {
            _document->printError( ERR1_TAGSYNTAX );
        }
    }
    if( temp.empty() ) {
        temp = L":hp2.";
        temp += _document->note();
        temp += L":ehp2.";
    }
    _document->pushInput( new IpfBuffer( fname, _document->lexerLine(),
        _document->lexerCol(), temp ) );
    bool oldBlockParsing( _document->blockParsing() );
    _document->setBlockParsing( true );
    _whiteSpace = Tag::LITERAL;
    appendChild( new P( _document, this, _document->dataName(), _document->lexerLine(),
        _document->lexerCol() ) );
    tok = _document->getNextToken(); //first token from buffer
    while( tok != Lexer::END ) {
        if( parseInline( lexer, tok ) ) {
            parseCleanup( lexer, tok );
        }
    }
    _whiteSpace = Tag::NONE;
    _document->setBlockParsing( oldBlockParsing );
    _document->popInput();
    appendChild( new WhiteSpace( _document, this, _document->dataName(), _document->lexerLine(),
        _document->lexerCol(), L"  ", Tag::LITERAL, false ) );
    tok = _document->getNextToken(); //next token from main stream
    while( tok != Lexer::END && !( tok == Lexer::TAG && lexer->tagId() == Lexer::EUSERDOC)) {
        if( parseInline( lexer, tok ) ) {
            break;
        }
    }
    return tok;
}
/*****************************************************************************/
void Note::prepBufferName( std::wstring* buffer, const std::wstring& fname )
{
    buffer->assign( L"Note text from " );
    buffer->append( fname );
}

