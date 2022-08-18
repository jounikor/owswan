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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "variety.h"
#include <jstring.h>


/**
*
*  Name:        jstrncmp        ������̔�r
*
*  Synopsis:    x = jstrncmp( a, b, n );
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
*  Name:        jstrncmp        compare strings with in KANJI letters
*
*  Synopsis:    x = jstrncmp( a, b, n );
*
*               int     x;      resoult as compared
*               JSTRING a, b;   two strings for compare each other
*               int     n;      length for comparison
*
*  Description: The jstrncmp function compares the KANJI string pointed to
*               by "a" to the string pointed to by "b".
*               This function compares only "n" letters from top of the strings.
*               The rule of comparison is as below.
*                 ASCII and numeric letter < KATAKANA letter < KANJI letter
*
*  Returns:     The jstrncmp function returns an integer less than, equal to,
*               or greater than zero, indicating that the string pointed to
*               by "a" is less than, equal to, or greater than the string
*               pointed to by "b".
*
**/

_WCRTLINK int jstrncmp( const JCHAR *a, const JCHAR *b, size_t n )
{
    JMOJI aa, bb;

    while( n-- ) {
        a = jgetmoji( a, &aa );
        b = jgetmoji( b, &bb );
        if( aa != bb ) return( ( aa < bb ) ? -1 : 1 );
        if( aa == 0 ) break;
    }
    return( 0 );
}
