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
* Description:  Turn expression tree into pseudo-assembly instructions.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "memcheck.h"
#include "cfloat.h"
#include "makeins.h"
#include "data.h"
#include "bldins.h"
#include "addrfold.h"
#include "display.h"
#include "utils.h"
#include "tree.h"
#include "treeprot.h"
#include "makeaddr.h"
#include "namelist.h"
#include "makeblk.h"
#include "encode.h"
#include "rgtbl.h"
#include "typemap.h"
#include "generate.h"
#include "types.h"
#include "feprotos.h"


static  type_def        *LastCmpType;
static  unsigned_32     UnrollValue = 0;

an      BGVarargsBasePtr( type_def *tipe )
/****************************************/
{
    an                  addr;

    addr = RegName( VarargsHomePtr(), tipe );
    return( addr );
}

an      BGStackValue( type_def *tipe )
/************************************/
{
    an                  addr;

    addr = RegName( StackReg(), tipe );
    return( addr );
}

an      BGInteger( signed_32 value, type_def *tipe )
/**************************************************/
{
    float_handle    cf;

    if( tipe->attr & TYPE_SIGNED ) {
        cf = CFCnvI32F( value );
    } else {
        cf = CFCnvU32F( value );
    }
    return( MakeConst( cf, tipe ) );
}

an      BGInt64( signed_64 value, type_def *tipe )
/************************************************/
{
    name   *cname;

    if( tipe->attr & TYPE_SIGNED ) {
        cname = AllocS64Const( value.u._32[I64LO32],value.u._32[I64HI32] );
    } else {
        cname = AllocU64Const( value.u._32[I64LO32],value.u._32[I64HI32] );
    }
    return( AddrName( cname, tipe ) );
}

an  BGFloat( const char *value, type_def *tipe )
/**********************************************/
{
    return( MakeConst( CFCnvSF( value ), tipe ) );
}


an      BGName( cg_class cl, pointer sym, type_def *tipe )
/********************************************************/
{
    return( MakeAddrName( cl, sym, tipe ) );
}


an      BGTempName( name *temp, type_def *tipe )
/**********************************************/
{
    temp->v.usage |= USE_IN_ANOTHER_BLOCK;
    return( MakeTypeTempAddr( temp, tipe ) );
}


bool    FiniLabel( label_handle lbl, block *blk )
/***********************************************/
{
    block_num   i;

    for( i = blk->targets; i-- > 0; ) {
        if( blk->edge[i].destination.u.lbl == lbl ) {
            blk->edge[i].flags |= DEST_LABEL_DIES;
            return( true );
        }
    }
    if( blk->label == lbl ) {
        blk->edge[0].flags |= BLOCK_LABEL_DIES;
        return( true );
    }
    return( false );
}

void    BGFiniLabel( label_handle lbl )
/*************************************/
{
    block       *blk;

    if( HaveCurrBlock && FiniLabel( lbl, CurrBlock ) )
        return;
    for( blk = BlockList; blk != NULL; blk = blk->prev_block ) {
        if( FiniLabel( lbl, blk ) ) {
            return;
        }
    }
    GenKillLabel( lbl );
}


bool    NeedConvert( type_def *from, type_def *to )
/*************************************************/
{
    if( from == to )
        return( false );
    if( from->attr & TYPE_FLOAT )
        return( true );
    if( to->attr & TYPE_FLOAT )
        return( true );
    if( from->length != to->length )
        return( true );
#if _TARGET & _TARG_8086
    if( (to->attr & TYPE_POINTER) != (from->attr & TYPE_POINTER) && to->length != WORD_SIZE )
        return( true ); /* long <=> pointer */
#endif
    return( false );
}


static  an Unary( cg_op op, an left, type_def *tipe )
/***************************************************/
{
    instruction *ins;
    an          res;

    ins = MakeNary( (opcode_defs)op, GenIns( left ), NULL, NULL, TypeClass( tipe ), TypeClass( left->tipe ), 1 );
    res = InsName( ins, tipe );
    AddIns( ins );
    BGDone( left );
    return( res );
}


