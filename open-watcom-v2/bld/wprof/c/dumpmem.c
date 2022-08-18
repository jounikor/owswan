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
* Description:  Walk and dump the heap.
*
****************************************************************************/


#include <stdio.h>
#if defined( __WATCOMC__ )
    #include <malloc.h>
#endif
#include "bool.h"
#include "watcom.h"
#include "dumpmem.h"


#define LOCSIZE     1

#ifdef _M_I86
#define PTRDIFF(a,b)    ((char __far *)a - (char __far *)b)
#else
#define PTRDIFF(a,b)    ((char *)(pointer_uint)a - (char *)(pointer_uint)b)
#endif

/*
 * WalkMem - walk through the memory locations.   For the PC version only.
 */

int WalkMem( void )
/*****************/
{
#if defined( __WATCOMC__ )
    char                    * str;
    int                     heap_status;
    struct _heapinfo        h_info;

    h_info._pentry = NULL;
    for( ;; ) {
        heap_status = _heapwalk( &h_info );
        if( heap_status != _HEAPOK ) {
            if( heap_status == _HEAPBADBEGIN ) {
                str = "ERROR - heap is damaged";
            } else if( heap_status == _HEAPBADPTR ) {
                str = "ERROR - bad pointer to heap";
            } else if( heap_status == _HEAPBADNODE ) {
                str = "ERROR - bad node in heap";
            } else {
                break;
            }
            printf( "%s\n\n", str );
            return( false );
        }
    }
#endif
    return( true );
}


/*
 * DumpMem - dump out memory locations.   For the PC version only.
 */

void DumpMem( void )
/******************/
{
#if defined( __WATCOMC__ )
    unsigned int            curr_addr;
    unsigned int            total;
    unsigned int            start_size;
    int                     loc_size;
    int                     loc_count;
    int                     heap_status;
    char                    loc_mark;
    struct _heapinfo        h_info;

    if( !WalkMem() ) return;
    h_info._pentry = NULL;
    start_size = 0;
    total = 0;
    for( ;; ) {
        heap_status = _heapwalk( &h_info );
        if( heap_status != _HEAPOK )
            break;
        if( start_size == 0 ) {
            start_size = PTRDIFF( h_info._pentry, 0 );
        }
        printf( "  %s block at %Fp of size %4.4X-%d\n",
                  (h_info._useflag == _USEDENTRY ? "USED" : "FREE"),
                  h_info._pentry, h_info._size, h_info._size );
        total += h_info._size;
    }
    loc_count = 60;
    h_info._pentry = NULL;
    for( ;; ) {
        heap_status = _heapwalk( &h_info );
        if( heap_status != _HEAPOK ) break;
        if( h_info._useflag == _USEDENTRY ) {
            loc_mark = '\xFE';
        } else {
            loc_mark = '\xF9';
        }
        curr_addr = PTRDIFF( h_info._pentry, 0 );
        loc_size = h_info._size;
        for( ;; ) {
            if( loc_count == 60 ) {
                printf( "\n%.4x: ", curr_addr );
                loc_count = 0;
            }
            printf( "%c", loc_mark );
            loc_count++;
            curr_addr += LOCSIZE;
            loc_size -= LOCSIZE;
            if( loc_size <= 0 ) break;
        }
    }
    printf( "\n\nStarting size   = %u\n", start_size );
    printf( "New allocations = %u\n", total );
    printf( "Ending address  = %u\n", start_size+total );
#endif
}
