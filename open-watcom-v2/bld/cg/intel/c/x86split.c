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
* Description:  Intel i86/386 instruction spliting reductions.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cfloat.h"
#include "system.h"
#include "zoiks.h"
#include "makeins.h"
#include "convins.h"
#include "data.h"
#include "utils.h"
#include "namelist.h"
#include "x86objd.h"
#include "regalloc.h"
#include "x86obj.h"
#include "split.h"
#include "insutil.h"
#include "rtcall.h"
#include "inssegs.h"
#include "fixindex.h"
#include "revcond.h"
#include "conflict.h"
#include "x86split.h"
#include "x86segs.h"
#include "liveinfo.h"
#include "x86table.h"
#include "x86rtrn.h"
#include "_split.h"
#include "x86splt1.h"
#include "x86splt2.h"
#include "_x86rtrn.h"
#include "_x86half.h"


instruction *(*ReduceTab[])( instruction * ) = {
    #define _R_( x, f )     f
    #include "r.h"
    #undef _R_
};


bool    UnChangeable( instruction *ins )
/**************************************/
{
    return( IsString( ins->table ) );
}

instruction      *rSAVEFACE( instruction *ins )
/*********************************************/
{
    instruction         *new_ins;

    // we have a EDX:EAX op 1 or DX:AX op 1 here which the constant
    // folder was not able to catch, because of the weird regs
    new_ins = MakeMove( AllocRegName( HW_xAX ), ins->result, ins->type_class );
    ReplIns( ins, new_ins );
    return( new_ins );
}

instruction      *rMULSAVEFACE( instruction *ins )
/************************************************/
{
    instruction         *new_ins;

    // we have a r1 mul 1 -> DX:AX here which the constant
    // folder was not able to catch, because of the weird regs
    new_ins = MakeMove( ins->operands[0], AllocRegName( HW_xAX ), ins->type_class );
    ReplIns( ins, new_ins );
    return( new_ins );
}

instruction      *rMAKECYPMUL( instruction *ins )
/***********************************************/
{
    HalfType( ins );
    return( ins );
}


instruction      *rSEG_SEG( instruction *ins )
/********************************************/
{
    instruction         *new_ins;
    name                *name1;

    name1 = AllocTemp( U2 );
    new_ins = MakeMove( ins->operands[0], name1, U2 );
    ins->operands[0] = name1;
    PrefixIns( ins, new_ins );
    MarkPossible( ins, name1, RL_WORD );
    ins->u.gen_table = NULL;
    GiveRegister( NameConflict( ins, name1 ), true );
    return( new_ins );
}


instruction      *rOP2CL( instruction *ins )
/******************************************/
{
    instruction         *new_ins;
    name                *name1;

    name1 = AllocRegName( HW_CL );
    new_ins = MakeConvert( ins->operands[1], name1, U1, ins->operands[1]->n.type_class );
    ins->operands[1] = name1;
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    return( new_ins );
}


instruction      *rOP2CX( instruction *ins )
/******************************************/
{
    instruction         *new_ins;
    name                *name1;

    name1 = AllocRegName( HW_CX );
    new_ins = MakeConvert( ins->operands[1], name1, U2, ins->operands[1]->n.type_class );
    ins->operands[1] = name1;
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    return( new_ins );
}


instruction      *rMULREGISTER( instruction *ins )
/************************************************/
{
    instruction         *new_ins;
    instruction         *ins2;
    name                *name1;
    name                *name2;

    name1 = AllocRegName( Op1Reg( ins ) );
    new_ins = MakeMove( ins->operands[0], name1, ins->type_class );
    ins->operands[0] = name1;
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    name2 = AllocRegName( ResultReg( ins ) );
    ins2 = MakeUnary( OP_MOV, name1, ins->result, ins->type_class );
    ins2->base_type_class = DoubleClass[ins->type_class];
    ins->result = name2;
    ins->zap = &AllocRegName( ZapReg( ins ) )->r;
    MoveSegRes( ins, ins2 );
    SuffixIns( ins, ins2 );
    return( new_ins );
}


