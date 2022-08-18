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
* Description:  Dump x87 instructions.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "dumpio.h"
#include "dumpins.h"
#include "dumpfpu.h"

#if ( _TARGET & ( _TARG_8086 | _TARG_80386 ) )

#include "i87sched.h"
#include "gen8087.h"

void    DumpSeqs( void )
/**********************/
{
    int                 i;
    virtual_st_locn     virtual_locn;
    temp_entry          *temp;
    actual_st_locn      actual_locn;

    if( STLocations ) {
        DumpLiteral( "seq: " );
        for( i = 0; i < MaxSeq; ++i ) {
            DumpChar( (char)( i + 'a' ) );
            DumpChar( ' ' );
        }
        DumpNL();
        for( virtual_locn = VIRTUAL_0; virtual_locn < VIRTUAL_NONE; virtual_locn++ ) {
            DumpLiteral( "  " );
            DumpChar( (char)( virtual_locn + '0' ) );
            DumpLiteral( ": " );
            for( i = 0; i < MaxSeq; ++i ) {
                actual_locn = RegSTLoc( i, virtual_locn );
                DumpChar( (actual_locn == ACTUAL_NONE) ? 'X' : (char)( actual_locn + '0' ) );
                DumpChar( ' ' );
            }
            DumpNL();
        }
        DumpNL();
    }
    for( temp = TempList; temp != NULL; temp = temp->next ) {
        DumpNL();
        DumpLiteral( " Name: " );
        DumpOperand( temp->op );
        DumpNL();
        DumpLiteral( "Location: " );
        DumpChar( (temp->actual_locn == ACTUAL_NONE) ? 'X': (char)( temp->actual_locn + '0' ) );
        DumpLiteral( " Savings: " );
        DumpInt( temp->savings );
        if( temp->cached )
            DumpLiteral( " CACHED" );
        DumpNL();
        DumpLiteral( "First: " );
        if( temp->first ) {
            DumpInsNoNL( temp->first );
        }
        DumpNL();
        DumpLiteral( " Last: " );
        if( temp->last ) {
            DumpInsNoNL( temp->last );
        }
        DumpNL();
    }
}


static  void    DumpOpcode( instruction *ins )
/********************************************/
{
    switch( ins->head.opcode ) {
    case OP_ADD:
        DumpLiteral( "fadd" );
        break;
    case OP_SUB:
        DumpLiteral( "fsub" );
        break;
    case OP_MUL:
        DumpLiteral( "fmul" );
        break;
    case OP_DIV:
        DumpLiteral( "fdiv" );
        break;
    default:
        DumpLiteral( "ouch" );
        break;
    }
}


bool    DumpFPUIns87( instruction *ins )
/**************************************/
{
    opcnt       i;

    if( ins->u.gen_table == NULL || ins->table == NULL )
        return( false );
    switch( G( ins ) ) {
    case G_RRFBIN:
    case G_MRFBIN:
        DumpOpcode( ins );
        DumpLiteral( "r " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        break;
    case G_RNFBIN:
    case G_MNFBIN:
        DumpOpcode( ins );
        DumpLiteral( "  " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        break;
    case G_RRFBINP:
        DumpOpcode( ins );
        DumpLiteral( "rp" );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        DumpLiteral( ",st" );
        break;
    case G_RNFBINP:
        DumpOpcode( ins );
        DumpLiteral( "p " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        DumpLiteral( ",st" );
        break;
    case G_MFLD:
    case G_RFLD:
        DumpLiteral( "fld   " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        break;
    case G_MFST:
    case G_RFST:
        DumpLiteral( "fstp  " );
        DumpFPInfo( ins );
        DumpOperand( ins->result );
        break;
    case G_MFSTRND:
        DumpLiteral( "fstrnd" );
        DumpFPInfo( ins );
        DumpOperand( ins->result );
        break;
    case G_FCHS:
        DumpLiteral( "fneg  " );
        DumpFPInfo( ins );
        break;
    case G_FCHOP:
        DumpLiteral( "fchop " );
        DumpFPInfo( ins );
        break;
    case G_FRNDINT:
        DumpLiteral( "frndnt" );
        DumpFPInfo( ins );
        break;
    case G_FLDZ:
        DumpLiteral( "fldz  " );
        DumpFPInfo( ins );
        break;
    case G_FLD1:
        DumpLiteral( "fld1  " );
        DumpFPInfo( ins );
        break;
    case G_FINIT:
        DumpLiteral( "finit " );
        DumpFPInfo( ins );
        break;
    case G_FCOMPP:
        DumpLiteral( "fcompp" );
        DumpFPInfo( ins );
        break;
    case G_MCOMP:
    case G_RCOMP:
        DumpLiteral( "fcomp " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        break;
    case G_MCOM:
    case G_RCOM:
        DumpLiteral( "fcom  " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        break;
    case G_MFSTNP:
    case G_RFSTNP:
        DumpLiteral( "fst   " );
        DumpFPInfo( ins );
        DumpOperand( ins->result );
        break;
    case G_FTST:
        DumpLiteral( "ftst  " );
        DumpFPInfo( ins );
        DumpOperand( ins->operands[0] );
        break;
    case G_FXCH:
        DumpLiteral( "fxch  " );
        DumpFPInfo( ins );
        DumpOperand( ins->result );
        break;
    case G_FSINCOS:
        DumpLiteral( "fsinco" );
        DumpFPInfo( ins );
        break;
    default:
        return( false );
    }
    DumpLiteral( " [" );
    if( ins->num_operands != 0 ) {
        DumpOperand( ins->operands[0] );
        for( i = 1; i < ins->num_operands; ++i ) {
            DumpChar( ',' );
            DumpOperand( ins->operands[i] );
        }
    }
    if( ins->result != NULL ) {
        DumpLiteral( " ==> " );
        DumpOperand( ins->result );
    }
    DumpChar( ']' );
    return( true );
}

#endif

void    DumpFPUIns( instruction *ins )
/************************************/
{
#if ( _TARGET & ( _TARG_8086 | _TARG_80386 ) )
    (void)DumpFPUIns87( ins );
#else
    /* unused parameters */ (void)ins;
#endif
}
