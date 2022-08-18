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
* Description:  RDOS DLL startup code.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <rdos.h>
#include "initfini.h"
#include "initarg.h"
#include "snglthrd.h"
#include "thread.h"
#include "initsig.h"
#include "mthread.h"
#include "cominit.h"
#include "libmain.h"
#include "cinit.h"

static int  processes;

#define DLL_PROCESS_DETACH  0
#define DLL_PROCESS_ATTACH  1
#define DLL_THREAD_ATTACH   2
#define DLL_THREAD_DETACH   3

int _LibMain( int hdll, int reason, void *reserved )
{
    thread_data             *tdata;
    int                     rc = 0;

    switch( reason ) {
    case DLL_THREAD_ATTACH:
        tdata = ( thread_data * )RdosAllocateMem( __ThreadDataSize );
        if( tdata ) {
            memset( tdata, 0, __ThreadDataSize );
            tdata->__data_size = __ThreadDataSize;
            __RdosAddThread( tdata );
        }       
        break;
    case DLL_PROCESS_ATTACH:
        __InitRtns( INIT_PRIORITY_THREAD );
        tdata = ( thread_data * )RdosAllocateMem( __ThreadDataSize );
        memset( tdata, 0, __ThreadDataSize );
        tdata->__data_size = __ThreadDataSize;
        __InitThreadData( tdata );
        __RdosInit( 1, tdata, hdll );

        __InitRtns( 255 );
        __CommonInit();
        __sig_init_rtn();

        if( !__RdosThreadInit() ) return( -1 );
        __InitMultipleThread();

        rc = LibMain( hdll, reason, reserved );
        if( !rc ) {
            __FiniRtns( 0, 255 );
        }
        break;
    case DLL_THREAD_DETACH:
        __RdosRemoveThread();
        break;
    case DLL_PROCESS_DETACH:
        __RdosRemoveThread();
        RdosFreeMem( __FirstThreadData );
        __FirstThreadData = NULL;
        __FiniRtns( 0, 255 );
        break;
    }
    return( rc );
}
