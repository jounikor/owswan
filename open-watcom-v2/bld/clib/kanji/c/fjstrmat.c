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
*  Name:        _fjstrmatch     �w�蕶������������
*
*  Synopsis:    p = _fjstrmatch( s, b );
*
*               FJSTRING p;     �����̃|�C���^
*               FJSTRING s;     ������
*               FJSTRING b;     �w�蕶���̕�����
*
*  Description: �Q���������������������́A�����񂓂��P�����Â�������
*               �ŏ��ɕ����񂂂Ɋ܂܂�镶���ƈ�v�����ʒu�̃|�C��
*               �^��Ԃ��B�Q�������������������͔��΂ɁA�ŏ��ɕ�����
*               �Ɋ܂܂�Ȃ����������ꂽ�ʒu�̃|�C���^��Ԃ��B
*
*  Returns:     �������Ƀk�������i�����̏I�[�j�ɒB�����ꍇ�̓k���|
*               �C���^��Ԃ��B
*
*
*  Name:        _fjstrmatch     compute the initial position where searching
*                               letters occur in string
*
*  Synopsis:    p = _fjstrmatch( s, b );
*
*               FJSTRING        p;      pointer to the initial position of
*                                       searching letter occurrence
*               FJSTRING        s;      pointer to the string for reference
*               FJSTRING        b;      pointer to the searching letters
*
*  Description: The jstrmatch function compute the initial position of the
*               string pointed to by s where occur the letters from the string
*               pointed to by b.  jstrskip function have the opposite
*               specification to jstrmatch, at the view of initial position
*               where not occur the searching letter. This function is
*               a data independent form that accept far pointer argunments.
*               It is most useful in mix memory model applications.
*
*  Returns:     This function will return the NULL pointer if it not occured
*               the letters from string pointed to by b.
*
**/

_WCRTLINK FJSTRING _fjstrmatch( const JCHAR _WCFAR *s, const JCHAR _WCFAR *b )
{
    const JCHAR _WCFAR *st, _WCFAR *bt;
    JMOJI ss, bb;

    for( ; st = _fjgetmoji( s, (JMOJI _WCFAR *)&ss ), ss; s = st )
        for( bt = b; bt = _fjgetmoji( bt, (JMOJI _WCFAR *)&bb ), bb; )
            if( ss == bb ) return( (FJSTRING) s );
    return( (FJSTRING) NULL );
}
