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
* Description:  Expand instructions so that machine code can be generated.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "memcheck.h"
#include "zoiks.h"
#include "data.h"
#include "namelist.h"
#include "blips.h"
#include "expand.h"
#include "split.h"
#include "insdead.h"
#include "rgtbl.h"
#include "optab.h"
#include "index.h"
#include "fixindex.h"
#include "generate.h"
#include "conflict.h"
#include "cgaux.h"
#include "bgcall.h"
#include "targetin.h"


static instruction  *DoReduce( instruction *ins, const opcode_entry *try, bool has_index )
/****************************************************************************************/
{
    hw_reg_set  *zap;
    hw_reg_set  zap_all;

    if( try->generate == G_NO )
        return( ins );
    if( try->generate >= FIRST_REDUCT )
        return( Reduce( ins ) );
    zap_all = ins->zap->reg;
    for( zap = RegSets[RegList[try->reg_set].zap]; !HW_CEqual( *zap, HW_EMPTY ); ++zap ) {
        HW_TurnOn( zap_all, *zap );
    }
    ins->zap = (register_name *)AllocRegName( zap_all );
    if( has_index || ins->num_operands > OpcodeNumOperands( ins ) ) {
        ins = NeedIndex( ins );
    } else {
        ins->t.index_needs = RL_;
    }
    return( ins );
}


static  bool    VerifyRegs( instruction *ins, operand_types ops )
/***************************************************************/
{
#define _Any( ops, mul )        ( ( ops & (ANY*mul) ) == (ANY*mul) )

    const opcode_entry  *try;
    hw_reg_set          *possible;
    name                *name;
    op_regs             *need;
    hw_reg_set          regs;
    reg_set_index       left_index;
    reg_set_index       right_index;
    reg_set_index       result_index;
    reg_set_index       special_index;

    try = ins->u.gen_table;
    need = &RegList[try->reg_set];
    result_index = RL_;
    name = ins->result;
    if( ( ops & R_R ) == 0 ) {
        if( ops & U_R ) {
            result_index = need->result;
        }
    } else if( need->result != RL_ ) {
        if( name->n.class != N_REGISTER ) {
            result_index = need->result;
        } else if( !_Any( try->op_type, RESULT_MUL ) ) {
            regs = name->r.reg;
            for( possible = RegSets[need->result]; ; ++possible ) {
                if( HW_CEqual( *possible, HW_EMPTY ) )
                    return( false );
                if( HW_Equal( *possible, regs ) ) {
                    break;
                }
            }
        }
    }
    left_index = RL_;
    if( ( ops & R_1 ) == 0 ) {
        if( ops & U_1 ) {
            left_index = need->left;
        }
    } else if( need->left != RL_ ) {
        name = ins->operands[0];
        if( name->n.class != N_REGISTER ) {
            left_index = need->left;
        } else if( !_Any( try->op_type, OP1_MUL ) ) {
            regs = name->r.reg;
            for( possible = RegSets[need->left]; ; ++possible ) {
                if( HW_CEqual( *possible, HW_EMPTY ) )
                    return( false );
                if( HW_Equal( *possible, regs ) ) {
                    break;
                }
            }
        }
    }
    right_index = RL_;
    if( ( ops & R_2 ) == 0 ) {
        if( ops & U_2 ) {
            right_index = need->right;
        }
    } else if( need->right != RL_ ) {
        name = ins->operands[1];
        if( name->n.class != N_REGISTER ) {
            right_index = need->right;
        } else if( !_Any( try->op_type, OP2_MUL ) ) {
            regs = name->r.reg;
            for( possible = RegSets[need->right]; ; ++possible ) {
                if( HW_CEqual( *possible, HW_EMPTY ) )
                    return( false );
                if( HW_Equal( *possible, regs ) ) {
                    break;
                }
            }
        }
    }
    if( try->generate < FIRST_REDUCT ) {
        special_index = SpecialPossible( ins );
        if( special_index != RL_ ) {
            MarkPossible( ins, ins->result, special_index );
        } else if( result_index != RL_ ) {
            MarkPossible( ins, ins->result, result_index );
        }
        if( left_index != RL_ ) {
            MarkPossible( ins, ins->operands[0], left_index );
        }
        if( right_index != RL_ ) {
            MarkPossible( ins, ins->operands[1], right_index );
        }
    }
    return( true );
}


