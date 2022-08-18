/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  386 instruction splitting (simplifications/optimizations).
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "regset.h"
#include "model.h"
#include "system.h"
#include "zoiks.h"
#include "cfloat.h"
#include "confldef.h"
#include "makeins.h"
#include "namelist.h"
#include "rgtbl.h"
#include "split.h"
#include "insutil.h"
#include "optab.h"
#include "inssegs.h"
#include "optimize.h"
#include "fixindex.h"
#include "x86split.h"
#include "overlap.h"
#include "x86segs.h"
#include "liveinfo.h"
#include "x86splt2.h"


name    *LowPart( name *tosplit, type_class_def type_class )
/**********************************************************/
{
    name                *new = NULL;
    signed_8            s8;
    unsigned_8          u8;
    signed_16           s16;
    unsigned_16         u16;
    constant_defn       *floatval;

    switch( tosplit->n.class ) {
    case N_CONSTANT:
        if( tosplit->c.const_type == CONS_ABSOLUTE ) {
            if( type_class == U1 ) {
                u8 = tosplit->c.lo.int_value & 0xff;
                new = AllocUIntConst( u8 );
            } else if( type_class == I1 ) {
                s8 = tosplit->c.lo.int_value & 0xff;
                new = AllocIntConst( s8 );
            } else if( type_class == U2 ) {
                u16 = tosplit->c.lo.int_value & 0xffff;
                new = AllocUIntConst( u16 );
            } else if( type_class == I2 ) {
                s16 = tosplit->c.lo.int_value & 0xffff;
                new = AllocIntConst( s16 );
            } else if( type_class == I4 ) {
                new = AllocS32Const( tosplit->c.lo.int_value );
            } else if( type_class == U4 ) {
                new = AllocU32Const( tosplit->c.lo.uint_value );
            } else if( type_class == FL ) {
                _Zoiks( ZOIKS_129 );
            } else { /* FD */
                floatval = GetFloat( tosplit, FD );
                new = AllocConst( CFCnvU32F( _TargetLongInt( *(unsigned_32 *)( floatval->value + 0 ) ) ) );
            }
        } else if( tosplit->c.const_type == CONS_ADDRESS ) {
            new = AddrConst( tosplit->c.value, (segment_id)tosplit->c.lo.int_value, CONS_OFFSET );
        } else {
            _Zoiks( ZOIKS_044 );
        }
        break;
    case N_REGISTER:
        if( type_class == U1 || type_class == I1 ) {
            new = AllocRegName( Low16Reg( tosplit->r.reg ) );
        } else if( type_class == U2 || type_class == I2 ) {
            new = AllocRegName( Low32Reg( tosplit->r.reg ) );
        } else {
            new = AllocRegName( Low64Reg( tosplit->r.reg ) );
        }
        break;
    case N_TEMP:
        new = TempOffset( tosplit, 0, type_class );
        if( new->t.temp_flags & CONST_TEMP ) {
            name *cons = tosplit->v.symbol;
            if( tosplit->n.type_class == FD ) {
                cons = Int64Equivalent( cons );
            }
            new->v.symbol = LowPart( cons, type_class );
        }
        break;
    case N_MEMORY:
        new = AllocMemory( tosplit->v.symbol, tosplit->v.offset,
                                tosplit->m.memory_type, type_class );
        new->v.usage = tosplit->v.usage;
        break;
    case N_INDEXED:
        new = ScaleIndex( tosplit->i.index, tosplit->i.base,
                                tosplit->i.constant, type_class,
                                0, tosplit->i.scale, tosplit->i.index_flags );
        break;
    }
    return( new );
}

