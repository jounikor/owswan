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
* Description:  Win32 main routines for executables and DLLs.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>
#include <time.h>
#include <io.h>
#ifdef _M_IX86
 #include <i86.h>
#endif
#include <windows.h>
#include "rtdata.h"
#include "rtstack.h"
#include "stacklow.h"
#include "iomode.h"
#include "strdup.h"
#include "liballoc.h"
#include "libwin32.h"
#include "ntext.h"
#include "initfini.h"
#include "rtinit.h"
#include "initarg.h"
#include "snglthrd.h"
#include "thread.h"
#include "mthread.h"
#include "fileacc.h"
#include "heapacc.h"
#include "trdlstac.h"
#include "cinit.h"
#include "osmainin.h"
#include "procfini.h"
#include "exitwmsg.h"


DWORD __TlsIndex = NO_INDEX;

_WCRTLINK int *__threadid( void )
{
    return( (int *)&(__THREADDATAPTR->thread_id) );
}

static void __NullAccessRtn( int handle )
{
    handle = handle;
}

static void __NullExitRtn() {}

static void __NullAccIOBRtn(void) {}
static void __NullAccHeapRtn(void) {}
static void __NullAccTDListRtn(void) {}

void    (*_AccessFileH)(int)     = &__NullAccessRtn;
void    (*_ReleaseFileH)(int)    = &__NullAccessRtn;
void    (*_AccessIOB)(void)      = &__NullAccIOBRtn;
void    (*_ReleaseIOB)(void)     = &__NullAccIOBRtn;
void    (*_AccessNHeap)(void)    = &__NullAccHeapRtn;
void    (*_AccessFHeap)(void)    = &__NullAccHeapRtn;
void    (*_ReleaseNHeap)(void)   = &__NullAccHeapRtn;
void    (*_ReleaseFHeap)(void)   = &__NullAccHeapRtn;
void    (*_AccessTDList)(void)   = &__NullAccTDListRtn;
void    (*_ReleaseTDList)(void)  = &__NullAccTDListRtn;
void    (*_AccessFList)(void)    = &__NullAccIOBRtn;
void    (*_ReleaseFList)(void)   = &__NullAccIOBRtn;
void    (*_ThreadExitRtn)(void)  = &__NullExitRtn;

static void __sig_null_rtn(void) {}

_WCRTDATA void  (*__sig_init_rtn)(void) = __sig_null_rtn;
_WCRTDATA void  (*__sig_fini_rtn)(void) = __sig_null_rtn;

#ifdef _M_IX86
 #pragma aux _end "*"
#endif
extern  char            _end;

int                     __Is_DLL;       /* TRUE => DLL, else not a DLL */
static char             *_cmd_ptr;
static wchar_t          *_wcmd_ptr;

// called once at DLL_PROCESS_ATTACH or by __NTMainInit

typedef WINBASEAPI LPCH WINAPI (*GETENVPROC)( VOID );

