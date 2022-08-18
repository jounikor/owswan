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
*  Name:        _fmtob
*
*  Synopsis:    nb = _fmtob( s, nm );
*
*               int nb;         �o�C�g��
*               FJSTRING s;     ������
*               int nm;         ������
*
*  Description: �����񂓂̐擪���炎���Ŏw�肵���������̊Ԃɂ���o�C�g��
*               ��Ԃ��B�����R�[�h�͂Q�o�C�g�łP�����Ɛ�����B
*
*
*  Name:        _fmtob
*
*  Synopsis:    nb = _fmtob( s, nm );
*
*               int nb;         Length of string s (Unit:byte)
*               JSTRING s;      String
*               int nm;         The number of character
*
*  Description: Return the length of the sring s by byte from the head of s
*               to the nb th character. Two bytes of KANJI code is treated as
*               one character. This function is a data independent form that
*               accept far pointer argunments. It is most useful in mix memory
*               model applications.
*
**/

_WCRTLINK size_t _fmtob( const JCHAR _WCFAR *s, size_t nm )
{
    const JCHAR _WCFAR *ss;
    JMOJI m;

    ss = s;
    while( nm-- ) {
        ss = _fjgetmoji( ss, (JMOJI _WCFAR *)&m );
        if( m == 0 ) break;
    }
    return( ss - s );
}
