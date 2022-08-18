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
*  Name:        _fjstrchr               ��������������
*
*  Synopsis:    p = _fjstrchr( s, c );
*
*               FJSTRING p;     �����̃|�C���^
*               FJSTRING s;     ������
*               JMOJI c;        ����
*
*  Description: �����񂓂����Ɍ������A�ŏ��ɕ������ƈ�v����
*               �ʒu�̃|�C���^��Ԃ��B���������������͐擪����
*               �������A�����������������͍Ōォ�猟������B
*
*  Returns:     ��v���Ȃ������ꍇ�̓k���|�C���^��Ԃ��B
*
*
*  Name:        _fjstrchr               search out specified KANJI letter from string
*
*  Synopsis:    p = _fjstrchr( s, c );
*
*               FJSTRING p;     a pointer located to the specified letter
*               FJSTRING s;     a pointer to the KANJI string for reference
*               JMOJI   c;      KANJI letter for search out
*
*  Description: The jstrchr function locates the first occurrence of "c" in
*               the KANJI string pointed to by "s". Jstrchr function starts to
*               search out from the top of string , and jstrrchr function starts
*               to search out from the end of string. This function is
*               a data independent form that accept far pointer argunments.
*               It is most useful in mix memory model applications.
*
*  Returns:     This function returns a pointer to the located letter, or NULL
*               if the letter does not occur in the string.
*
**/

_WCRTLINK FJSTRING _fjstrchr( const JCHAR _WCFAR *s, JMOJI c )
{
    JMOJI cc;
    const JCHAR _WCFAR *ss;

    do {
        ss = _fjgetmoji( s, (JMOJI _WCFAR *)&cc );
        if( c == cc ) return( (FJSTRING) s );
        s = ss;
    } while( cc );
    return( (FJSTRING) NULL );
}
