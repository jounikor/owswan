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
#include "coderep.h"
#include "score.h"
#include "makeins.h"
#include "utils.h"
#include "namelist.h"
#include "regset.h"
#include "rgtbl.h"
#include "expand.h"
#include "insutil.h"
#include "confldef.h"
#include "liveinfo.h"
#include "split.h"


static  name    *FindPiece( hw_reg_set opnd, hw_reg_set frm, hw_reg_set to )
/***************************************************************************
    Find the right piece "to_piece" of "to" such that "x" is to "to" as "opnd"
    is to "frm". Eg: for 386, if opnd = AH, frm = EAX, to = EDX, then x = DH
*/
{
    hw_reg_set  to_piece;
    hw_reg_set  frm_piece;
    hw_reg_set  tmp;
    name        *piece;

    HW_Asgn( to_piece, to );
    HW_Asgn( frm_piece, frm );
    for( ;; ) {
        if( HW_Equal( frm_piece, opnd ) ) {
            piece = AllocRegName( to_piece );
            if( piece->r.reg_index == -1 ) {
                return( NULL );
            }
        }
        if( HW_CEqual( to_piece, HW_EMPTY ) )
            return( NULL );
        tmp = LowReg( frm_piece );
        if( HW_Subset( opnd, tmp ) ) {
            frm_piece = tmp;
            to_piece = LowReg( to_piece );
        } else {
            tmp = HighReg( frm_piece );
            if( HW_Subset( opnd, tmp ) ) {
                frm_piece = tmp;
                to_piece = HighReg( to_piece );
            } else {
                return( NULL );
            }
        }
    }
}


static  bool    CanChange( instruction **pins,
                           name *frm, name *to, instruction *new_ins ) {
/**********************************************************************/

    instruction         *ins;
    const opcode_entry  *try;
    opcnt               i;
    name                *opnd;
    bool                temp_index;
    bool                has_index;

    ins = *pins;
    if( ins->head.opcode == OP_CONVERT
     && ins->operands[0]->n.class == N_REGISTER
     && HW_Ovlap( frm->r.reg, ins->operands[0]->r.reg ) ) {
        opnd = FindPiece( ins->operands[0]->r.reg, frm->r.reg, to->r.reg );
        if( opnd == NULL )
            return( false );
        ins->operands[0] = opnd;
    } else {
        for( i = new_ins->num_operands; i-- > 0; ) {
            opnd = new_ins->operands[i];
            if( opnd == frm ) {
                new_ins->operands[i] = to;
                opnd = to;
            } else if( opnd->n.class == N_REGISTER ) {
                // If operand overlaps with but wasn't identical to 'from'
                // register, leave this instruction alone! In specific cases
                // it might be possible to do the FindPiece trick like above
                // but not in general.
                if( HW_Ovlap( frm->r.reg, opnd->r.reg ) ) {
                    return( false );
                }
            }
            if( opnd->n.class == N_INDEXED ) {
                if( opnd->i.index == frm ) {
                    temp_index = false;
                    if( HasTrueBase( opnd ) ) {
                        if( opnd->i.base->n.class == N_TEMP ) {
                            temp_index = true;
                        }
                    }
                    if( !IndexRegOk( to->r.reg, temp_index ) )
                        return( false );
                    new_ins->operands[i] = ScaleIndex( to, opnd->i.base,
                                                         opnd->i.constant,
                                                         opnd->n.type_class,
                                                         opnd->n.size,
                                                         opnd->i.scale,
                                                         opnd->i.index_flags );
                } else if( opnd->i.index->n.class == N_REGISTER ) {
                    if( HW_Ovlap( opnd->i.index->r.reg, frm->r.reg ) ) {
                        return( false );
                    }
                }
            }
        }
    }
    new_ins->result = to;
    try = FindGenEntry( new_ins, &has_index );
    if( try == NULL )
        return( false );
    if( try->generate >= G_UNKNOWN )
        return( false );
    ReplIns( ins, new_ins );
    *pins = new_ins;
    return( true );
}


