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
* Description:  Optimize x87 FPU instruction sequences.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "zoiks.h"
#include "makeins.h"
#include "data.h"
#include "namelist.h"
#include "redefby.h"
#include "makeblk.h"
#include "fpu.h"
#include "x87.h"
#include "expand.h"
#include "split.h"
#include "insutil.h"
#include "rgtbl.h"
#include "typemap.h"
#include "optab.h"
#include "inssegs.h"
#include "bldcall.h"
#include "fixindex.h"
#include "inssched.h"
#include "bldins.h"
#include "liveinfo.h"


void    FPParms( void )
/**************************
    Find sequences like
             PARM_DEF         => ST(0),
             MOV        ST(0) => foobar
    in HeadBlock and record where the parameters in the 8087 are going
    to end up in Parm8087, so that the code to deal with the parameters
    can be folded into the prolog sequence.
*/
{
    instruction *ins;
    instruction *next;
    int         i;

    if( CurrProc->label == HeadBlock->label ) {
        for( i = MAX_8087_REG + 1; i-- > 0; ) {
            Parm8087[i] = NULL;
        }
        i = 0;
        for( ins = HeadBlock->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( ins->head.opcode == OP_PARM_DEF ) {
                if( FPRegNum( ins->result ) == 0 ) {
                    next = ins->head.next;
                    if( next->head.opcode == OP_BLOCK
                     || (instruction *)ins->operands[2] != next /* not associated*/
                     || (instruction *)next->operands[2] != ins ) {
                        Parm8087[i] = (name *)CurrProc; /* klude - means parm ignored*/
                    } else {
                        Parm8087[i] = next->result;
                        DoNothing( next );
                        ins = next;
                    }
                    ++i;
                }
            }
        }
    }
}

static  bool    CanPushImmed( pn parm, int *num_parms )
/******************************************************
    find out if we can "push" floating point parameters onto the 8087
    stack immediately as they are calculated, or if we must delay
    pushing until just before the call instruction.
*/
{
    instruction *next;
    instruction *ins;
    an          addr;

    for( ; parm != NULL; parm = parm->next ) {
        if( parm->ins == NULL ) {
            addr = parm->name;
            if( addr->tipe->attr & TYPE_FLOAT ) {
                ++*num_parms;
                ins = addr->u.i.ins;
                for( next = ins->head.next; next->head.opcode != OP_BLOCK; next = next->head.next ) {
                    if( _OpIsCall( next->head.opcode ) )
                        return( false );
                    if( _OpIsIFunc( next->head.opcode ) ) {
                        if( FPStkReq( next ) != 0 ) {
                            return( false );
                        }
                    }
                }
                if( _BLOCK( next ) != CurrBlock ) {
                    return( false );
                }
            }
        }
    }
    return( true );
}


static  void    Pushes( instruction *ins )
/****************************************/
{
    ins->stk_entry = 0;
    ins->stk_exit = 1;
}


static  int     FPPushImmed( pn parm )
/*************************************
    Push the list of floating point parameters onto the 8087 stack,
    immediately following the definition of each parameter.  (Just set
    the result of each instruction to ST(0)).
*/
{
    an          addr;
    int         parms;

    parms = 0;
    for( ; parm != NULL; parm = parm->next ) {
        if( parm->ins == NULL ) {
            addr = parm->name;
            if( (addr->tipe->attr & TYPE_FLOAT) && HW_COvlap( parm->regs,HW_FLTS ) ){
                if( addr->format != NF_INS ) {
                    _Zoiks( ZOIKS_043 );
                }
                addr->u.i.ins->result = AllocRegName( HW_ST0 );
                Pushes( addr->u.i.ins );
                parm->ins = addr->u.i.ins;
                addr->format = NF_ADDR; /* so instruction doesn't get freed! */
                BGDone( addr );
                ++parms;
            }
        }
    }
    return( parms );
}

