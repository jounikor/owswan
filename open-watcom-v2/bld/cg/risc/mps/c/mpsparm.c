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
* Description:  Select registers used for passing an arguments.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "procdef.h"
#include "types.h"
#include "regset.h"
#include "zoiks.h"
#include "rgtbl.h"
#include "typemap.h"
#include "parmreg.h"
#include "feprotos.h"


/* This is just like the generic version in parmreg.c with the important exception
 * that we always update state->parm.offset. This is so that we'd get the right
 * home locations on the stack for arguments passed in registers.
 */

type_length     ParmAlignment( type_def *tipe )
/*********************************************/
{
    /* unused parameters */ (void)tipe;

    return( 1 );
}

hw_reg_set      ParmReg( type_class_def type_class, type_length len, type_length alignment, call_state *state )
/*************************************************************************************************************/
{
    hw_reg_set  *possible;
    hw_reg_set  *reg_set;
    hw_reg_set  regs;

    /* unused parameters */ (void)len; (void)alignment;

    possible = ParmChoices( type_class );
    if( possible == NULL || HW_CEqual( *possible, HW_EMPTY ) ) {
        if( !HW_CEqual( *state->parm.curr_entry, HW_EMPTY ) ) {
            state->parm.curr_entry++;
        }
        return( HW_EMPTY );
    }
    while( !HW_CEqual( *state->parm.curr_entry, HW_EMPTY ) ) {
        reg_set = possible;
        for( ;; ) {
            regs = *reg_set;
            if( HW_CEqual( regs, HW_EMPTY ) )
                break;
            if( !HW_Ovlap( regs, state->parm.used ) ) {
                if( HW_Subset( *state->parm.curr_entry, regs ) ) {
                    HW_TurnOn( state->parm.used, regs );
                    HW_TurnOn( state->parm.used, ParmRegConflicts( regs ) );
                    if( _IsFloating( type_class ) || _IsI64( type_class ) ) {
                        state->parm.offset += _RoundUp( len, REG_SIZE );
                    } else {
                        // Note that structs (class XX) end up here - additional
                        // registers for struct may be allocated later
                        state->parm.offset += REG_SIZE;
                    }
                    return( regs );
                }
            }
            ++reg_set;
        }
        state->parm.curr_entry ++;
    }
    return( HW_EMPTY );
}