static  an      CnvRnd( an name, type_def *tipe, cg_op op ) {
/***********************************************************/

    if( NeedConvert( name->tipe, tipe ) ) {
        name = Unary( op, name, tipe );
    } else if( name->tipe != tipe ) {
        AddrDemote( name ); /* it's not quite the right type */
    }
    name->tipe = tipe;
    return( name );
}


name        *BGNewTemp( type_def *tipe )
/**************************************/
{
    name        *temp;

    temp = AllocTemp( TypeClass( tipe ) );
    if( temp->n.size == 0 ) {
        temp->n.size = tipe->length;
    }
    return( temp );
}


name        *BGGlobalTemp( type_def *tipe )
/*****************************************/
{
    name        *temp;

    temp = BGNewTemp( tipe );
    temp->v.usage |= USE_IN_ANOTHER_BLOCK;
    return( temp );
}


static  an      FlowOut( an node, type_def *tipe ) {
/**************************************************/

    name                *temp;
    label_handle        lbl;

    lbl = AskForNewLabel();
    temp = BGGlobalTemp( tipe );
    AddIns( MakeMove( AllocIntConst( FETrue() ), temp, temp->n.type_class ) );
    *(node->u.b.t) = CurrBlock->label;
    GenBlock( BLK_JUMP, 1 );
    AddTarget( lbl, false );
    EnLink( AskForNewLabel(), true );
    AddIns( MakeMove( AllocIntConst( 0 ), temp, temp->n.type_class ) );
    *(node->u.b.f) = CurrBlock->label;
    GenBlock( BLK_JUMP, 1 );
    AddTarget( lbl, false );
    EnLink( lbl, true );
    NamesCrossBlocks();
    AddrFree( node );
    return( AddrName( temp, tipe ) );
}


an      Arithmetic( an name, type_def *tipe )
/*******************************************/
{
    if( name->format == NF_BOOL ) {
        if( (tipe->attr & TYPE_FLOAT) != 0 || ( tipe->length > TypeInteger->length ) ) {
            name = FlowOut( name, TypeInteger );
            name = Unary( O_CONVERT, name, tipe );
        } else {
            name = FlowOut( name, tipe );
        }
    }
    return( name );
}

an      BGCompare( cg_op op, an left, an rite, label_handle entry, type_def *tipe )
/*********************************************************************************/
{
    an                  new;
    instruction         *ins;
    name                *newleft;
    name                *newrite;

    LastCmpType = tipe;
    newleft = GenIns( left );
    newrite = GenIns( rite );
    BGDone( left );
    BGDone( rite );
    NamesCrossBlocks();
    ins = MakeCondition( (opcode_defs)op, newleft, newrite, 0, 1, TypeClass( tipe ) );
    AddIns( ins );
    GenBlock( BLK_CONDITIONAL, 2 );
    AddTarget( NULL, false );
    AddTarget( NULL, false );
    new = NewBoolNode();
    new->u.b.e = entry;
    new->u.b.t = &CurrBlock->edge[0].destination.u.lbl;
    new->u.b.f = &CurrBlock->edge[1].destination.u.lbl;
    EnLink( AskForNewLabel(), true );
    return( new );
}



an      Boolean( an node, label_handle entry )
/********************************************/
{
    if( node->format != NF_BOOL ) {
        node = BGCompare( O_NE, node, BGInteger( 0, node->tipe ), entry, node->tipe );
    }
    return( node );
}


label_handle BGGetEntry( void )
/*****************************/
{
    return( CurrBlock->label );
}