static  instruction     *PushDelayed( instruction *ins, an addr, call_state *state )
/***********************************************************************************
    Put the parm "addr" into a temporary, and then add a "push" just
    before the call instruction.
*/
{
    ins->result = BGNewTemp( addr->tipe );
    if( addr->flags & FL_ADDR_CROSSED_BLOCKS ) {
        ins->result->v.usage |= USE_IN_ANOTHER_BLOCK;
    }
    ins = MakeMove( ins->result, AllocRegName( HW_ST0 ), TypeClass( addr->tipe ) );
    Pushes( ins );
    AddIns( ins );
    addr->format = NF_ADDR; /* so instruction doesn't get freed! */
    BGDone( addr );
#if _TARGET & _TARG_8086
    /* unused parameters */ (void)state;
#else
    if( state->attr & ROUTINE_STACK_RESERVE ) {
        ReserveStack( state, ins, addr->tipe->length );
    }
#endif
    return( ins );
}



static bool PushDelayedIfStackOperand( instruction *ins, pn parm, call_state *state )
/************************************************************************************
    Delay the "push" of a floating point parameter if any of its
    operands are 8087 stack operands.
*/
{
    opcnt       i;
    an          addr;

    addr = parm->name;
    ins = addr->u.i.ins;
    for( i = ins->num_operands; i-- > 0; ) {
        if( FPIsStack( ins->operands[i] ) ) {
            parm->ins = PushDelayed( ins, addr, state );
            // CGFree( parm );
            return( true );
        }
    }
    return( false );
}



static bool PushDelayedIfRedefinition( instruction *ins, pn parm, call_state *state )
/************************************************************************************
    Delay the "push" of a floating point parameter if any of its
    operands are  redefined between the calculation of the parm and the
    call.
*/
{
    instruction *next;
    opcnt       i;

    next = ins->head.next;
    for(;;) {
        for( ; next->head.opcode != OP_BLOCK; next = next->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                if( _IsReDefinedBy( next, ins->operands[i] ) ) {
                    parm->ins = PushDelayed( ins, parm->name, state );
                    // CGFree( parm );
                    return( true );
                }
            }
        }
        if( _BLOCK( next ) == CurrBlock )
            break;
        if( _BLOCK( next )->next_block == NULL ) {
            next = CurrBlock->ins.hd.next;
        } else {
            next = _BLOCK( next )->next_block->ins.hd.next;
        }
    }
    return( false );
}

static  void    UseInOther( name *op )
{
    if( op == NULL )
        return;
    switch( op->n.class ) {
    case N_TEMP:
    case N_MEMORY:
        op->v.usage |= USE_IN_ANOTHER_BLOCK;
        break;
    case N_INDEXED:
        UseInOther( op->i.index );
        UseInOther( op->i.base );
        break;
    default:
        break;
    }
}

static  int     FPPushDelay( pn parm, call_state *state )
/********************************************************
    For each parm, move it into a temp, and "push" the temp onto the
    8087 stack just before the call instruction.  If the calculation of
    the parm can be moved down just in front of the call, we do that,
    instead of using a temporary.
*/
{
    instruction *ins;
    instruction *new_ins;
    an          addr;
    int         parms;


    parms = 0;
    for( ; parm != NULL; parm = parm->next ) {
        if( parm->ins == NULL ) {
            addr = parm->name;
            if( (addr->tipe->attr & TYPE_FLOAT) && HW_COvlap( parm->regs,HW_FLTS ) ) {
                ++parms;
                if( addr->format != NF_INS ) {
                    _Zoiks( ZOIKS_043 );
                }
                ins = addr->u.i.ins;
                if( PushDelayedIfStackOperand( ins, parm, state ) )
                    continue;
                if( PushDelayedIfRedefinition( ins, parm, state ) )
                    continue;
                /* we can push it just before the CALL */
                if( addr->flags & FL_ADDR_CROSSED_BLOCKS ) {
                    UseInOther( ins->operands[0] );
                    if( ins->num_operands > 1 ) {
                        UseInOther( ins->operands[1] );
                    }
                }
                new_ins = MakeNary( ins->head.opcode, ins->operands[0],
                                    ins->operands[1], AllocRegName( HW_ST0 ),
                                    ins->type_class, ins->base_type_class,
                                    ins->num_operands );
                Pushes( new_ins );
                new_ins->num_operands = ins->num_operands;
                AddIns( new_ins );
#if _TARGET & _TARG_80386
                if( state->attr & ROUTINE_STACK_RESERVE ) {
                    ReserveStack( state, new_ins, addr->tipe->length );
                }
#endif
                BGDone( addr ); /* so ins DOES get freed*/
                parm->ins = new_ins;
            }
        }
    }
    return( parms );
}

