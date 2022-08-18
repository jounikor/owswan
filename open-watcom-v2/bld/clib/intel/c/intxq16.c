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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "variety.h"
#include <i86.h>
#include "dointr.h"


_WCRTLINK int (int86x)( int intno,
                union REGS *inregs,
                union REGS *outregs,
                struct SREGS *segregs )
{
    union REGPACK regs;

    regs.w.ax = inregs->w.ax;
    regs.w.bx = inregs->w.bx;
    regs.w.cx = inregs->w.cx;
    regs.w.dx = inregs->w.dx;
    regs.w.si = inregs->w.si;
    regs.w.di = inregs->w.di;
    regs.w.ds = segregs->ds;
    regs.w.es = segregs->es;
//    regs.w.bp = 0;              /* no bp in REGS union */
//    regs.w.flags = ( inregs->w.cflag ) ? INTR_CF : 0;

    _DoINTR( intno, &regs, 0 );

    outregs->w.ax = regs.w.ax;
    outregs->w.bx = regs.w.bx;
    outregs->w.cx = regs.w.cx;
    outregs->w.dx = regs.w.dx;
    outregs->w.si = regs.w.si;
    outregs->w.di = regs.w.di;
    outregs->w.cflag = ( (regs.w.flags & INTR_CF) != 0 );
    segregs->ds = regs.w.ds;
    segregs->es = regs.w.es;

    return( outregs->w.ax );
}