name    *OffsetPart( name *tosplit )
/**********************************/
{
    name        *new;

    switch( tosplit->n.class ) {
    case N_REGISTER:
        return( AllocRegName( Low48Reg( tosplit->r.reg) ) );
    case N_CONSTANT:
        if( tosplit->c.const_type == CONS_ABSOLUTE ) {
            return( tosplit );
        } else if( tosplit->c.const_type == CONS_ADDRESS ) {
            return( AddrConst( tosplit->c.value, (segment_id)tosplit->c.lo.int_value, CONS_OFFSET ) );
        } else {
            _Zoiks( ZOIKS_044 );
            return( tosplit );
        }
    case N_TEMP:
        new = LowPart( tosplit, U4 );
        if( new->t.temp_flags & CONST_TEMP ) {
            new->v.symbol = OffsetPart( tosplit->v.symbol );
        }
        return( new );
    case N_MEMORY:
    case N_INDEXED:
        return( LowPart( tosplit, U4 ) );
    default:
        _Zoiks( ZOIKS_129 );
        return( NULL );
    }
}


name    *SegmentPart( name *tosplit )
/***********************************/
{
    name        *new;
    name        *seg;

    switch( tosplit->n.class ) {
    case N_CONSTANT:
        if( tosplit->c.const_type == CONS_ABSOLUTE ) {
            return( AllocIntConst( 0 ) );
        } else if( tosplit->c.const_type == CONS_ADDRESS ) {
            return( AddrConst( tosplit->c.value, (segment_id)tosplit->c.lo.int_value, CONS_SEGMENT ) );
        } else {
            _Zoiks( ZOIKS_044 );
            return( NULL );
        }
        break;
    case N_REGISTER:
        return( AllocRegName( High48Reg( tosplit->r.reg ) ) );
    case N_TEMP:
        new = TempOffset( tosplit, 4, U2 );
        if( new->t.temp_flags & CONST_TEMP ) {
            seg = SegmentPart( tosplit->v.symbol );
            if( seg->n.class == N_REGISTER )
                return( seg );
            new->v.symbol = seg;
        }
        return( new );
    case N_MEMORY:
        new = AllocMemory( tosplit->v.symbol,
                                tosplit->v.offset + 4,
                                tosplit->m.memory_type, U2 );
        new->v.usage = tosplit->v.usage;
        return( new );
    case N_INDEXED:
        new = ScaleIndex( tosplit->i.index, tosplit->i.base,
                tosplit->i.constant+ 4, U2, 0,
                tosplit->i.scale, tosplit->i.index_flags );
        return( new );
    default:
        _Zoiks( ZOIKS_129 );
        return( NULL );
    }
}


name    *HighPart( name *tosplit, type_class_def type_class )
/***********************************************************/
{
    name                *new = NULL;
    signed_8            s8;
    unsigned_8          u8;
    signed_16           s16;
    unsigned_16         u16;
    constant_defn       *floatval;

    switch( tosplit->n.class ) {
    case N_CONSTANT:
        if( tosplit->c.const_type == CONS_ABSOLUTE ) {
            if( type_class == U1 ) {
                u8 = ( tosplit->c.lo.int_value >> 8 ) & 0xff;
                new = AllocUIntConst( u8 );
            } else if( type_class == I1 ) {
                s8 = ( tosplit->c.lo.int_value >> 8 ) & 0xff;
                new = AllocIntConst( s8 );
            } else if( type_class == U2 ) {
                u16 = ( tosplit->c.lo.int_value >> 16 ) & 0xffff;
                new = AllocUIntConst( u16 );
            } else if( type_class == I2 ) {
                s16 = ( tosplit->c.lo.int_value >> 16 ) & 0xffff;
                new = AllocIntConst( s16 );
            } else if( type_class == I4 ) {
                new = AllocS32Const( tosplit->c.hi.int_value );
            } else if( type_class == U4 ) {
                new = AllocU32Const( tosplit->c.hi.uint_value );
            } else if( type_class == FL ) {
                _Zoiks( ZOIKS_129 );
            } else { /* FD */
                floatval = GetFloat( tosplit, FD );
                new = AllocConst( CFCnvU32F( _TargetLongInt( *(unsigned_32 *)( floatval->value + 2 ) ) ) );
            }
        } else if( tosplit->c.const_type == CONS_ADDRESS ) {
            new = AddrConst( tosplit->c.value, (segment_id)tosplit->c.lo.int_value, CONS_SEGMENT );
        } else {
            _Zoiks( ZOIKS_044 );
        }
        break;
    case N_REGISTER:
        if( type_class == U1 || type_class == I1 ) {
            new = AllocRegName( High16Reg( tosplit->r.reg ) );
        } else if( type_class == U2 || type_class == I2 ) {
            new = AllocRegName( High32Reg( tosplit->r.reg ) );
        } else {
            new = AllocRegName( High64Reg( tosplit->r.reg ) );
        }
        break;
    case N_TEMP:
        new = TempOffset( tosplit, tosplit->n.size / 2, type_class );
        if( new->t.temp_flags & CONST_TEMP ) {
            name *cons = tosplit->v.symbol;
            if( tosplit->n.type_class == FD ) {
                cons = Int64Equivalent( cons );
            }
            new->v.symbol = HighPart( cons, type_class );
        }
        break;
    case N_MEMORY:
        new = AllocMemory( tosplit->v.symbol,
                                tosplit->v.offset + tosplit->n.size / 2,
                                tosplit->m.memory_type, type_class );
        new->v.usage = tosplit->v.usage;
        break;
    case N_INDEXED:
        new = ScaleIndex( tosplit->i.index, tosplit->i.base,
                tosplit->i.constant+ tosplit->n.size / 2, type_class,
                0, tosplit->i.scale, tosplit->i.index_flags );
        break;
    }
    return( new );
}


