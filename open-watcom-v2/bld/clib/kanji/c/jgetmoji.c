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
#include <jctype.h>
#include <jstring.h>

/**
*
*  Name:        jgetmoji        �P�������o��
*
*  Synopsis:    newp = jgetmoji( p, moji );
*
*               JSTRING newp;   ���̃|�C���^
*               JSTRING p;      ������
*               JMOJI *moji;    ����
*
*  Description: �����񂩂�擪�̂P���������o���B
*               ��P�o�C�g�������R�[�h�ő�Q�o�C�g���k�������̎���
*               �k�������Ƃ݂Ȃ��A��P�o�C�g���k�������ɏ���������B
*
*  Return:      ���̕����̃|�C���^��Ԃ��B
*               �k�������̎��̓k���������̂̃|�C���^��Ԃ��B
*
*
*  Name:        jgetmoji        Get one character
*
*  Synopsis:    newp = jgetmoji( p, moji );
*
*               JSTRING newp;   Pointer to the next character to
*                               gotten character
*               JSTRING p;      String
*               JMOJI *moji;    Character
*
*  Description: Get one character from the head of the string
*               If the first byte is KANJI code and the second is NULL,
*               this function treats it as NULL character and changes the
*               first byte to NULL.
*
*  Return:      Return a pointer to the next character to the gotten
*               character. If the head of the string is NULL, return a pointer
*               to the NULL character.
*
**/

_WCRTLINK JSTRING jgetmoji( const JCHAR *p, JMOJI *moji )
{
    if( *moji = *p ) {
        p++;
        if( iskanji( *moji ) ) {
            if( *p )
                *moji = ( *moji << 8 ) | *p++;
            else
//              *--p = '\0';        // JBS 92/09/30
                *moji = '\0';       // JBS 92/09/30
        }
    }
    return( (JSTRING) p );
}
