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
* Description:  RCS DLL loading and function import for vi.
*
****************************************************************************/


#include "vi.h"
#include "rcscli.h"
#include "rcs.h"
#include "winexprt.h"


/* Local Windows CALLBACK function prototypes */
#if defined( VI_RCS )
#if defined( __WINDOWS_386__ )
WINEXPORT int RCSAPI Batcher( const char *cmd, void *cookie );
#else
WINEXPORT int RCSAPI Batcher( rcsstring cmd, void *cookie );
#endif
#endif

#if defined( __WINDOWS_386__ )

#define _RCSGetVersion
#define _RCSInit(w,c)                       (rcsdata)_Call16( _16RCSInit, "dp", w, c )
#define _RCSCheckout(d,n,p,t)               (WORD)_Call16( _16RCSCheckout, "dppp", d, n, p, t )
#define _RCSCheckin(d,n,p,t)                (WORD)_Call16( _16RCSCheckin, "dppp", d, n, p, t )
#define _RCSHasShell
#define _RCSRunShell
#define _RCSSetSystem
#define _RCSQuerySystem(d)                  (WORD)_Call16( _16RCSQuerySystem, "d", d )
#define _RCSRegisterBatchCallback(d,f,c)    _Call16( _16RCSRegisterBatchCallback, "ddp", d, f, c )
#define _RCSRegisterMessageBoxCallback
#define _RCSSetPause(d,o)                   _Call16( _16RCSSetPause, "dw", d, (WORD)o )
#define _RCSFini(d)                         _Call16( _16RCSFini, "d", d )

#define _ReleaseProc(p)                     ReleaseProc16(p)

#else

#define _RCSGetVersion                      RCSGetVersion
#define _RCSInit                            RCSInit
#define _RCSCheckout                        RCSCheckout
#define _RCSCheckin                         RCSCheckin
#define _RCSHasShell                        RCSHasShell
#define _RCSRunShell                        RCSRunShell
#define _RCSSetSystem                       RCSSetSystem
#define _RCSQuerySystem                     RCSQuerySystem
#define _RCSRegisterBatchCallback           RCSRegisterBatchCallback
#define _RCSRegisterMessageBoxCallback      RCSRegisterMessageBoxCallback
#define _RCSSetPause                        RCSSetPause
#define _RCSFini                            RCSFini

#define _ReleaseProc(p)                     FreeProcInstance(p)

#endif

/* function pointers */

#if defined( __WINDOWS_386__ )

static FARPROC                              _16RCSGetVersion = NULL;
static FARPROC                              _16RCSInit = NULL;
static FARPROC                              _16RCSCheckout = NULL;
static FARPROC                              _16RCSCheckin = NULL;
static FARPROC                              _16RCSHasShell = NULL;
static FARPROC                              _16RCSRunShell = NULL;
static FARPROC                              _16RCSSetSystem = NULL;
static FARPROC                              _16RCSQuerySystem = NULL;
static FARPROC                              _16RCSRegisterBatchCallback = NULL;
static FARPROC                              _16RCSRegisterMessageBoxCallback = NULL;
static FARPROC                              _16RCSSetPause = NULL;
static FARPROC                              _16RCSFini = NULL;

#elif defined( __WINDOWS__ ) || defined( __NT__ ) || defined( __OS2__ )

extern RCSGetVersionFn                      *RCSGetVersion = NULL;
extern RCSInitFn                            *RCSInit = NULL;
extern RCSCheckoutFn                        *RCSCheckout = NULL;
extern RCSCheckinFn                         *RCSCheckin = NULL;
extern RCSHasShellFn                        *RCSHasShell = NULL;
extern RCSRunShellFn                        *RCSRunShell = NULL;
extern RCSSetSystemFn                       *RCSSetSystem = NULL;
extern RCSQuerySystemFn                     *RCSQuerySystem = NULL;
extern RCSRegBatchCbFn                      *RCSRegisterBatchCallback = NULL;
extern RCSRegMsgBoxCbFn                     *RCSRegisterMessageBoxCallback = NULL;
extern RCSSetPauseFn                        *RCSSetPause = NULL;
extern RCSFiniFn                            *RCSFini = NULL;

#endif

#if defined( __WINDOWS__ )

static HINSTANCE LibHandle = (HINSTANCE)0;

