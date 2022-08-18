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
* Description:  Process color tag
*
*   :color fc=name bc=name
*   Valid names are: default, black, blue, red, pink, green, cyan, yellow, neutral,
*   brown, darkgray, darkblue, darkred, darkpink, darkgreen darkcyan, palegray
*   Remains active until changed, or the next header
*
****************************************************************************/


#include "wipfc.hpp"
#include <string>
#include "color.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include "page.hpp"
#include "util.hpp"

Lexer::Token Color::parse( Lexer* lexer )
{
    Lexer::Token tok;

    while( (tok = _document->getNextToken()) != Lexer::TAGEND ) {
        //parse attributes
        if( tok == Lexer::ATTRIBUTE ) {
            std::wstring key;
            std::wstring value;
            splitAttribute( lexer->text(), key, value );
            if( key == L"fc" ) {
                _foreground = parseColor( value );
                _setForeground = true;
            } else if( key == L"bc" ) {
                _background = parseColor( value );
                _setBackground = true;
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
    return _document->getNextToken();
}
/***************************************************************************/
Color::ColorName Color::parseColor( std::wstring& name )
{
    if( name == L"default" ) {
        return Color::DEFAULT;
    } else if( name == L"blue" ) {
        return Color::BLUE;
    } else if( name == L"red" ) {
        return Color::RED;
    } else if( name == L"pink" ) {
        return Color::PINK;
    } else if( name == L"green" ) {
        return Color::GREEN;
    } else if( name == L"cyan" ) {
        return Color::CYAN;
    } else if( name == L"yellow" ) {
        return Color::YELLOW;
    } else if( name == L"neutral" ) {
        return Color::NEUTRAL;
    } else if( name == L"brown" ) {
        return Color::BROWN;
    } else if( name == L"darkgray" ) {
        return Color::DARKGRAY;
    } else if( name == L"darkblue" ) {
        return Color::DARKBLUE;
    } else if( name == L"darkred" ) {
        return Color::DARKRED;
    } else if( name == L"darkpink" ) {
        return Color::DARKPINK;
    } else if( name == L"darkgreen" ) {
        return Color::DARKGREEN;
    } else if( name == L"darkcyan" ) {
        return Color::DARKCYAN;
    } else if( name == L"black" ) {
        return Color::BLACK;
    } else if( name == L"palegray" ) {
        return Color::PALEGRAY;
    } else {
        _document->printError( ERR2_VALUE );
    }
    return Color::DEFAULT;
}
/***************************************************************************/
// 13 for foreground
// 14 for background
void Color::buildText( Cell* cell )
{
    if( _setForeground ) {
        cell->addByte( Cell::ESCAPE );  //esc
        cell->addByte( 0x03 );          //size
        cell->addByte( 0x13 );          //set foreground color
        cell->add( static_cast< byte >( _foreground ) );
    }
    if( _setBackground ) {
        cell->addByte( Cell::ESCAPE );  //esc
        cell->addByte( 0x03 );          //size
        cell->addByte( 0x14 );          //set background color
        cell->add( static_cast< byte >( _background ) );
    }
    if( cell->textFull() ) {
        printError( ERR1_LARGEPAGE );
    }
}