void    FPPushParms( pn parm, call_state *state )
/**********************************************************
    "push" parameters onto the 8087 stack.  If parameters can be pushed
    as soon as they are calculated, do that (FPPushImmed), otherwise we
    have to "push" it onto the 8087 just before the call instruction
    (FPPushDelay). For each parameter there is an address_node (an) of
    type NF_INS, which not had the result field filled in yet.
*/
{
    int parms;

    if( _FPULevel( FPU_87 ) ) {
        parms = 0;
        if( CanPushImmed( parm, &parms ) &&
                parms < Max87Stk &&
                ( state->attr & ROUTINE_STACK_RESERVE ) == 0 ) {
            parms = FPPushImmed( parm );
        } else {
            if( parms != 0 ) {
                parms = FPPushDelay( parm, state );
            }
        }
        SetFPParmsUsed( state, parms );
    }
}

static const opcode_entry    FSINCOS[] = {
/*           op1   op2   res   eq      verify          reg           gen             fu  */
_OE(                         PRESERVE, V_NO,           RG_,          G_FSINCOS,      FU_TRIG )
};

#define _OTHER( x ) ( (x) == OP_SIN ? OP_COS : OP_SIN )

static  bool    FSinCos( instruction *ins1 )
/********************************************
    find the FSINCOS sequence
*/
{
    instruction *ins2;
    instruction *ins3;
    instruction *ins4;

    if( _IsntTargetModel( I_MATH_INLINE ) ) {
        return( false );
    }
    ins2 = ins1->head.next;
    ins3 = ins2->head.next;
    ins4 = ins3->head.next;
    if( G( ins1 ) == G_MFLD ) {
        if( ins2->head.opcode != OP_SIN && ins2->head.opcode != OP_COS ) {
            return( false );
        }
        if( G( ins3 ) != G_MFLD ) {
            return( false );
        }
        if( ins1->operands[0] != ins3->operands[0] ) {
            return( false );
        }
        if( ins4->head.opcode != _OTHER( ins2->head.opcode ) ) {
            return( false );
        }
    } else if( G( ins1 ) == G_RFLD ) {
        if( ins2->head.opcode != OP_SIN && ins2->head.opcode != OP_COS ) {
            return( false );
        }
        if( G( ins3 ) != G_RFLD ) {
            return( false );
        }
        if( FPRegNum( ins1->operands[0] ) != FPRegNum( ins3->operands[0] ) - 1 ) {
            return( false );
        }
        if( ins4->head.opcode != _OTHER( ins2->head.opcode ) ) {
            return( false );
        }
    } else if( ins1->head.opcode == OP_SIN ) {
        if( G( ins2 ) != G_RFLD ) {
            return( false );
        }
        if( FPRegNum( ins2->operands[0] ) != 1 ) {
            return( false );
        }
        if( ins3->head.opcode != _OTHER( ins1->head.opcode ) ) {
            return( false );
        }
        ins4 = ins3;
        ins3 = ins2;
        ins2 = ins1;
    } else {
        return( false );
    }
    ins2->u.gen_table = FSINCOS;
    if( ins2->head.opcode != OP_SIN ) {
        PrefFXCH( ins3, 1 );
    }
    FreeIns( ins3 );
    FreeIns( ins4 );
    return( true );
}

static  instruction     *Next87Ins( instruction *ins )
/****************************************************/
{
    instruction *next;

    for( next = ins->head.next; next->head.opcode != OP_BLOCK; next = next->head.next ) {
        if( DoesSomething( next ) ) {
            return( next );
        }
    }
    return( ins );
}

static instruction *BackUpAndFree( instruction *ins, instruction *junk1,
                                        instruction *junk2 )
/**********************************************************/
{
    instruction *ret;

    ret = ins->head.prev;
    if( junk1 != NULL )
        FreeIns( junk1 );
    if( junk2 != NULL )
        FreeIns( junk2 );
    if( ret->head.opcode == OP_BLOCK )
        ret = ret->head.next;
    return( ret );
}

