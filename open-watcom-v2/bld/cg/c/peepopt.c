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
* Description:  Perform peephole optimizations.
*
****************************************************************************/


/* - collapse adjacent BIT operations (AND,OR) */
/* - collapse adjacent integral ADD,SUB operations */
/* - collapse adjacent integral MUL operations */

#include "_cgstd.h"
#include "coderep.h"
#include "indvars.h"
#include "score.h"
#include "zoiks.h"
#include "data.h"
#include "makeins.h"
#include "namelist.h"
#include "peepopt.h"
#include "redefby.h"
#include "insdead.h"
#include "optimize.h"
#include "inssched.h"


#define OP2VAL( ins ) ( (ins)->operands[1]->c.lo.int_value )

typedef bool    (ONE_OP)(instruction *,instruction *);
typedef bool    (TWO_OP)(instruction *,instruction *);
typedef bool    (THREE_OP)(instruction *,instruction *);

static  bool    AfterRegAlloc;

static  bool    InsChangeable( instruction *ins, name *new_op, name **op )
/************************************************************************/
{
    if( AfterRegAlloc ) {
        return( ChangeIns( ins, new_op, op, CHANGE_GEN | CHANGE_CHECK ) );
    }
    return( true );
}

static  void    InsReset( instruction *ins ) {
/********************************************/
    if( AfterRegAlloc ) {
        ResetGenEntry( ins );
    }
}

static  bool    MergeTwo( instruction *a, instruction *b, signed_32 value )
/*************************************************************************/
{
    name        *cons;
    name        *old_res_a;
    name        *old_res_b;

    cons = AllocS32Const( value );
    if( a->result == b->result ) {
        /* A op X -> A     A op f(X,Y) -> A */
        /* A op Y -> A                      */
        if( InsChangeable( a, cons, &a->operands[1] ) ) {
            a->operands[1] = cons;
            ResetGenEntry( a );
            FreeIns( b );
            return( true );
        }
    } else if( a->operands[0] == b->operands[0] ) {
        /* A op X -> A     A op f(X,Y) -> B */
        /* A op Y -> B     A op X      -> A */
        /* becomes */

        /* This could be bad if we had something like
                A op X -> A
                USE( B )
                A op Y -> B
            which can occur quite frequently after register allocation,
            so we make sure all instructions between a and b are
            InsOrderDependant with b. BBB - Apr 27, 1994
        */
        instruction *ins;

        for( ins = a->head.next; ins != b; ins = ins->head.next ) {
            if( InsOrderDependant( ins, b ) )
                return( false );
            if( ins->head.opcode == OP_BLOCK ) {
                Zoiks( ZOIKS_111 );
            }
        }

        if( InsChangeable( a, b->result, &a->result ) &&
            InsChangeable( b, a->operands[0], &b->result ) ) {
            old_res_a = a->result;
            old_res_b = b->result;
            a->result = b->result;
            b->result = a->operands[0];
            if( InsChangeable( a, cons, &a->operands[1] ) &&
                InsChangeable( b, a->operands[1], &b->operands[1] ) ) {
                b->operands[1] = a->operands[1];
                a->operands[1] = cons;
                ResetGenEntry( a );
                ResetGenEntry( b );
                return( true );
            } else {
                /* retreat! retreat! */
                a->result = old_res_a;
                b->result = old_res_b;
            }
        }
    } else if( b->operands[0] != a->operands[0] || b->operands[1] != cons ) {
        /* A op X -> B     A op X      -> B */
        /* B op Y -> C     A op f(X,Y) -> C */
        if( InsChangeable( b, a->operands[0], &b->operands[0] ) &&
            InsChangeable( b, cons, &b->operands[1] ) ) {
            b->operands[0] = a->operands[0];
            b->operands[1] = cons;
            ResetGenEntry( b );
            return( true );
        }
    }
    return( false );
}

