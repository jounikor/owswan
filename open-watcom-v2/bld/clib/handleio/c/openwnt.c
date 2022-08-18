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
* Description:  Win32 implementation of open() and _sopen().
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>
#include <windows.h>
#include "rtdata.h"
#include "rtumask.h"
#include "iomode.h"
#include "fileacc.h"
#include "ntext.h"
#include "openmode.h"
#include "libwin32.h"
#include "seterrno.h"
#include "defwin.h"


static int __F_NAME(__sopen,__wsopen)( const CHAR_TYPE *name, unsigned mode, unsigned share, va_list args )
{
    DWORD               create_disp, exists_disp;
    mode_t              perm;
    DWORD               os_attr, fileattr;
    DWORD               desired_access, share_mode;
    SECURITY_ATTRIBUTES security;
    HANDLE              handle;
    int                 hid;
    unsigned            rwmode;
    unsigned            iomode_flags;

    // First try to get the required slot.
    // No point in creating a file only to not use it.  JBS 99/10/26
    hid = __allocPOSIXHandleDummy();
    if( hid == -1 ) {
        return( -1 );
    }

    rwmode = mode & OPENMODE_ACCESS_MASK;
    __GetNTAccessAttr( rwmode, &desired_access, &os_attr );
    __GetNTShareAttr( share | rwmode, &share_mode );
    fileattr = FILE_ATTRIBUTE_NORMAL;

    security.nLength = sizeof( SECURITY_ATTRIBUTES );
    security.lpSecurityDescriptor = NULL;
    security.bInheritHandle = ( mode & O_NOINHERIT ) ? FALSE : TRUE;

#ifdef DEFAULT_WINDOWING
    if( _WindowsNewWindow != NULL && !__F_NAME(_stricmp,_wcsicmp)( name, CHAR_CONST( "con" ) ) )
    {
        handle = __NTGetFakeHandle();

        // Now use the slot we got.
        __setOSHandle( hid, handle );   // JBS 99/11/01
        _WindowsNewWindow( NULL, hid, -1 );

        iomode_flags = _ISTTY;
    } else {
#endif
        if( mode & O_CREAT ) {
            perm = va_arg( args, int );
            perm &= ~_RWD_umaskval;             /* 05-jan-95 */
            if( ( perm & S_IREAD ) && !( perm & S_IWRITE ) ) {
                fileattr = FILE_ATTRIBUTE_READONLY;
            }
            if( mode & O_EXCL ) {
                create_disp = CREATE_NEW;
                exists_disp = CREATE_NEW;
            } else if( mode & O_TRUNC ) {
                create_disp = CREATE_ALWAYS;
                exists_disp = CREATE_NEW;
            } else {
                create_disp = OPEN_ALWAYS;
                exists_disp = OPEN_EXISTING;
            }
        } else if( mode & O_TRUNC ) {
            exists_disp = TRUNCATE_EXISTING;
        } else {
            exists_disp = OPEN_EXISTING;
        }

        /*** Open the file ***/
        handle = __lib_CreateFile( name, desired_access, share_mode, &security,
                                        exists_disp, fileattr, NULL );
        if( handle == INVALID_HANDLE_VALUE ) {
            if( mode & O_CREAT ) {
                handle = __lib_CreateFile( name, desired_access, share_mode, NULL,
                                            create_disp, fileattr, NULL );
            }
            if( handle == INVALID_HANDLE_VALUE ) {
                __freePOSIXHandle( hid );
                return( __set_errno_nt() );
            }
        }

        // Now use the slot we got.
        __setOSHandle( hid, handle );   // JBS 99/11/01

        iomode_flags = 0;

        if( isatty( hid ) ) {
            iomode_flags = _ISTTY;
        }
#ifdef DEFAULT_WINDOWING
    }
#endif

    if( rwmode == O_RDWR )       iomode_flags |= _READ | _WRITE;
    else if( rwmode == O_RDONLY) iomode_flags |= _READ;
    else if( rwmode == O_WRONLY) iomode_flags |= _WRITE;
    if( mode & O_APPEND )        iomode_flags |= _APPEND;
    if( mode & (O_BINARY|O_TEXT) ) {
        if( mode & O_BINARY )    iomode_flags |= _BINARY;
    } else {
        if( _RWD_fmode == O_BINARY ) iomode_flags |= _BINARY;
    }
    __SetIOMode( hid, iomode_flags );
    return( hid );
}


_WCRTLINK int __F_NAME(open,_wopen)( const CHAR_TYPE *name, int mode, ... )
{
    mode_t      permission;
    va_list     args;

    va_start( args, mode );
    permission = va_arg( args, int );
    va_end( args );
    return( __F_NAME(_sopen,_wsopen)( name, mode, SH_COMPAT, permission ) );
}


_WCRTLINK int __F_NAME(_sopen,_wsopen)( const CHAR_TYPE *name, int mode, int shflag, ... )
{
    va_list     args;
    int         ret;

    va_start( args, shflag );
    ret = __F_NAME(__sopen,__wsopen)( name, mode, shflag, args );
    va_end( args );
    return( ret );
}