static  operand_types   ClassifyOps( instruction *ins, bool *has_index )
/**********************************************************************/
{
    name                *name;
    operand_types       ops;
    opcnt               num_operands;

    num_operands = OpcodeNumOperands( ins );
    *has_index = false;
    ops = NONE;
    name = ins->result;
    if( name != NULL && num_operands != 0 ) {
        if( ins->operands[0] == name ) {
            ops &= EQ_R1;
        }
        if( num_operands != 1 ) {
            if( ins->operands[1] == name ) {
                ops &= EQ_R2;
            }
        }
    }
    if( name != NULL ) {
        switch( name->n.class ) {
        case N_REGISTER:
            ops |= R_R;
            break;
        case N_INDEXED:
            *has_index = true;
            ops |= M_R;
            break;
        case N_MEMORY:
        case N_TEMP:
            if( name->v.conflict == NULL ) {
                ops |= M_R;
            } else if( name->v.usage & USE_MEMORY ) {
                ops |= M_R;
            } else {
                ops |= U_R;
                ins->head.state = OPERANDS_NEED_WORK;
            }
            break;
        }
    }
    if( num_operands != 0 ) {
        name = ins->operands[0];
        switch( name->n.class ) {
        case N_CONSTANT:
            ops |= C_1;
            break;
        case N_REGISTER:
            ops |= R_1;
            break;
        case N_INDEXED:
            *has_index = true;
            ops |= M_1;
            break;
        case N_MEMORY:
        case N_TEMP:
            if( name->v.conflict == NULL ) {
                ops |= M_1;
            } else if( name->v.usage & USE_MEMORY ) {
                ops |= M_1;
            } else {
                ops |= U_1;
                ins->head.state = OPERANDS_NEED_WORK;
            }
            break;
        }
    }
    if( num_operands > 1 ) {
        name = ins->operands[1];
        switch( name->n.class ) {
        case N_CONSTANT:
            ops |= C_2;
            break;
        case N_REGISTER:
            ops |= R_2;
            break;
        case N_INDEXED:
            *has_index = true;
            ops |= M_2;
            break;
        case N_MEMORY:
        case N_TEMP:
            if( name->v.conflict == NULL ) {
                ops |= M_2;
            } else if( name->v.usage & USE_MEMORY ) {
                ops |= M_2;
            } else {
                ops |= U_2;
                ins->head.state = OPERANDS_NEED_WORK;
            }
            break;
        }
        if( num_operands > 2 ) {
            name = ins->operands[2];
            if( name->n.class == N_INDEXED ) {
                *has_index = true;
            }
            if( ins->head.opcode == OP_CALL_INDIRECT ) {
                switch( name->n.class ) {
                case N_MEMORY:
                case N_TEMP:
                    if( name->v.conflict != NULL && (name->v.usage & USE_MEMORY) == 0 ) {
                        ins->head.state = OPERANDS_NEED_WORK;
                    }
                }
            }
        }
    }
    return( ops );
}


const opcode_entry  *FindGenEntry( instruction *ins, bool *has_index )
/********************************************************************/
{
    const opcode_entry  *try;
    operand_types       ops;
    vertype             verify;

    ins->head.state = INS_READY;
    try = ins->table;
    if( try == NULL ) {
        try = CodeTable( ins );
        ins->table = try;
    }
    if( try == NULL )
        return( try );
    ops = ClassifyOps( ins, has_index );
    for( ;; ++try ) {
        if( ( try->op_type & ops ) != ops )
            continue;
        verify = try->verify;
        if( verify != V_NO ) {
            if( !DoVerify( verify & ~NOT_VOLATILE, ins ) )
                continue;
            if( (verify & NOT_VOLATILE) && VolatileIns( ins ) ) {
                continue;
            }
        }
        ins->u.gen_table = try;
        if( try->reg_set != RG_ ) {
            if( !VerifyRegs( ins, ops ) ) {
                continue;
            }
        }
        if( ins->head.opcode == OP_CALL_INDIRECT ) {
            MarkPossible(ins, ins->operands[CALL_OP_ADDR], CallIPossible(ins));
        }
        break;
    }
    return( try );
}


void    FixGenEntry( instruction *ins )
/*************************************/
{
/* update the instruction to reflect new instruction needs*/

    const opcode_entry  *old;
    bool                dummy;

    old = ins->u.gen_table;
    FindGenEntry( ins, &dummy );
    if( G( ins ) >= FIRST_REDUCT ) {
        ins->u.gen_table = old;
    }
    ins->head.state = INS_NEEDS_WORK;
}


instruction     *PostExpandIns( instruction *ins )
/************************************************/
{
    const opcode_entry  *try;
    bool                dummy;

    try = FindGenEntry( ins, &dummy );
    if( try == NULL )
        return( ins );
    if( try->generate == G_NO )
        return( ins );
    if( try->generate >= FIRST_REDUCT )
        return( Reduce( ins ) );
    return( ins );
}


instruction     *ExpandIns( instruction *ins )
/********************************************/
{
    const opcode_entry  *try;
    bool                has_index;

    try = FindGenEntry( ins, &has_index );
    if( try == NULL )
        return( ins );
    return( DoReduce( ins, try, has_index ) );
}


int     ExpandOps( bool keep_on_truckin )
/***************************************/
{
    block       *blk;
    instruction *ins;
    int         unknowns;

    unknowns = 0;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        EXBlip();
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ) {
            if( !keep_on_truckin && _MemLow ) {
                return( -1 );
            }
            ins = ExpandIns( ins );
            if( ins->head.state == INS_NEEDS_WORK ) {
                ins = ExpandIns( ins );
            } else {
                if( ins->head.state == OPERANDS_NEED_WORK ) {
                    ++unknowns;
                }
                ins = ins->head.next;
            }
        }
    }
    return( unknowns );
}