static  ONE_OP  DoAdd;
static  bool    DoAdd( instruction *a, instruction *b )
/*******************************************************/
{
    return( MergeTwo( a, b, OP2VAL( a ) + OP2VAL( b ) ) );
}


static  ONE_OP  DoMul;
static  bool    DoMul( instruction *a, instruction *b )
/*******************************************************/
{
    return( MergeTwo( a,b, OP2VAL( a ) * OP2VAL( b ) ) );
}


static  ONE_OP  DoAnd;
static  bool    DoAnd( instruction *a, instruction *b )
/**************************************************************/
{
    return( MergeTwo( a,b, OP2VAL( a ) & OP2VAL( b ) ) );
}


static  ONE_OP  DoOr;
static  bool    DoOr( instruction *a, instruction *b )
/*******************************************************/
{
    return( MergeTwo( a,b, OP2VAL( a ) | OP2VAL( b ) ) );
}


static TWO_OP AndOr;
static bool AndOr( instruction *and_ins, instruction *or_ins )
/************************************************************/
{
    signed_32   new_and;
    name        *mask;

    if( and_ins->operands[0] != and_ins->result )
        return( false );
    if( or_ins->operands[0] != or_ins->result )
        return( false );
    new_and = OP2VAL( and_ins ) & ~OP2VAL( or_ins );
    if( new_and == OP2VAL( and_ins ) )
        return( false );
    mask = AllocS32Const( new_and );
    if( !InsChangeable( and_ins, mask, &and_ins->operands[1] ) )
        return( false );
    and_ins->operands[1] = mask;
    InsReset( and_ins );
    return( true );
}


static TWO_OP OrAnd;
static bool OrAnd( instruction *or_ins, instruction *and_ins )
/************************************************************/
{
    signed_32   new_or;
    signed_32   or;
    signed_32   and;
    name        *mask;

    if( or_ins->operands[0] != or_ins->result )
        return( false );
    if( and_ins->operands[0] != and_ins->result )
        return( false );
    or = OP2VAL( or_ins );
    and = OP2VAL( and_ins );
    if( ( and & or ) == and ) { // the AND bits are a subset of the OR bits
        or_ins->operands[0] = AllocS32Const( and );
        or_ins->num_operands = 1;
        or_ins->head.opcode = OP_MOV;
        or_ins->table = NULL;
        InsReset( or_ins );
        FreeIns( and_ins );
        return( true );
    } else {
        new_or = or & and;
        if( new_or == OP2VAL( or_ins ) )
            return( false );
        mask = AllocS32Const( new_or );
        if( !InsChangeable( or_ins, mask, &or_ins->operands[1] ) )
            return( false );
        or_ins->operands[1] = mask;
        InsReset( or_ins );
        return( true );
    }
}


static THREE_OP OrAndOr;
static bool OrAndOr( instruction *a, instruction *b )
/***************************************************/
{
    signed_32   and_val;
    name        *mask;

    if( a->result != b->result )
        return( false );
    a->head.opcode = OP_AND;
    b->head.opcode = OP_OR;
    and_val = OP2VAL( a ) & OP2VAL( b );
    mask = AllocS32Const( and_val );
    if( InsChangeable( a, b->operands[1], &a->operands[1] ) &&
        InsChangeable( b, mask, &b->operands[1] ) ) {
        a->operands[1] = b->operands[1];
        b->operands[1] = mask;
        a->table = b->table = NULL;
        InsReset( a );
        InsReset( b );
        return( true );
    }
    /* Put things back the way they were */
    a->head.opcode = OP_OR;
    b->head.opcode = OP_AND;
    return( false );
}