instruction      *rDIVREGISTER( instruction *ins )
/************************************************/
{
    instruction         *new_ins;
    instruction         *ins2;
    name                *name1;
    name                *name2;

    name1 = AllocRegName( Op1Reg( ins ) );
    new_ins = MakeConvert( ins->operands[0], name1, DoubleClass[ins->type_class], ins->type_class );
    ins->operands[0] = name1;
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    name2 = AllocRegName( ResultReg( ins ) );
    ins2 = MakeMove( name2, ins->result, ins->type_class );
    ins->result = name2;
    ins->zap = &AllocRegName( ZapReg( ins ) )->r;
    MoveSegRes( ins, ins2 );
    SuffixIns( ins, ins2 );
    return( new_ins );
}

name    *IntEquivalent( name *name )
/**********************************/
{
    constant_defn       *defn;

    defn = GetFloat( name, FS );
    return( AllocConst( CFCnvU32F( _TargetBigInt( *(unsigned_32 *)( defn->value + 0 ) ) ) ) );
}

name    *Int64Equivalent( name *name )
/************************************/
{
    constant_defn       *defn;

    defn = GetFloat( name, FD );
    return( AllocU64Const( *(unsigned_32 *)( defn->value + 0 ), *(unsigned_32 *)( defn->value + 2 ) ) );
}

instruction      *rFSCONSCMP( instruction *ins )
/**********************************************/
{
    name                *name1;

    name1 = ins->operands[1];
    if( CFTest( name1->c.value ) > 0 ) {
        ChangeType( ins, I4 );
    } else {
        ChangeType( ins, U4 );
        RevCond( ins );
    }
    ins->operands[1] = IntEquivalent( name1 );
    ins->table = NULL;
    return( ins );
}


instruction      *rHIGHLOWMOVE( instruction *ins )
/************************************************/
{
    instruction         *new_ins;

/* for moving constants such as 0xabcdabcd*/
    new_ins = MakeMove( LowPart( ins->result, U2 ), HighPart( ins->result, U2 ), U2 );
    ins->result = LowPart( ins->result, U2 );
    ChangeType( ins, U2 );
    DupSegRes( ins, new_ins );
    SuffixIns( ins, new_ins );
    return( ins );
}

#if _TARGET & _TARG_8086
    #define     LP              PT
    #define     MOV_SIZE        3
#else
    #define     LP              CP
    #define     MOV_SIZE        5
#endif

bool UseRepForm( unsigned size )
/*************************************
    Do we want to use "rep movs{w|d}", or string out multiple "movs{w|d}"
*/
{
    unsigned    count;
    unsigned    extra = 0;
    unsigned    rep_startup;
    unsigned    rep_iter;
    unsigned    movs_cost;

    count = size / WORD_SIZE;
    /* if move than 10 movs, then always use rep form */
    if( count > 10 )
        return( true );
    if( OptForSize > 50 ) {
        switch( size % WORD_SIZE ) {
        case 0: extra = 0;      break;
        case 1: extra = 1;      break;
#if WORD_SIZE > 2
        case 2: extra = 1;      break;
        case 3: extra = 2;      break;
#endif
        }
        return( count + extra > MOV_SIZE + 2 );
    }
    if( _CPULevel( CPU_586 ) ) {
        rep_startup = 11;
        rep_iter    = 1;
        movs_cost   = 4;
    } else if( _CPULevel( CPU_486 ) ) {
        rep_startup = 12;
        rep_iter    = 3;
        movs_cost   = 7;
    } else if( _CPULevel( CPU_386 ) ) {
        rep_startup = 8;
        rep_iter    = 4;
        movs_cost   = 8;
    } else if( _CPULevel( CPU_286 ) ) {
        rep_startup = 5;
        rep_iter    = 4;
        movs_cost   = 5;
    } else {
        rep_startup = 9;
        rep_iter    = 17;
        movs_cost   = 18;
    }
    /* The "+ 4" is to account for the "MOV [E]CX, const" clocks and some
        slop for larger instructions (cache effects) */
    return( rep_startup + (count * rep_iter) + 4 < movs_cost * count );
}

