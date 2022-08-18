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
#include <jctype.h>
#include <ctype.h>
/**
*
*  Name:        _fjstrnicmp
*
*  Synopsis:    x = _fjstrnicmp( a, b, n );
*
*               int x;          ����
*               JSTRING a, b;   ������
*               int n;          ������
*
*  Description: �Q�̕�������r����B�����������������͐擪����
*               ���������݂̂̔�r������B
*               �`�m�j�����@���@�J�i�����@���@���������@�̑召�֌W��
*               �ɂȂ��Ă���B
*
*  Returns:     �����񂁂̕����������������ꍇ�͕������A�傫���ꍇ
*               �͐�����Ԃ��A���Ƃ����������Ƃ��͂O��Ԃ��B
*
*
*  Name:        _fjstrnicmp
*
*  Synopsis:    x = _fjstrnicmp( a, b, n );
*
*               int x;          Result
*               JSTRING a, b;   Compared strings
*               int n;          Compared number of characters
*
*  Description: Compare two strings ignoring case in n bytes. All ASCII
*               characters are less than any 1-byte KATAKANA chracters.
*               All 1-byte KATAKANA charaters are less than any 2-byte
*               KANJI charaters. This function is a data independent form
*               that accept far pointer argunments. It is most useful in mix
*               memory model applications.
*
*  Returns:     If the string a is less than the sring b, _fjstrnicmp returns
*               negative value, if a is greater than b, it returns positive.
*               If a is equal to b, it returns 0.
*
**/

#define _mjtoupper( c ) (( c >= 0x8281 && c <= 0x829a ) ? ( c - 0x21 ) : c )
#define _mtoupper( c ) (( c >= (unsigned char)'a' && c <= (unsigned char)'z' )\
                         ? ( c - ('a' - 'A') ) : c )

_WCRTLINK int _fjstrnicmp( const JCHAR _WCFAR *a, const JCHAR _WCFAR *b, size_t n )
{
    JMOJI aa, bb;

    while( n-- ) {
        a = _fjgetmoji( a, &aa );
        if( aa > 0x7f )
            aa = _mjtoupper( aa );
        else
            aa = _mtoupper( aa );
        b = _fjgetmoji( b, &bb );
        if( bb > 0x7f )
            bb = _mjtoupper( bb );
        else
            bb = _mtoupper( bb );
        if( aa != bb ) return( ( aa < bb ) ? -1 : 1 );
        if( aa == 0 ) break;
    }
    return( 0 );
}
