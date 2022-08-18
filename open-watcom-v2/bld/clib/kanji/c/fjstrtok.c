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
#include <jctype.h>
#include <jstring.h>


/**
*
*  Name:        _fjstrtok               ���������ؕ����ŕ�������
*
*  Synopsis:    sp = _fjstrtok( targ, brk );
*
*               FJSTRING sp;    ��������������
*               FJSTRING targ;  ������
*               FJSTRING brk;   ��ؕ���
*
*  Description: �����񂔂������𕶎��񂂂������ɂ��镶������ؕ���
*               �Ƃ��ĕ�������B�ŏ��̌Ăяo���łP�ڂ̕���������
*               ����ւ̃|�C���^���Ԃ����B�Q��ڂ���͂���������
*               �m�t�k�k�ɂ��ČĂяo�����Ƃɂ��A���ɕ�����������
*               ��ւ̃|�C���^���Ԃ����B
*
*  Returns:     �k�������ɒB�����ꍇ�̓k���E�|�C���^���Ԃ����B
*
*  Caution:     ���̕�����́A�k���������㏑������鎖�ɂ�菑����
*               ������B
*
*
*  Name:        _fjstrtok       Break 2-byte KANJI string into tokens of KANJI
*
*  Synopsis:    sp = _fjstrtok( targ, brk );
*
*               JSTRING sp;     Broken string
*               JSTRING targ;   2-byte KANJI string
*               JSTRING brk;    Sequence of 2-byte KANJI delimiters
*
*  Description: Break the 2-byte KANJI string pointed to by targ into a
*               sequence of tokens of 2-byte KANJI, each of which is delimited
*               by a character from the string pointed to by brk. The first
*               call to _fjstrtok will return a pointer to the first token in the
*               string pointed to by targ. Subsequence calls to _fjstrtok must
*               pass a NULL pointer as the first argument, in order to get the
*               next token in the string. This function is a data independent
*               form that accept far pointer argunments. It is most useful
*               in mix memory model applications.
*
*  Returns:     _fjstrtok function returns a pointer to the first character of
*               token or NULL if there is no token found.
*
*  Caution:     The given string is overwritten by NULL character.
*
**/

_WCRTLINK FJSTRING _fjstrtok( JCHAR _WCFAR *targ, const JCHAR _WCFAR *brk )
{
    static JCHAR _WCFAR *sp;
    JCHAR _WCFAR *p, _WCFAR *q, _WCFAR *r;

    p = ( targ == NULL ) ? sp : targ;
    if( p == NULL ) return( NULL );
    q = _fjstrskip( p, brk );
    if( q == NULL ) return( NULL );
    if( *q == '\0' || ( iskanji( *q ) && q[1] == '\0' ) )
        return( NULL );
    r = _fjstrmatch( q, brk );
    if( r == NULL ) return( NULL );
    if( *r == '\0' ) {
        sp = NULL;
    } else {
        if( iskanji( *r ) ) *r++ = '\0';
        *r = '\0';
        sp = ++r;
    }
    return( q );
}
