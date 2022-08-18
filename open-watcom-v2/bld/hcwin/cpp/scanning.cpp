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
* Description:  Scanning of .rtf files.
*
****************************************************************************/


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "scanning.h"
#include "hcerrors.h"

#define BUF_SIZE    4096


//  Token::Token

Token::Token() : _text( BUF_SIZE )
{ }


//  C-tor and D-tor for class Scanner.

Scanner::Scanner( InFile *src )
    : _source( src ),
      _lineNum( 1 ),
      _buffer( BUF_SIZE )
{
    tokens[2] = new Token;
    tokens[1] = new Token;
    tokens[0] = new Token;
    if( !src->bad() ) {
        _maxBuf = src->read( _buffer, BUF_SIZE );
        _curPos = 0;
        getToken( tokens[1] );
        getToken( tokens[2] );
    }
}

Scanner::~Scanner()
{
    delete tokens[0];
    delete tokens[1];
    delete tokens[2];
}


//  Scanner::nextch --Get the next character to be processed.
//            Inlined to save a little speed.

#define     S_ENDC  0xFF

inline int Scanner::nextch()
{
    if( _maxBuf == 0 ) {
        return( S_ENDC );
    }
    if( _curPos == _maxBuf ) {
        _maxBuf = _source->read( _buffer, BUF_SIZE );
        if( _maxBuf == 0 ) {
            return( S_ENDC );
        } else {
            _curPos = 0;
        }
    } else if( _curPos == _maxBuf - 1 ) {
        size_t  newPos = 0;
        _buffer[newPos] = _buffer[_curPos];
        _maxBuf = _source->read( _buffer + 1, BUF_SIZE - 1 ) + 1;
        _curPos = newPos;
    }
    return( _buffer[_curPos++] );
}


//  Scanner::putback    --Unget a character.

void Scanner::putback( int c )
{
    if( _maxBuf > 0 ) {
        _buffer[--_curPos] = static_cast<uint_8>(c);
    }
}


//  Scanner::handleSlash  --Process RTF tokens beginning with a backslash.

TokenTypes Scanner::handleSlash( Token * tok )
{
    TokenTypes  result;
    int         current = nextch();

    if( current == S_ENDC ) {
        HCWarning( RTF_BADEOF, _source->name() );
        result = TOK_END;
    } else if( current == '*' ) {

        // Certain RTF commands begin with "\*\", not "\".

        current = nextch();
        if( current != '\\' ) {
            HCWarning( RTF_BADCOMMAND, _lineNum, _source->name() );
            if( current != S_ENDC ) {
                putback( current );
            }
            result = TOK_NONE;
        } else {
            result = handleSlash( tok );
        }
    } else if( current == '\n' ) {

        // A "\" just before a new-line is the same as "\par".

        memcpy( tok->_text, "par", 4 );
        result = TOK_COMMAND;
        ++_lineNum;
    } else if( isSpecial( current ) ) {

        // Some characters are escaped, like "\{".

        result = TOK_SPEC_CHAR;
        tok->_value = current;
    } else if( current == '\'' ) {

        // "\'nnn" signifies the byte with value nnn.

        result = TOK_SPEC_CHAR;
        pullHex( tok );
    } else if( islower( current ) ) {

        // All RTF commands are in lower case.

        putback( current );
        result = TOK_COMMAND;
        pullCommand( tok );
    } else {
        HCWarning( RTF_BADCOMMAND, _lineNum, _source->name() );
        result = TOK_NONE;
    }
    return( result );
}


//  Scanner::isSpecial  --Check if the argument is a special character.

bool Scanner::isSpecial( int c )
{
    static uint_8 const specials[] = "-:\\_{|}\"";
    unsigned            i;

    for( i = 0; i < sizeof( specials ) - 1; i++ ) {
        if( c == specials[i] ) {
            break;
        }
    }
    return( specials[i] != '\0' );
}


//  Scanner::isFootnoteChar  --Check if c is a "footnote" character.
//               This is a feature specific to the .HLP format.

bool Scanner::isFootnoteChar( int c )
{
    bool result = false;
    switch( c ) {
    case '#':   // Context string
    case '$':   // Title
    case 'K':   // Keywords
    case '+':   // Macros
    case '!':   // Browse Sequence Identifiers
    case '*':   // Build Tags (not supported)
        result = true;
    }
    return( result );
}


