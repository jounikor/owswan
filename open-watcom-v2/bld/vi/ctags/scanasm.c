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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ctags.h"

/*
 * ScanAsm - scan an asm file
 */
void ScanAsm( void )
{
    char        buff[MAX_STR];
    char        token[MAX_STR];
    char        *buffptr;
    int         i;

    while( GetString( buff, sizeof( buff ) ) ) {

        buffptr = buff;
        SKIP_SPACES( buffptr );
        if( *buffptr == '\0' ) {
            continue;
        }
        i = 0;
        while( IsTokenChar( *(unsigned char *)buffptr ) ) {
            token[i++] = *buffptr++;
        }
        if( i == 0 ) {
            continue;
        }
        token[i] = '\0';
        SKIP_SPACES( buffptr );
        if( MyStricmp( &buffptr, "proc" ) ) {
            continue;
        }
        RecordCurrentLineData();
        AddTag( token );
    }

} /* ScanAsm */
