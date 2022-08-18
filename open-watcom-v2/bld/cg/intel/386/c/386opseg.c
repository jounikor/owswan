/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2016 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Add register 'gblreg' to all memory references.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "data.h"
#include "zeropage.h"
#include "namelist.h"
#include "object.h"
#include "rgtbl.h"
#include "x86segs.h"
#include "x86opseg.h"


zero_page_scheme        ZPageType;

static  void    CheckName( name **pop, name *gblreg )
/****************************************************
    add gblreg to a memory reference. eg: x[EAX*2] -> x[gblreg+EAX*2]
*/
{
    name        *op;
    i_flags     flags;
    name        *index;
    hw_reg_set  tmp;

    op = *pop;
    if( op->n.class == N_MEMORY && !SegIsCS( op ) ) {
        *pop = SAllocIndex( gblreg, op, op->v.offset, op->n.type_class, op->n.size );
    } else if( op->n.class == N_INDEXED ) {
        if( op->i.base != NULL && op->i.base->n.class == N_MEMORY &&
            !SegIsCS( op->i.base ) ) {
            tmp =  gblreg->r.reg;
            HW_TurnOn( tmp, op->i.index->r.reg );
            index = AllocRegName( tmp );
            tmp = High64Reg( index->r.reg );
            if( HW_Equal( tmp, gblreg->r.reg ) ) {
                flags = X_HIGH_BASE;
            } else {
                flags = X_LOW_BASE;
            }
            *pop = ScaleIndex( index, op->i.base, op->i.constant,
            op->n.type_class, op->n.size, op->i.scale, op->i.index_flags|flags);
        }
    }
}


static  void    AddGlobalIndex( void )
/*************************************
    add EBX to every memory reference in the program
*/
{
    block       *blk;
    instruction *ins;
    opcnt       i;
    name        *gblreg;

    gblreg = AllocRegName( HW_EBX );
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                CheckName( &ins->operands[i], gblreg );
            }
            if( ins->result != NULL ) {
                CheckName( &ins->result, gblreg );
            }
        }
    }
}


void    InitZeroPage( void )
/***************************
    This is here to handle the "indexed" globals option.
    It should really be in its own module, but
    the ZeroPage stuff was called at just the
    right time, and wasn't used by the 386 compiler
*/
{
    ZPageType = ZP_USES_SS;
    if( _IsTargetModel( INDEXED_GLOBALS ) ) {
        AddGlobalIndex();
    }
}


void    FiniZeroPage( void )
/**************************/
{
}