static  bool    CanLoadStringOps( instruction *ins ) {
/****************************************************/

    hw_reg_set  needs;

    if( UseRepForm( ins->operands[0]->n.size ) ) {
        needs = HW_xCX;
    } else {
        HW_CAsgn( needs, HW_EMPTY );
    }
    HW_CAsgn( needs, HW_xSI );
    HW_CTurnOn( needs, HW_xDI );
    if( _IsTargetModel( FLOATING_DS ) ) {
        HW_CTurnOn( needs, HW_DS );
    }
    if( _IsTargetModel( FLOATING_ES ) ) {
       HW_CTurnOn( needs, HW_ES );
    }
    return( !HW_Ovlap( needs, ins->head.live.regs ) );
}

static name *FakeIndex( name *op, hw_reg_set index ) {
/****************************************************/

    name                *base;
    i_flags             flags;

    if( op->n.class == N_TEMP || op->n.class == N_MEMORY ) {
        base = op;
        flags = X_FAKE_BASE;
    } else {
        base = NULL;
        flags = EMPTY;
    }
    return( ScaleIndex( AllocRegName( index ), base, 0, op->n.type_class, op->n.size, 0, flags ) );
}

static  bool    SegmentFloats( name *op ) {
/***************************************/

    name        *segname;

    segname = SegName( op );
    if( segname->n.class != N_REGISTER )
        return( true );
    if( HW_COvlap( segname->r.reg, HW_DS ) )
        return( _IsTargetModel( FLOATING_DS ) );
    if( HW_COvlap( segname->r.reg, HW_SS ) )
        return( _IsTargetModel( FLOATING_SS ) );
    return( true );
}