static  bool    CantChange( instruction **pins, name *frm, name *to ) {
/*********************************************************************/

    instruction         *new_ins;
    opcnt               num_operands;

    num_operands = (*pins)->num_operands;
    new_ins = NewIns( num_operands );
    Copy( *pins, new_ins, offsetof( instruction, operands ) + num_operands * sizeof( name * ) );
    if( CanChange( pins, frm, to, new_ins ) )
        return( false );
    new_ins->head.next = new_ins;
    new_ins->head.prev = new_ins;
    FreeIns( new_ins );
    return( true );
}

static  bool    Modifies( name *Y, name *Z, name *result ) {
/**********************************************************/


    if( result == NULL )
        return( false );
    if( result->n.class != N_REGISTER )
        return( false );
    if( HW_Ovlap( result->r.reg, Z->r.reg ) )
        return( true );
    if( HW_Ovlap( result->r.reg, Y->r.reg ) )
        return( true );
    return( false );
}

static  bool    UsedIn( name *op, name *reg ) {
/*********************************************/

    if( op->n.class == N_REGISTER ) {
        return( HW_Ovlap( op->r.reg, reg->r.reg ) );
    } else if( op->n.class == N_INDEXED ) {
        if( op->i.index->n.class == N_REGISTER ) {
            if( HW_Ovlap( op->i.index->r.reg, reg->r.reg ) ) {
                return( true );
            }
        }
//      90-06-12 - base is never N_REGISTER!
//      if( op->i.base != NULL && op->i.base->n.class == N_REGISTER ) {
//          if( op->i.base->r.reg & reg->r.reg ) {
//              return( true );
//          }
//      }
    }
    return( false );
}


static  bool    ThrashDown( instruction *ins ) {
/**********************************************/

/* We have a move instruction of the form*/
/*           MOV Y ==> Z*/
/* where*/
/* 1. Y and Z are registers,*/
/* 2. Y dies immediately after the move.*/
/**/
/* Now if the instruction stream contains an instruction of the form*/
/*           OP( ?Y, ?Y ) ==> Y*/
/* in front of the move such that*/
/* 1. Y is live but not used or zapped between the instructions*/
/* 2. Z must be dead and not redefined between the instructions*/
/* 3. Z must be dead just prior to the first instruction*/
/**/
/* then we can delete the move and replace the first instruction with*/
/*           MOV Y      ==> Z*/
/*           OP(?Z,?Z)  ==> Z*/

    name        *Y;
    name        *Z;
    opcnt       i;
    instruction *oth_ins;

    Y = ins->operands[0];
    Z = ins->result;
    for( oth_ins = ins->head.prev; ; oth_ins = oth_ins->head.prev ) {
        if( oth_ins->head.opcode == OP_BLOCK )
            return( false );
        if( _OpIsCall( oth_ins->head.opcode ) )
            return( false );
        if( HW_Ovlap( oth_ins->head.next->head.live.regs, Z->r.reg) ) {
            return( false );
        }
        if( !HW_Subset( oth_ins->head.next->head.live.regs, Y->r.reg ) ) {
            return( false );
        }
        if( HW_Ovlap( oth_ins->zap->reg, Y->r.reg ) )
            return( false );
        if( HW_Ovlap( oth_ins->zap->reg, Z->r.reg ) )
            return( false );
        if( oth_ins->result == Y )
            break;
        if( Modifies( Y, Z, oth_ins->result ) )
            return( false );
        for( i = oth_ins->num_operands; i-- > 0; ) {
            if( UsedIn( oth_ins->operands[i], Y ) ) {
                return( false );
            }
        }
        if( oth_ins->result != NULL ) {
            if( UsedIn( oth_ins->result, Y ) ) {
                return( false );
            }
        }
    }
    if( HW_Ovlap( oth_ins->head.live.regs, Z->r.reg ) ) {
        if( !ChangeIns(oth_ins,Z,&oth_ins->result,CHANGE_GEN) )
            return(false);
        FreeIns( ins );
    } else {
        if( CantChange( &oth_ins, Y, Z ) )
            return( false );
        ins->head.prev->head.next = ins->head.next;
        ins->head.next->head.prev = ins->head.prev;
        PrefixIns( oth_ins, ins );
    }
    return( true );
}