int __NTInit( int is_dll, thread_data *tdata, HANDLE hdll )
{
    DWORD       ver;
    WORD        os_ver;

    __Is_DLL = is_dll;                                  /* 15-feb-93 */
    // tdata is guaranteed to never be NULL. If starting up for an EXE,
    // it's pointing on the stack (alloca). If this is run on behalf of
    // a DLL startup, it's already NULL checked by the caller.
    __FirstThreadData = tdata;
    __initPOSIXHandles();

    /*
     * Force reference to environ so that __setenvp is linked in; hence,
     * __ParsePosixHandleStr will be called.
     */
    environ = NULL;

    // get/init Windows version info
    ver = GetVersion();
    os_ver = LOWORD( ver );
    _RWD_osmajor = os_ver & 0xff;
    _RWD_osminor = (os_ver >> 8) & 0xff;
    _RWD_osbuild = HIWORD( ver );
    _RWD_osver = _RWD_osbuild;
    _RWD_winmajor = _RWD_osmajor;
    _RWD_winminor = _RWD_osminor;
    _RWD_winver = (_RWD_winmajor << 8) | _RWD_winminor;

    /* WIN32 API GetEnvironmentStrings and FreeEnvironmentStrings functions have strange history.
     * In early versions of Windows NT it was available only as GetEnvironmentStrings
     * entry point (multi-byte version) and FreeEnvironmentStrings was missing.
     * Later it was fixed to have both versions of GetEnvironmentStrings API entry
     * (multi-byte and wide versions) GetEnvironmentStringsA/GetEnvironmentStringsW and
     * FreeEnvironmentStringsA/FreeEnvironmentStringsW were added too.
     * This problem affected only some versions of Windows NT 3.x and Win32s.
     * Due to this mess it is necessary check presence of appropriate entry point in WIN32
     * DLL on run-time.
     */

    /* NOTES:
     * Win32s V1.20 and later has GetEnvironmentStringsA available but return NULL.
     * Win32s V1.15 and earlier does not have GetEnvironmentStringsA entry point at all.
     * NT 3.1 and earlier has only GetEnvironmentStrings entry point.
     * NT 3.51 and later has all GetEnvironmentStrings/GetEnvironmentStringsA/GetEnvironmentStringsW
     * entry points.
     * For Win32s and NT 3.x GetEnvironmentStrings should be used to get environment block.
     * For all other versions GetEnvironmentStringsA should be used to get environment block.
     */

    {
        GETENVPROC  _getenvs;
        char        *name;

        if( _RWD_osmajor < 4 ) {
            /* for NT 3.x and Win32s use GetEnvironmentStrings */
            name = "GetEnvironmentStrings";
        } else {
            /* for Windows 9x and NT ( 4.0 and above ) use GetEnvironmentStringsA */
            name = "GetEnvironmentStringsA";
        }
        _Envptr = NULL;
        _getenvs = (GETENVPROC)GetProcAddress( GetModuleHandle( "KERNEL32.DLL" ), name );
        if( _getenvs != NULL ) {
            _Envptr = _getenvs();
        }
    }

    {
        static char     fn[_MAX_PATH];
        __lib_GetModuleFileNameA( NULL, fn, sizeof( fn ) );
        _LpPgmName = fn;
    }
    {
        static wchar_t  wfn[_MAX_PATH];
        __lib_GetModuleFileNameW( NULL, wfn, sizeof( wfn ) );
        _LpwPgmName = wfn;
    }

    {
        char    *cmd;
        _cmd_ptr = cmd = __clib_strdup( GetCommandLineA() );
        if( *cmd == '"' ) {
            cmd++;
            while( *cmd != '"' && *cmd != '\0' ) {
                cmd++;
            }
            if( *cmd ) cmd++;
        } else {
            while( !isspace( (unsigned char)*cmd ) && *cmd != '\0' ) {
                cmd++;
            }
        }
        while( isspace( (unsigned char)*cmd ) ) {
            cmd++;
        }
        _LpCmdLine = cmd;
    }
    {
        wchar_t *wcmd;
        wcmd = GetCommandLineW();       /* Win95 supports GetCommandLineW */
        if( wcmd != NULL ) {
            _wcmd_ptr = wcmd = __clib_wcsdup( wcmd );
            if( *wcmd == L'"' ) {
                wcmd++;
                while( *wcmd != L'"' && *wcmd != L'\0' ) {
                    wcmd++;
                }
                if( *wcmd ) wcmd++;
            } else {
                while( !iswspace( *wcmd ) && *wcmd != L'\0' ) {
                    wcmd++;
                }
            }
            while( iswspace( *wcmd ) ) {
                wcmd++;
            }
        } else {
            wcmd = L"";
        }
        _LpwCmdLine = wcmd;
    }

    if( is_dll ) {
        {
            static char    fn[_MAX_PATH];
            __lib_GetModuleFileNameA( hdll, fn, sizeof( fn ) );
            _LpDllName = fn;
        }
        {
            static wchar_t wfn[_MAX_PATH];
            __lib_GetModuleFileNameW( hdll, wfn, sizeof( wfn ) );
            _LpwDllName = wfn;
        }
    }

    return( TRUE );
}

typedef WINBASEAPI BOOL WINAPI (*FREEENVPROC)( LPCH );

void __NTFini( void )
{
    // calls to free memory have to be done before semaphores closed
    if( _cmd_ptr ) {
        lib_free( _cmd_ptr );
        _cmd_ptr = NULL;
    }
    if( _wcmd_ptr ) {
        lib_free( _wcmd_ptr );
        _wcmd_ptr = NULL;
    }
    /* NOTES:
     * FreeEnvironmentStringsA entry point is not available for earlier versions of Win32s and
     * for NT 3.1 and earlier. On all other Windows versions this entry is available.
     * (see comment above in __NTInit)
     */

    if( _Envptr != NULL ) {
        FREEENVPROC _freeenvs;

        _freeenvs = (FREEENVPROC)GetProcAddress( GetModuleHandle( "KERNEL32.DLL" ), "FreeEnvironmentStringsA" );
        if( _freeenvs != NULL ) {
            _freeenvs( _Envptr );
        }
        _Envptr = NULL;
    }
}

void __NTMainInit( REGISTRATION_RECORD *rr, thread_data *tdata )
{
    __DefaultExceptionHandler();
    __NTInit( FALSE, tdata, GetModuleHandle(NULL) );
    __init_stack_limits( &_STACKLOW, &_STACKTOP );
    __NewExceptionFilter( rr );
    __InitRtns( INIT_PRIORITY_LIBRARY+1 );
    __sig_init_rtn();
    __InitRtns( 255 );
}

_WCRTDATA void (*__process_fini)(unsigned,unsigned) = NULL;

_WCRTLINK _WCNORETURN void __exit( int ret_code )
{
    __NTFini(); // must be done before following finalizers get called
    if( __Is_DLL ) {
        if( __process_fini != NULL ) {
            (*__process_fini)( 0, FINI_PRIORITY_EXIT-1 );
        }
    } else {
        __DoneExceptionFilter();
        __FiniRtns( 0, FINI_PRIORITY_EXIT-1 );
        (*_ThreadExitRtn)();
    }
    // Also gets done by __FreeThreadDataList which is activated from FiniSema4s
    // for multi-threaded apps
    __FirstThreadData = NULL;
    ExitProcess( ret_code );
    // never return
}