static THREE_OP AndOrAnd;
static bool AndOrAnd( instruction *a, instruction *b )
/****************************************************/
{
    signed_32   or_val;
    name        *mask;

    if( a->result != b->result )
        return( false );
    a->head.opcode = OP_OR;
    b->head.opcode = OP_AND;
    or_val = OP2VAL( a ) | OP2VAL( b );
    mask = AllocS32Const( or_val );
    if( InsChangeable( a, b->operands[1], &a->operands[1] ) &&
        InsChangeable( b, mask, &b->operands[1] ) ) {
        a->operands[1] = b->operands[1];
        b->operands[1] = mask;
        a->table = b->table = NULL;
        InsReset( a );
        InsReset( b );
        return( true );
    }
    /* Put things back the way they were */
    a->head.opcode = OP_AND;
    b->head.opcode = OP_OR;
    return( false );
}


static  bool    DoRShift( instruction *a, instruction *b )
/*******************************************************/
{
    if( a->type_class != b->type_class )
        return( false );
    return( DoAdd( a, b ) );
}


static  bool    DoLShift( instruction *a, instruction *b )
/********************************************************/
{
    return( DoAdd( a, b ) );
}


static TWO_OP LRShift;
static bool LRShift( instruction *a, instruction *b )
/*************************************************/
{
    /* unused parameters */ (void)a; (void)b;

    return( false );
}


static TWO_OP RLShift;
static bool RLShift( instruction *a, instruction *b )
/*************************************************/
{
    /* unused parameters */ (void)a; (void)b;

    return( false );
}


static THREE_OP Nop3;
static bool Nop3( instruction *a, instruction *b )
/****************************************************/
{
    /* unused parameters */ (void)a; (void)b;

    return( false );
}


static bool SameOpWithConst( instruction *ins, instruction *next )
/*****************************************************************/
{
    name        *op;

    if( ins->result != next->operands[0] )
        return( false );
    op = next->operands[1];
    if( op->n.class != N_CONSTANT )
        return( false );
    if( op->c.const_type != CONS_ABSOLUTE )
        return( false );
    return( true );
}


static bool OpConst( instruction *ins )
/*************************************/
{
    name        *op;

    if( ins->num_operands != 2 )
        return( false );
    op = ins->operands[1];
    if( op->n.class != N_CONSTANT )
        return( false );
    if( op->c.const_type != CONS_ABSOLUTE )
        return( false );
    if( !_IsIntegral( ins->type_class ) )
        return( false );
    return( true );
}


static  instruction     *FindInsPair( instruction *ins,
                                     instruction **stopper,
                                     bool *pchange, opcode_defs op,
                                     opcode_defs op2,
                                     ONE_OP *oprtn )
/**********************************************************************/
{
    instruction         *next;

    if( stopper != NULL )
        *stopper = NULL;
    for( next = ins->head.next; next->head.opcode != OP_BLOCK; next = next->head.next ) {
        if( next->head.opcode == op && SameOpWithConst( ins, next ) ) {
            if( oprtn( ins, next ) ) {
                *pchange = true;
                return( ins );
            }
        }
        if( InsOrderDependant( ins, next ) ) {
            if( stopper != NULL && next->head.opcode == op2 &&
                SameOpWithConst( ins, next ) ) {
                *stopper = next;
            }
            return( ins->head.next );
        }
    }
    return( ins->head.next );
}


static instruction *FindInsTriple( instruction *ins, bool *pchange,
                                   opcode_defs op1, opcode_defs op2,
                                   ONE_OP *op1rtn, ONE_OP *op2rtn,
                                   TWO_OP *op12rtn, THREE_OP *op121rtn )
/************************************************************************/
{
    bool        change;
    instruction *ins2;
    instruction *ins3;
    instruction *next;

    change = false;
    next = FindInsPair( ins, &ins2, &change, op1, op2, op1rtn );
    if( change ) {
        *pchange = true;
        return( next );
    } else if( ins2 != NULL ) {
        FindInsPair( ins2, &ins3, &change, op2, op1, op2rtn );
        if( change ) {
            *pchange = true;
            return( ins );
        }
        if( ins3 == NULL ) {
            if( op12rtn( ins, ins2 ) ) {
                *pchange = true;
                return( ins );
            }
            return( next );
        }
        if( op121rtn( ins, ins2 ) ) {
            *pchange = true;
            return( ins );
        }
        return( ins->head.next );
    }
    return( next );
}


