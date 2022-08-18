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
* Description:  Compile and run a line drawing function.
*
****************************************************************************/


#include "gdefn.h"
#include "walloca.h"
#include "stkavail.h"
#include "gbios.h"


#if defined( __QNX__ )
    #define COMP_FAR    __far           // compile into another segment
    #define FUNC_FAR    __far           // code segment is always far for QNX
#else
    #define COMP_FAR                    // compile onto the stack
    #define FUNC_FAR    _WCI86FAR       // near for 32-bit/far for 16-bit
#endif

#if defined( _M_I86 )
  #if defined( VERSION2 )
    #define MAXLEN      34
  #else
    #define MAXLEN      20
  #endif
    #define LINERET     0xCB
#else
    #define MAXLEN      25
  #if defined( __QNX__ )
    #define LINERET     0xCB  // QNX uses a segmented 32 bit model
  #else
    #define LINERET     0xC3
  #endif
#endif

/*  Specify calling convention for calling the 'compiled' line drawing routine.    */

#if defined( _M_I86 )
  #if defined( VERSION2 )
    //last parm on the stack...
    #pragma aux LINE_FUNC "*" __far __parm __caller [__es __di] [__si __ax] [__bx] [__cx] [__dx]
  #else
    #pragma aux LINE_FUNC "*" __far __parm __caller [__es __di] [__ax] [__bx] [__cx] [__dx] [__si]
  #endif
#else
    #pragma aux LINE_FUNC "*" __parm __caller [__es __edi] [__eax] [__ebx] [__ecx] [__edx] [__esi]
#endif

typedef void (FUNC_FAR line_fn)( char __far *, grcolor, int, int, int, int );
#pragma aux (LINE_FUNC) line_fn;

#define OutByte( p )    *stack++ = p
#define OutInt( p )     *(unsigned int COMP_FAR *)stack = p; stack += sizeof( int )

void _L0DrawLine( char __far *screen_ptr, grcolor color, unsigned short style,
                  short bit_mask, short majordif, short minordif,
                  move_fn *majorfn, move_fn *minorfn, putdot_fn *plot )
/*========================================================================*/
{
    short                   size;
    char                    plot_len;
    char                    minor_len;
    char                    major_len;
    unsigned char COMP_FAR  *stack;
    unsigned char COMP_FAR  *L1_label;
    line_fn                 *line;

/*  This function 'compiles' the line drawing routine on the stack.
    The corresponding assembler pseudo-code is:

32-bit version and 16-bit version (VERSION1)
--------------
    L1:         rol     bx,1                         ... check line style
                jnc     L2
    ( L1: )     [ inline code for plot function ]
    L2:         dec     dx
                jl      L3
                [ inline code for major function ]
                sub     si,minordif
                jg      L1
                add     si,majordif
                [ inline code for minor function ]
                jmp     L1
    L3:         retf


16-bit version (VERSION2)
--------------
                push    bp
                mov     bp,sp
                push    si
                mov     si,[bp+6]        ;...cause it's far call..
    L1:         rol     bx,1             ;... check line style
                jnc     L2
                push    dx               ;.... save ctr
                mov     dx,[bp-2]        ;.... get hi color word
    ( L1: )     [ inline code for plot function ]
                pop     dx               ;.... restore ctr
    L2:         dec     dx
                jl      L3
                [ inline code for major function ]
                sub     si,minordif
                jg      L1
                add     si,majordif
                [ inline code for minor function ]
                jmp     L1
    L3:         pop     si
                pop     bp
                retf

*/

    plot_len = *( (char FUNC_FAR *)plot - 1 );
    minor_len = *( (char FUNC_FAR *)minorfn - 1 );
    major_len = *( (char FUNC_FAR *)majorfn - 1 );
    size = _RoundUp( plot_len + minor_len + major_len + MAXLEN );

#if defined( __QNX__ )
    stack = _MK_FP( _CompileBuf, 0 );
    line = _MK_FP( _CompileSeg, 0 );    // shadow seg which is executable
#else
    if( _stackavail() - size > 0x100 ) {
        stack = __alloca( size );
    } else {
        _ErrorStatus = _GRINSUFFICIENTMEMORY;
        return;         /* not enough memory to proceed */
    }
  #if defined( _M_I86 )
    line = _MK_FP( _StackSeg, _FP_OFF( stack ) );
  #else
    line = (line_fn *)stack;
  #endif
#endif

#if defined( _M_I86 ) && defined( VERSION2 )
    OutByte( 0x55 );                    /* push        bp       */
    OutByte( 0x8B );                    /* mov         bp,sp    */
    OutByte( 0xEC );
    OutByte( 0x56 );                    /* push        si       */
    OutByte( 0x8B );                    /* mov         si,word ptr 0x6[bp] */
    OutByte( 0x76 );
    OutByte( 0x06 );
#endif
    L1_label = stack;                   /* save L1 label address */
    if( style != SOLID_LINE ) {         /* add instructions for a style line    */
#if !defined( _M_I86 )
        OutByte( 0x66 );                /* rotate only bx, not ebx  */
#endif
        OutByte( 0xD1 );                /* rol         bx,1     */
        OutByte( 0xC3 );
        OutByte( 0x73 );                /* jnc         short L2 */
#if defined( _M_I86 ) && defined( VERSION2 )
        OutByte( plot_len + 5 );
#else
        OutByte( plot_len );
#endif
    }
#if defined( _M_I86 ) && defined( VERSION2 )
    OutByte( 0x52 );                    /* push        dx       */
    OutByte( 0x8B );                    /* mov         dx,word ptr -0x2[bp]  */
    OutByte( 0x56 );
    OutByte( 0xFE );
#endif
    movedata( _FP_SEG( plot ), _FP_OFF( plot ),     /* copy plot routine    */
              _FP_SEG( stack ), _FP_OFF( stack ), plot_len );
    stack += plot_len;
#if defined( _M_I86 ) && defined( VERSION2 )
    OutByte( 0x5A );                    /* pop         dx       */
#endif
                                        /* L2:                  */
    OutByte( 0x4A );                    /* dec         dx       */
    OutByte( 0x7C );                    /* jl          short L3 */
    OutByte( major_len + minor_len + 8 + 2 * sizeof( int ) );
    movedata( _FP_SEG( majorfn ), _FP_OFF( majorfn ), /* copy major function  */
              _FP_SEG( stack ), _FP_OFF( stack ), major_len );
    stack += major_len;
    OutByte( 0x81 );                    /*  sub        si, minordif    */
    OutByte( 0xEE );
    OutInt( minordif );
    OutByte( 0x7F );                    /*  jg         short L1 */
    OutByte( L1_label - stack - 1 );
    OutByte( 0x81 );                    /*  add        si, majordif    */
    OutByte( 0xC6 );
    OutInt( majordif );
    movedata( _FP_SEG( minorfn ), _FP_OFF( minorfn ), /* copy minor function  */
              _FP_SEG( stack ), _FP_OFF( stack ), minor_len );
    stack += minor_len;
    OutByte( 0xEB );                    /*  jmp        short L1 */
    OutByte( L1_label - stack - 1 );
                                        /* L3:                  */
#if defined( _M_I86 ) && defined( VERSION2 )
    OutByte( 0x5E );                    /* pop         si       */
    OutByte( 0x5D );                    /* pop         bp       */
#endif
    OutByte( LINERET );
    (*line)( screen_ptr, color, style, bit_mask, majordif >> 1, majordif >> 1 );
}