static  instruction     *LoadStringOps( instruction *ins,
                                        name **op1, name **op2 ) {
/****************************************************************/


    instruction         *load_op1;
    instruction         *load_op2;
    instruction         *load_len;
    instruction         *first_ins;
    instruction         *pop;
    name                *ds_reg;
    name                *es_reg;
    hw_reg_set          new_op1;
    bool                ds_needs_save;
    bool                es_needs_save;

    if( ins->head.opcode == OP_MOV && !UseRepForm( (*op1)->n.size ) ) {
        load_len = NULL;
        HW_CAsgn( new_op1, HW_EMPTY );
    } else {
        if( ( (*op1)->n.size & (WORD_SIZE - 1) ) == 0
          || ( OptForSize <= 50 && ins->head.opcode == OP_MOV ) ) {
            load_len = MoveConst( (*op1)->n.size / WORD_SIZE, AllocRegName( HW_xCX ), WD );
            PrefixIns( ins, load_len );
        } else {
            load_len = MoveConst( (*op1)->n.size, AllocRegName( HW_xCX ), WD );
            PrefixIns( ins, load_len );
        }
        new_op1 = HW_xCX;
    }
    /* careful here. Make sure we load DS last*/
    if( ins->num_operands > OpcodeNumOperands( ins ) ) {
        if( (*op1)->n.class == N_INDEXED || (*op1)->n.class == N_MEMORY ) {
            load_op1 = MakeUnary( OP_LA, *op2, AllocRegName( HW_ES_xDI ), LP );
            PrefixIns( ins, load_op1 );
            load_op2 = MakeUnary( OP_LA, *op1, AllocRegName( HW_xSI ), WD );
            PrefixIns( ins, load_op2 );
            load_op2 = MakeMove( ins->operands[ins->num_operands - 1], AllocRegName( HW_DS ), U2 );
            PrefixIns( ins, load_op2 );
            es_needs_save = SegmentFloats( *op2 );
            ds_needs_save = true;
        } else {
            load_op1 = MakeMove( ins->operands[ins->num_operands - 1], AllocRegName( HW_ES ), U2 );
            PrefixIns( ins, load_op1 );
            load_op2 = MakeUnary( OP_LA, *op2, AllocRegName( HW_xDI ), WD );
            PrefixIns( ins, load_op2 );
            load_op2 = MakeUnary( OP_LA, *op1, AllocRegName( HW_DS_xSI ), LP );
            PrefixIns( ins, load_op2 );
            ds_needs_save = SegmentFloats( *op1 );
            es_needs_save = true;
        }
        DelSeg( ins );
    } else if( _IsTargetModel( FLAT_MODEL ) && !SegmentFloats( *op1 ) && !SegmentFloats( *op2 ) ) {
        load_op1 = MakeUnary( OP_LA, *op2, AllocRegName( HW_xDI ), WD );
        PrefixIns( ins, load_op1 );
        load_op2 = MakeUnary( OP_LA, *op1, AllocRegName( HW_xSI ), WD );
        PrefixIns( ins, load_op2 );
        ds_needs_save = es_needs_save = false;
    } else {
        load_op1 = MakeUnary( OP_LA, *op2, AllocRegName( HW_ES_xDI ), LP );
        PrefixIns( ins, load_op1 );
        load_op2 = MakeUnary( OP_LA, *op1, AllocRegName( HW_DS_xSI ), LP );
        PrefixIns( ins, load_op2 );
        ds_needs_save = SegmentFloats( *op1 );
        es_needs_save = SegmentFloats( *op2 );
    }
    if( _IsntTargetModel( FLOATING_DS ) && ds_needs_save ) { /* restore DS*/
        ds_reg = AllocRegName( HW_DS );
        pop = MakeUnary( OP_POP, NULL, ds_reg, U2 );
        pop->num_operands = 0;
        SuffixIns( ins, pop );
        PrefixIns( load_op2, MakeUnary( OP_PUSH, ds_reg, NULL, U2 ) );
    }
    if( _IsntTargetModel( FLOATING_ES ) && es_needs_save ) { /* restore ES */
        es_reg = AllocRegName( HW_ES );
        pop = MakeUnary( OP_POP, NULL, es_reg, U2 );
        pop->num_operands = 0;
        SuffixIns( ins, pop );
        PrefixIns( load_op1, MakeUnary( OP_PUSH, es_reg, NULL, U2 ) );
        load_op1 = load_op1->head.prev;
    }
    HW_CTurnOn( new_op1, HW_DS_xSI );
    HW_CTurnOn( new_op1, HW_ES_xDI );
    ins->table = GetString();
    ins->head.state = INS_NEEDS_WORK;
    *op1 = FakeIndex( ins->operands[0], new_op1 );
    ins->operands[1] = ins->operands[0];
    HW_CTurnOff( new_op1, HW_SS );
    HW_CTurnOff( new_op1, HW_xSP );
    HW_CTurnOff( new_op1, HW_ES );
    HW_CTurnOff( new_op1, HW_DS );
    ins->zap = &AllocRegName( new_op1 )->r;
    if( ins->head.opcode == OP_MOV ) {
        ins->result = FakeIndex( ins->result, HW_ES_xDI );
    } else {
        ins->result = NULL;
    }
    if( load_len != NULL ) {
        first_ins = load_len;
    } else {
        first_ins = load_op1;
    }
    UpdateLive( first_ins, ins );
    return( first_ins );
}


instruction      *rMAKESTRCMP( instruction *ins )
/***********************************************/
{
    return( LoadStringOps( ins, &ins->operands[0], &ins->operands[1] ) );
}


instruction      *rMAKESTRMOVE( instruction *ins )
/************************************************/
{
    return( LoadStringOps( ins, &ins->operands[0], &ins->result ) );
}


instruction      *rMAYBSTRMOVE( instruction *ins )
/************************************************/
{
    if( CanLoadStringOps( ins ) ) {
        return( LoadStringOps( ins, &ins->operands[0], &ins->result ) );
    } else {
        return( rSPLIT8( ins ) );
    }
}


instruction      *rEXT_PUSHC( instruction *ins )
/**********************************************/
{
    CnvOpToInt( ins, 0 );
    ChangeType( ins, WD );
    return( ins );
}


instruction      *rMOVELOW( instruction *ins )
/********************************************/
/* e.g. convert U2==>U1*/
{
    ins->head.opcode = OP_MOV;
    ins->operands[0] = LowPart( ins->operands[0], ins->type_class );
    ins->table = NULL;
    return( ins );
}


instruction      *rSPLITUNARY( instruction *ins )
/***********************************************/
{
    instruction         *new_ins;

    CnvOpToInt( ins, 0 );
    new_ins = SplitUnary( ins );
    return( new_ins );
}