static instruction *AddOpt( instruction *ins, bool *pchange )
/***********************************************************/
{
    return( FindInsPair( ins, NULL, pchange, OP_ADD, OP_NOP, DoAdd ) );
}

static instruction *SubOpt( instruction *ins, bool *pchange )
/***********************************************************/
{
    // DoAdd will work fine for subtracts
    return( FindInsPair( ins, NULL, pchange, OP_SUB, OP_NOP, DoAdd ) );
}


static instruction *MulOpt( instruction *ins, bool *pchange )
/***********************************************************/
{
    return( FindInsPair( ins, NULL, pchange, OP_MUL, OP_NOP, DoMul ) );
}


static instruction *OrOpt( instruction *ins, bool *pchange )
/***********************************************************/
{
    return( FindInsTriple( ins, pchange, OP_OR, OP_AND,
                           DoOr, DoAnd, OrAnd, OrAndOr ) );
}


static instruction *AndOpt( instruction *ins, bool *pchange )
/***********************************************************/
{
    return( FindInsTriple( ins, pchange, OP_AND, OP_OR,
                           DoAnd, DoOr, AndOr, AndOrAnd ) );
}


static instruction *LShiftOpt( instruction *ins, bool *pchange )
/**************************************************************/
{
    return( FindInsTriple( ins, pchange, OP_LSHIFT, OP_RSHIFT,
                           DoLShift, DoRShift, LRShift, Nop3 ) );
}


static instruction *RShiftOpt( instruction *ins, bool *pchange )
/**************************************************************/
{
    return( FindInsTriple( ins, pchange, OP_RSHIFT, OP_LSHIFT,
                           DoRShift, DoLShift, RLShift, Nop3 ) );
}

static bool DoArithmeticOps( instruction *ins, bool *change, instruction **n )
/****************************************************************************/
{
    instruction *next;

    if( ins->type_class == I8 || ins->type_class == U8 )
        return( false );
    if( !OpConst( ins ) )
        return( false );
    switch( ins->head.opcode ) {
    case OP_ADD:
        next = AddOpt( ins, change );
        break;
    case OP_SUB:
        next = SubOpt( ins, change );
        break;
    case OP_MUL:
        next = MulOpt( ins, change );
        break;
    case OP_AND:
        next = AndOpt( ins, change );
        break;
    case OP_OR:
        next = OrOpt( ins, change );
        break;
    case OP_LSHIFT:
        next = LShiftOpt( ins, change );
        break;
    case OP_RSHIFT:
        next = RShiftOpt( ins, change );
        break;
    default:
        return( false );
    }
    *n = next;
    return( true );
}