static  void    MoveThrough( name *from, name *to, instruction *from_ins,
                             instruction *to_ins, name *reg,
                             type_class_def type_class )
/****************************************************
    Move from "from" to "to" using register name "reg". Segments if
    any should be taken from "from_ins" and "to_ins".
*/
{
    bool        dummy;
    instruction *new;

    new = MakeMove( from, reg, type_class );
    new->u.gen_table = FindGenEntry( new, &dummy );
    DupSeg( from_ins, new );
    PrefixIns( to_ins, new );
    new = MakeMove( reg, to, type_class );
    DupSeg( to_ins, new );
    new->u.gen_table = FindGenEntry( new, &dummy );
    PrefixIns( to_ins, new );
}

static  instruction    *To86Move( instruction *ins, instruction *next )
/**********************************************************************
    Turn a move which uses the 8087 (FLD X, FSTP Y) into a move using
    the 8086 using an available 8086 register.
*/
{
    hw_reg_set  *regs;
    name        *reg;
    instruction *ret;

    ret = ins->head.next;
    for( regs = RegSets[RL_MOVE]; ; ++regs ) {
        if( HW_CEqual( *regs, HW_EMPTY ) )
            return( ret );
        if( HW_Ovlap( *regs, next->head.live.regs ) )
            continue;
        if( HW_Ovlap( *regs, ins->head.live.regs ) )
            continue;
        reg = AllocRegName( *regs );
        break;
    }
    if( next->result->n.type_class == FS ) {
#if _TARGET & _TARG_8086
        if( OptForSize > 50 )
            return( ret );
        MoveThrough( LowPart( ins->operands[0], U2 ),
                     LowPart( next->result, U2 ), ins, next, reg, U2 );
        MoveThrough( HighPart( ins->operands[0], U2 ),
                     HighPart( next->result, U2 ), ins, next, reg, U2 );
#else
        MoveThrough( ins->operands[0], next->result, ins, next, reg, U4 );
#endif
    } else {
        if( OptForSize > 50 )
            return( ret );
#if _TARGET & _TARG_8086
        MoveThrough( LowPart( LowPart( ins->operands[0], U4 ), U2 ),
                     LowPart( LowPart( next->result      , U4 ), U2 ),
                     ins, next, reg, U2 );
        MoveThrough( HighPart( LowPart( ins->operands[0], U4 ), U2 ),
                     HighPart( LowPart( next->result      , U4 ), U2 ),
                     ins, next, reg, U2 );
        MoveThrough( LowPart( HighPart( ins->operands[0], U4 ), U2 ),
                     LowPart( HighPart( next->result      , U4 ), U2 ),
                     ins, next, reg, U2 );
        MoveThrough( HighPart( HighPart( ins->operands[0], U4 ), U2 ),
                     HighPart( HighPart( next->result      , U4 ), U2 ),
                     ins, next, reg, U2 );
#else
        MoveThrough( LowPart( ins->operands[0], U4 ),
                     LowPart( next->result, U4 ), ins, next, reg, U4 );
        MoveThrough( HighPart( ins->operands[0], U4 ),
                     HighPart( next->result, U4 ), ins, next, reg, U4 );
#endif
    }
    return( BackUpAndFree( ins, ins, next ) );
}


static  bool    RedundantStore( instruction *ins )
/************************************************/
{
    instruction         *next;

    for( next = ins->head.next; next->head.opcode != OP_BLOCK; next = next->head.next ) {
        if( next->result == ins->result ) {
            if( G( next ) == G_MFSTNP || G( next ) == G_MFST ) {
                return( true );
            }
        }
        if( InsOrderDependant( ins, next ) ) {
            return( false );
        }
    }
    return( false );
}


