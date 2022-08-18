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
#include <stddef.h>
#include <windows.h>
#include "rterrno.h"
#include "fileacc.h"
#include "rtcheck.h"
#include "iomode.h"
#include "seterrno.h"
#include "defwin.h"
#include "close.h"
#include "thread.h"

int __close( int hid )
{
    int         is_closed;
    int         rc;
    HANDLE      h;
#ifdef DEFAULT_WINDOWING
    LPWDATA res;
#endif

    __handle_check( hid, -1 );

    is_closed = 0;
    rc = 0;
    h = __getOSHandle( hid );

#ifdef DEFAULT_WINDOWING
    if( _WindowsCloseWindow != NULL ) {
        res = _WindowsIsWindowedHandle( hid );
        if( res != NULL ) {
            _WindowsRemoveWindowedHandle( hid );
            _WindowsCloseWindow( res );
            is_closed = 1;
        }
    }
#endif
    if( !is_closed && !CloseHandle( h ) ) {
        rc = __set_errno_nt();
    }
    __freePOSIXHandle( hid );
    __SetIOMode_nogrow( hid, 0 );
    return( rc );
}
