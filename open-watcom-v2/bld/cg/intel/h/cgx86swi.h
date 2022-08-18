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
* Description:  Target dependent code generation switches for x86.
*
****************************************************************************/


#define I_MATH_INLINE           0x00000001L     /* Inline math functions */
#define EZ_OMF                  0x00000002L     /* Generate EZ-OMF objects */
#define BIG_DATA                0x00000004L     /* Data pointers are far */
#define BIG_CODE                0x00000008L     /* Code pointers are far */
#define CHEAP_POINTER           0x00000010L     /* Model isn't huge */
#define FLAT_MODEL              0x00000020L     /* Flat memory model */
#define FLOATING_FS             0x00000040L     /* FS selector is floating */
#define FLOATING_GS             0x00000080L     /* GS selector is floating */
#define FLOATING_ES             0x00000100L     /* ES selector is floating */
#define FLOATING_SS             0x00000200L     /* SS selector is floating */
#define FLOATING_DS             0x00000400L     /* DS selector is floating */
#define USE_32                  0x00000800L     /* Generate 32-bit segments */
#define INDEXED_GLOBALS         0x00001000L     /* Position Independent Code (faulty!) */
#define WINDOWS                 0x00002000L     /* Generate Win16 prologs */
#define CHEAP_WINDOWS           0x00004000L     /* Cheap Win16 prologs */
#define NO_CALL_RET_TRANSFORM   0x00008000L     /* Don't turn calls into jumps */
#define CONST_IN_CODE           0x00010000L     /* FP consts in code segment */
#define NEED_STACK_FRAME        0x00020000L     /* Always generate stack frame */
#define LOAD_DS_DIRECTLY        0x00040000L     /* No runtime call to load DS */
#define SMART_WINDOWS           0x00100000L     /* Smart Win16 prolog (DS==SS) */
#define P5_PROFILING            0x00200000L     /* Pentium RDTSC profiling (-et) */
#define P5_DIVIDE_CHECK         0x00400000L     /* Check for bad Pentium FDIV */
#define GENERIC_TLS             0x00800000L     /* TLS code not NT specific (unused?) */
#define NEW_P5_PROFILING        0x01000000L     /* "New" profiling (-etp) */
#define STATEMENT_COUNTING      0x02000000L     /* Statement counting (-esp) */
#define NULL_SELECTOR_BAD       0x04000000L     /* Avoid null selectors on i86 */
#define P5_PROFILING_CTR0       0x08000000L     /* Use RDPMC instead of RDTSC */
#define GEN_FWAIT_386           0x10000000L     /* Generate FWAITs on 386 and up */
#define LAST_TARG_CGSWITCH      0x10000000L


typedef enum {
    /* CPU revisions */
    CPU_86      = 0x0000,
    CPU_186     = 0x0001,
    CPU_286     = 0x0002,
    CPU_386     = 0x0003,
    CPU_486     = 0x0004,
    CPU_586     = 0x0005,
    CPU_686     = 0x0006,
    /* 8087 revisions */
    FPU_NONE    = 0x0000,
    FPU_87      = 0x0010,
    FPU_387     = 0x0020,
    FPU_586     = 0x0030,
    FPU_686     = 0x0040,
    FPU_EMU     = 0x0080,
    FPU_E87     = FPU_EMU + FPU_87,
    FPU_E387    = FPU_EMU + FPU_387,
    FPU_E586    = FPU_EMU + FPU_586,
    FPU_E686    = FPU_EMU + FPU_686,
    /* Weitek revisions */
    WTK_NONE    = 0x0000,
    WTK_1167    = 0x0100,
    WTK_3167    = 0x0200,
    WTK_4167    = 0x0300,
} proc_revision;

#define CPU_MASK        0x000f
#define FPU_MASK        0x00f0
#define WTK_MASK        0x0f00

#define GET_CPU( r )       ((r) & CPU_MASK)
#define GET_FPU( r )       ((r) & FPU_MASK)
#define GET_FPU_LEVEL( r ) (((r) & FPU_MASK)&~FPU_EMU)
#define GET_WTK( r )       ((r) & WTK_MASK)

#define SET_PROC( r, v, m ) \
    { \
        proc_revision   new; \
        new = (v); \
        (r) &= ~(m); \
        (r) |= new; \
    }
#define SET_CPU( r, v ) SET_PROC( r, v, CPU_MASK );
#define SET_FPU( r, v ) SET_PROC( r, v, FPU_MASK );
#define SET_WTK( r, v ) SET_PROC( r, v, WTK_MASK );

#define SET_FPU_LEVEL( r, v ) SET_FPU( r, (v) | ( GET_FPU( r ) & FPU_EMU ) )
#define SET_FPU_EMU( r )      SET_FPU( r, GET_FPU( r ) | FPU_EMU )
#define SET_FPU_INLINE( r )   SET_FPU( r, GET_FPU( r ) & ~FPU_EMU )

#define GET_FPU_EMU( r )      ( ( GET_FPU( r ) & FPU_EMU ) != FPU_NONE )