static  instruction    *Opt87Sequence( instruction *ins, bool *again )
/*********************************************************************
    Look for silly 8087 sequences and fix them up.
*/
{
    instruction         *next;
    instruction         *third;
    instruction         *ret;
    type_class_def      type_class;
    int                 st_reg;

    next = Next87Ins( ins );
    ret = ins->head.next;
    if( next == ins )
        return( ret );
    if( G( ins ) == G_RFLD && FPRegNum( ins->operands[0] ) == 0 ) {
        if( G( next ) == G_MFST ) {

            /* FLD ST, FSTP X ===> FST X */

            FreeIns( ins );
            if( FPResultNotNeeded( next ) ) {
                ret = BackUpAndFree( ins, next, NULL );
            } else {
                ret = next;
                NoPop( next );
            }
            *again = true;
        } else if( G( next ) == G_RFST ) {

            /* FLD ST, FSTP ST(i) ===> FST ST(i-1) */

            st_reg = FPRegNum( next->result );
            if( st_reg == 0 ) {
                ret = BackUpAndFree( ins, ins, next );
            } else {
                next->result = ST( st_reg - 1 );
                FreeIns( ins );
                ret = next;
                NoPop( next );
            }
            *again = true;
        } else if( G( next ) == G_RNFBINP || G( next ) == G_RRFBINP ) {

            /* FLD ST, FopP ST(i),ST ==> Fop ST(i-1),ST */

            st_reg = FPRegNum( next->operands[0] );
            if( st_reg == 0 ) {
                ret = BackUpAndFree( ins, ins, next );
            } else {
                next->operands[0] = ST( st_reg - 1 );
                NoPopRBin( next );
                FreeIns( ins );
                ret = next;
            }
            *again = true;
        } else if( G( next ) == G_FXCH && FPRegNum( next->result ) == 1 ) {
            FreeIns( next );
            *again = true;
            ret = ins;
        }
    } else if( G( ins ) == G_RFLD || G( ins ) == G_MFLD ) {
        if( ( G( next ) == G_RRFBINP || G( next ) == G_RNFBINP ) ) {
            if( G( ins ) == G_MFLD ) {
                type_class = ins->operands[0]->n.type_class;
                if( !_IsFloating( type_class ) ) {
                    return( ret ); /* need convert! */
                }
            }
            if( FPRegNum( next->operands[0] ) == 1 ) {

                /* FLD X, FxxP ST(1) ==> Fxx X */

                next->result = ST( 0 );
                next->operands[0] = ins->operands[0];
                if( ins->num_operands > OpcodeNumOperands( ins ) && next->num_operands <= OpcodeNumOperands( next ) ) {
                    next->operands[next->num_operands] = ins->operands[OpcodeNumOperands( ins )];
                    next->num_operands++;
                }
                FreeIns( ins );
                NoPopBin( next );
                *again = true;
                ret = next;
            }
        } else if( G( next ) == G_RFLD || G( next ) == G_MFLD ) {
            if( G( ins ) == G_MFLD && G( next ) == G_MFLD &&

                /* FLD X, FLD X ==> FLD X, FLD ST(0) */

                ins->operands[0] == next->operands[0] ) {
                next->operands[0] = ST( 0 );
                ToRFld( next );
                *again = true;
                ret = next;
            } else {

                /* FLD X, FLD Y, FXCH ST(1) ==> FLD Y, FLD X */

                third = Next87Ins( next );
                if( third == next )
                    return( ret );
                if( G( third ) == G_FXCH && FPRegNum( third->result ) == 1 ) {
                    FreeIns( third );
                    if( FPRegNum( next->operands[0] ) != 0 ) {
                        ins->head.next->head.prev = ins->head.prev;
                        ins->head.prev->head.next = ins->head.next;
                        SuffixIns( next, ins );
                        if( G( ins ) == G_RFLD ) {
                            st_reg = FPRegNum( ins->operands[0] );
                            ins->operands[0] = ST( st_reg + 1 );
                        }
                        if( G( next ) == G_RFLD ) {
                            st_reg = FPRegNum( next->operands[0] );
                            next->operands[0] = ST( st_reg - 1 );
                        }
                        ret = next;
                    } else {
                        ret = ins;
                    }
                    *again = true;
                }
            }
        } else if( G( ins ) == G_MFLD ) {
            if( G( next ) == G_MFST ) {
                if( ins->operands[0]->n.type_class == next->result->n.type_class ) {
                    ret = To86Move( ins, next );
                }
            } else if( G( next ) == G_MRFBIN || G( next ) == G_MNFBIN ) {
                if( ins->operands[0] == next->operands[0] ) {

                    /* FLD X, FOP X ==> FLD X, FOP ST */

                    next->operands[0] = ST( 0 );
                    DelSeg( next );
                    NoMemBin( next );
                    *again = true;
                    ret = ins;
                }
            }
        }
    } else if( G( ins ) == G_MFST || G( ins ) == G_RFST ) {
        if( G( next ) == G_MFLD
          && ( next->operands[0] == ins->result )
          &&   _IsFloating( ins->result->n.type_class )
          && !IsVolatile( ins->result ) ) {

            /* FSTP X, FLD X ==> FST X */

            FreeIns( next );
            if( FPResultNotNeeded( ins ) ) {
                ret = BackUpAndFree( ins, ins, NULL );
            } else {
                ret = ins;
                NoPop( ins );
            }
            *again = true;
        } else if( G( next ) == G_RFLD
             && FPRegNum( next->operands[0] ) + 1 == FPRegNum( ins->result ) ) {

            /* FSTP ST(i), FLD ST(i-1) ==> FST ST(i) */

            FreeIns( next );
            NoPop( ins );
            *again = true;
            ret = ins;
        }
    } else if( G( ins ) == G_FXCH ) {
        if( G( next ) == G_FXCH ) {

            /* FXCH ST(i) FXCH ST(i) => nothing */

            if( FPRegNum( ins->result ) == FPRegNum( next->result ) ) {
                FreeIns( next );
                next = BackUpAndFree( ins, ins, NULL );
                *again = true;
                ret = next;
            }
        } else {
            if( G( next ) == G_RRFBINP || G( next ) == G_RNFBINP ) {

                if( FPRegNum( ins->result ) == FPRegNum( next->operands[0] ) ) {

                    /* FXCH ST(i), FopP ST(i),ST -> FopRP ST(i),ST */

                    FreeIns( ins );
                    ReverseFPGen( next );
                    *again = true;
                    ret = next;
                }
            }
        }
    } else if( G( ins ) == G_MFSTNP ) {
        if( G( next ) == G_MFLD && ins->result == next->operands[0] ) {

            /* FST X, FLD X => FST X, FLD ST */

            next->operands[0] = ST( 0 );
            ToRFld( next );
            *again = true;
            ret = ins;
        } else if( RedundantStore( ins ) ) {
            ret = BackUpAndFree( ins, ins, NULL );
            *again = true;
        }
    } else if( G( ins ) == G_RNFBIN || G( ins ) == G_RRFBIN ) {
        if( G( next ) == G_RFST && next->result == ins->operands[0] ) {
            ToPopBin( ins );
            FreeIns( next );
            *again = true;
            ret = ins;
        }
    }
    return( ret );
}

