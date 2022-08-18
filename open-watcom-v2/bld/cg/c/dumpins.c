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
* Description:  Dump instruction.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cfloat.h"
#include "data.h"
#include "intrface.h"
#include "rgtbl.h"
#include "namelist.h"
#include "dumpio.h"
#include "dumpins.h"
#include "dumpblk.h"
#include "dumpconf.h"
#include "dumptab.h"
#include "dumpref.h"
#include "dumpfpu.h"
#include "feprotos.h"


#define DO_DUMPOFFSET(str,off) DumpLiteral( str ); DoOffset( (off) )


static  void    DoOffset( unsigned_32 o ) {
/*****************************************/

    DumpChar( '\t' );
    Dump8h( o );
    DumpNL();
}


void    DumpInsOffsets( void )
/****************************/
{
    DO_DUMPOFFSET( "head.prev", offsetof( instruction, head.prev ) );
    DO_DUMPOFFSET( "head.next", offsetof( instruction, head.next ) );
    DO_DUMPOFFSET( "head.live", offsetof( instruction, head.live ) );
    DO_DUMPOFFSET( "head.line_num", offsetof( instruction, head.line_num ) );
    DO_DUMPOFFSET( "head.opcode", offsetof( instruction, head.opcode ) );
    DO_DUMPOFFSET( "head.state", offsetof( instruction, head.state ) );
    DO_DUMPOFFSET( "table\t", offsetof( instruction, table ) );
    DO_DUMPOFFSET( "u.gen_table", offsetof( instruction, u.gen_table ) );
    DO_DUMPOFFSET( "u2.cse_link", offsetof( instruction, u2.cse_link ) );
    DO_DUMPOFFSET( "u2.parm_list", offsetof( instruction, u2.parm_list ) );
    DO_DUMPOFFSET( "zap\t", offsetof( instruction, zap ) );
    DO_DUMPOFFSET( "result\t", offsetof( instruction, result ) );
    DO_DUMPOFFSET( "id\t", offsetof( instruction, id ) );
    DO_DUMPOFFSET( "type_class", offsetof( instruction, type_class ) );
    DO_DUMPOFFSET( "base_type_class", offsetof( instruction, base_type_class ) );
    DO_DUMPOFFSET( "sequence", offsetof( instruction, sequence ) );
    DO_DUMPOFFSET( "flags.byte", offsetof( instruction, flags.byte ) );
    DO_DUMPOFFSET( "flags.bool_flag", offsetof( instruction, flags.bool_flag ) );
    DO_DUMPOFFSET( "flags.call_flag", offsetof( instruction, flags.call_flags ) );
    DO_DUMPOFFSET( "flags.nop_flags", offsetof( instruction, flags.nop_flags ) );
    DO_DUMPOFFSET( "t.index_needs", offsetof( instruction, t.index_needs ) );
    DO_DUMPOFFSET( "t.stk_max", offsetof( instruction, t.stk_max ) );
    DO_DUMPOFFSET( "stk_entry", offsetof( instruction, stk_entry ) );
    DO_DUMPOFFSET( "num_operands", offsetof( instruction, num_operands ) );
    DO_DUMPOFFSET( "ins_flags", offsetof( instruction, ins_flags ) );
    DO_DUMPOFFSET( "stk_exit", offsetof( instruction, stk_exit ) );
    DO_DUMPOFFSET( "s.stk_extra", offsetof( instruction, s.stk_extra ) );
    DO_DUMPOFFSET( "operands[0]", offsetof( instruction, operands[0] ) );
}


void    DumpInOut( instruction *ins )
/***********************************/
{
    DumpLiteral( "     " );
    DumpGBit( &ins->head.live.out_of_block );
    DumpChar( ' ' );
    DumpLBit( &ins->head.live.within_block );
    if( !HW_CEqual( ins->head.live.regs, HW_EMPTY ) && !HW_COvlap( ins->head.live.regs, HW_UNUSED ) ) {
        DumpLiteral( "  " );
        DumpRegName( ins->head.live.regs );
    }
    DumpNL();
}


