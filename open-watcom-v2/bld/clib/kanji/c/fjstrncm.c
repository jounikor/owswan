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
*  Name:        _fjstrncmp
*
*  Synopsis:    x = _fjstrncmp( a, b, n );
*
*               int x;          結果
*               FJSTRING a, b;  文字列
*               int n;          文字数
*
*  Description: ２つの文字列を比較する。ｊｓｔｒｎｃｍｐは先頭から
*               ｎ文字分のみの比較をする。
*               ＡＮＫ文字　＜　カナ文字　＜　漢字文字　の大小関係に
*               になっている。
*
*  Returns:     文字列ａの方がｂよりも小さい場合は負数を、大きい場合
*               は正数を返し、ａとｂが等しいときは０を返す。
*
*
*  Name:        _fjstrncmp      compare strings with in KANJI letters
*
*  Synopsis:    x = _fjstrncmp( a, b, n );
*
*               int     x;      resoult as compared
*               FJSTRING        a, b;   two strings for compare each other
*               int     n;      length for comparison
*
*  Description: The _fjstrncmp function compares the KANJI string pointed to
*               by "a" to the string pointed to by "b".
*               This function compares only "n" letters from top of the strings.
*               The rule of comparison is as below.
*                 ASCII and numeric letter < KATAKANA letter < KANJI letter
*               This function is a data independent form that accept far
*               pointer argunments. It is most useful in mix memory model
*               applications.
*
*  Returns:     The _fjstrncmp function returns an integer less than, equal to,
*               or greater than zero, indicating that the string pointed to
*               by "a" is less than, equal to, or greater than the string
*               pointed to by "b".
*
**/


_WCRTLINK int _fjstrncmp( const JCHAR _WCFAR *a, const JCHAR _WCFAR *b, size_t n )
{
    JMOJI aa, bb;

    while( n-- ) {
        a = _fjgetmoji( a, (JMOJI _WCFAR *)&aa );
        b = _fjgetmoji( b, (JMOJI _WCFAR *)&bb );
        if( aa != bb ) return( ( aa < bb ) ? -1 : 1 );
        if( aa == 0 ) break;
    }
    return( 0 );
}
