/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  thread proccessing functions
*
****************************************************************************/


#include "ftnstd.h"
#include <process.h>
#if defined( __NT__ )
    #include <windows.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#endif
#include "fthread.h"
#include "rundat.h"
#include "rmemmgr.h"
#include "thread.h"
#include "ftnapi.h"
#include "fthrdini.h"
#include "rttraps.h"
#include "rtspawn.h"
#include "rt_init.h"
#include "extfunc.h"


#if defined(_M_IX86)
    #define FTHREAD_CALL(x) ((fthread_func *)(x))
#else
    #define FTHREAD_CALL(x) (x)
#endif

typedef void fthread_fn( void * );

#if defined(_M_IX86)
    typedef fthread_fn fthread_func;
    #pragma aux (__outside_CLIB) fthread_func;
#endif

typedef struct {
    fthread_fn  *rtn;
    void        *arglist;
} fthread_info;

beginner            FBeginThread;
ender               FEndThread;
initializer         FInitDataThread;

static  beginner    *__BeginThread;
static  ender       *__EndThread;
static  initializer *__InitThreadData;

static  bool        ThreadsInitialized;

static  unsigned  InitFThreads( void )
//====================================
{
    if( ThreadsInitialized )
        return( 0 );
    if( __InitFThreadProcessing() != 0 )
        return( 1 );
    ThreadsInitialized = true;
    RTSysInit();
    return( 0 );
}


static  void    FiniFThreads( void )
//==================================
{
    if( ThreadsInitialized ) {
        __FiniFThreadProcessing();
    }
}


static void     FThreadInit( void )
//=================================
{
    R_TrapInit();
}


static void     FThreadFini( void )
//=================================
{
    R_TrapFini();
}


static  void    ThreadStarter( void )
//===================================
{
    FTHREAD_CALL( __FTHREADDATAPTR->__rtn )( __FTHREADDATAPTR->__arglist );
}


static  void    ThreadHelper( void *arg_fti )
//===========================================
{
    fthread_info *fti = arg_fti;

    FThreadInit();
    __FTHREADDATAPTR->__rtn = fti->rtn;
    __FTHREADDATAPTR->__arglist = fti->arglist;
    RMemFree( fti );
    RTSpawn( ThreadStarter );
    FThreadFini();
    __EndThread();
}


int FBeginThread( thread_fn *rtn, void *stack, unsigned stk_size, void *arglist )
//================================================================================
{
    fthread_info *fti;

    if( InitFThreads() != 0 )
        return( -1 );
    fti = RMemAlloc( sizeof( fthread_info ) );
    if( fti == NULL )
        return( -1 );
    fti->rtn = (fthread_fn *)rtn;
    fti->arglist = arglist;

    return( __BeginThread( ThreadHelper, stack, stk_size, fti ) );
}


void FEndThread( void )
//=====================
{
    RTSuicide();
}


int  FInitDataThread( void *td )
//==============================
{
    __InitFThreadData( td );
    return( __InitThreadData( td ) );
}


// User-callable thread functions:
// -------------------------------

int     __fortran BEGINTHREAD( fthread_fn *rtn, unsigned long *stk_size )
//=======================================================================
{
#ifdef __NT__
    return( (int)_beginthread( (thread_fn *)rtn, *stk_size, NULL ) );
#else
    return( _beginthread( (thread_fn *)rtn, NULL, *stk_size, NULL ) );
#endif
}


void    __fortran ENDTHREAD( void )
//=================================
{
    _endthread();
}


unsigned        __fortran THREADID( void )
//========================================
{
    return( *_threadid );
}


// Initializer/finalizer for thread processing:
// --------------------------------------------


void    __FiniBeginThread( void )
//===============================
{
    FiniFThreads();
}

#pragma off (check_stack)
void    __InitBeginThread( void )
//===============================
{
    __BeginThread = &FBeginThread;
    __EndThread = &FEndThread;
    __InitThreadData = &FInitDataThread;
    __RegisterThreadData( &__BeginThread, &__EndThread, &__InitThreadData );
    ThreadsInitialized = false;
}
