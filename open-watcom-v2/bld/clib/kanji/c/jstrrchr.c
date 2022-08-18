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
*  Name:        jstrrchr        ��������������
*
*  Synopsis:    p = jstrrchr( s, c );
*
*               JSTRING p;      �����̃|�C���^
*               JSTRING s;      ������
*               JMOJI c;        ����
*
*  Description: �����񂓂����Ɍ������A�ŏ��ɕ������ƈ�v����
*               �ʒu�̃|�C���^��Ԃ��B���������������͐擪����
*               �������A�����������������͍Ōォ�猟������B
*
*  Returns:     ��v���Ȃ������ꍇ�̓k���|�C���^��Ԃ��B
*
*
*  Name:        jstrrchr        search out specified letter from the string
*
*  Synopsis:    p = jstrrchr( s, c );
*
*               JSTRING p;      return value which point to the searched out
*                               letter
*               JSTRING s;      pointer to the string which is referenced
*               JMOJI   c;      the value of searching letter
*
*  Description: The jstrrchr function locates the final occurrence of "c"
*               in the string pointed to by "s". Jstrrchr function start to
*               search out from the end of string, and jstrchr function
*               start to search out from the top of string.
*               If you specify "c" with KANJI letter, JMOJI consist with 16
*               bit and its' high byte consist with first byte code of KANJI
*               letter, and low byte consist with second byte code of KANJI
*               letter. If second byte code is NULL then the first byte code
*               also consider to NULL. If you specify "c" with 1 byte character
*               then, it must be in the low byte of JMOJI and the high byte of
*               JMOJI has to NULL.
*
*  Returns:     This function returns a pointer to the located letter, or NULL
*               if the letter does not occur in the string.
*
**/


_WCRTLINK JSTRING jstrrchr( const JCHAR *s, JMOJI c )
{
    const JCHAR *p, *ss;
    JMOJI cc;

    p = (JSTRING)NULL;
    while( 1 ) {
        ss = jgetmoji( s, &cc );
        if( cc == 0 ) break;
        if( cc == c ) p = s;
        s = ss;
    }
    return( (JSTRING) p );
}
