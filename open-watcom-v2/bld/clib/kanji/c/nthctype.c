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
*  Name:        nthctype
*
*  Synopsis:    ret = nthctype( s, b );
*
*               int ret;        �^�C�v
*               JSTRING s;      ������
*               int b;          �ʒu
*
*  Description: �o�C�g�P�ʂ̃^�C�v�𒲂ׂ�B�����������������ł�
*               ���������ɂP�ȊO�̒l��^����ƁA�����̑�P�o�C�g
*               ���ǂ����𒲂ׁA��P�o�C�g�Ȃ�b�s�Q�j�i�P��Ԃ�
*               ����ȊO�Ȃ�΂b�s�Q�`�m�j��Ԃ��B���������ɂP��
*               �^����Ɗ����̑�Q�o�C�g���ǂ����𒲂ׁA��Q�o�C
*               �g�Ȃ�΂b�s�Q�j�i�Q��Ԃ�����ȊO�Ȃ�΂b�s�Q�h
*               �k�f�k��Ԃ��B
*               �����������������͕�����̐擪���O�Ƃ���ʒu����
*               ����o�C�g�ɂ��ă^�C�v�𒲂ׂ�B
*
*  Returns:     �k�������̏ꍇ�͂b�s�Q�h�k�f�k��Ԃ��B
*               �����������������ł͈ʒu���܂ł̊ԂɃk����������
*               �����ꍇ���b�s�Q�h�k�f�k��Ԃ��B
*               �߂�l          �l      �Ӗ�
*               CT_ANK          0       ANK�i�����ȊO�j
*               CT_KJ1          1       �����̑�P�o�C�g
*               CT_KJ2          2       �����̑�Q�o�C�g
*               CT_ILGL         -1      �s��
*
*
*  Name:        nthctype
*
*  Synopsis:    ret = nthctype( s, b );
*
*               int ret;        Type
*               JSTRING s;      String
*               int b;          Location (The b th byte from the head of s)
*
*  Description: Check the type of the b th byte of string s.
*               Nthctype uses chkctype which checks whether argument is the
*               first byte of KANJI code if mode is except 1. If argument is
*               the first byte of KANJI, chkctype returns CT_KJ1, otherwise
*               it returns CT_ANK. If mode is 1, chkctype checks whether
*               argument is the second byte of KANJI code. If it is the
*               second byte, chkctype returns CT_KJ2, otherwise it returns
*               CT_ILGL.
*               Nthctype checks the b th byte of string.
*
*  Returns:     If checked character is NULL character, it returns CT_ILGL.
*               If nthctype finds NULL character until the b th byte,
*               it returns CT_ILGL, too.
*               Retrun symbol   Value   Meanings
*               CT_ANK          0       ANK(Except KANJI)
*               CT_KJ1          1       The first byte of KANJI
*               CT_KJ2          2       The second byte of KANJI
*               CT_ILGL         -1      Illegal
*
**/


_WCRTLINK int nthctype( const JCHAR *string, size_t nbyte )
{
    int ctype;

    ctype = CT_ILGL;
/*
    // nbyte is now unsigned so following code does not work    JBS 92/10/01
    while( nbyte-- >= 0 ) {
        if( *string == '\0' ) return( CT_ILGL );
        ctype = chkctype( *string++, ctype );
    }
    // following is revised to preserve the sense of the above code
    // (i.e., if nbyte==0 we go through the loop once)
*/
    do {
        if( *string == '\0' ) return( CT_ILGL );
        ctype = chkctype( *string++, ctype );
    } while( nbyte-- != 0 );
    return( ctype );
}