instruction     *SplitUnary( instruction *ins )
/**********************************************
 * Pushing floating point */
{
    instruction     *new_ins;
    name            *high_res;
    name            *low_res;

    HalfType( ins );
    if( ins->result == NULL ) {
        high_res = NULL;
        low_res = NULL;
    } else {
        high_res = HighPart( ins->result, ins->type_class );
        low_res  = LowPart( ins->result, ins->type_class );
    }
    new_ins = MakeUnary( ins->head.opcode,
                         LowPart( ins->operands[0], ins->type_class ),
                         low_res, ins->type_class );
    ins->operands[0] = HighPart( ins->operands[0],ins->type_class );
    ins->result = high_res;
    if( ins->head.opcode == OP_PUSH ) {
        DupSeg( ins, new_ins );
        SuffixIns( ins, new_ins );
        new_ins = ins;
    } else {
        DupSeg( ins, new_ins );
        PrefixIns( ins, new_ins );
    }
    return( new_ins );
}


instruction     *rSPLITPUSH( instruction *ins )
/**********************************************
 * Pushing a 6 byte pointer */
{
    instruction     *new_ins;
    instruction     *ins2;
    name            *op;
    name            *temp;

    new_ins = MakeUnary( ins->head.opcode, OffsetPart( ins->operands[0] ), NULL, U4 );
    ins->operands[0] = SegmentPart( ins->operands[0] );
    op = ins->operands[0];
    DupSeg( ins, new_ins );
    if( ins->head.opcode == OP_PUSH ) {
        SuffixIns( ins, new_ins );
        ChangeType( ins, U2 );
        return( ins );
    } else {
        PrefixIns( ins, new_ins );
        if( op->n.class != N_REGISTER ) {
            ChangeType( ins, U4 );
            temp = AllocTemp( U4 );
            ins2 = MakeMove( LowPart( temp, U2 ), op, U2 );
            ins->operands[0] = temp;
            MoveSegOp( ins, ins2, 1 );
            SuffixIns( ins, ins2 );
        } else {
            ChangeType( ins, U2 );
        }
        return( new_ins );
    }
}