static void    Opt8087( void )
/*****************************
    Look for silly 8087 sequences and change them into better ones.
*/
{
    block       *blk;
    instruction *ins;
    int         i;
    bool        again;
    instruction *next;

    for( blk = HeadBlock; blk != NULL; ) {
        i = 0;
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            ins->sequence = ++i;
        }
        again = false;
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
            next = ins->head.next;
            if( _GenIs8087( G( ins ) ) ) {
                if( !FSinCos( ins ) ) {
                    next = Opt87Sequence( ins, &again );
                }
            }
        }
        if( !again ) {
            for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
                if( FPResultNotNeeded( ins ) ) {
                    ins->result = ST( 0 );
                    ToRFstp( ins );
                }
            }
            blk = blk->next_block;
        }
    }
}


void    FPOptimize( void )
/*****************************

    Fix up the 8087 instructions.  The instructions so far
*/
{
    if( _FPULevel( FPU_87 ) ) {
        if( _IsntModel( NO_OPTIMIZATION ) ) {
            Opt8087();
        }
        Wait8087();
    }
}

bool    FPDivIsADog( type_class_def type_class )
/**********************************************/
{

    return( _FPULevel( FPU_87 ) && _IsFloating( type_class ) );
}

#if 0
void    FixP5Divs( void )
/***********************/
{
    block       *blk;
    instruction *ins;

    if( _IsntTargetModel( P5_DIVIDE_CHECK ) )
        return;
    if( !_FPULevel( FPU_87 ) )
        return;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( ins->head.opcode != OP_DIV )
                continue;
            if( !_IsFloating( ins->type_class ) )
                continue;
            ins->head.opcode = OP_P5DIV;
        }
    }
}
#endif
