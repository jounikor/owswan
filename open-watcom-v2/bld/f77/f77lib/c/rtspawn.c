/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Run-time Spawn() and Suicide()
*
****************************************************************************/


#include "ftnstd.h"
#if defined( __WINDOWS__ ) && defined( _M_I86 )
  #include <windows.h>
#else
  #include <setjmp.h>
#endif
#if defined( __NT__ )
    #include <windows.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#endif
#include "frtdata.h"
#include "fthread.h"
#include "thread.h"
#include "rtspawn.h"


#if defined( __WINDOWS__ ) && defined( _M_I86 )
  #define __setjmp      Catch
  #define __longjmp     Throw
  #define __jmp_buf     CATCHBUF
#else
  #define __setjmp      setjmp
  #define __longjmp     longjmp
  #define __jmp_buf     jmp_buf
#endif

#ifdef __SW_BM

#define __SpawnStack    (*(__jmp_buf **)&(__FTHREADDATAPTR->__SpawnStack))

#else

static  __jmp_buf       *SpawnStack = { NULL };
#define __SpawnStack    SpawnStack

#endif


int     RTSpawn( void (*fn)( void ) ) {
    __jmp_buf   *save_env;
    __jmp_buf   env;
    int         status;

    save_env = __SpawnStack;
    __SpawnStack = &env;
    status = __setjmp( env );
    if( status == 0 ) {
        (*fn)();
    }
    __SpawnStack = save_env;
    return( status );
}


void    RTSuicide( void )
{
    if( __SpawnStack == NULL )
        exit( -1 );
    __longjmp( *__SpawnStack, 1 );
}