void    BG3WayControl( an node, label_handle lt, label_handle eq, label_handle gt )
/*********************************************************************************/
{
    instruction         *ins;
    name                *op;
    label_handle        lbl;
    type_class_def      type_class;

    node = Arithmetic( node, node->tipe );
    type_class = TypeClass( node->tipe );
    NamesCrossBlocks();
    op = GenIns( node );
    BGDone( node );
    ins = NULL;
#if _TARGET & _TARG_80386
    if( type_class == FS ) {
        ins = MakeCondition( OP_BIT_TEST_FALSE, op, AllocS32Const( 0x7FFFFFFFL ), 0, 1, SW );
    }
#endif
    if( ins == NULL ) {
        ins = MakeCondition( OP_CMP_EQUAL, op, AllocIntConst( 0 ), 0, 1, type_class );
    }
    AddIns( ins );
    GenBlock( BLK_CONDITIONAL, 2 );
    AddTarget( eq, false );
    lbl = AskForNewLabel();
    AddTarget( lbl, false );

    EnLink( lbl, true );
#if _TARGET & _TARG_80386
    if( type_class == FS ) {
        type_class = SW;
    }
#endif
    ins = MakeCondition( OP_CMP_LESS, op, AllocIntConst( 0 ), 0, 1, type_class );
    AddIns( ins );
    GenBlock( BLK_CONDITIONAL, 2 );
    AddTarget( lt, false );
    AddTarget( gt, false );
    Generate( false );
    HaveCurrBlock = false;
}


void    BGControl( cg_op op, an expr, label_handle lbl )
/******************************************************/
{
    BGGenCtrl( op, expr, lbl, false );
}


void    BGGenCtrl( cg_op op, an expr, label_handle lbl, bool gen )
/****************************************************************/
{
    switch( op ) {
    case O_LABEL:
        if( HaveCurrBlock ) {
            GenBlock( BLK_JUMP, 1 );  /* block with 1 target*/
            AddTarget( lbl, false );
            if( gen ) {
                Generate( false );
            }
        }
        EnLink( lbl, false );
        HaveCurrBlock = true;
        CurrBlock->unroll_count = UnrollValue;
        break;
    case O_GOTO:
        if( HaveCurrBlock ) {
            GenBlock( BLK_JUMP, 1 );
            AddTarget( lbl, false );
            if( gen ) {
                Generate( false );
            }
        }
        HaveCurrBlock = false;
        break;
    case O_INVOKE_LABEL:
        if( HaveCurrBlock ) {
            GenBlock( BLK_CALL_LABEL, 1 );
            AddTarget( lbl, false );
#if 0
            Need to make sure that next_block != NULL when we generate the
            code for a CALL_LABEL block - see comments in GenObject

            if( gen ) {
                Generate( false );
            }
#endif
            EnLink( AskForNewLabel(), true );
        }
        break;
    case O_LABEL_RETURN:
        if( HaveCurrBlock ) {
            AddIns( MakeNop() );
            GenBlock( BLK_LABEL_RETURN, 0 );
            if( gen ) {
                Generate( false );
            }
        }
        HaveCurrBlock = false;
        break;
    case O_IF_TRUE:
        *(expr->u.b.f) = CurrBlock->label;
        *(expr->u.b.t) = lbl;
        AddrFree( expr );
        if( gen ) {
            Generate( false );
        }
        break;
    case O_IF_FALSE:
        *(expr->u.b.t) = CurrBlock->label;
        *(expr->u.b.f) = lbl;
        AddrFree( expr );
        if( gen ) {
            Generate( false );
        }
        break;
    default:
        break;
    }
}


void    BGBigLabel( back_handle bck )
/***********************************/
{
    if( HaveCurrBlock ) {
        GenBlock( BLK_JUMP, 1 );  /* block with 1 target*/
        AddTarget( bck->lbl, false );
        Generate( false );
    }
    EnLink( bck->lbl, false );
    HaveCurrBlock = true;
    BigLabel();
    _MarkBlkAttr( CurrBlock, BLK_BIG_LABEL );
}