void    DumpITab( instruction *ins )
/**********************************/
{
    if( ins->u.gen_table != NULL ) DumpTab( ins->u.gen_table );
}


static const char * ClassNames[] = {
/**********************************/

    " U1 ",
    " I1 ",
    " U2 ",
    " I2 ",
    " U4 ",
    " I4 ",
    " U8 ",
    " I8 ",
    " CP ",
    " PT ",
    " FS ",
    " FD ",
    " FL ",
    " XX ",
    ""
};


void    DumpClass( type_class_def type_class )
/********************************************/
{
    DumpString( ClassNames[type_class] );
}


void    DumpOperand( name *operand )
/**********************************/
{
    char        buffer[20];
    hw_reg_set  reg;
    name        *base;

    if( operand->n.class == N_INDEXED ) {
        if( operand->i.base != NULL ) {
            if( (operand->i.index_flags & X_FAKE_BASE) == 0 ) {
                if( operand->i.index_flags & X_LOW_ADDR_BASE ) {
                    DumpLiteral( "l^" );
                }
                DumpOperand( operand->i.base );
                if( operand->i.constant > 0 ) {
                    DumpChar( '+' );
                }
            }
        }
        if( operand->i.constant != 0 ) {
            DumpLong( operand->i.constant );
        }
        DumpChar( '[' );
        if( operand->i.index_flags & X_BASE ) {
            reg = operand->i.index->r.reg;
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
            if( HW_COvlap( reg, HW_SEGS ) ) {

                hw_reg_set tmp;

                tmp = reg;
                HW_COnlyOn( tmp, HW_SEGS );
                DumpRegName( tmp );
                DumpChar( ':' );
                HW_CTurnOff( reg, HW_SEGS );
            }
#endif
            if( operand->i.index_flags & X_HIGH_BASE ) {
                DumpRegName( HighReg( reg ) );
                DumpChar( '+' );
                DumpRegName( LowReg( reg ) );
            } else {
                DumpRegName( LowReg( reg ) );
                DumpChar( '+' );
                DumpRegName( HighReg( reg ) );
            }
        } else {
            DumpOperand( operand->i.index );
        }
        if( operand->i.scale > 0 ) {
            DumpChar( '*' );
            DumpInt( 1 << operand->i.scale );
        }
        if( operand->i.index_flags & ( X_ALIGNED_1 | X_ALIGNED_2 | X_ALIGNED_4 | X_ALIGNED_8 ) ) {
            DumpChar( '$' );
            DumpInt( FlagsToAlignment( operand->i.index_flags ) );
        }
        DumpChar( ']' );
        base = operand->i.base;
        if( base != NULL ) {
            if( operand->i.index_flags & X_FAKE_BASE ) {
                DumpChar( '{' );
                if( base->n.class == N_MEMORY ) {
                    DumpXString( AskName( base->v.symbol, base->m.memory_type ) );
                } else if( base->n.class == N_TEMP ) {
                    if( _FrontEndTmp( base ) ) {
                        DumpXString( FEName( base->v.symbol ) );
                    } else {
                        DumpOperand( base );
                    }
                }
                DumpChar( '}' );
            }
        }
    } else if( operand->n.class == N_CONSTANT ) {
        if( operand->c.const_type == CONS_ABSOLUTE ) {
            if( operand->c.lo.int_value != 0 ) {
                if( operand->c.hi.int_value != 0 && operand->c.hi.int_value != -1 )
                    Dump8h( operand->c.hi.int_value );
                Dump8h( operand->c.lo.int_value );
            } else {
                CFCnvFS( operand->c.value, buffer, 20 );
                DumpXString( buffer );
            }
        } else {
            if( operand->c.const_type == CONS_SEGMENT ) {
                DumpLiteral( "SEG(" );
                if( operand->c.value == NULL ) {
                    DumpInt( operand->c.lo.int_value );
                } else {
                    DumpOperand( operand->c.value );
                }
            } else if( operand->c.const_type == CONS_OFFSET ) {
                DumpLiteral( "OFFSET(" );
#if _TARGET & _TARG_370
                DumpInt( operand->c.lo.int_value );
#else
                DumpOperand( operand->c.value );
#endif
            } else if( operand->c.const_type == CONS_ADDRESS ) {
                DumpLiteral( "ADDRESS(" );
                DumpOperand( operand->c.value );
            } else if( operand->c.const_type == CONS_TEMP_ADDR ) {
                DumpLiteral( "TMPADDR(" );
                DumpOperand( operand->c.value );
            } else if( operand->c.const_type == CONS_HIGH_ADDR ) {
                DumpLiteral( "HIGH_ADDR(h^" );
                if( operand->c.value != NULL ) {
                    DumpOperand( operand->c.value );
                } else {
                    if( operand->c.lo.int_value != 0 ) {
                        DumpLong( operand->c.lo.int_value );
                    } else {
                        DumpLiteral( "NULL" );
                    }
                }
            }
            DumpChar( ')' );
        }
    } else if( operand->n.class == N_MEMORY ) {
        DumpXString( AskName( operand->v.symbol, operand->m.memory_type ) );
        if( operand->m.memory_type != CG_FE ) {
            DumpPtr( operand->v.symbol );
        }
        if( operand->v.offset > 0 ) {
            DumpChar( '+' );
            DumpLong( operand->v.offset );
        } else if( operand->v.offset < 0 ) {
            DumpLong( operand->v.offset );
        }
    } else if( operand->n.class == N_TEMP ) {
        DumpChar( 't' );
        DumpInt( operand->t.v.id );
        if( operand->v.offset > 0 ) {
            DumpChar( '+' );
            DumpLong( operand->v.offset );
        } else if( operand->v.offset < 0 ) {
            DumpLong( operand->v.offset );
        }
        if( _FrontEndTmp( operand ) ) {
            DumpChar( '(' );
            DumpXString( FEName( operand->v.symbol ) );
            DumpChar( ')' );
        } else if( operand->v.symbol != NULL ) {
            DumpChar( '(' );
            DumpOperand( (name *)operand->v.symbol );
            DumpChar( ')' );
        }
    } else if( operand->n.class == N_REGISTER ) {
        DumpRegName( operand->r.reg );
    } else {
        DumpLiteral( "Unknown class " );
        DumpInt( operand->n.class );
    }
}


