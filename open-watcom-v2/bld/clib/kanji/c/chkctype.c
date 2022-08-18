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
#include <jctype.h>

/**
*
*  Name:        chkctype        �o�C�g�P�ʂ̕����^�C�v�𒲂ׂ�
*
*  Synopsis:    ret = chkctype( c, mode );
*
*               int ret;        �^�C�v
*               unsigned char c;�����i�o�C�g�P�ʁj
*               int mode;       ���[�h
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
*               �����ꍇ���b�s�Q�n�k�f�k��Ԃ��B
*
*
*  Name:        chkctype        Check type of character by byte unit
*
*  Synopsis:    ret = chkctype( c, mode );
*
*               int ret;        Type
*               unsigned char c;Character (byte unit)
*               int mode;       mode
*
*  Description: Check type of character by byte unit. Checktype
*               checks whether c is the first byte of KANJI character
*               when mode is something except 1, if c is the first byte,
*               it returns CT_KJ1. If c is other, it returns CT_ANK.
*               When mode is 1, it checks whether c is the second byte of
*               KANJI character and if c is the second byte of KANJI character,
*               it returns CT_KJ2. If c is other, it returns CT_ILGL.
*               Nthctype checks type of the b th byte of string.
*
*  Returns:     If c is NULL character, chkctype returns CT_ILGL.
*               Nthctype returns CT_ILGL when it find NULL character until
*               the b th byte.
*
**/


_WCRTLINK int chkctype( JCHAR c, int type )
{
    if( c == '\0' ) return( CT_ILGL );                  // JBS 92/10/01
    if( type == CT_KJ1 )
        return( iskanji2( c ) ? CT_KJ2 : CT_ILGL );
    else
        return( iskanji( c ) ? CT_KJ1 : CT_ANK );
}
