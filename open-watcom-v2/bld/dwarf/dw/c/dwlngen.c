/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Generate line information.
*
****************************************************************************/


#include "dwpriv.h"
#include "dwutils.h"
#include "dwline.h"
#include "dwlngen.h"


uint_8 * DWLineGen( dw_linenum_delta line_incr, dw_addr_delta addr_incr, uint_8 *end )
{
    uint                opcode;
    dw_addr_delta       addr;

    if( line_incr < DWLINE_BASE || line_incr > DWLINE_BASE + DWLINE_RANGE - 1 ) {
        /* line_incr is out of bounds... emit standard opcode */
        *end++ = DW_LNS_advance_line;
        end = LEB128( end, line_incr );
        line_incr = 0;
    }

    if( addr_incr < 0 ) {
        *end++ = DW_LNS_advance_pc;
        end = LEB128( end, addr_incr );
        addr_incr = 0;
    } else {
        addr_incr /= DW_MIN_INSTR_LENGTH;
    }
    if( addr_incr == 0 && line_incr == 0 ) {
        *end++ = DW_LNS_copy;
        return( end );
    }

    /* calculate the opcode with overflow checks */
    line_incr -= DWLINE_BASE;
    if( DWLINE_RANGE * addr_incr < addr_incr )
        goto overflow;

    opcode = DWLINE_RANGE * addr_incr;
    if( opcode + line_incr < opcode )
        goto overflow;

    opcode += line_incr;

    /* can we use a special opcode? */
    if( opcode <= 255 - DWLINE_OPCODE_BASE ) {
        *end++ = opcode + DWLINE_OPCODE_BASE;
        return( end );
    }

    /*
        We can't use a special opcode directly... but we may be able to
        use a CONST_ADD_PC followed by a special opcode.  So we calculate
        if addr_incr lies in this range.  MAX_ADDR_INCR is the addr
        increment for special opcode 255.
    */
#define MAX_ADDR_INCR   ( ( 255 - DWLINE_OPCODE_BASE ) / DWLINE_RANGE )

    if( addr_incr < 2 * MAX_ADDR_INCR ) {
        *end++ = DW_LNS_const_add_pc;
        *end++ = opcode - MAX_ADDR_INCR * DWLINE_RANGE + DWLINE_OPCODE_BASE;
        return( end );
    }

    /*
        Emit an ADVANCE_PC followed by a special opcode.

        We use MAX_ADDR_INCR - 1 to prevent problems if special opcode
        255 - DWLINE_OPCODE_BASE - DWLINE_BASE + 1 isn't an integral multiple
        of DWLINE_RANGE.
    */
overflow:
    *end++ = DW_LNS_advance_pc;
    if( line_incr == 0 - DWLINE_BASE ) {
        opcode = DW_LNS_copy;
    } else {
        addr = addr_incr % ( MAX_ADDR_INCR - 1 );
        addr_incr -= addr;
        opcode = line_incr + ( DWLINE_RANGE * addr ) + DWLINE_OPCODE_BASE;
    }
    end = LEB128( end, addr_incr );
    *end++ = opcode;
    return( end );
}