void    BGBigGoto( label_handle lbl, level_depth level )
/******************************************************/
{
    GenBlock( BLK_BIG_JUMP, 1 ); // No longer supported!
    AddTarget( lbl, false );
    BigGoto( level );
    Generate( false );
    EnLink( AskForNewLabel(), true );
}


unsigned_32 BGUnrollCount( unsigned_32 unroll_count )
/***************************************************/
{
    unsigned_32         old_value;

    old_value = UnrollValue;
    UnrollValue = unroll_count;
    return( old_value );
}


an      BGUnary( cg_op op, an left, type_def *tipe )
/**************************************************/
{
    an          new;

    new = NULL;
    switch( op ) {
    case O_POINTS:
        new = MakePoints( left, tipe );
        break;
    case O_CONVERT:
        if( tipe == left->tipe || !NeedPtrConvert( left, tipe ) ) {
            new = left;
        } else {
            new = CnvRnd( left, tipe, O_CONVERT );
        }
        break;
    case O_ROUND:
        if( tipe == left->tipe ) {
            new = left;
        } else {
            new = CnvRnd( left, tipe, O_ROUND );
        }
        break;
#if _TARGET & _TARG_RISC
    case O_STACK_ALLOC:
        CurrProc->targ.base_is_fp = true;
        break;
#endif
#if 0
    case O_PTR_TO_FOREIGN:
    case O_PTR_TO_NATIVE:
        // no large model runtime libraries
        if( left->tipe->length != WORD_SIZE ) {
            FEMessage( MSG_ERROR, "runtime call cannot be made when DS not pegged" );
            left = Unary( O_CONVERT, left, TypeAddress( TY_NEAR_POINTER ) );
        }
#endif
    default:
        break;
    }
    if( new == NULL ) {
        new = Unary( op, left, tipe );
    }
    new->tipe = tipe;
    return( new );
}


static  an      CheckType( an op, type_def *tipe ) {
/**************************************************/

    if( op->format != NF_ADDR )
        return( op );
    return( BGUnary( O_CONVERT, op, tipe ) );
}


an      BGBinary( cg_op op, an left, an rite, type_def *tipe, bool fold_addr )
/****************************************************************************/
{
    an          result;
    instruction *ins;

    result = NULL;
    if( op == O_PLUS || op == O_MINUS
       && ( tipe->attr & TYPE_POINTER )
       && ( left->tipe->attr & TYPE_POINTER )
       && (rite->tipe->attr & (TYPE_POINTER | TYPE_FLOAT)) == 0 ) {
        /* Special case for pointer +- int. Don't convert! */
    } else {
        left = CheckType( left, tipe );
        rite = CheckType( rite, tipe );
    }
    switch( op ) {
    case O_PLUS:
        if( fold_addr ) {
            if( TGIsAddress() || CypAddrPlus( left, rite, tipe ) ) {
                result = AddrPlus( left, rite, tipe );
            }
        }
        break;
    case O_LSHIFT:
        if( fold_addr ) {
            if( TGIsAddress() || CypAddrShift( left, rite, tipe ) ) {
                result = AddrShift( left, rite, tipe );
            }
        }
        break;
    default:
        break;
    }
    if( result == NULL ) {
        left = CheckType( left, tipe );
        ins = MakeBinary( (opcode_defs)op, GenIns( left ), GenIns( rite ), NULL, TypeClass( tipe ) );
        result = InsName( ins, tipe );
        AddIns( ins );
        BGDone( left );
        BGDone( rite );
    }
    result->tipe = tipe;
    return( result );
}



