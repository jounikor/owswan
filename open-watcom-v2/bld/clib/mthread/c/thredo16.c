/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  OS/2 16-bit threading routines.
*
****************************************************************************/


#include "variety.h"
#include <dos.h>
#include <process.h>
#include <string.h>
#include <stddef.h>
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#include <wos2.h>
#include "rtstack.h"
#include "stacklow.h"
#include "exitwmsg.h"
#include "extfunc.h"
#include "thread.h"
#include "maxthrds.h"


static  HSEM            data_sem = 0;

static  thread_fn       *Routine;
static  void            _WCFAR *Argument;
static  void            _WCFAR *StackBottom;


static void _WCFAR begin_thread_helper( void )
/********************************************/
{
    __thread_fn     *start_addr;
    void            _WCFAR *_arg;
    thread_data     my_thread_data;

    if( *_threadid > __MaxThreads )
        return;
    /* save a copy of the global variables so we can clear the semaphore */
    start_addr = (__thread_fn *)Routine;
    _arg = Argument;
    memset( &my_thread_data, 0, sizeof( thread_data ) );
    my_thread_data.__randnext = 1;
    __ThreadData[*_threadid] = &my_thread_data;
    _STACKLOW = _FP_OFF( StackBottom );
    DosSemClear( &data_sem );
    (*start_addr)( _arg );
    _endthread();
}


_WCRTLINK int _WCFAR _beginthread( thread_fn *start_addr,
                            void _WCFAR *stack_bottom,
                            unsigned stack_size,
                            void _WCFAR *arglist )
/*****************************************************/
{
    _TID        tid;
    APIRET      rc;

    DosSemRequest( &data_sem, -1L );
    Routine = start_addr;
    Argument = arglist;
    StackBottom = stack_bottom;
    rc = DosCreateThread( begin_thread_helper, (PTID)&tid,
                          (PBYTE)((char _WCFAR *)stack_bottom + stack_size) );
    if( rc != 0 ) {
        DosSemClear( &data_sem );
        return( -1 );
    }
    if( tid > __MaxThreads ) {
        return( -1 );
    }
    return( tid );
}

_WCRTLINK void _WCFAR *__chkstack( void _WCFAR *ptr )
/***************************************************/
{
    if( _FP_SEG( ptr ) != _FP_SEG( &Routine ) ) {
        __fatal_runtime_error( "thread stack not in DGROUP", 1 );
        // never return
    }
    return( ptr );
}

_WCRTLINK void _WCFAR _endthread( void )
/**************************************/
{
    __ThreadData[*_threadid] = NULL;
    DosExit( EXIT_THREAD, 0 );
    // never return
}
