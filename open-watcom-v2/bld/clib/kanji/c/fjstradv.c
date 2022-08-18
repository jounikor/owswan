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
*  Name:        _fjstradv               ����������̃|�C���^�𓾂�
*
*  Synopsis:    news = _fjstradv( s, nm );
*
*               FJSTRING news;  �V������
*               FJSTRING s;     ������
*               int nm;         ������
*
*  Description: �����񂓂̐擪���炎���Ŏw�肵��������������̕������w��
*               �|�C���^��Ԃ��B�����R�[�h�͂Q�o�C�g�łP�����Ɛ�����B
*
*  Returns:     �����񂓂����������������Z���ꍇ�́A������̍Ō�ɂ���
*               �k���������w���|�C���^��Ԃ��B
*
*
*  Name:        _fjstradv               get forward string position in
*                                       specified string.
*
*  Synopsis:    news = _fjstradv( s, nm );
*
*               FJSTRING        news;   forward position
*               FJSTRING        s;      current position of string
*               int     nm;     number of letters for forwarding
*
*  Description: This function returns the forward position where is advanced nm
*               times per letter from current position for s string. A kanji
*               letter consists of 2 byte. This function is a data
*               independent form that accept far pointer argunments.
*               It is most useful in mix memory model applications.
*
*  Returns:     If the length of s string is shorter than nm number then this
*               function will return the end of potision for s string.
**/


_WCRTLINK FJSTRING _fjstradv( const JCHAR _WCFAR *s, size_t nm )
{
    return( (FJSTRING) (s + _fmtob( s, nm )) );
}
