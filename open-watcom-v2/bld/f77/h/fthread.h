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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#ifndef _FTHREAD_H_INCLUDED
#define _FTHREAD_H_INCLUDED

#include "trcback.h"

#ifdef __SW_BM
    extern      void    (*_AccessFIO)( void );
    extern      void    (*_ReleaseFIO)( void );
    extern      void    (*_PartialReleaseFIO)( void );
#else
    #define     _AccessFIO()
    #define     _ReleaseFIO()
    #define     _PartialReleaseFIO()
#endif

#ifdef __SW_BM

// Thread-specific data:
// =====================

typedef struct fthread_data {
    void                *__SpawnStack;
    traceback           *__ExCurr;
    volatile unsigned short __XceptionFlags;
    void                (*__rtn)(void *);
    void                *__arglist;
} fthread_data;

extern  unsigned        __FThreadDataOffset;

#define THREADPTR2FTHREADPTR(p) ((fthread_data *)(((char *)(p)) + __FThreadDataOffset))

#define __FTHREADDATAPTR        THREADPTR2FTHREADPTR( __THREADDATAPTR )

extern void             __FiniFThreadProcessing( void );
extern int              __InitFThreadProcessing( void );
extern void             __InitFThreadData( void * );
extern void             __InitMultiThreadIO( void );

extern void             __FiniBeginThread( void );
extern void             __InitBeginThread( void );

#endif

#endif