//  Scanner::pullCommand   --Read the text of an RTF command.

void Scanner::pullCommand( Token * tok )
{
    int     current = 0;
    char    num_string[7];
    size_t  i = 0;

    tok->_text[i] = static_cast<char>(nextch());

    for( i = 1; i < BUF_SIZE - 1; i++ ) {
        current = nextch();
        if( !islower( current ) )
            break;
        tok->_text[i] = static_cast<char>(current);
    }
    tok->_text[i] = '\0';

    if( current == S_ENDC || (!isdigit(current) && current != '-') ) {
        tok->_hasValue = false;
    } else {
        tok->_hasValue = true;
        for( i = 0; i < sizeof( num_string ) - 1; i++ ) {
            num_string[i] = static_cast<char>(current);
            current = nextch();
            if( !isdigit(current) ) {
                i++;
                break;
            }
        }
        num_string[i] = '\0';
        tok->_value = atoi( num_string );
    }
    if( current != S_ENDC && current != ' ' ) {
        putback( current );
    }
}


//  Scanner::pullText   --Pull a block of plain text from a .RTF file.

#define HARD_SPACE  0xA0

void Scanner::pullText( Token * tok )
{

    size_t  i = 0;
    int     current;
    tok->_text[i] = static_cast<char>(nextch());

    current = 0;
    for( i = 1; i < BUF_SIZE - 1; ) {
        current = nextch();

        if( current == S_ENDC
          || current == '{'
          || current == '}'
          || isFootnoteChar( current ) )
            break;

        if( current == '\\' ) {
            if( _curPos < _maxBuf && _buffer[_curPos] == '~' ) {
                nextch();
                current = HARD_SPACE;
            } else {
                break;
            }
        }

        if( current == '\n' ) {
            ++_lineNum;
            continue;
        }
        tok->_text[i++] = static_cast<char>(current);
    }
    tok->_text[i] = '\0';
    tok->_value = (int)i;
    if( current != S_ENDC && i < BUF_SIZE - 1 ) {
        putback( current );
    }
}


//  Scanner::pullHex    --Get the hex value of bytes specified by "\'nnn".

void Scanner::pullHex( Token * tok )
{
    char    result[3];
    int     current = 0;
    int     i;

    for( i = 0; i < 2; ++i ) {
        current = nextch();
        if( !isxdigit( current ) ) {
            break;
        }
        result[i] = static_cast<char>(current);
    }
    result[i] = '\0';
    if( i < 2 && current != S_ENDC ) {
        putback( current );
    }
    if( i == 0 ) {
        tok->_type = TOK_NONE;
    } else {
        tok->_hasValue = true;
        tok->_value = strtol( result, NULL, 16 );
    }
}


//  Scanner::getToken   --Get the next token from the input stream.

void Scanner::getToken( Token * tok )
{
    int     current;

    while( (current = nextch()) == '\n' ) {
        ++_lineNum;
    }

    tok->_lineNum = _lineNum;
    switch( current ) {
    case S_ENDC:
        tok->_type = TOK_END;
        break;

    case '{':
        tok->_type = TOK_PUSH_STATE;
        break;

    case '}':
        tok->_type = TOK_POP_STATE;
        break;

    case '\\':
        current = nextch();
        if( current == '~' ) {
            tok->_type = TOK_TEXT;
            putback( HARD_SPACE );
            pullText( tok );
        } else {
            putback( current );
            tok->_type = handleSlash( tok );
        }
        break;

    case '\t':
        tok->_type = TOK_COMMAND;
        memcpy( tok->_text, "tab", 4 );
        break;

    default:
        if( isFootnoteChar( current ) ) {
            tok->_type = TOK_SPEC_CHAR;
            tok->_value = current;
        } else {
            tok->_type = TOK_TEXT;
            putback( current );
            pullText( tok );
        }
    }
}


//  Scanner::next   --Return the next token in the lookahead buffer.

Token   *Scanner::next()
{
    Token   *temp = tokens[0];

    tokens[0] = tokens[1];
    tokens[1] = tokens[2];
    tokens[2] = temp;

    getToken( tokens[2] );
    return( tokens[0] );
}