bool ViRCSInit( void )
{
    UINT    uErrMode;

    /* Use SetErrorMode to prevent annoying error popups. */
    uErrMode = SetErrorMode( SEM_NOOPENFILEERRORBOX );
    LibHandle = LoadLibrary( RCS_DLLNAME );
    SetErrorMode( uErrMode );
    if( LibHandle < (HINSTANCE)32 ) {
        LibHandle = (HINSTANCE)0;
        return( false );
    }
  #if defined( __WINDOWS_386__ )
    _16RCSGetVersion = (FARPROC)GetProcAddress( LibHandle, GETVER_FN_NAME );
    _16RCSInit = (FARPROC)GetProcAddress( LibHandle, INIT_FN_NAME );
    _16RCSCheckout = (FARPROC)GetProcAddress( LibHandle, CHECKOUT_FN_NAME );
    _16RCSCheckin = (FARPROC)GetProcAddress( LibHandle, CHECKIN_FN_NAME );
    _16RCSHasShell = (FARPROC)GetProcAddress( LibHandle, HAS_SHELL_FN_NAME );
    _16RCSRunShell = (FARPROC)GetProcAddress( LibHandle, RUNSHELL_FN_NAME );
    _16RCSSetSystem = (FARPROC)GetProcAddress( LibHandle, SETSYS_FN_NAME );
    _16RCSQuerySystem = (FARPROC)GetProcAddress( LibHandle, GETSYS_FN_NAME );
    _16RCSRegisterBatchCallback = (FARPROC)GetProcAddress( LibHandle, REG_BAT_CB_FN_NAME );
    _16RCSRegisterMessageBoxCallback = (FARPROC)GetProcAddress( LibHandle, REG_MSGBOX_CB_FN_NAME );
    _16RCSSetPause = (FARPROC)GetProcAddress( LibHandle, SET_PAUSE_FN_NAME );
    _16RCSFini = (FARPROC)GetProcAddress( LibHandle, FINI_FN_NAME );
  #else
    RCSGetVersion = (RCSGetVersionFn *)GetProcAddress( LibHandle, GETVER_FN_NAME );
    RCSInit = (RCSInitFn *)GetProcAddress( LibHandle, INIT_FN_NAME );
    RCSCheckout = (RCSCheckoutFn *)GetProcAddress( LibHandle, CHECKOUT_FN_NAME );
    RCSCheckin = (RCSCheckinFn *)GetProcAddress( LibHandle, CHECKIN_FN_NAME );
    RCSHasShell = (RCSHasShellFn *)GetProcAddress( LibHandle, HAS_SHELL_FN_NAME );
    RCSRunShell = (RCSRunShellFn *)GetProcAddress( LibHandle, RUNSHELL_FN_NAME );
    RCSSetSystem = (RCSSetSystemFn *)GetProcAddress( LibHandle, SETSYS_FN_NAME );
    RCSQuerySystem = (RCSQuerySystemFn *)GetProcAddress( LibHandle, GETSYS_FN_NAME );
    RCSRegisterBatchCallback = (RCSRegBatchCbFn *)GetProcAddress( LibHandle, REG_BAT_CB_FN_NAME );
    RCSRegisterMessageBoxCallback = (RCSRegMsgBoxCbFn *)GetProcAddress( LibHandle, REG_MSGBOX_CB_FN_NAME );
    RCSSetPause = (RCSSetPauseFn *)GetProcAddress( LibHandle, SET_PAUSE_FN_NAME );
    RCSFini = (RCSFiniFn *)GetProcAddress( LibHandle, FINI_FN_NAME );
  #endif
  #if defined( __WINDOWS_386__ )
    RCSActive = !( _16RCSInit == NULL || _16RCSRegisterBatchCallback == NULL || _16RCSQuerySystem == NULL
                     || _16RCSSetPause == NULL || _16RCSCheckout == NULL || _16RCSFini == NULL );
  #else
    RCSActive = !( RCSInit == NULL || RCSRegisterBatchCallback == NULL || RCSQuerySystem == NULL
                     || RCSSetPause == NULL || RCSCheckout == NULL || RCSFini == NULL );
  #endif
    if( !RCSActive ) {
        ViRCSFini();
        return( false );
    }
    return( true );
}

bool ViRCSFini( void )
{
    if( LibHandle != (HINSTANCE)0 )
        FreeLibrary( LibHandle );
    return( true );
}

#elif defined( __NT__ )

static HINSTANCE LibHandle = NULL;

