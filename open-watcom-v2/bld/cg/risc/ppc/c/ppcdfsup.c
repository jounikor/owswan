/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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


#include "_cgstd.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "coderep.h"
#include "procdef.h"
#include "model.h"
#include "zoiks.h"
#include "cgaux.h"
#include "dw.h"
#include "ppcregn.h"
#include "dwarf.h"
#include "dfdbg.h"
#include "dfsupp.h"
#include "dbsyms.h"
#include "dfsyms.h"
#include "regset.h"
#include "rgtbl.h"
#include "cgprotos.h"


#define MapReg2DW(r)   ((dw_regs)r)

typedef enum {
    #define DW_REG( __n  )   DW_AXP_##__n,
    #include "dwregppc.h"
    DW_REG( MAX )
    #undef DW_REG
} dw_regs;

static dw_regs  DFRegMap( hw_reg_set hw_reg )
/*******************************************/
{
    return( MapReg2DW( RegTrans( hw_reg ) ) );
}


static dw_regs  DFRegMapN( name *reg )
/************************************/
{
    return( MapReg2DW( RegTransN( reg ) ) );
}


void    DFOutReg( dw_loc_id locid, name *reg )
/********************************************/
{
    dw_regs     regnum;

    regnum = DFRegMapN( reg );
    DWLocReg( Client, locid, regnum );
}


void    DFOutRegInd( dw_loc_id locid, name *reg )
/***********************************************/
{
    dw_regs     regnum;

    regnum = DFRegMapN( reg );
    DWLocOp( Client, locid, DW_LOC_breg, regnum, 0 );
}


uint     DFStkReg( void )
/***********************/
{
    return( DFRegMap( StackReg() ) );
}
