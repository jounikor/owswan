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

/******************************************************************************
*
*  Name:        jstrcspn        ������̐擪���猟�������܂ł̕������𓾂�
*
*  Synopsis:    c = jstrcspn( str1, str2 );
*
*               int     c;      ������̐擪���猟��������������܂ł̕�����
*               JSTRING str1;   �팟��������
*               JSTRING str2;   ���������̃Z�b�g
*
*  Description: jstrcspn �֐��� strcspn �֐��������R�[�h�ɑΉ����������ł��B
*               str1 �Ŏw�肵�������񒆂ŁAstr2 �Ŏw�肵�������̂����ꂩ�P������*               �ŏ��Ɍ����܂ł̕�������Ԃ��܂��B
*
*  Returns:     str2 �Ŏw�肵������������������������Ȃ�Astr1 �̐擪����̕���*               ������̕��������A�����l�ŕԂ��܂��B
*
*
*  Name:        jstrcspn        compute the length of the initial segment of
*                               the string consists letters not from the
*                               array of the searching letters.
*
*  Synopsis:    c = jstrcspn( str1, str2 );
*               int     c;      resoult length of computing
*               JSTRING str1;   pointer to be referenced string
*               JSTRING str2;   pointer to the array of searching letters
*
*  Description: jstrcspn function has same specification as strcspn function
*               except having advantage point for correspond to KANJI letters.
*               The jstrcspn function computes the length of the initial segment
*               of the string pointed to by str1 which consists entirely of
*               letters not from the string pointed to by str2. Japanese 2 byte
*               code letter is considered one character.
*
*  Returns:     The length of the initial segment is returned.
*/

_WCRTLINK size_t jstrcspn( const JCHAR *str1, const JCHAR *str2 )
{
    const JCHAR *s2;
    JMOJI c1, c2;
    size_t count;

    for( count = 0; str1 = jgetmoji( str1, &c1 ), c1; ++count ) {
        for( s2 = str2; s2 = jgetmoji( s2, &c2 ), c2; ) {
            if( c1 == c2 ) return count;
        }
    }
    return count;
}
