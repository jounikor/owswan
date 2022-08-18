/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2017 The Open Watcom Contributors. All Rights Reserved.
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
#include <stdio.h>
#if defined(__NT__)
    #include <windows.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#elif defined( __DOS__ ) || defined( __WINDOWS__ )
    #include "tinyio.h"
#endif
#include "rterrno.h"
#include "iomode.h"
#include "fileacc.h"
#include "rtcheck.h"
#include "seterrno.h"
#include "defwin.h"
#include "qwrite.h"
#include "thread.h"


/*
    Use caution when setting the file pointer in a multithreaded
    application. You must synchronize access to shared resources. For
    example, an application whose threads share a file handle, update the
    file pointer, and read from the file must protect this sequence by
    using a critical section object or a mutex object.
 */


#ifdef __WINDOWS_386__

#define MAXBUFF 0x8000

static tiny_ret_t __TinyWrite( int handle, const void *buffer, unsigned len )
{
    unsigned    total = 0;
    unsigned    writamt;
    tiny_ret_t  rc;

    while( len > 0 ) {

        if( len > MAXBUFF ) {
            writamt = MAXBUFF;
        } else {
            writamt = len;
        }
        rc = TinyWrite( handle, buffer, writamt );
        if( TINY_ERROR( rc ) )
            return( rc );
        total += TINY_LINFO( rc );
        if( TINY_LINFO( rc ) != writamt )
            return( total );

        len -= writamt;
        buffer = ((const char *)buffer) + writamt;

    }
    return( total );
}
#endif

int __qwrite( int handle, const void *buffer, unsigned len )
{
    int             atomic;
#if defined(__NT__)
    DWORD           len_written;
    HANDLE          h;
    int             error;
#elif defined(__OS2__)
    OS_UINT         len_written;
    APIRET          rc;
    unsigned long   dummy;
#else
    unsigned        len_written;
    tiny_ret_t      rc;
#endif

    __handle_check( handle, -1 );
#if defined(__NT__)
    h = __getOSHandle( handle );
#endif
    atomic = 0;
    if( __GetIOMode( handle ) & _APPEND ) {
        _AccessFileH( handle );
        atomic = 1;
#if defined(__NT__)
        if( SetFilePointer( h, 0, NULL, FILE_END ) == -1 ) {
            error = GetLastError();
            _ReleaseFileH( handle );
            return( __set_errno_dos( error ) );
        }
#elif defined(__OS2__)
        rc = DosChgFilePtr( handle, 0L, SEEK_END, &dummy );
        if( rc ) {
            _ReleaseFileH( handle );
            return( __set_errno_dos( rc ) );
        }
#else
        rc = TinySeek( handle, 0L, SEEK_END );
        if( TINY_ERROR( rc ) ) {
            _ReleaseFileH( handle );
            return( __set_errno_dos( TINY_INFO( rc ) ) );
        }
#endif
    }
#ifdef DEFAULT_WINDOWING
    if( _WindowsStdout != NULL ) {
        LPWDATA res;

        res = _WindowsIsWindowedHandle( handle );
        if( res != NULL ) {
            int rt;
            rt = _WindowsStdout( res, buffer, len );
            if( atomic == 1 ) {
                _ReleaseFileH( handle );
            }
            return( rt );
        }
    }
#endif
#if defined(__NT__)
    if( !WriteFile( h, buffer, len, &len_written, NULL ) ) {
        error = GetLastError();
        if( atomic == 1 ) {
            _ReleaseFileH( handle );
        }
        return( __set_errno_dos( error ) );
    }
#elif defined(__OS2__)
    rc = DosWrite( handle, (PVOID)buffer, len, &len_written );
    if( rc ) {
        if( atomic == 1 ) {
            _ReleaseFileH( handle );
        }
        return( __set_errno_dos( rc ) );
    }
#else
  #if defined(__WINDOWS_386__)
    rc = __TinyWrite( handle, buffer, len );
  #else
    rc = TinyWrite( handle, buffer, len );
  #endif
    if( TINY_ERROR( rc ) ) {
        if( atomic == 1 ) {
            _ReleaseFileH( handle );
        }
        return( __set_errno_dos( TINY_INFO( rc ) ) );
    }
    len_written = TINY_LINFO( rc );
#endif
    if( len_written != len ) {
        _RWD_errno = ENOSPC;
    }
    if( atomic == 1 ) {
        _ReleaseFileH( handle );
    }
    return( len_written );
}
