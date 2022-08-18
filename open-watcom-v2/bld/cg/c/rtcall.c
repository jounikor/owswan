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
* Description:  Generate runtime support call.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "makeins.h"
#include "convins.h"
#include "data.h"
#include "rtrtn.h"
#include "namelist.h"
#include "rgtbl.h"
#include "insutil.h"
#include "rtcall.h"
#include "optab.h"
#include "inssegs.h"
#include "generate.h"
#include "liveinfo.h"


#if _TARGET & _TARG_AXP
    #define _ParmReg( x )       FirstReg( x )
#else
    #define _ParmReg( x )       FirstReg( x )
#endif

instruction     *rMAKECALL( instruction *ins )
/*********************************************
    Using the table RTInfo[], do all the necessary stuff to turn
    instruction "ins" into a call to a runtime support routine.  Move
    the parms into registers, and move the return register of the
    runtime routine into the result. Used for 386 and 370 versions
*/
{
    rtn_info            *info;
    label_handle        lbl;
    instruction         *left_ins;
    instruction         *new_ins;
    instruction         *last_ins;
    name                *reg_name;
    hw_reg_set          regs;
    hw_reg_set          all_regs;
    hw_reg_set          tmp;
    rt_class            rtindex;

    if( !_IsConvert( ins ) ) {
        rtindex = LookupRoutine( ins );
    } else { /* look it up again in case we ran out of memory during expansion*/
        rtindex = LookupConvertRoutine( ins );
    }
    info = &RTInfo[rtindex];
    regs = _ParmReg( info->left );
    all_regs = regs;
    left_ins = MakeMove( ins->operands[0], AllocRegName( regs ),
                          info->operand_class );
    ins->operands[0] = left_ins->result;
    MoveSegOp( ins, left_ins, 0 );
    PrefixIns( ins, left_ins );
    regs = _ParmReg( info->right );
    if( !HW_CEqual( regs, HW_EMPTY ) ) {
        new_ins = MakeMove( ins->operands[1], AllocRegName( regs ),
                                info->operand_class );
        ins->operands[1] = new_ins->result;
        MoveSegOp( ins, new_ins, 0 );
        HW_TurnOn( all_regs, regs );
        PrefixIns( ins, new_ins );
    }
#if _TARGET & _TARG_370
    tmp = RAReg();
    HW_TurnOn( all_regs, tmp );
    tmp = LNReg();
    HW_TurnOn( all_regs, tmp );
#elif _TARGET & _TARG_80386
    {
    tmp = ReturnReg( WD, false );
    HW_TurnOn( all_regs, tmp );
    }
#endif
    reg_name = AllocRegName( all_regs );
    lbl = RTLabel( rtindex );
    new_ins = NewIns( 3 );
    new_ins->head.opcode = OP_CALL;
    new_ins->type_class = ins->type_class;
    new_ins->operands[CALL_OP_USED] = reg_name;
    new_ins->operands[CALL_OP_USED2] = reg_name;
    new_ins->operands[CALL_OP_ADDR] = AllocMemory( lbl, 0, CG_LBL, ins->type_class );
    new_ins->result = NULL;
    new_ins->num_operands = 2;         /* special case for OP_CALL*/
#if _TARGET & _TARG_AXP
    {
    HW_CTurnOn( all_regs, HW_FULL );
    HW_TurnOff( all_regs, SavedRegs() );
    HW_CTurnOff( all_regs, HW_UNUSED );
    HW_TurnOn( all_regs, ReturnAddrReg() );
    }
#endif
    new_ins->zap = (register_name *)AllocRegName( all_regs );   /* all parm regs could be zapped*/
    last_ins = new_ins;
    if( ins->result == NULL || _OpIsCondition( ins->head.opcode ) ) {
        /* comparison, still need conditional jumps*/
        ins->operands[0] = AllocIntConst( 0 );
        ins->operands[1] = AllocIntConst( 1 );
        DelSeg( ins );
        DoNothing( ins );               /* just conditional jumps for ins*/
        PrefixIns( ins, new_ins );
        new_ins->ins_flags |= INS_CC_USED;
        last_ins = ins;
    } else {
        regs = _ParmReg( info->result );
        tmp = regs;
        HW_TurnOn( tmp, new_ins->zap->reg );
        new_ins->zap = (register_name *)AllocRegName( tmp );
        reg_name = AllocRegName( regs );
        new_ins->result = reg_name;
        last_ins = MakeMove( reg_name, ins->result, ins->type_class );
        ins->result = last_ins->operands[0];
        MoveSegRes( ins, last_ins );
        SuffixIns( ins, last_ins );
        ReplIns( ins, new_ins );
    }
    FixCallIns( new_ins );
    UpdateLive( left_ins, last_ins );
    return( left_ins );
}
