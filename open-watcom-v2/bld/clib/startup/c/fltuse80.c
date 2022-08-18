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
* Description:  80-bit long double size initialization.
*
****************************************************************************/


#include "variety.h"
#include "rtinit.h"
#include "rtcntrl.h"
#include "setefg.h"


#ifdef _M_I86
unsigned _fltused_ = 1;
#else
unsigned _fltused_ = 0;
#endif

#if defined(_M_IX86)
  #pragma aux _fltused_ "*";
#endif

#pragma alias ( "_fltused_80bit_" , "_fltused_" )

extern void __setEFGfmt( void );
extern void _SetLD80bit( void );

AXI( _SetLD80bit, INIT_PRIORITY_LIBRARY )
AXI( __setEFGfmt, INIT_PRIORITY_LIBRARY )