instruction     *rMAKEU2( instruction *ins )
/*******************************************
 * for 48 bit pointers operations */
{
    instruction     *new_ins;
    instruction     *ins2;
    name            *temp;

    if( Overlaps( ins->result, ins->operands[0] )
     || ( ins->num_operands >= 2 &&
            Overlaps( ins->result, ins->operands[1] ) ) ) {
        ChangeType( ins, U4 );
        if( ins->result != NULL ) {
            new_ins = MakeMove( SegmentPart( ins->operands[0] ), AllocTemp( U2 ), U2 );
            temp = SegmentPart( ins->result );
            ins->result = OffsetPart( ins->result );
            DupSegOp( ins, new_ins, 0 );
            PrefixIns( ins, new_ins );
        } else {
            temp = NULL;
            new_ins = ins;
        }
        ins->operands[0] = OffsetPart( ins->operands[0] );
        if( ins->result != NULL ) {
            ins2 = MakeMove( new_ins->result, temp, U2 );
            DupSegRes( ins, ins2 );
            SuffixIns( ins, ins2 );
        }
    } else {
        ChangeType( ins, U4 );
        if( ins->result != NULL ) {
            new_ins = MakeMove( SegmentPart( ins->operands[0] ), SegmentPart( ins->result ), U2 );
            ins->result = OffsetPart( ins->result );
            DupSegOp( ins, new_ins, 0 );
            DupSegRes( ins, new_ins );
            PrefixIns( ins, new_ins );
        } else {
            new_ins = ins;
        }
        ins->operands[0] = OffsetPart( ins->operands[0] );
        if( OpcodeNumOperands( ins ) >= 2 ) {
            ins->operands[1] = OffsetPart( ins->operands[1] );
        }
    }
    return( new_ins );
}



instruction      *rLOADLONGADDR( instruction *ins )
/*************************************************/
{
    instruction         *new_ins;
    name                *name1;

    new_ins = MakeMove( SegName( ins->operands[0] ),
                            SegmentPart( ins->result ), U2 );
    ChangeType( ins, U4 );
    ins->result = OffsetPart( ins->result );
    name1 = ins->operands[0];
    if( name1->n.class == N_INDEXED ) {
        if( name1->i.index->n.size == 6 ) {
            ins->operands[0] = ScaleIndex(
                OffsetPart( name1->i.index ),
                name1->i.base, name1->i.constant,
                name1->n.type_class, name1->n.size,
                name1->i.scale, name1->i.index_flags );
        }
    }
    DupSegRes( ins, new_ins );
    PrefixIns( ins, new_ins );
    return( new_ins );
}


instruction     *rHIGHCMP( instruction *ins )
/********************************************
 * floating point comparison with 0 */
{
    if( ins->type_class == FD ) {
        ins->operands[0] = HighPart( ins->operands[0], SW );
    }
    ChangeType( ins, SW );
    return( ins );
}


instruction     *rMAKEU4( instruction *ins )
/*******************************************
 * change pointer '==' or '!=' to 6 byte compare */
{
    name                *left;
    name                *rite;
    instruction         *new_ins;
    instruction         *low;
    instruction         *high;
    instruction         *first;
    byte                true_idx;
    byte                false_idx;
    name                *lseg;
    name                *rseg;
    name                *temp;

    left = ins->operands[0];
    rite = ins->operands[1];
    true_idx = _TrueIndex( ins );
    false_idx = _FalseIndex( ins );
    if( ins->head.opcode == OP_CMP_NOT_EQUAL ) {
        low = MakeCondition( ins->head.opcode, OffsetPart( left ),
                             OffsetPart( rite ), true_idx, NO_JUMP, U4 );
    } else {
        low = MakeCondition( ins->head.opcode, OffsetPart( left ),
                             OffsetPart( rite ), false_idx, NO_JUMP, U4 );
    }
    lseg = SegmentPart( left );
    first = NULL;
    if( lseg->n.class == N_REGISTER && ( HW_COvlap( lseg->r.reg, HW_SEGS ) ) ) {
        temp = AllocTemp( U2 );
        new_ins = MakeMove( lseg, temp, U2 );
        PrefixIns( ins, new_ins );
        if( first == NULL ) first = new_ins;
        lseg = temp;
    }
    rseg = SegmentPart( rite );
    if( rseg->n.class == N_REGISTER && ( HW_COvlap( rseg->r.reg, HW_SEGS ) ) ) {
        temp = AllocTemp( U2 );
        new_ins = MakeMove( rseg, temp, U2 );
        PrefixIns( ins, new_ins );
        if( first == NULL ) first = new_ins;
        rseg = temp;
    }
    high = MakeCondition( ins->head.opcode, lseg,
                          rseg, true_idx, false_idx, U2 );
    if( low->head.opcode == OP_CMP_EQUAL ) {
        low->head.opcode = OP_CMP_NOT_EQUAL;
    }
    if( first == NULL ) first = low;
    DupSeg( ins, low );
    PrefixIns( ins, low );
    DupSeg( ins, high );
    ReplIns( ins, high );
    UpdateLive( first, high );
    return( first );
}


