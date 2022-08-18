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


#ifndef HWREG_INCLUDED
#define HWREG_INCLUDED

#include "cghwreg.h"

/*       Target dependent set of hardware registers available */

HW_DEFINE_SIMPLE( HW_G0,     0x00000001U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G1,     0x00000002U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G2,     0x00000004U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G3,     0x00000008U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G4,     0x00000010U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G5,     0x00000020U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G6,     0x00000040U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G7,     0x00000080U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G8,     0x00000100U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G9,     0x00000200U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G10,    0x00000400U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G11,    0x00000800U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G12,    0x00001000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G13,    0x00002000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G14,    0x00004000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_G15,    0x00008000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y0,     0x00010000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y0d,    0x00020000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y2,     0x00040000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y2d,    0x00080000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y4,     0x00100000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y4d,    0x00200000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y6,     0x00400000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_Y6d,    0x00800000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_FULL,   0xffffffffU, 0xffffffffU );
HW_DEFINE_SIMPLE( HW_UNUSED, 0xff000000U, 0xffffffffU );
HW_DEFINE_SIMPLE( HW_EMPTY,  0x00000000U, 0x00000000U );
HW_DEFINE_SIMPLE( HW_SEGS,   0x00000000U, 0x00000000U );

#define HW_DEFINE_COMPOUND( x )         \
enum {                                  \
                                        \
HW_D0_##x    = (HW_Y0_##x+HW_Y0d_##x),  \
HW_D2_##x    = (HW_Y2_##x+HW_Y2d_##x),  \
HW_D4_##x    = (HW_Y4_##x+HW_Y4d_##x),  \
HW_D6_##x    = (HW_Y6_##x+HW_Y6d_##x),  \
                                        \
HW_E0_##x    = (HW_D0_##x+HW_D2_##x),   \
HW_E4_##x    = (HW_D4_##x+HW_D6_##x),   \
                                        \
HW_FLTS_##x  = (HW_E0_##x+HW_E4_##x),   \
                                        \
HW_EVEN_##x  = (HW_G0_##x+HW_G2_##x+HW_G4_##x                       \
                                 +HW_G6_##x+HW_G8_##x+HW_G10_##x    \
                                 +HW_G12_##x+HW_G14_##x),           \
                                                                    \
HW_ODD_##x   = (HW_G1_##x+HW_G3_##x+HW_G5_##x                       \
                                 +HW_G7_##x+HW_G9_##x+HW_G11_##x    \
                                 +HW_G13_##x+HW_G15_##x),           \
                                                                    \
HW_SNGL_##x  = (HW_Y0_##x+HW_Y2_##x+HW_Y4_##x                       \
                                 +HW_Y6_##x),                       \
                                                                    \
HW_G0_G1_G2_##x=(HW_G0_##x+HW_G1_##x+HW_G2_##x),                    \
                                        \
HW__COMPOUND_END_##x                    \
}

HW_ITER( HW_DEFINE_COMPOUND );

HW_DEFINE_GLOBAL_CONST( HW_EMPTY );
HW_DEFINE_GLOBAL_CONST( HW_G0_G1_G2 );
HW_DEFINE_GLOBAL_CONST( HW_G0 );
HW_DEFINE_GLOBAL_CONST( HW_G1 );
HW_DEFINE_GLOBAL_CONST( HW_G2 );
HW_DEFINE_GLOBAL_CONST( HW_G3 );
HW_DEFINE_GLOBAL_CONST( HW_G4 );
HW_DEFINE_GLOBAL_CONST( HW_G5 );
HW_DEFINE_GLOBAL_CONST( HW_G6 );
HW_DEFINE_GLOBAL_CONST( HW_G7 );
HW_DEFINE_GLOBAL_CONST( HW_G8 );
HW_DEFINE_GLOBAL_CONST( HW_G9 );
HW_DEFINE_GLOBAL_CONST( HW_G10 );
HW_DEFINE_GLOBAL_CONST( HW_G11 );
HW_DEFINE_GLOBAL_CONST( HW_G12 );
HW_DEFINE_GLOBAL_CONST( HW_G13 );
HW_DEFINE_GLOBAL_CONST( HW_G14 );
HW_DEFINE_GLOBAL_CONST( HW_G15 );
HW_DEFINE_GLOBAL_CONST( HW_D0 );
HW_DEFINE_GLOBAL_CONST( HW_D2 );
HW_DEFINE_GLOBAL_CONST( HW_D4 );
HW_DEFINE_GLOBAL_CONST( HW_D6 );
HW_DEFINE_GLOBAL_CONST( HW_Y0 );
HW_DEFINE_GLOBAL_CONST( HW_Y2 );
HW_DEFINE_GLOBAL_CONST( HW_Y4 );
HW_DEFINE_GLOBAL_CONST( HW_Y6 );
HW_DEFINE_GLOBAL_CONST( HW_E0 );
HW_DEFINE_GLOBAL_CONST( HW_E4 );

#define MAX_POSSIBLE_REG        8

#endif