void DoDumpIInfo( instruction *ins, bool fp )
/*******************************************/
{
    if( ins->ins_flags & INS_DEMOTED ) {
        DumpChar( 'd' );
    } else {
        DumpChar( ' ' );
    }
    if( ins->ins_flags & INS_PROMOTED ) {
        DumpChar( 'p' );
    } else {
        DumpChar( ' ' );
    }
    if( ins->ins_flags & INS_RISCIFIED ) {
        DumpChar( 'r' );
    } else {
        DumpChar( ' ' );
    }
    if( _OpIsIFunc( ins->head.opcode ) || _OpIsCall( ins->head.opcode ) || fp ) {
        DumpByte( ins->sequence );
        DumpChar( ' ' );
        DumpChar( ins->stk_entry + '0' );
        DumpChar( ins->stk_exit + '0' );
        DumpChar( ins->s.stk_depth + '0' );
    } else {
        DumpLiteral( "     " );
    }
    if( ins->ins_flags & INS_CC_USED ) {
        DumpChar( 'c' );
    } else {
        DumpChar( ' ' );
    }
    if( ins->head.opcode == OP_CALL ) {
        DumpChar( ' ' );
        DumpOperand( ins->operands[CALL_OP_ADDR] );
    }
    DumpClass( ins->type_class );
    if( ins->head.opcode == OP_CONVERT ) {
        DumpClass( ins->base_type_class );
    }
    if( _OpIsCondition( ins->head.opcode ) ) {
        if( _TrueIndex( ins ) == 0 ) {
            DumpLiteral( "T=0 " );
        } else {
            DumpLiteral( "T=1 " );
        }
    }
}


