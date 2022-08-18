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
#include "hanzentb.h"


/**
*
*  Name:        hantozen        �S�p�����ɕϊ�
*               zentohan        �A�X�L�[�����ɕϊ�
*
*  Synopsis:    nc = hantozen( c );
*               nc = zentohan( c );
*
*               JMOJI nc;       �ϊ���������
*               JMOJI c;        �ϊ����镶��
*
*  Description: hantozen�̓A�X�L�[����������ɑΉ������S�p������
*               �ϊ�����Bzentohan�͋t�ɑS�p�������A�X�L�[������
*               �ϊ�����B�ϊ��\�ȕ����͈̔͂́A�A�X�L�[�E�R�[�h
*               �̂O���Q�O����O���V���ɑΉ����镶���ł���B
*
*  Returns:     �ϊ��s�\�̏ꍇ�͂Ȃɂ�������c �̒l��Ԃ��B
*
*
*  Name:        hantozen        Change ASCII character to 2-byte KANJI
*                               character
*               zentohan        Change 2-byte KANJI character to ASCII
*                               character
*
*  Synopsis:    nc = hantozen( c );
*               nc = zentohan( c );
*
*               JMOJI nc;       Changed result character
*               JMOJI c;        Source character
*
*  Description: Hantozen changes ASCII character to 2-byte KANJI character
*               corresponding to it. Oppositly zentohan changes 2-byte KANJI
*               character to ASCII character corresponding to it. The range of
*               changable character set is more than or equal to 0x20 and less
*               than or equal to 0x7e.
*
*  Returns:     Return c if c is unchangable.
*
**/

_WCRTLINK JMOJI hantozen( JMOJI c )
{
    return( ( c >= 0x20 && c <= 0x7e ) ? __HanZen1[c - 0x20] : c );
}



_WCRTLINK JMOJI zentohan( JMOJI c )
{
    JMOJI i;

    for( i = 0; i <= 0x5e; i++ )
        if( c == __HanZen1[i] ) return( i + 0x20 );
    return( c );
}
