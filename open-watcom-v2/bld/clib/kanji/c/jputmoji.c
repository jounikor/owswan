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
*  Name:        jputmoji                ������ւ̕����̏o��
*
*  Synopsis:    nstr = jputmoji( JSTRING str, JMOJI moji )
*
*               JSTRING nstr;   ���̃|�C���^
*               JSTRING str;    ������
*               JMOJI moji;     �o�͂��镶��
*
*  Description: �����񂓂����ɕ��������������i�[����B
*
*  Return:      �����������i�[�������̃A�h���X���w���B
*
*  Name:        jputmoji                Output one KANJI character to string
*
*  Synopsis:    nstr = jputmoji( JSTRING str, JMOJI moji )
*
*               JSTRING nstr;   Pointer to the next output character
*               JSTRING str;    String
*               JMOJI moji;     Kanji character
*
*  Description: Output 2-byte KANJI character "moji" to string "str".
*
*  Return:      Pointer to the next output moji to the string.
*
**/

_WCRTLINK JSTRING jputmoji( JSTRING str, JMOJI moji )
{
    if( jiszen( moji ) )
        *str++ = (JCHAR)( moji >> 8 );
    *str++ = (JCHAR)moji;
//  *str = 0;                                       JBS 92/07/31
    return str;
}
