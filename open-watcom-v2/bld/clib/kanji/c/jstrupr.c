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
*  Name:        jstrupr         �S�p��������������ɕϊ�
*
*  Synopsis:    s2 = jstrupr( s1 );
*
*               JSTRING s1;     �ϊ�����S�p������
*               JSTRING s2;     �ϊ����ꂽ�S�p������
*
*  Description: �^����ꂽ�S�p������̏�������啶����ϊ�����B
*
*  Returns:     �ϊ��s�\�̕����ɂ͂Ȃɂ����Ȃ��B
*
*
*  Name:        jstrupr         Convert string to upper case 2-byte KANJI code
*
*  Synopsis:    s2 = jstrupr( s1 );
*
*               JSTRING s1;     2-byte KANJI code string
*               JSTRING s2;     upper case 2-byte KANJI code string
*
*  Description: Convert lower case characters of given 2-byte KANJI string to
*               upper case.
*
*  Returns:     If some characters in given string are not able to be
*               converted, they are not changed.
*
**/

_WCRTLINK JSTRING jstrupr( JCHAR *str )
{
    JCHAR *dest, *keep = str;
    JMOJI c;

    for( dest = str; str = jgetmoji( str, &c ), c; ) {
        dest = jputmoji( dest, jtoupper( c ) );
    }
    return keep;
}
