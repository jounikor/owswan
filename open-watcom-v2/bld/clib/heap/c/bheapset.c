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
* Description:  based heap fill memory function
*               (16-bit code only)
*
****************************************************************************/


#include "dll.h"        // needs to be first
#include "variety.h"
#include <malloc.h>
#include "heap.h"


_WCRTLINK int _bheapset( __segment seg, unsigned int fill )
{
    int         heap_status;

    if( seg == _DGroup() )
        return( _nheapset( fill ) );
    if( seg == _NULLSEG ) {
        for( seg = __bheapbeg; seg != _NULLSEG; seg = BHEAP( seg )->next.segm ) {
            heap_status = _bheapset( seg, fill );
            if( heap_status != _HEAPOK ) {
                return( heap_status );
            }
        }
        return( _HEAPOK );
    }
    heap_status = _bheapchk( seg );
    if( heap_status != _HEAPOK )
        return( heap_status );
    return( __HeapSet( seg, fill ) );
}
