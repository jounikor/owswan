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
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <stddef.h>
#include <process.h>
#include <windows.h>
#include "rtdata.h"
#include "libwin32.h"
#include "osver.h"
#include "seterrno.h"
#include "_process.h"


int __F_NAME(_dospawn,_wdospawn)( int mode, CHAR_TYPE *pgmname, CHAR_TYPE *cmdline,
                                  CHAR_TYPE *envp, const CHAR_TYPE * const argv[] )
{
    STARTUPINFO         sinfo;
    PROCESS_INFORMATION pinfo;
    DWORD               rc;
    BOOL                osrc;

    __F_NAME(__ccmdline,__wccmdline)( pgmname, argv, cmdline, 0 );

    memset( &sinfo, 0, sizeof( sinfo ) );
    sinfo.cb = sizeof( sinfo );
    // set ShowWindow default value for nCmdShow parameter
    sinfo.dwFlags = STARTF_USESHOWWINDOW;
    if( mode == P_DETACH ) {
        sinfo.wShowWindow = SW_HIDE;
    } else {
        sinfo.wShowWindow = SW_SHOWNORMAL;
    }
    /* When passing in Unicode environments, the OS may not know which code
     * page to use when translating to MBCS in spawned program's startup
     * code.  Result: Possible corruption of Unicode environment variables.
     */
    osrc = __lib_CreateProcess( cmdline, (mode != P_DETACH), envp, &sinfo, &pinfo );

    if( osrc == FALSE ) {
        DWORD err;
        err = GetLastError();
        if( (err == ERROR_ACCESS_DENIED)
         || (err == ERROR_BAD_EXE_FORMAT)
         || (err == ERROR_BAD_PATHNAME) ) {
            err = ERROR_FILE_NOT_FOUND;
        }
        return( __set_errno_dos( err ) );
    }

    if( mode == P_WAIT ) {
        if( WIN32_IS_WIN32S ) {
            // this is WIN32s
            Sleep( 1000 );
            rc = STILL_ACTIVE;
            while( rc == STILL_ACTIVE ) {
                Sleep( 100 );
                if( !GetExitCodeProcess( pinfo.hProcess, &rc ) ) {
                    return( __set_errno_nt() );
                }
            }
        } else {
            /* this is WIN32 or Windows95 */
            if( WaitForSingleObject( pinfo.hProcess, INFINITE ) == 0 ) {
                GetExitCodeProcess( pinfo.hProcess, &rc );
            } else {
                rc = __set_errno_nt();
            }
        }
        CloseHandle( pinfo.hProcess );
    } else if( mode == P_DETACH ) {
        /* P_DETACH should just return 0 for success */
        rc = 0;
    } else {
        /* no difference between P_NOWAIT and P_NOWAITO */
        rc = (int)pinfo.hProcess;
        /* cwait will close (free) the handle */
    }
    CloseHandle( pinfo.hThread );
    return( rc );
}