an      BGOpGets( cg_op op, an left, an rite, type_def *tipe, type_def *optipe )
/******************************************************************************/
{
    an                  result;
    an                  leftp;
    name                *temp;
    type_class_def      op_type_class;
    type_class_def      type_class;
    name                *left_name;
    instruction         *ins;

    leftp = MakePoints( left, tipe );
    left_name = GenIns( leftp );
    type_class = TypeClass( tipe );
    op_type_class = TypeClass( optipe );
    if( NeedConvert( tipe, optipe ) ) {
        temp = AllocTemp( op_type_class );
        ins = MakeConvert( left_name, temp, op_type_class, type_class );
        AddIns( ins );
        AddIns( MakeBinary( (opcode_defs)op, temp, GenIns( rite ), temp, op_type_class ) );
        ins = MakeConvert( temp, left_name, type_class, op_type_class );
        AddIns( ins );
    } else {
        ins = MakeBinary( (opcode_defs)op, left_name, GenIns( rite ), left_name, op_type_class );
        if( tipe != optipe ) {
            ins->ins_flags |= INS_DEMOTED; /* its not quite the right type */
        }
        AddIns( ins );
    }
    result = AddrName( left_name, tipe );
    BGDone( rite );
    BGDone( leftp );
    return( result );
}


an      BGConvert( an left, type_def *tipe )
/******************************************/
{
    an          new;
    type_attr   left_attr;

    left_attr = left->tipe->attr;
    new = BGUnary( O_CONVERT, left, tipe );
    if( left_attr & TYPE_CODE ) {
        FixCodePtr( new ); /*% kludge for code pointer conversions*/
    }
    return( new );
}


an      BGFlow( cg_op op, an left, an rite )
/******************************************/
{
    an                  new = NULL;
    label_handle        *temp;

    if( op == O_FLOW_NOT ) {
        temp = left->u.b.t;
        left->u.b.t = left->u.b.f;
        left->u.b.f = temp;
        new = left;
    } else {
        switch( op ) {
        case O_FLOW_AND:
            *(left->u.b.t) = rite->u.b.e;
            *(left->u.b.f) = CurrBlock->label;
            *(rite->u.b.f) = CurrBlock->label;
            GenBlock( BLK_JUMP, 1 );
            AddTarget( NULL, false );
            left->u.b.f = &CurrBlock->edge[0].destination.u.lbl;
            left->u.b.t = rite->u.b.t;
            new = left;
            AddrFree( rite );
            EnLink( AskForNewLabel(), true );
            break;
        case O_FLOW_OR:
            *(left->u.b.f) = rite->u.b.e;
            *(left->u.b.t) = CurrBlock->label;
            *(rite->u.b.t) = CurrBlock->label;
            GenBlock( BLK_JUMP, 1 );
            AddTarget( NULL, false );
            left->u.b.t = &CurrBlock->edge[0].destination.u.lbl;
            left->u.b.f = rite->u.b.f;
            new = left;
            AddrFree( rite );
            EnLink( AskForNewLabel(), true );
            break;
        default:
            break;
        }
    }
    return( new );
}


an      BGAssign( an dst, an src, type_def *tipe )
/************************************************/
{
    an          res;

    res = MakeGets( dst, src, tipe );
    res->tipe = tipe;
    return( res );
}


an      BGCopy( an node )
/***********************/
{
    return( AddrCopy( node ) );
}


an      BGDuplicate( an node )
/****************************/
{
    return( AddrDuplicate( node ) );
}


void    BGDone( an node )
/***********************/
{
    if( node->format == NF_BOOL ) {
        FlowOff( node );
    }
    AddrFree( node );
}


void    BGTrash( an node )
/************************/
{
    BGDone( node );
}


void    FlowOff( an name )
/************************/
{
    *(name->u.b.t) = CurrBlock->label;
    *(name->u.b.f) = CurrBlock->label;
    NamesCrossBlocks();
}


void    BGStartBlock( void )
/**************************/
{
    label_handle    lbl;

    if( _MemLow ) { /* break the block here and generate code*/
        lbl = AskForNewLabel();
        GenBlock( BLK_JUMP, 1 );
        AddTarget( lbl, false );
        Generate( false );
        EnLink( lbl, true );
    } else { /* check if the block is getting too big*/
        BlkTooBig();
    }
}
