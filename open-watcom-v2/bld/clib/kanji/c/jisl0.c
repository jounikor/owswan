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
*  Name:        jisl0           �Q�o�C�g�L���̃e�X�g
*
*  Synopsis:    ret = jisl0( c ); (0x8140 <= c <= 0x889e)
*
*               int ret;        �^�Ȃ��0�ȊO�A�^�łȂ����0
*               JMOJI c;        �����R�[�h
*
*  Description: �^����ꂽ�����R�[�h�������ɍ����Ă��邩�𒲂ׂ�B
*               �����ɍ����Ă���ꍇ�͂O�ȊO��Ԃ��A����Ȃ��ꍇ��
*               �O��Ԃ��B
*
*  Caution:     �����̊֐��͒P���ɒl�͈̔͂𒲂ׂĂ��邾���ł���A
*               �����������R�[�h�ł��邩�ǂ����͒��ׂȂ��B
*
*
*  Name:        jisl0           Check 2-byte KANJI code except KANJI
*
*  Synopsis:    ret = jisl0( c ); (0x8140 <= c <= 0x889e)
*
*               int ret;        0 if fault, non-zero if true
*               JMOJI c;        KANJI code
*
*  Description: Check whether the givin character c is 2-byte KANJI code
*               except KANJI. It returns non-zero if true, zero if fault.
*               The range of 2-byte KANJI code set is the below.
*               0x8140 <= c <= 0x889e
*
*  Caution:     These functions checks the range only and do not check
*               the validity whether it is a correct KANJI code or not.
*
**/

_WCRTLINK int jisl0( JMOJI c )
{
    return( jiszen( c ) && c <= 0x889e );
}