static bool DoConversionOps( instruction *ins, bool *change, instruction **n )
/****************************************************************************/
{
    instruction *next;

    /* unused parameters */ (void)n;

    switch( ins->head.opcode ) {
    case OP_CONVERT:
    case OP_PTR_TO_NATIVE:
    case OP_PTR_TO_FOREIGN:
        break;
    default:
        return( false );
    }
    next = ins->head.next;
    if( _IsReDefinedBy( ins, ins->operands[0] ) )
        return( false );       // BBB - cnv U2 U1 [eax] -> ax
    for( ; next->head.opcode != OP_BLOCK; next = next->head.next ) {
        if( _IsReDefinedBy( next, ins->result ) )
            return( false );
        if( _IsReDefinedBy( next, ins->operands[0] ) )
            return( false );
        if( ins->head.opcode == OP_CONVERT && next->head.opcode == OP_CONVERT ) {
            if( ins->result == next->operands[0] &&
                ins->type_class == next->base_type_class ) {

                // change (cnv i1 -> i2; cnv i2 -> i4) into
                //     (cnv i1 -> i2; cnv i1 -> i4) and let dead code take care of
                //         the first ins if it is not needed

                // pointer conversions are too dangerous to fold
                if( _IsPointer( next->type_class ) || _IsPointer( next->base_type_class ) )
                    return( false );
                if( _IsPointer( ins->type_class ) )
                    return( false );
                // watch for converting down - bad to fold
                if( ins->type_class < ins->base_type_class )
                    return( false );
                next->base_type_class = ins->base_type_class;
                next->operands[0] = ins->operands[0];
                if( next->operands[0]->n.class == N_TEMP ) {
                    next->operands[0]->t.temp_flags &= ~CAN_STACK;
                }
                // in case we are in register allocator
                next->table = NULL;
                *change = true;
                return( true );
            }
        }
        if( ( ins->head.opcode == OP_PTR_TO_NATIVE && next->head.opcode == OP_PTR_TO_FOREIGN ) ||
            ( ins->head.opcode == OP_PTR_TO_FOREIGN && next->head.opcode == OP_PTR_TO_NATIVE ) ) {
            if( ins->result == next->operands[0] ) {
                next->operands[0] = ins->operands[0];
                next->head.opcode = OP_MOV;
                next->table = NULL;
                *change = true;
                return( true );
            }
        }
    }
    return( false );
}

static bool ReferencedBy( instruction *ins, name *op )
/******************************************************/
{
    opcnt       i;
    name        *curr;

    if( _OpIsCall( ins->head.opcode ) ) {
        if( _IsVisibleToCall( ins, op, false ) ) {
            return( true );
        }
    }
    // this should only be called for index names and USE_ADDRESS temps
    for( i = 0; i < ins->num_operands; i++ ) {
        curr = ins->operands[i];
        // assume anything which looks at memory uses op
        if( curr->n.class == N_INDEXED ||
            curr->n.class == N_MEMORY )
            return( true );
        if( SameThing( curr, op ) ) {
            return( true );
        }
    }
    return( false );
}

static bool DoMemWrites( instruction *ins, bool *change, instruction **n ) {
/**************************************************************************/
    instruction *next;

    /* unused parameters */ (void)n;

    if( ins->result != NULL ) {
        switch( ins->result->n.class ) {
        case N_TEMP:
            if( ( ins->result->v.usage & USE_ADDRESS ) == 0 )
                break;
            /* fall down */
        case N_INDEXED:
        case N_MEMORY:
            if( SideEffect( ins ) )
                return( false );
            for( next = ins->head.next; next->head.opcode != OP_BLOCK; next = next->head.next ) {
                if( ReferencedBy( next, ins->result ) )
                    break;
                if( _IsReDefinedBy( next, ins->result ) ) {
                    if( next->result == ins->result ) {
                        *change = true;
                        FreeIns( ins );
                        return( true );
                    }
                    break;
                }
            }
        }
    }
    return( false );
}

bool PeepOptBlock( block *blk, bool after_reg_alloc )
/***************************************************/
{
    instruction *ins;
    instruction *next;
    bool        change;

    change = false;
    AfterRegAlloc = after_reg_alloc;
    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
        next = ins->head.next;
        if( VolatileIns( ins ) )
            continue;
        if( DoArithmeticOps( ins, &change, &next ) )
            continue;
        if( DoConversionOps( ins, &change, &next ) )
            continue;
        if( DoMemWrites( ins, &change, &next ) ) {
            continue;
        }
    }
    return( change );
}

bool PeepOpt( block *start, block *(*func)(block *, void *parm), void *parm, bool after_reg_alloc )
/*************************************************************************************************/
{
    block       *blk;
    bool        change;

    change = false;
    for( blk = start; blk != NULL; blk = func( blk, parm ) ) {
        change |= PeepOptBlock( blk, after_reg_alloc );
    }
    return( change );
}