instruction      *rMOVRESMEM( instruction *ins )
/**********************************************/
{
    instruction         *new_ins;
    name                *name_flt;
    name                *name_int;
    type_class_def      type_class;

    type_class = ins->type_class;
    name_flt = AllocTemp( type_class );
    if( type_class == FD || type_class == FL ) {
        name_int = name_flt;
    } else {
        name_int = TempOffset( name_flt, 0, U4 );
    }
    name_flt->v.usage |= USE_MEMORY | NEEDS_MEMORY;
    new_ins = MakeMove( name_int, ins->result, name_int->n.type_class);
    ins->result = name_flt;
    MoveSegRes( ins, new_ins );
    SuffixIns( ins, new_ins );
    return( ins );
}


instruction      *rMOVOP1MEM( instruction *ins )
/**********************************************/
{
    instruction         *new_ins;
    name                *name_flt;
    name                *name_int;
    type_class_def      type_class;

    type_class = ins->type_class;
    name_flt = AllocTemp( type_class );
    if( type_class == FD || type_class == FL ) {
        name_int = name_flt;
    } else {
        name_int = TempOffset( name_flt, 0, U4 );
    }
    name_flt->v.usage |= USE_MEMORY | NEEDS_MEMORY;
    new_ins = MakeMove( ins->operands[0], name_int, name_int->n.type_class);
    ins->operands[0] = name_flt;
    MoveSegOp( ins, new_ins, 0 );
    PrefixIns( ins, new_ins );
    return( new_ins );
}


instruction      *rOP1CMEM( instruction *ins )
/********************************************/
{
    ins->operands[0] = Addressable( ins->operands[0], ins->type_class );
    return( ins );
}


instruction      *rOP2CMEM( instruction *ins )
/********************************************/
{
    ins->operands[1] = Addressable( ins->operands[1], ins->type_class );
    return( ins );
}


instruction      *rU_TEST( instruction *ins )
/*******************************************/
{
    instruction         *new_ins;
    instruction         *ins2;
    name                *name1;
    name                *name2;

    ChangeType( ins, U2 );
    name1 = ins->operands[0];
    name2 = AllocTemp( U2 );
    new_ins = MakeMove( HighPart( name1, U2 ), name2, U2 );
    ins2 = MakeBinary( OP_OR, name2, LowPart( name1, U2 ), name2, U2 );
    ins->operands[0] = name2;
    DupSeg( ins, new_ins );
    DupSeg( ins, ins2 );
    DelSeg( ins );
    PrefixIns( ins, new_ins );
    PrefixIns( ins, ins2 );
    return( new_ins );
}


instruction      *rCPSUB( instruction *ins )
/******************************************/
{
    instruction         *new_ins;

    new_ins = MakeBinary( OP_SUB, OffsetPart( ins->operands[0] ),
                           OffsetPart( ins->operands[1] ),
                           ins->result, SW );
    DupSeg( ins, new_ins );
    ReplIns( ins, new_ins );
    return( new_ins );
}


instruction      *rPTSUB( instruction *ins )
/******************************************/
{
    instruction         *new_ins;

    ChangeType( ins, PT );
    new_ins = rMAKECALL( ins );
    return( new_ins );
}


instruction      *rEXTPT( instruction *ins )
/******************************************/
{
    instruction         *new_ins;
    name                *name1;

    if( ins->ins_flags & INS_CODE_POINTER ) {
        name1 = AllocRegName( HW_CS );
    } else {
        name1 = NearSegment();
    }
    new_ins = MakeMove( name1, SegmentPart( ins->result ), U2 );
    ins->head.opcode = OP_MOV;
    ins->type_class = WD;
    ins->result = OffsetPart( ins->result );
    ins->table = NULL;
    DupSegRes( ins, new_ins );
    PrefixIns( ins, new_ins );
    return( new_ins );
}


instruction     *rMAKEU4CONS( instruction *ins )
/**********************************************/
{
    ChangeType( ins, U4 );
    ins->operands[0] = IntEquivalent( ins->operands[0] );
    return( ins );
}


