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
*  Name:        jtolower        �������ɕϊ�
*
*  Synopsis:    nc = jtolower( c );
*
*               JMOJI nc;       �ϊ������S�p����
*               JMOJI c;        �ϊ�����S�p����
*
*  Description: �^����ꂽ�S�p������ϊ�����Bjtolower�͑啶����
*               �������ɁAjtoupper�͏�������啶���ɕϊ�����B
*               jtohira �̓J�^�J�i���Ђ炪�ȂɁAjtokata �͂Ђ炪��
*               ���J�^�J�i�ɕϊ�����B
*
*  Returns:     �ϊ��s�\�̏ꍇ�͂Ȃɂ�������c �̒l��Ԃ��B
*
*
*  Name:        jtolower        Convert to lower case 2-byte KANJI code
*
*  Synopsis:    nc = jtolower( c );
*
*               JMOJI nc;       lower case 2-byte KANJI code
*               JMOJI c;        2-byte KANJI code
*
*  Description: Convert to given 2-byte KANJI code. Jtolower converts upper
*               case 2-byte KANJI code to lower case, jtoupper converts lower
*               case 2-byte KANJI code to upper case. Jtohira converts
*               katakana 2-byte KANJI code to hiragana and jtokata hiragana
*               2-byte KANJI code to katakana.
*
*  Returns:     If it is not able to be converted, it returns the value of c.
*
**/


_WCRTLINK JMOJI jtolower( JMOJI c )
{
    return( jisupper( c ) ? ( c + 0x21 ) : c );
}
