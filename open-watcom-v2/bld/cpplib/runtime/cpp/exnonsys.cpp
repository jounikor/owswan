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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "cpplib.h"
#include "rtexcept.h"

extern "C"
void CPPLIB( raise_exception )  // RAISE EXCEPTION
    ( FsExcRec* excrec )        // - exception record
{
    RW_DTREG* rw;
    THREAD_CTL *thr;            // - thread control
    thr = PgmThread();
    for( rw = thr->registered; ; rw = rw->base.prev ) {
        if( (*rw->base.handler)( excrec, rw, NULL, 0 ) != EXC_HAND_CONTINUE ) {
            break;
        }
    }
}


extern "C"
void CPPLIB( unwind_global )    // GLOBAL UNWIND
    ( RW_DTREG* bound           // - bounding R/W entry
    , void (*)( void )          // - code address (ignored)
    , FsExcRec* excrec )        // - exception record
{
    RW_DTREG* rw;
    THREAD_CTL *thr;            // - thread control
    thr = PgmThread();
    excrec->flags |= EXC_TYPE_UNWIND_NORMAL;
    for( rw = thr->registered; rw != bound; rw = rw->base.prev ) {
        if( (*rw->base.handler)( excrec, rw, NULL, 0 ) != EXC_HAND_CONTINUE ) {
            break;
        }
    }
    excrec->flags &= ~EXC_TYPE_UNWIND_NORMAL;
}