instruction     *rCLRHI_D( instruction *ins )
/*******************************************/
{
    name                *high;
    name                *low;
    instruction         *new_ins;
    type_class_def      half_type_class;

    half_type_class = HalfClass[ins->type_class];
    low = LowPart( ins->result, half_type_class );
    high = HighPart( ins->result, half_type_class );
    ChangeType( ins, half_type_class );
    ins->head.opcode = OP_MOV;
    ins->result = low;
    new_ins = MakeMove( AllocS32Const( 0 ), high, half_type_class );
    DupSegRes( ins, new_ins );         /* 2004-11-01  RomanT (same as bug #341) */
    PrefixIns( ins, new_ins );
    return( new_ins );
}


instruction     *rEXT_PUSH1( instruction *ins )
/*********************************************/
{
    name        *temp;
    instruction *new_ins;

    temp = AllocTemp( U4 );
    new_ins = MakeConvert( ins->operands[0], temp, U4, U1 );
    ins->operands[0] = temp;
    ChangeType( ins, U4 );
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    return( new_ins );
}



instruction     *rEXT_PUSH2( instruction *ins )
/*********************************************/
{
    name        *temp;
    instruction *new_ins;

    temp = AllocTemp( U4 );
    new_ins = MakeConvert( ins->operands[0], temp, U4, U2 );
    ins->operands[0] = temp;
    ChangeType( ins, U4 );
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    return( new_ins );
}

instruction     *rINTCOMP( instruction *ins )
/*******************************************/
{
    name                *left;
    name                *right;
    instruction         *low;
    instruction         *high;
    type_class_def      half_type_class;
    byte                true_idx;
    byte                false_idx;
    byte                first_idx;
    bool                rite_is_zero;

    rite_is_zero = CFTest( ins->operands[1]->c.value ) == 0;
    if( ins->type_class == FS ) {
        ChangeType( ins, I4 );
        // V_INTCOMP ensures that this is only called
        // for OP_CMP_EQUAL and OP_CMP_NOT_EQUAL if
        // rite_is_zero
        if( rite_is_zero ) {
            ins->operands[1] = AllocS32Const( 0x7fffffff );
            if( ins->head.opcode == OP_CMP_EQUAL ) {
                ins->head.opcode = OP_BIT_TEST_FALSE;
            } else if( ins->head.opcode == OP_CMP_NOT_EQUAL ) {
                ins->head.opcode = OP_BIT_TEST_TRUE;
            }
        } else {
            ins->operands[1] = IntEquivalent( ins->operands[1] );
        }
        return( ins );
    }
    half_type_class = HalfClass[ins->type_class];
    left = ins->operands[0];
    if( left->n.class == N_CONSTANT && left->c.const_type == CONS_ABSOLUTE ) {
        CnvOpToInt( ins, 0 );
        left = ins->operands[0];
    }
    right = ins->operands[1];
    if( right->n.class == N_CONSTANT && right->c.const_type == CONS_ABSOLUTE ) {
        CnvOpToInt( ins, 1 );
        right = ins->operands[1];
    }
    true_idx = _TrueIndex( ins );
    false_idx = _FalseIndex( ins );
    first_idx = ( ins->head.opcode == OP_CMP_EQUAL ) ? false_idx : true_idx;
    if( rite_is_zero ) {
        high = MakeCondition( OP_BIT_TEST_TRUE,
                        HighPart( left, half_type_class ),
                        AllocS32Const( 0x7fffffff ),
                        first_idx, NO_JUMP, half_type_class );
    } else {
        high = MakeCondition( OP_CMP_NOT_EQUAL,
                        HighPart( left, half_type_class ),
                        HighPart( right, half_type_class ),
                        first_idx, NO_JUMP, half_type_class );
    }
    DupSeg( ins, high );
    PrefixIns( ins, high );
    low = MakeCondition( ins->head.opcode,
                    LowPart( left, half_type_class ),
                    LowPart( right, half_type_class ),
                    true_idx, false_idx,
                    half_type_class );
    DupSeg( ins, low );
    ReplIns( ins, low );
    return( high );
}

