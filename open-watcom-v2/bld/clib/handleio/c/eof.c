/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Implementation of _eof().
*
****************************************************************************/


#include "variety.h"
#include <stdio.h>
#include <unistd.h>
#if defined(__NT__)
    #include <windows.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#endif
#include "rterrno.h"
#include "iomode.h"
#include "rtcheck.h"
#include "thread.h"


_WCRTLINK int _eof( int handle )        /* determine if at EOF */
{
    off_t   current_posn, file_len;

    __handle_check( handle, -1 );
    file_len = _filelength( handle );
    if( file_len == -1L )
        return( -1 );
    current_posn = _tell( handle );
    if( current_posn == -1L )
        return( -1 );
    if( current_posn == file_len )
        return( 1 );
    return( 0 );
}