static  bool    ThrashUp( instruction *ins ) {
/**********************************************/

/* OK, if there is an instruction of the form*/
/*           OP(?Z,?Z) ==> Z*/
/* after the move such that*/
/* 1. Z is live but not used or zapped between the instructions*/
/* 2. Y is not modified between the instructions*/
/**/
/* and that is followed by a*/
/*           MOV Z     ==> Y*/
/* where*/
/* 1. Z is not modified between the instructions*/
/* 2. Y is not modified between the instructions*/
/**/
/* then can we delete the two moves and replace the second instr. with*/
/*           OP(?Y,?Y)  ==> Y*/
/*           MOV  Y     ==> Z*/

    instruction *thrsh_ins;
    name        *Y;
    name        *Z;
    opcnt       i;
    instruction *oth_ins;

    Y = ins->operands[0];
    Z = ins->result;
    for( oth_ins = ins->head.next; ; oth_ins = oth_ins->head.next ) {
        if( oth_ins->head.opcode == OP_BLOCK )
            return( false );
        if( _OpIsCall( oth_ins->head.opcode ) )
            return( false );
        if( HW_Ovlap( oth_ins->zap->reg, Y->r.reg ) )
            return( false );
        if( HW_Ovlap( oth_ins->zap->reg, Z->r.reg ) )
            return( false );
        if( oth_ins->result == Z )
            break;
        if( Modifies( Y, Z, oth_ins->result ) )
            return( false );
        for( i = oth_ins->num_operands; i-- > 0; ) {
            if( UsedIn( oth_ins->operands[i], Z ) ) {
                return( false );
            }
        }
        if( oth_ins->result != NULL ) {
            if( UsedIn( oth_ins->result, Z ) ) {
                return( false );
            }
        }
    }
    for( thrsh_ins = oth_ins->head.next; ; thrsh_ins = thrsh_ins->head.next ) {
        if( thrsh_ins->head.opcode == OP_BLOCK )
            return( false );
        if( HW_Ovlap( thrsh_ins->zap->reg, Y->r.reg ) )
            return( false );
        if( HW_Ovlap( thrsh_ins->zap->reg, Z->r.reg ) )
            return( false );
        if( (thrsh_ins->head.opcode == OP_MOV)
            && (thrsh_ins->operands[0] == Z)
            && (thrsh_ins->result        == Y) )
            break;
        if( Modifies( Y, Z, oth_ins->result ) ) {
            return( false );
        }
    }
    if( CantChange( &oth_ins, Z, Y ) ) {
        return( false );
    }
    FreeIns( thrsh_ins );
    ins->head.prev->head.next = ins->head.next;
    ins->head.next->head.prev = ins->head.prev;
    SuffixIns( oth_ins, ins );
    return( true );
}


bool    RegThrash( block *blk )
/*****************************/
{
    instruction *ins;
    instruction *next;

    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
        next = ins->head.next;
        if( ins->head.opcode == OP_MOV
         && !UnChangeable( ins )
         && ins->operands[0]->n.class == N_REGISTER
         && !HW_Ovlap( ins->head.next->head.live.regs, ins->operands[0]->r.reg )
         && ins->result->n.class == N_REGISTER ) {
            if( ThrashDown( ins ) || ThrashUp( ins ) ) {
                UpdateLive( blk->ins.hd.next, blk->ins.hd.prev );
                return( true );
            }
        }
    }
    return( false );
}