instruction     *rCDQ( instruction *ins )
/***************************************/
{
    instruction *ins2;
    name        *high;

    high = HighPart( ins->result, WD );
    ins->result = LowPart( ins->result, WD );
    ins->head.opcode = OP_MOV;
    ins->type_class = WD;
    ins->base_type_class = WD;
    ins->table = NULL;
    ins2 = MakeBinary( OP_RSHIFT, ins->operands[0], AllocIntConst( 31 ), high, SW );
    DupSeg( ins, ins2 );
    SuffixIns( ins, ins2 );
    return( ins );
}

instruction     *rCONVERT_UP( instruction *ins )
/**********************************************/
{
    name                *temp;
    instruction         *ins1;
    instruction         *ins2;
    type_class_def      type_class;

    // change a CNV I8 I1 op -> res into a pair of instructions
    //          CNV I4 I1 op -> temp and CNV I8 I4 temp -> res
    //
    // 2005-09-25 RomanT
    // Fixed wrong type of temp.variable (must have sign of source operand).
    // Now it goes: U1/2->U4->U8/I8, I1/2->I4->U8/I8.
    //
    type_class = ( _IsSigned( ins->base_type_class ) ) ? I4 : U4;
    temp = AllocTemp( type_class );
    ins1 = MakeConvert( ins->operands[0], temp, type_class, ins->base_type_class );
    DupSegOp( ins, ins1, 0 );    // 2005-09-25 RomanT (bug #341)
    PrefixIns( ins, ins1 );
    ins2 = MakeConvert( temp, ins->result, ins->type_class, type_class );
    DupSegRes( ins, ins2 );      // 2004-11-01 RomanT (same as bug #341, *pInt64=char)
    ReplIns( ins, ins2 );
    return( ins1 );
}

instruction     *rCYP_SEX( instruction *ins )
/*******************************************/
{
    name            *op;
    name            *new_op;
    instruction     *shft;
    int             size;

    op = ins->operands[0];
    size = WORD_SIZE - op->n.size;
    new_op = OpAdjusted( op, -size, SW );
    ChangeType( ins, SW );
    ins->operands[0] = new_op;
    ins->head.opcode = OP_MOV;
    op = ins->result;
    shft = MakeBinary( OP_RSHIFT, op, AllocIntConst( size * 8 ), op, SW );
    DupSeg( ins, shft );
    SuffixIns( ins, shft );
    return( ins );
}

instruction     *rSPLIT8( instruction *ins ) { return( ins ); }
instruction     *rSPLIT8BIN( instruction *ins ) { return( ins ); }
instruction     *rSPLIT8NEG( instruction *ins ) { return( ins ); }
instruction     *rSPLIT8TST( instruction *ins ) { return( ins ); }
instruction     *rSPLIT8CMP( instruction *ins ) { return( ins ); }
#if 0
instruction     *rCLRHIGH_DW( instruction *ins ) { return( ins ); }
instruction     *rSEX_DW( instruction *ins ) { return( ins ); }
#endif
instruction     *rCYPSHIFT( instruction *ins ) { return( ins ); }
instruction     *rBYTESHIFT( instruction *ins ) { return( ins ); }
instruction     *rMOVE8LOW( instruction *ins ) { return( ins ); }
