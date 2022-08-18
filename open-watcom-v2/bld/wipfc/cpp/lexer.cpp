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
* Description:  the IPF lexer implementation
*
****************************************************************************/


#include "wipfc.hpp"
#include <cstdio>
#include <cwctype>
#include "lexer.hpp"
#include "errors.hpp"
#include "ipfdata.hpp"

Lexer::Lexer() : _charNum( 0 ), _lineNum( 0 ), _tagCode( BADTAG ), _cmdCode( BADCMD ),
    _inTag( false )
{
    _buffer.reserve( 256 );
    #undef PICK
    #define PICK(a,b) _tagIdMap.insert( std::map< std::wstring, TagId >::value_type( b, a ) );
    #include "tags.hpp"
}
/*****************************************************************************/
// Parsing relies on the std:isw* family of functions doing the "right thing"
// Otherwise, we'd use the SBCS/DBCS tables from the *.nls files as parsed in
// the nls object
Lexer::Token Lexer::lex( IpfData* input )
{
    Token type( END );
    if( input ) {
        wchar_t ch;
        wchar_t quoteChar = L'\0';
        bool inQuote( false );
        _buffer.clear();
        _charNum = input->currentCol();
        _lineNum = input->currentLine();
        while( ( ch = input->get() ) != EOB ) {
            _buffer.push_back( ch );
            if( _inTag ) {
                if( type == END ) { //first char after tag name
                    if( ch == L'.' ) {
                        type = TAGEND;
                        _inTag = false;
                        break;
                    } else if( std::iswspace( ch ) ) {  //ignore leading whitespace
                        _buffer.erase( _buffer.size() - 1 );
                    } else if( std::iswalpha( ch ) ) {
                        type = FLAG;    //tentatively
                        inQuote = false;
                    }
                } else {
                    if( !inQuote && ch == L'.' ) {
                        input->unget( ch );
                        _buffer.erase( _buffer.size() - 1 );
                        break;
                    } else if( !inQuote && ch == L':' ) {
                        //syntax error
                        type = ERROR_TAG;
                        break;
                    } else if( type == FLAG ) {
                        if( ch == L'=' ) {
                            type = ATTRIBUTE;
                        } else if( std::iswspace( ch ) ) {
                            _buffer.erase( _buffer.size() - 1 );
                            break;
                        }
                    } else if( type == ATTRIBUTE ) {
                        if( ch == L'\'' || ch == '\"' ) {
                            if( !inQuote ) {
                                inQuote = true;
                                quoteChar = ch;
                            } else if( ch == quoteChar ) {
                                inQuote = false;
                            }
                        } else if( !inQuote && std::iswspace( ch ) ) {
                            _buffer.erase( _buffer.size() - 1 );
                            break;
                        }
                    } else if( std::iswspace( ch ) ) {
                        //ignore trailing space
                        _buffer.erase( _buffer.size() - 1 );
                        break;
                    }
                }
            } else {
                if( type == END ) {
                    //first character of token
                    if( std::iswspace( ch ) ) {
                        type = WHITESPACE;
                        if( ch == L'\n' )   //don't concatenate spaces
                            break;
                    } else if( ch == L':' ) {
                        wchar_t ch2( input->get() );
                        input->unget( ch2 );
                        if( std::iswalpha( ch2 ) ) {
                            type = TAG;
                            _inTag = true;
                        } else {
                            type = PUNCTUATION;
                            break;
                        }
                    } else if( ch == L'&' ) {
                        wchar_t ch2( input->get() );
                        input->unget( ch2 );
                        if( std::iswalnum( ch2 ) ) {
                            type = ENTITY;
                        } else {
                            type = WORD;
                        }
                    } else if( ch == L'.' ) {
                        if( _charNum == 1 ) {
                            type = COMMAND;
                        } else {
                            type = PUNCTUATION;
                            break;
                        }
                    } else if( std::iswpunct( ch ) ) {
                        //single character, but not '.' or '&' or ':'
                        type = PUNCTUATION;
                        break;
                    } else {    //if( std::iswalnum( ch ) )
                        type = WORD;
                    }
                } else {
                    if( type == COMMAND ) {
                        if( ch == L'\n' ) {
                            break;
                        }
                    } else if( ch == L':' || ch == L'&' ) {
                        //beginning of another token
                        input->unget( ch );
                        _buffer.erase( _buffer.size() - 1 );
                        if( type == ENTITY )
                            type = ERROR_ENTITY;    //'.' not found
                        break;
                    } else if( type == ENTITY ) {
                        if( ch == L'.' )
                            //end of entity
                            break;
                        if( !std::iswalnum( ch ) ) {
                            //non-fatal malformed entity
                            input->unget( ch );
                            _buffer.erase( _buffer.size() - 1 );
                            type = ERROR_ENTITY;
                            break;
                        }
                    } else if( type == WHITESPACE &&
                           ( !std::iswspace( ch ) || ch == L'\n' ) ) {
                        //end of whitespace
                        //don't concatenate \n's
                        input->unget( ch );
                        _buffer.erase( _buffer.size() - 1 );
                        break;
                    } else if( type == WORD &&
                        ( std::iswspace( ch ) || std::iswpunct( ch ) ) ) {
                        //!std::iswalnum( ch )
                        //end of token
                        input->unget( ch );
                        _buffer.erase( _buffer.size() - 1 );
                        break;
                    }
                }

            }
        }
        if( type == TAG ) {
            getTagId();
        } else if( type == COMMAND ) {
            getCmdId();
        }
    }
    return type;
}
/*****************************************************************************/
void Lexer::getTagId()
{
    TagIdMapIter pos( _tagIdMap.find( _buffer ) );
    if( pos != _tagIdMap.end() ) {
        _tagCode = pos->second;
    } else {
        _tagCode = BADTAG;
    }
}
/*****************************************************************************/
void Lexer::getCmdId()
{
    if( _buffer.find( L".*", 0, 2 ) == 0 ) {
        _cmdCode = COMMENT;
        _buffer.erase( 0, 2 );
    } else if( _buffer.find( L".br", 0, 3 ) == 0 ) {
        _cmdCode = BREAK;
    } else if( _buffer.find( L".ce", 0, 3 ) == 0 ) {
        std::size_t cut( 0 );
        _cmdCode = CENTER;
        _buffer.erase( 0, 3 );
        for( cut = 0; cut <= _buffer.length() && _buffer[ cut ] == L' '; ++cut )
             ;
        if( cut ) {
            _buffer.erase( 0, cut );     //trim leading spaces
        }
    } else if( _buffer.find( L".im", 0, 3 ) == 0 ) {
        std::size_t cut( 0 );
        _cmdCode = IMBED;
        _buffer.erase( 0, 3 );
        _buffer.erase( _buffer.size() - 1 );  //trim '/n'
        for( cut = 0; cut <= _buffer.length() && _buffer[cut] == L' '; ++cut )
             ;
        if( cut ) {
            _buffer.erase( 0, cut );     //trim leading spaces
        }
    } else if( _buffer.find( L".nameit", 0, 7 ) == 0 ) {
        _cmdCode = NAMEIT;
        std::size_t cut( 0 );
        _buffer.erase( 0, 7 );
        _buffer.erase( _buffer.size() - 1 );  //trim '/n'
        for( cut = 0; cut <= _buffer.length() && _buffer[cut] == L' '; ++cut )
             ;
        if( cut ) {
            _buffer.erase( 0, cut );     //trim leading spaces
        }
    } else {
        _cmdCode = BADCMD;
    }
}