void DumpFPInfo( instruction *ins )
/*********************************/
{
    DoDumpIInfo( ins, true );
}


void DumpIInfo( instruction *ins )
/********************************/
{
    DoDumpIInfo( ins, false );
}


void DumpInsOnly( instruction *ins )
/**********************************/
{
    opcnt   i;

    DumpOpcodeName( ins->head.opcode );
    DumpIInfo( ins );
    if( ins->num_operands != 0 ) {
        DumpOperand( ins->operands[0] );
        for( i = 1; i < ins->num_operands; ++i ) {
            DumpLiteral( ", " );
            DumpOperand( ins->operands[i] );
        }
    }
    if( ins->result != NULL ) {
        DumpLiteral( " ==> " );
        DumpOperand( ins->result );
    }
}


void    DumpLineNum( instruction *ins )
/*************************************/
{
    if( ins->head.line_num != 0 ) {
        DumpLiteral( "    Line number=" );
        DumpInt( ins->head.line_num );
        DumpNL();
    }
}


void    DumpInsNoNL( instruction *ins )
/*************************************/
{
    DumpLineNum( ins );
    DumpPtr( ins );
    DumpChar( ' ' );
    if( ins->head.opcode == OP_BLOCK ) {
        DumpId( 9999 );
        DumpLiteral( ":  " );
        DumpLiteral( "BLOCK" );
    } else {
        DumpId( ins->id );
        DumpLiteral( ":  " );
#if _TARGET & ( _TARG_80386 | _TARG_8086 )
        if( DumpFPUIns87( ins ) )
            return;
#endif
        DumpInsOnly( ins );
        if( !HW_CEqual( ins->zap->reg, HW_EMPTY ) ) {
            DumpLiteral( "           zaps " );
            DumpRegName( ins->zap->reg );
        }
    }
}


void    DumpIns( instruction *ins )
/*********************************/
{
   DumpInsNoNL( ins );
   DumpNL();
}


void DumpInstrsOnly( block *blk )
/*******************************/
{
    instruction *ins;

    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        DumpIns( ins );
    }
}


void    DumpCond( instruction *ins, block *blk )
/**********************************************/
{
    byte    dst_idx;

    if( !_OpIsCondition( ins->head.opcode ) )
        return;
    if( ins->result == NULL ) {
        dst_idx = _TrueIndex( ins );
        if( dst_idx != NO_JUMP ) {
            DumpLiteral( " then " );
            DumpBlkId( blk->edge[dst_idx].destination.u.blk );
        }
        dst_idx = _FalseIndex( ins );
        if( dst_idx != NO_JUMP ) {
            DumpLiteral( " else " );
            DumpBlkId( blk->edge[dst_idx].destination.u.blk );
        }
    }
}


static const char * Usage[] = {
/*****************************/

    "USE_IN_BLOCK, ",
    "USE_IN_OTHER, ",
    "DEF_IN_BLOCK, ",
    "USE_ADDRESS , ",
    "USE_MEMORY  , ",
    "VAR_VOLATILE, ",
    "HAS_MEMORY  , ",
    "NEEDS_MEMORY, ",
    ""
};


void    DumpVUsage( name *v )
/***************************/
{
    var_usage   u;
    int         i;
    int         j;

    u = v->v.usage;
    DumpLiteral( "  Usage " );
    i = 0;
    j = 0;
    for(;;) {
        if( u & 1 ) {
            j += 14;
            if( j > 71 ) {
                DumpNL();
                j = 0;
            }
            DumpString( Usage[i] );
        }
        u >>= 1;
        ++ i;
        if( u == 0 ) {
            break;
        }
    }
    DumpNL();
}


