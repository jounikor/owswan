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
* Description:  Internal heap free space filling
*               (16-bit code only)
*
****************************************************************************/


#include "dll.h"        // needs to be first
#include "variety.h"
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include "heap.h"
#include "heapacc.h"


extern void     _mymemset( void_fptr, unsigned, unsigned );
#pragma aux _mymemset = \
        memset_i86      \
    __parm __caller     [__es __di] [__ax] [__cx] \
    __value             \
    __modify __exact    [__ax __di __cx]

int __HeapSet( __segment seg, unsigned int fill )
{
    FRLPTR( seg )   frl;

    _AccessFHeap();
    for( ; seg != _NULLSEG; seg = BHEAP( seg )->next.segm ) {
        frl = BHEAP( seg )->freehead.next.nptr;
        while( _FP_OFF( frl ) != offsetof( heapblk, freehead ) ) {
            _mymemset( frl + 1, fill, frl->len - sizeof( freelist ) );
            frl = frl->next.nptr;
        }
    }
    _ReleaseFHeap();
    return( _HEAPOK );
}
