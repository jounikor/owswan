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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "gdefn.h"
#include "walloca.h"
#include "gbios.h"
#include "stkavail.h"


struct rgb {
    char                red;
    char                green;
    char                blue;
};


#if defined( _M_I86 ) || defined( __QNX__ )

extern void VideoIntDAC( short, short, short, void __far * );
#if defined( _M_I86 )
#pragma aux VideoIntDAC = \
        "push bp"   \
        "int 10h"   \
        "pop  bp"   \
    __parm __caller [__ax] [__bx] [__cx] [__es __dx] \
    __value         \
    __modify        []
#else
#pragma aux VideoIntDAC = \
        "push bp"   \
        "int 10h"   \
        "pop  bp"   \
    __parm __caller [__ax] [__bx] [__cx] [__es __edx] \
    __value         \
    __modify        []
#endif


short _FastMap( long _WCI86FAR *colours, short num )
//=============================================

{
    short               i;
    long                colour;
    struct rgb          *rgb;

    i = _RoundUp( sizeof( struct rgb ) * num );
    if( _stackavail() - i <= 0x100 ) {
        return( FALSE );
    } else {
        rgb = __alloca( i );
    }
    for( i = 0; i < num; ++i ) {
        colour = *colours;
        rgb[i].blue = ( (unsigned long)colour & 0x00ff0000 ) >> 16;
        rgb[i].green = (unsigned short)( colour & 0x0000ff00 ) >> 8;
        rgb[i].red = colour & 0x000000ff;
        ++colours;
    }
    VideoIntDAC( _BIOS_SET_PALETTE + 0x12, 0, num, rgb );
    return( TRUE );
}


#else


#include "extender.h"
#include "rmalloc.h"


short _FastMap( long _WCI86FAR *colours, short num )
//=============================================

{
    short               i;
    long                colour;
    struct rgb __far    *rgb;
    RM_ALLOC            mem;

    if( _RMAlloc( num * sizeof( struct rgb ), &mem ) ) {
        rgb = mem.pm_ptr;
        for( i = 0; i < num; ++i ) {
            colour = *colours;
            rgb[i].blue = ( (unsigned long)colour & 0x00ff0000 ) >> 16;
            rgb[i].green = (unsigned short)( colour & 0x0000ff00 ) >> 8;
            rgb[i].red = colour & 0x000000ff;
            ++colours;
        }
        _RMInterrupt( 0x10, _BIOS_SET_PALETTE + 0x12, 0, num, 0, mem.rm_seg, 0 );
        _RMFree( &mem );
        return( TRUE );
    } else {
        return( FALSE );
    }
}


typedef struct {
    unsigned long       edi;
    unsigned long       esi;
    unsigned long       ebp;
    unsigned long       reserved;
    unsigned long       ebx;
    unsigned long       edx;
    unsigned long       ecx;
    unsigned long       eax;
    unsigned short      flags;
    unsigned short      es;
    unsigned short      ds;
    unsigned short      fs;
    unsigned short      gs;
    unsigned short      ip;
    unsigned short      cs;
    unsigned short      sp;
    unsigned short      ss;
} RMI;

extern long DPMIAllocDOSMemory( short para );
#pragma aux DPMIAllocDOSMemory = \
        "mov  ax,100h"  \
        "int 31h"       \
        "jnc short L1"  \
        "sub  ax,ax"    \
        "sub  dx,dx"    \
    "L1: shl  eax,16"   \
        "mov  ax,dx"    \
    __parm      [__bx] \
    __value     [__eax] \
    __modify    [__dx]

extern void DPMIFreeDOSMemory( short sel );
#pragma aux DPMIFreeDOSMemory = \
        "mov  ax,101h"  \
        "int 31h"       \
    __parm      [__bx] \
    __value     \
    __modify    [__ax]

extern void DPMIRealModeInterrupt( char interrupt, char flags,
                            short words_to_copy, RMI __far *call_st );
#pragma aux DPMIRealModeInterrupt = \
        "mov  ax,300h"  \
        "int 31h"       \
    __parm      [__bl] [__bh] [__cx] [__es __edi] \
    __value     \
    __modify    [__ax]

typedef struct {
    short               intnum;
    short               ds;
    short               es;
    short               fs;
    short               gs;
    long                eax;
    long                edx;
} PARM_BLOCK;

extern short PharlapAlloc( short );
#pragma aux PharlapAlloc = \
        "mov  ax,25c0h" \
        "int 21h"       \
        "jnc short L1"  \
        "sub  ax,ax"    \
    "L1:"               \
    __parm      [__bx] \
    __value     [__ax] \
    __modify    []

extern void PharlapFree( short );
#pragma aux PharlapFree = \
        "mov  ax,25c1h" \
        "int 21h"       \
    __parm      [__cx] \
    __value     \
    __modify    [__ax]

extern short PharlapRMI( void __far *parms, short bx, short cx, short di );
#pragma aux PharlapRMI = \
        "push ds"       \
        "push fs"       \
        "pop  ds"       \
        "mov  ax,2511h" \
        "int 21h"       \
        "pop  ds"       \
    __parm __caller     [__fs __edx] [__ebx] [__ecx] [__edi] \
    __value             [__ax] \
    __modify            []


short _RMAlloc( int size, RM_ALLOC *stg )
//=======================================

{
    unsigned short      seg;
    unsigned long       mem;
    int                 paragraphs;

    paragraphs = ( size + 15 ) / 16;
    if( _IsRational() ) {
        mem = DPMIAllocDOSMemory( paragraphs );
        if( mem == 0 ) {
            return( FALSE );
        } else {
            stg->rm_seg = mem >> 16L;
            stg->pm_ptr = _MK_FP( (unsigned short)( mem & 0x0000ffffL ), 0 );
            return( TRUE );
        }
    } else if( _IsPharLap() ) {
        seg = PharlapAlloc( paragraphs );
        if( seg == 0 ) {
            return( FALSE );
        } else {
            stg->rm_seg = seg;
            stg->pm_ptr = _MK_FP( 0x34, seg << 4 );
            return( TRUE );
        }
    } else {
        return( FALSE );
    }
}


void _RMFree( RM_ALLOC *stg )
//===========================

{
    if( _IsRational() ) {
        DPMIFreeDOSMemory( _FP_SEG( stg->pm_ptr ) );
    } else {
        PharlapFree( stg->rm_seg );
    }
}


short _RMInterrupt( short intnum, short ax, short bx, short cx,
                                  short dx, short es, short di )
//==============================================================

{
    RMI                 rmi;
    PARM_BLOCK          parms;

    if( _IsRational() ) {
        memset( &rmi, 0, sizeof( RMI ) );
        rmi.eax = ax;
        rmi.ebx = bx;
        rmi.ecx = cx;
        rmi.edx = dx;
        rmi.es = es;
        rmi.edi = di;
        DPMIRealModeInterrupt( intnum, 0, 0, &rmi );
        return( rmi.eax );
    } else {
        memset( &parms, 0, sizeof( PARM_BLOCK ) );
        parms.intnum = intnum;
        parms.eax = ax;
        parms.edx = dx;
        parms.es = es;
        return( PharlapRMI( &parms, bx, cx, di ) );
    }
}

#endif
