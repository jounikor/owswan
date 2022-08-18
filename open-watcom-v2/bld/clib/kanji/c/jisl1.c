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
*  Name:        jisl1           ������P�����R�[�h
*
*  Synopsis:    ret = jisl1( c ); (0x889f <= c <= 0x9872)
*
*               int ret;        �����ɍ����Ă���΂O�ȊO�̒l
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
*
*  Name:        jisl1           Check 2-byte KANJI code of JIS level 1
*
*  Synopsis:    ret = jisl1( c ); (0x889f <= c <= 0x9872)
*
*               int ret;        0 if fault, non-zero if true
*               JMOJI c;        KANJI code
*
*  Description: Check whether the givin character c is 2-byte KANJI code of
*               JIS level 1. It returns non-zero if true, zero if fault.
*               The range of 2-byte KANJI code set is the below.
*               0x889f <= c <= 0x9872
*
*  Caution:     These functions checks the range only and do not check
*               the validity whether it is a correct KANJI code or not.
*
* Note:         JIS is Japan Industrial Standard Institutes. JIS decides
*               two level of 2-byte KANJI code set. One is called 2-byte KANJI
*               code set level 1 and another is called 2-byte KANJI code set
*               level 2. Usually Japanese personal computers have font ROM/
*               RAM of both levels.
*
**/

_WCRTLINK int jisl1( JMOJI c )
{
    return( jiszen( c ) && c >= 0x889f && c <= 0x9872 );
}
