/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Korean character set support
*
****************************************************************************/

#include "ftnstd.h"
#include "csetinfo.h"
#include "charset.h"
#include "dbcsutil.h"


// Double-byte characters are represented as follows:
//
//    0x81 <= chr <= 0xbf --> 1st byte of 2-byte Korean character
//
// The second byte of 2-byte Korean characters is in the range:
//
//    0x40 <= chr <= 0xfc, chr != 0x7f


static  bool    IsDoubleByteBlank( const char *ptr )
// Determine if character is a double-byte blank character.
{
    return( ( *(unsigned char *)ptr == 0x81 ) && ( *(unsigned char *)(ptr + 1) == 0x40 ) );
}


static  bool    IsDoubleByteChar( char ch )
// Determine if character is a double-byte character.
{
    return( ( 0x81 <= (unsigned char)ch ) && ( (unsigned char)ch <= 0xbf ) );
}


static size_t   CharacterWidth( const char PGM *ptr )
// Determine character width.
{
    unsigned char   ch;

    if( IsDoubleByteChar( *ptr ) ) {
        ch = *(unsigned char *)(ptr + 1);
        if( ( 0x40 <= ch ) && ( ch <= 0xfc ) ) {
            if( ch == 0x7f) {
                return( 1 );
            }
        }
        return( 2 );
    }
    return( 1 );
}


static  bool    IsForeign( char ch )
// Determine if character is a foreign character (i.e. non-ASCII).
{
    return( IsDoubleByteChar( ch ) );
}


void    __UseKoreanCharSet( void )
{
    CharSetInfo.extract_text = ExtractTextDBCS;
    CharSetInfo.is_double_byte_blank = IsDoubleByteBlank;
    CharSetInfo.character_width = CharacterWidth;
    CharSetInfo.is_foreign = IsForeign;
}