instruction      *rCHPPT( instruction *ins )
/******************************************/
{
    ins->head.opcode = OP_MOV;
    ChangeType( ins, WD );
    ins->operands[0] = OffsetPart( ins->operands[0] );
    return( ins );
}


/* NB: The following two routines are intended for 386 only */

instruction      *rMOVPTI8( instruction *ins )
/********************************************/
{
    instruction         *new_ins;
    instruction         *ins2;

    new_ins = MakeMove( OffsetPart( ins->operands[0] ), LowPart( ins->result, U4 ), U4 );
    ins2    = MakeConvert( SegmentPart( ins->operands[0] ), HighPart( ins->result, U4 ), U4, U2 );
    DupSegRes( ins, ins2 );
    SuffixIns( ins, ins2 );
    ReplIns( ins, new_ins );
    return( new_ins );
}

instruction      *rMOVI8PT( instruction *ins )
/********************************************/
{
    instruction         *new_ins;
    instruction         *ins2;

    new_ins = MakeMove( LowPart( ins->operands[0], U4 ), OffsetPart( ins->result ), U4 );
    ins2    = MakeMove( HighPart( ins->operands[0], U2 ), SegmentPart( ins->result ), U2 );
    DupSegRes( ins, ins2 );
    SuffixIns( ins, ins2 );
    ReplIns( ins, new_ins );
    return( new_ins );
}


void    CheckCC( instruction *ins, instruction *new_ins )
/*******************************************************/
{
    if( ins->head.opcode == OP_EXT_ADD || ins->head.opcode == OP_EXT_SUB ) {
        new_ins->table = GetMoveNoCCEntry(); /* ensure it doesn't set the condition codes */
        new_ins->ins_flags |= INS_CC_USED;
    }
}



static  instruction     *SplitPush( instruction *ins, type_length size ) {
/************************************************************************/

    instruction         *new_ins;
    instruction         *first_ins;
    name                *op;
    name                *new_op = NULL;

    size = _RoundUp( size, WORD_SIZE );
    op = ins->operands[0];
    first_ins = NULL;
    for( ;; ) {
        size -= WORD_SIZE;
        switch( op->n.class ) {
        case N_MEMORY:
            new_op = AllocMemory( op->v.symbol, op->v.offset + size,
                                  op->m.memory_type, WD );
            break;
        case N_TEMP:
            new_op = TempOffset( op, size, WD );
            new_op->t.temp_flags |= CG_INTRODUCED;
            break;
        case N_INDEXED:
            new_op = ScaleIndex( op->i.index, op->i.base,
                              op->i.constant + size, WD, 0,
                              op->i.scale, op->i.index_flags );
            break;
        default:
            Zoiks( ZOIKS_051 );
            break;
        }
        new_ins = MakeUnary( OP_PUSH, new_op, NULL, WD );
        if( size == 0 )
            break;
        if( first_ins == NULL ) {
            first_ins = new_ins;
        }
        DupSeg( ins, new_ins );
        PrefixIns( ins, new_ins );
    }
    DupSeg( ins, new_ins );
    ReplIns( ins, new_ins );
    if( first_ins == NULL )
        return( new_ins );
    return( first_ins );
}


instruction     *rDOLONGPUSH( instruction *ins )
/**********************************************/
{
    name        *sp;
    name        *at_sp;
    name        *temp;
    type_length size;
    instruction *move;
    instruction *sub_sp;
    instruction *temp_ins;
    hw_reg_set  hw_ss_sp;

    size = ins->operands[0]->n.size;
    if( size <= 4 * WORD_SIZE ) {
        return( SplitPush( ins, size ) );
    } else {
        HW_CAsgn( hw_ss_sp, HW_SS );
        HW_CTurnOn( hw_ss_sp, HW_xSP );
        sp = AllocRegName( HW_xSP );
        if( _IsTargetModel( FLOATING_SS ) ) {
            temp = AllocTemp( CP );
            temp_ins = MakeMove( AllocRegName( hw_ss_sp ), temp, CP );
        } else {
            temp = AllocTemp( WD );
            temp_ins = MakeMove( sp, temp, WD );
        }
        at_sp = AllocIndex( temp, NULL, 0, XX );
        at_sp->n.size = size;
        move = MakeMove( ins->operands[0], at_sp, XX );
        ReplIns( ins, move );
        ins = LoadStringOps( move, &move->operands[0], &move->result );
        /*% mov CX,const will be the first if it's there so try for SUB SP,CX*/
        sub_sp = MakeBinary( OP_SUB, sp, AllocIntConst( _RoundUp( size, WORD_SIZE ) ), sp, WD );
        if( ins->result != NULL && HW_CEqual( ins->result->r.reg, HW_xCX ) ) {
            SuffixIns( ins, sub_sp );
        } else {
            PrefixIns( ins, sub_sp );
            ins = sub_sp;
        }
        SuffixIns( sub_sp, temp_ins );
    }
    return( ins );
}


