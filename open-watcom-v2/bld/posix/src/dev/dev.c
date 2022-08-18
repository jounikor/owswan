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
* Description:  POSIX dev utility.
*
****************************************************************************/


#include <stdio.h>
#if defined( __NT__ )
int main( void ) { printf( "Not implemented for NT\n" ); return( 0 ); }
#elif defined( __OS2__ )
int main( void ) { printf( "Not implemented for OS/2\n" ); return( 0 ); }
#elif defined( __QNX__ )
int main( void ) { printf( "Not implemented for QNX\n" ); return( 0 ); }
#else
#include <stdlib.h>
#include <dos.h>
#include <process.h>


typedef unsigned short segment_t;
typedef unsigned short segsize_t;

struct memblk {
    char        memtag;
    segment_t   owner;
    segsize_t   size;
};

enum {
    DA_STDIN            = 0x0001,
    DA_STDOUT           = 0x0002,
    DA_NUL              = 0x0004,
    DA_CLOCK            = 0x0008,
    DA_FAST_CON         = 0x0010,
    /* reserved         = 0x0020, */
    DA_GENERIC_IOCTL    = 0x0040,
    /* reserved         = 0x0080, */
    /* reserved         = 0x0100, */
    /* reserved         = 0x0200, */
    /* reserved         = 0x0400, */
    DA_OPEN_CLOSE       = 0x0800,
    /* reserved         = 0x1000, */
    DA_NON_IBM_B        = 0x2000, /* block device */
    DA_OUTPUT_BUSY      = 0x2000, /* char device */
    DA_RW_IOCTL         = 0x4000,
    DA_CHAR_DEVICE      = 0x8000
};

struct device {
    struct device __far   *next;
    unsigned short      attr;
    unsigned short      strategy_entry;
    unsigned short      interrupt_entry;
    char                name[8];
};

void __far *__first_pid( void );
#ifdef _M_I86
#pragma aux __first_pid = \
        "mov ah,52h"    \
        "int 21h"       \
        "mov ax,es"     \
    __parm      [] \
    __value     [__ax __bx] \
    __modify    [__es]
#else
#pragma aux __first_pid = \
        "xor ebx,ebx"   \
        "mov ah,52h"    \
        "int 21h"       \
        "mov cx,es"     \
    __parm      [] \
    __value     [__cx __ebx] \
    __modify    [__es]
#endif

static void do_dev( void )
{
    void __far *first_pid;
    struct device __far *device_chain;
    unsigned blocks;
    int i;

    blocks = 0;
    first_pid = __first_pid();
    device_chain = _MK_FP( _FP_SEG( first_pid ), _FP_OFF( first_pid ) + 0x22 );
    while( _FP_OFF( device_chain ) != 0xffff ) {
        if(( device_chain->attr & DA_CHAR_DEVICE ) == 0 ) {
            blocks += device_chain->name[ 0 ];
        }
        device_chain = device_chain->next;
    }
    device_chain = _MK_FP( _FP_SEG( first_pid ), _FP_OFF( first_pid ) + 0x22 );
    while( _FP_OFF( device_chain ) != 0xffff ) {
        printf( "%04x:%04x ", _FP_SEG( device_chain ), _FP_OFF( device_chain ) );
        if( device_chain->attr & DA_CHAR_DEVICE ) {
            printf( "CHAR  " );
            for( i = 0; i < 8; ++i ) {
                putchar( device_chain->name[ i ] );
            }
        } else {
            printf( "BLOCK " );
            i = device_chain->name[0];
            if( i == 1 ) {
                printf( "(%c:)", 'A' + --blocks );
            } else {
                printf( "(%c:-", 'A' + ( blocks - i ) );
                printf( "%c:)", 'A' + ( blocks - 1 ) );
                blocks -= i;
            }
        }
        putchar( '\n' );
        device_chain = device_chain->next;
    }
}

int main( void )
{
    do_dev();
    return( 0 );
}
#endif
