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
#include <jctype.h>
#include <jstring.h>


/**
*
*  Name:        _fjstrncpy      ��������R�s�[����
*
*  Synopsis:    p = _fjstrncpy( to, from, n );
*
*               FJSTRING p;     �����Ɠ����l
*               FJSTRING to;    �R�s�[����镶����
*               FJSTRING from;  �R�s�[���镶����
*               int n;          ������
*
*  Description: �����񂆂������̐擪���炎�����𕶎��񂔂���
*               �㏑������B
*
*
*  Name:        _fjstrncpy      copy the characters from the string
*
*  Synopsis:    p = _fjstrncpy( to, from, n );
*
*               FJSTRING        p;      return value same as distination
*                                       pointer
*               FJSTRING        to;     distination pointer into the string
*               FJSTRING        from;   pointer to the source string
*               int             n;      number of letters should be copied
*
*  Description: The _fjstrncpy function copy no more than "n" characters from
*               the string pointed to by "from" into the array pointed to
*               by "to". Copying of overlapping objects is not guaranteed
*               to work properly. This function is a data independent form
*               that accept far pointer argunments. It is most useful in mix
*               memory model applications.
*
*  Returns:     The _fjstrncpy function returns the value of "to".
*
**/

_WCRTLINK FJSTRING _fjstrncpy( JCHAR _WCFAR *to, const JCHAR _WCFAR *from, size_t n )
{
    int i;
    JCHAR _WCFAR *p;

    p = to;
    for( i = 0; i < n; i++ ) {
        if( iskanji( *from ) ) {
            *p++ = *from++;
            if( ( *p++ = *from++ ) == '\0' ) {
                p -= 2;
                while( ++i <= n ) *p++ = '\0';
                break;
            }
        } else if( ( *p++ = *from++ ) == '\0' ) {
            while( ++i < n ) *p++ = '\0';
            break;
        }
    }
    return( to );
}