void    DumpSym( name *sym )
/**************************/
{
    DumpPtr( sym );
    DumpChar( ' ' );
    DumpOperand( sym );
    DumpClass( sym->n.type_class );
    if( sym->n.type_class == XX ) {
        DumpLong( sym->n.size );
    }
    if( sym->n.class == N_MEMORY || sym->n.class == N_TEMP ) {
        DumpNL();
        DumpVUsage( sym );
        DumpLiteral( "  offset " );
        DumpLong( sym->v.offset );
        if( sym->n.class == N_TEMP ) {
            DumpLiteral( "  location " );
            DumpLong( sym->t.location );
            DumpLiteral( "  block id " );
            DumpInt( sym->t.u.block_id );
            // DumpPossible( sym->t.possible );
            if( sym->t.temp_flags & ALIAS ) {
                DumpLiteral( " ALIAS " );
                sym = DeAlias( sym );
                DumpPtr( sym );
            } else if( (sym->t.temp_flags & ALIAS) == 0 ) {
                DumpLiteral( " MASTER" );
            }
            if( sym->t.temp_flags & VISITED ) {
                DumpLiteral( " VISITED" );
            }
            if( sym->t.temp_flags & INDEXED ) {
                DumpLiteral( " INDEXED" );
            }
            if( sym->t.temp_flags & CAN_STACK ) {
                DumpLiteral( " CAN_STACK" );
            }
            if( sym->t.temp_flags & PUSH_LOCAL ) {
                DumpLiteral( " PUSH_LOCAL" );
            }
            if( sym->t.temp_flags & HAD_CONFLICT ) {
                DumpLiteral( " HAD_CONFLICT" );
            }
            if( sym->t.temp_flags & STACK_PARM ) {
                DumpLiteral( " STACK_PARM" );
            }
            if( sym->t.temp_flags & CROSSES_BLOCKS ) {
                DumpLiteral( " CROSSES_BLOCKS" );
            }
        } else {
            DumpLiteral( "  alignment " );
            DumpLong( sym->m.alignment );
        }
    } else if( sym->n.class == N_INDEXED ) {
        if( sym->i.index_flags & X_SEGMENTED ) {
            DumpLiteral( " X_SEGMENTED" );
        }
        if( sym->i.index_flags & X_VOLATILE ) {
            DumpLiteral( " X_VOLATILE" );
        }
        if( sym->i.index_flags & X_ALIGNED_1 ) {
            DumpLiteral( " X_UNALIGNED" );
        }
        if( sym->i.index_flags & X_ALIGNED_2 ) {
            DumpLiteral( " X_ALIGNED_2" );
        }
        if( sym->i.index_flags & X_ALIGNED_4 ) {
            DumpLiteral( " X_ALIGNED_4" );
        }
        if( sym->i.index_flags & X_ALIGNED_8 ) {
            DumpLiteral( " X_ALIGNED_8" );
        }
    }
    DumpNL();
}

void    DumpTempWId( int id )
/***************************/
{
    name        *sym;

    DumpNL();
    for( sym = Names[N_TEMP]; sym != NULL; sym = sym->n.next_name ) {
        if( sym->t.v.id == id ) {
            DumpSym( sym );
        }
    }
}


void    DumpSymList( name *sym )
/******************************/
{
    DumpNL();
    for( ; sym != NULL; sym = sym->n.next_name ) {
        DumpSym( sym );
    }
}


void    DumpNTemp( void )
/***********************/
{
    DumpSymList( Names[N_TEMP] );
}


void    DumpNMemory( void )
/*************************/
{
    DumpSymList( Names[N_MEMORY] );
}


void    DumpNIndexed( void )
/**************************/
{
    DumpSymList( Names[N_INDEXED] );
}


void    DumpNConst( void )
/************************/
{
    DumpSymList( Names[N_CONSTANT] );
}


void    DumpNRegister( void )
/***************************/
{
    DumpSymList( Names[N_REGISTER] );
}


void    DumpInsList( block *blk )
/*******************************/
{
    instruction *ins;

    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        DumpInOut( ins );
        DumpInsNoNL( ins );
        DumpChar( ' ' );
        DumpCond( ins, blk );
        DumpNL();
    }
    DumpInOut( ins );
}