name    *OpAdjusted( name *op, int bias, type_class_def type_class )
/*********************************************************************
    Return a new op of type 'type' which is offset from the old op by the
    amount specified by 'bias'.
*/
{
    name        *new_op = NULL;

    switch( op->n.class ) {
    case N_MEMORY:
        new_op = AllocMemory( op->v.symbol, op->v.offset + bias, op->m.memory_type, type_class );
        break;
    case N_TEMP:
        new_op = TempOffset( op, bias, type_class );
        new_op->t.temp_flags |= CG_INTRODUCED;
        break;
    case N_INDEXED:
        new_op = ScaleIndex( op->i.index, op->i.base,
                          op->i.constant + bias, SW, 0,
                          op->i.scale, op->i.index_flags );
        break;
    default:
        Zoiks( ZOIKS_051 );
        break;
    }
    return( new_op );
}


instruction     *rFLIPSIGN( instruction *ins )
/********************************************/
{
    instruction         *new;
    name                *new_op;

    new_op = OpAdjusted( ins->operands[0], ins->operands[0]->n.size - 1, U1 );
    new = MakeBinary( OP_XOR, new_op, AllocIntConst( 0x80 ), new_op, U1 );
    DupSegRes( ins, new );
    ReplIns( ins, new );
    return( new );
}


instruction     *rTEMP2CONST( instruction *ins )
/**********************************************/
{
    opcnt       i;
    name        *op;
    instruction *new;

    /* 2005-05-14 RomanT
     * Never modify const temps operands "in place" - ReplIns() will be
     * unable to move conflict edges and they will point to nowhere.
     * Instead, new instruction must be created and ReplIns()'ed.
     */
    new = NewIns( ins->num_operands );
    Copy( ins, new, sizeof( instruction ) );  // without operands
    for( i = ins->num_operands; i-- > 0; ) {
        op = ins->operands[i];
        if( _ConstTemp( op ) ) {
            new->operands[i] = op->v.symbol;
        } else {
            new->operands[i] = op;
        }
    }
    ReplIns( ins, new );
    return ( new );
}


void    CnvOpToInt( instruction * ins, opcnt op )
/***********************************************/
{
    name                *name1;

    switch( ins->type_class ) {
    case FS:
        name1 = ins->operands[op];
        if( name1->n.class == N_CONSTANT ) {
            ins->operands[op] = IntEquivalent( name1 );
        }
        break;
#if _TARGET & _TARG_80386
    // this is for the I8 stuff - can't tell what to do in
    // HighPart and LowPart if we don't get rid on constant
    // here
    case FD:
        name1 = ins->operands[op];
        if( name1->n.class == N_CONSTANT ) {
            ins->operands[op] = Int64Equivalent( name1 );
        }
        break;
#endif
    default:
        break;
    }
}


instruction     *rCMPCP( instruction *ins )
/*****************************************/
{
    assert( ins->type_class == CP );
    assert( ins->operands[1]->n.class == N_CONSTANT );
    assert( ins->operands[1]->c.lo.int_value == 0 );
    if( ins->head.opcode == OP_CMP_EQUAL ||
        ins->head.opcode == OP_CMP_NOT_EQUAL ) {
        if( _IsTargetModel( NULL_SELECTOR_BAD ) ) {
            ins->operands[0] = HighPart( ins->operands[0], U2 );
            ChangeType( ins, U2 );
            return( ins );
        }
    }
    return( rMAKEU4( ins ) );
}
