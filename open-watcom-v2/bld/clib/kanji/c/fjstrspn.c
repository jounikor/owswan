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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "variety.h"
#include <jstring.h>

/**
*
*  Name:        _fjstrspn
*
*  Synopsis:    unsigned int _fjstrspn( JSTRING str1, JSTRING str2 );
*
*               JSTRING str1;   サーチされる文字列
*               JSTRING str2;   str1 の中からサーチする文字の集合
*               unsigned ret;   str2 で指定した文字に一致しない最初の文字を
*                               指すインデックス
*
*  Description: _fjstrspn 関数は, 文字列 str1 の中から、文字列 str2 で指定した
*               文字列中のいずれの文字とも一致しない、最初の文字を指す
*               インデックスを返します。str1 の先頭から戻り値が示すバイト数分
*               は、str2 で指定した文字列中の文字だけで構成されていることを
*               意味します。str1 が str2 で指定した文字以外で始まっている
*               ときは、0 を返します。
*
*  Name:        _fjstrspn
*
*  Synopsis:    ret = _fjstrspn( str1, str2 );
*
*               JSTRING str1;   2-byte KANJI string
*               JSTRING str2;   2-byte KANJI characters
*               unsigned ret;   Index to the first character in string str1
*                               which does not consist of string str2.
*
*  Description: _fjstrspn function returns the index to the first character in
*               thestrinf str1 which is not included in the string str2.
*               In the other words, The string from the head of the string
*               str1 through the ret bytes is consists of the characters in
*               the string str2. This function is a data independent form
*               that accept far pointer argunments. It is most useful in mix
*               memory model applications.
*
*  Return:      If the string str1 starts with a character which is not
*               included in the string str2, this function returns 0.
*
**/

_WCRTLINK size_t _fjstrspn( const JCHAR _WCFAR *str1, const JCHAR _WCFAR *str2 )
{
    const JCHAR _WCFAR *s2;
    JMOJI c1, c2;
    size_t count;

    for( count = 0;
        str1 = _fjgetmoji( str1, ( JMOJI _WCFAR * )&c1 ), c1;
        ++count ) {
        for( s2 = str2;
            ( s2 = _fjgetmoji( s2, ( JMOJI _WCFAR * )&c2 ), c2 ) && ( c1 != c2 );
            );
        if( !c2 ) break;
    }
    return count;
}