bool ViRCSInit( void )
{
    UINT    uErrMode;

    /* Use SetErrorMode to prevent annoying error popups. */
    uErrMode = SetErrorMode( SEM_NOOPENFILEERRORBOX );
    LibHandle = LoadLibrary( RCS_DLLNAME );
    SetErrorMode( uErrMode );
    if( LibHandle == NULL ) {
        return( false );
    }
    RCSGetVersion = (RCSGetVersionFn *)GetProcAddress( LibHandle, GETVER_FN_NAME );
    RCSInit = (RCSInitFn *)GetProcAddress( LibHandle, INIT_FN_NAME );
    RCSCheckout = (RCSCheckoutFn *)GetProcAddress( LibHandle, CHECKOUT_FN_NAME );
    RCSCheckin = (RCSCheckinFn *)GetProcAddress( LibHandle, CHECKIN_FN_NAME );
    RCSHasShell = (RCSHasShellFn *)GetProcAddress( LibHandle, HAS_SHELL_FN_NAME );
    RCSRunShell = (RCSRunShellFn *)GetProcAddress( LibHandle, RUNSHELL_FN_NAME );
    RCSSetSystem = (RCSSetSystemFn *)GetProcAddress( LibHandle, SETSYS_FN_NAME );
    RCSQuerySystem = (RCSQuerySystemFn *)GetProcAddress( LibHandle, GETSYS_FN_NAME );
    RCSRegisterBatchCallback = (RCSRegBatchCbFn *)GetProcAddress( LibHandle, REG_BAT_CB_FN_NAME );
    RCSRegisterMessageBoxCallback = (RCSRegMsgBoxCbFn *)GetProcAddress( LibHandle, REG_MSGBOX_CB_FN_NAME );
    RCSSetPause = (RCSSetPauseFn *)GetProcAddress( LibHandle, SET_PAUSE_FN_NAME );
    RCSFini = (RCSFiniFn *)GetProcAddress( LibHandle, FINI_FN_NAME );

    RCSActive = !( RCSInit == NULL || RCSRegisterBatchCallback == NULL
                    || RCSSetPause == NULL || RCSCheckin == NULL || RCSFini == NULL );

    if( !RCSActive ) {
        ViRCSFini();
        return( false );
    }
    return( true );
}

bool ViRCSFini( void )
{
    if( LibHandle != NULL )
        FreeLibrary( LibHandle );
    return( true );
}

#elif defined( __OS2__ )

#ifdef _M_I86
#define GET_PROC_ADDRESS(m,s,f) DosGetProcAddr( m, s, (PFN FAR *)&f )
#else
#define GET_PROC_ADDRESS(m,s,f) DosQueryProcAddr( m, 0, s, (PFN FAR *)&f )
#endif

static HMODULE LibHandle = (HMODULE)0;

bool ViRCSInit( void )
{
    #define BUFF_LEN 128
    char    fail_name[BUFF_LEN];
    int     rc;

    rc = DosLoadModule( fail_name, BUFF_LEN, RCS_DLLNAME, &LibHandle );
    if( rc != 0 || LibHandle == 0 ) {
        return( false );
    }
    GET_PROC_ADDRESS( LibHandle, GETVER_FN_NAME,        RCSGetVersion );
    GET_PROC_ADDRESS( LibHandle, INIT_FN_NAME,          RCSInit );
    GET_PROC_ADDRESS( LibHandle, CHECKOUT_FN_NAME,      RCSCheckout );
    GET_PROC_ADDRESS( LibHandle, CHECKIN_FN_NAME,       RCSCheckin );
    GET_PROC_ADDRESS( LibHandle, HAS_SHELL_FN_NAME,     RCSHasShell );
    GET_PROC_ADDRESS( LibHandle, RUNSHELL_FN_NAME,      RCSRunShell );
    GET_PROC_ADDRESS( LibHandle, SETSYS_FN_NAME,        RCSSetSystem );
    GET_PROC_ADDRESS( LibHandle, GETSYS_FN_NAME,        RCSQuerySystem );
    GET_PROC_ADDRESS( LibHandle, REG_BAT_CB_FN_NAME,    RCSRegisterBatchCallback );
    GET_PROC_ADDRESS( LibHandle, REG_MSGBOX_CB_FN_NAME, RCSRegisterMessageBoxCallback );
    GET_PROC_ADDRESS( LibHandle, SET_PAUSE_FN_NAME,     RCSSetPause );
    GET_PROC_ADDRESS( LibHandle, FINI_FN_NAME,          RCSFini );

    RCSActive = !( RCSInit == NULL || RCSRegisterBatchCallback == NULL
                    || RCSSetPause == NULL || RCSCheckin == NULL || RCSFini == NULL );

    if( !RCSActive ) {
        ViRCSFini();
        return( false );
    }
    return( true );
}

