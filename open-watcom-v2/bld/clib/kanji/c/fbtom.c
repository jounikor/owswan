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
*  Name:        _fbtom
*
*  Synopsis:    nm = _fbtom( s, nb );
*
*               int nm;         ������
*               FJSTRING s;     ������
*               int nb;         �o�C�g��
*
*  Description: �����񂓂̐擪���炎���Ŏw�肵���o�C�g���̊Ԃɂ��镶����
*               ��Ԃ��B�����R�[�h�͂Q�o�C�g�łP�����Ɛ�����B
*
*
*  Name:        _fbtom
*
*  Synopsis:    nm = _fbtom( s, nb );
*
*               int nm;         Number of characters
*               FJSTRING s;     String
*               int nb;         Number of bytes
*
*  Description: Return the number of characters between the head of the
*               string specified by s and nb bytes from s. 2-byte KANJI
*               character is treated as one character. This function is
*               a data independent form that accept far pointer argunments.
*               It is most useful in mix memory model applications.
**/

_WCRTLINK size_t _fbtom( const JCHAR _WCFAR *s, size_t nb )
{
    size_t count;
    const JCHAR _WCFAR *ss;
    JMOJI m;

    count = 0;
    ss = s + nb;
    while( s = _fjgetmoji( s, (JMOJI _WCFAR *)&m ), m ) {
        if( s > ss ) break;
        count++;
    }
    return( count );
}
