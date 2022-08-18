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
* Description:  run-time system initialization
*
****************************************************************************/

#include "ftnstd.h"
#if defined( __NT__ )
    #include <windows.h>
#elif defined( __OS2__ )
    #define INCL_DOSPROCESS
    #include <wos2.h>
#endif
#include "rtstack.h"
#include "frtdata.h"
#include "fthread.h"
#include "xfflags.h"
#include "rundat.h"
#include "errcod.h"
#include "fapptype.h"
#include "rtinit.h"
#include "errrtns.h"
#include "_defwin.h"    /* for _WindowsStdout() declaration */
#include "thread.h"
#include "rttraps.h"
#include "rt_init.h"
#include "widechar.h"   /* C run-time library internal variable */
#include "initarg.h"    /* C run-time library internal variable */


#if defined( __WINDOWS__ )
  #if defined( __386__ )
    // so compile-generated symbol "__fthread_init" is defined
    // when we link a 32-bit Windows DLL
    #pragma aux         __fthread_init "*";
    char                __fthread_init = { 0 };
  #endif
  char          __FAppType = { FAPP_GUI };
#else
  char          __FAppType = { FAPP_CHARACTER_MODE };
#endif

void            (* _ExceptionInit)( void ) = { &R_TrapInit };
void            (* _ExceptionFini)( void ) = { &R_TrapFini };

static  char            RTSysInitialized = { 0 };

#ifdef __SW_BM

static  void    __NullFIOAccess( void ) {}

void            (*_AccessFIO)( void )         = &__NullFIOAccess;
void            (*_ReleaseFIO)( void )        = &__NullFIOAccess;
void            (*_PartialReleaseFIO)( void ) = &__NullFIOAccess;
unsigned        __FThreadDataOffset;


void    __InitFThreadData( void *td )
//==========================================
{
    fthread_data *ftd;

    ftd = THREADPTR2FTHREADPTR( td );

    // Must match __InitRTData().

    ftd->__ExCurr = NULL;
    ftd->__XceptionFlags = 0;
}

#else

volatile unsigned short __XcptFlags;

#endif


static  void    __InitRTData( void )
//==============================
{
    // Must match __InitFThreadData().

    _RWD_ExCurr = NULL;
    _RWD_XcptFlags = 0;
}


static void RTSysFini( void ) {
//=============================

    _ExceptionFini();
    // WATFOR-77 calls __ErrorFini() when it terminates
    __ErrorFini();
}

unsigned        RTSysInit( void ) {
//===========================

    if( RTSysInitialized )
        return( 0 );
#if defined( __OS2__ ) && defined( __386__ )
    {
        TIB     *ptib;
        PIB     *ppib;

        DosGetInfoBlocks( &ptib, &ppib );
        if( ppib->pib_ultype == PT_PM ) {
            if( _WindowsStdout == NULL ) {
                __FAppType = FAPP_GUI;
            } else {
                __FAppType = FAPP_DEFAULT_GUI;
            }
        }
    }
#elif defined( __NT__ )
    {
        if( _WindowsStdout != NULL ) {
            __FAppType = FAPP_DEFAULT_GUI;
        }
    }
#endif
    // WATFOR-77 calls __ErrorInit() when it starts
    __ErrorInit( _LpPgmName );
    RTSysInitialized = 1;
    __InitRTData(); // for main thread
    _ExceptionInit();
    // call to RTSysFini() is done in LGSysFini() for load'n go
    // (i.e. we must call RTSysFini() after each time we execute, not when
    // WATFOR-77 exits in case we are operating in batch mode)
    atexit( &RTSysFini );
    return( 0 );
}


// WARNING: ALL routines below this point are XI initialization routines with no
// stack checking on at all times.  do not place routines below this point
// unless stack checking must be turned off at all times.
#pragma off (check_stack)

#ifdef __SW_BM

static  void    __InitThreadDataSize( void ) {
//======================================

    __FThreadDataOffset = __RegisterThreadDataSize( sizeof( fthread_data ) );
}

XI( __fthread_data_size, __InitThreadDataSize, INIT_PRIORITY_THREAD )

#endif

// Alternative Stack Activation for non-Intel
#if !defined( _M_IX86 )

#define F77_ALT_STACK_SIZE      8 * 1024

static void     __InitAlternateStack( void ) {
//======================================

        __ASTACKSIZ = F77_ALT_STACK_SIZE;
}

AXI( __InitAlternateStack, INIT_PRIORITY_LIBRARY );

#endif
