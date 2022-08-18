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
* Description:  OS/2 32-bit main routines for executables and DLLs.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <io.h>
#include <i86.h>
#include <string.h>
#define INCL_DOSMISC
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#include <wos2.h>
#include "rtdata.h"
#include "rtstack.h"
#include "stacklow.h"
#include "exitwmsg.h"
#include "initfini.h"
#include "rtinit.h"
#include "liballoc.h"
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
#include "itsto32.h"
#include "trdlist.h"
#include "rtexcpt.h"


extern unsigned         __hmodule;
unsigned short          __saved_CS;

static void __NullAccessRtn( int hdl ) { hdl = hdl; }
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

#if defined(_M_IX86)
extern  unsigned short  GetCS( void );
#pragma aux GetCS = "mov ax,cs" __value [__ax] __modify __exact [__ax]
extern  unsigned short  GetFS( void );
#pragma aux GetFS = "mov ax,fs" __value [__ax] __modify __exact [__ax]
extern  int     *GetTIDp( void );
#pragma aux GetTIDp = "mov eax,fs:[12]" __value [__eax] __modify __exact [__eax]
extern  unsigned GetThreadStack(void);
#pragma aux GetThreadStack = "mov eax,fs:[4]" __value [__eax] __modify __exact [__eax]
#endif

unsigned __threadstack( void )
/****************************/
{
//    return( (unsigned)__TIBAddr()->tib_pstack );
    return( GetThreadStack() );
}

_WCRTLINK int *__threadid( void )
/*******************************/
{
//    return( (int *)(&__TIBAddr()->tib_ptib2->tib2_ultid) );
    return( GetTIDp() );
}

static  void    NullSigRtn( void ) {}

_WCRTDATA void  (*__sig_init_rtn)(void) = { NullSigRtn };
_WCRTDATA void  (*__sig_fini_rtn)(void) = { NullSigRtn };

extern  char            _end;
#if defined(_M_IX86)
#pragma aux _end "*"
#endif

int                     __Is_DLL;       /* TRUE => DLL, else not a DLL */

unsigned        __threadstksize = { 0 };

typedef struct sys_info {
    unsigned long       version_major;
    unsigned long       version_minor;
} sys_info;

void __OS2MainInit( EXCEPTIONREGISTRATIONRECORD *xcpt, void *ptr,
                    unsigned hmod, char *env, char *cmd )
/*******************************************************/
{
    thread_data                 *tdata;
    char                        *args;
    char                        *cmd_path;

    tdata = ptr;
    for( args = cmd; *args != '\0'; ++args ); /* skip over program name */
    ++args;

    _Envptr = env;
    _LpCmdLine = args;
#ifdef _UNICODE
    _LpwCmdLine = lib_malloc( (strlen( _LpCmdLine ) + 1) * sizeof( wchar_t ) );
    _atouni( _LpwCmdLine, _LpCmdLine );
#endif
    for( cmd_path = cmd - 2; *cmd_path != '\0'; --cmd_path );
    ++cmd_path;
    _LpPgmName = cmd_path;
#ifdef _UNICODE
    _LpwPgmName = lib_malloc( (strlen( _LpPgmName ) + 1) * sizeof( wchar_t ) );
    _atouni( _LpwPgmName, _LpPgmName );
#endif

    __hmodule = hmod;
    __OS2Init( FALSE, tdata );
    /*
      initializers must be executed before signals initialized since
      __sig_init_rtn may get set by an initializer
    */
    __InitRtns( INIT_PRIORITY_LIBRARY );
    __XCPTHANDLER = xcpt;
    __sig_init_rtn();
#ifndef __SW_BM
    _STACKLOW = (unsigned)&_end;
#endif
    __InitRtns( 255 );
}

// this definition needs to occur after __OS2MainInit() so that
// the #undef of _STACKLOW doesn't break things
void __OS2Init( int is_dll, thread_data *tdata )
/**********************************************/
{
    sys_info                    _sysinfo;

    __Is_DLL = is_dll;
    __FirstThreadData = tdata;

    DosQuerySysInfo( QSV_VERSION_MAJOR, QSV_VERSION_MINOR,
                     &_sysinfo, sizeof( sys_info )  );
    _RWD_osmajor = _sysinfo.version_major;
    _RWD_osminor = _sysinfo.version_minor;
    __saved_CS = GetCS();
#ifndef __SW_BM
    #undef _STACKLOW
    _STACKLOW = (unsigned)&_end;            // cortns in F77
#endif
}

void __OS2Fini( void )
/********************/
{
    // Thread data is either freed by the module that allocated it (for DLLs)
    // or not at all (for executables - allocated from stack). Here we just
    // make sure the pointer gets invalidated.
    __FirstThreadData = NULL;
}

_WCRTDATA void (*__process_fini)(unsigned,unsigned) = NULL;

_WCRTLINK _WCNORETURN void __exit( int ret_code )
{
    __OS2Fini(); // must be done before following finalizers get called
    if( __Is_DLL ) {
        if( __process_fini != NULL ) {
            (*__process_fini)( 0, FINI_PRIORITY_EXIT - 1 );
        }
    } else {
        __FiniRtns( 0, FINI_PRIORITY_EXIT - 1 );
        __shutdown_stack_checking();
    }

    DosExit( EXIT_PROCESS, ret_code );
    // never return
}