bool ViRCSFini( void )
{
    DosFreeModule( LibHandle );
    return( true );
}

#else

bool ViRCSInit( void )
{
    RCSActive = true;
    return( true );
}

bool ViRCSFini( void )
{
    return( true );
}

#endif

#if defined( VI_RCS )

  #if defined( __WINDOWS_386__ )

WINEXPORT int RCSAPI Batcher( const char *cmd, void *cookie )
{
    char    *p;
    size_t  len;
    int     rc;

    cookie = cookie;
    len = _fstrlen( MK_FP32( (void *)cmd ) );
    p = MemAlloc( len + 1 );
    _fmemcpy( p, MK_FP32( (void *)cmd ), len + 1 );
    rc = ( ExecCmd( NULL, NULL, p ) == 0 );
    MemFree( p );
    return( rc );
}

  #else

BatchCallback    Batcher;

WINEXPORT int RCSAPI Batcher( rcsstring cmd, void *cookie )
{
    cookie = cookie;
    return( ExecCmd( NULL, NULL, cmd ) == 0 );
}

  #endif

vi_rc ViRCSCheckout( vi_rc rc )
{
    rcsdata r;
  #ifdef __WIN__
    FARPROC fp;
  #endif

  #if defined( __WIN__ ) && defined( __WINDOWS_386__ )
    r = _RCSInit( root_window_id, getenv( "WATCOM" ) );
    DefineUserProc16( GETPROC_USERDEFINED_1, (PROCPTR)Batcher, UDP16_PTR, UDP16_PTR, UDP16_ENDLIST );
    fp = GetProc16( (PROCPTR)Batcher, GETPROC_USERDEFINED_1 );
    _RCSRegisterBatchCallback( r, fp, NULL );
  #elif defined( __WIN__ )
    r = _RCSInit( root_window_id, getenv( "WATCOM" ) );
    fp = MakeProcInstance( (FARPROC)Batcher, InstanceHandle );
    _RCSRegisterBatchCallback( r, (BatchCallback *)fp, NULL );
  #else
    r = _RCSInit( 0, getenv( "WATCOM" ) );
    RCSRegisterBatchCallback( r, Batcher, NULL );
  #endif
    if( _RCSQuerySystem( r ) != NO_RCS ) {
        if( GenericQueryBool( "File is read only, check out?" ) ) {
            char full1[FILENAME_MAX];

            _fullpath( full1, CurrentFile->name, FILENAME_MAX );
            _RCSSetPause( r, true );
            if( _RCSCheckout( r, full1, NULL, NULL ) ) {
                rc = ERR_NO_ERR;
                EditRCSCurrentFile();
            }
        }
    }
  #ifdef __WIN__
    _ReleaseProc( fp );
  #endif
    _RCSFini( r );
    return( rc );
}

vi_rc ViRCSCheckin( vi_rc rc )
{
    rcsdata r;
  #ifdef __WIN__
    FARPROC fp;
  #endif

  #if defined( __WIN__ ) && defined( __WINDOWS_386__ )
    r = _RCSInit( root_window_id, getenv( "WATCOM" ) );
    DefineUserProc16( GETPROC_USERDEFINED_1, (PROCPTR)Batcher, UDP16_PTR, UDP16_PTR, UDP16_ENDLIST );
    fp = GetProc16( (PROCPTR)Batcher, GETPROC_USERDEFINED_1 );
    _RCSRegisterBatchCallback( r, fp, NULL );
  #elif defined( __WIN__ )
    r = _RCSInit( root_window_id, getenv( "WATCOM" ) );
    fp = MakeProcInstance( (FARPROC)Batcher, InstanceHandle );
    _RCSRegisterBatchCallback( r, (BatchCallback *)fp, NULL );
  #else
    r = _RCSInit( 0, getenv( "WATCOM" ) );
    RCSRegisterBatchCallback( r, Batcher, NULL );
  #endif
    _RCSSetPause( r, true );
    if( CurrentFile->modified ) {
        FilePromptForSaveChanges( CurrentFile );
    }
    if( _RCSCheckin( r, CurrentFile->name, NULL, NULL ) ) {
        rc = ERR_NO_ERR;
        EditRCSCurrentFile();
    }
  #ifdef __WIN__
    _ReleaseProc( fp );
  #endif
    _RCSFini( r );
    return( rc );
}

#endif
