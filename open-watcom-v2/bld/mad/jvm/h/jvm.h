/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdio.h>
#include <string.h>
#include "madimp.h"
#include "madjvm.h"
#include "jvmregs.h"
#include "dis.h"

enum toggle_states {
    /* cpu register display toggles */
    CT_HEX              = 0x01,

    /* disassembler toggles */
    DT_PSUEDO_OPS       = 0x01,
    DT_UPPER            = 0x02
};

enum {
    CPU_REG_SET,
    NUM_REG_SET
};

struct imp_mad_state_data {
    unsigned    reg_state[NUM_REG_SET];
    unsigned    disasm_state;
};

typedef struct jvm_reg_info     jvm_reg_info;
struct jvm_reg_info {
    mad_reg_info        info;
};

struct mad_disasm_data {
    address             addr;
    mad_radix           radix;
    dis_dec_ins         ins;
};

struct mad_trace_data {
    addr_off            ra;
};

struct mad_call_up_data {
    unsigned_8          dummy;
};

typedef struct mad_type_data {
    mad_string          name;
    unsigned            hex     : 1;
    union {
        const mad_type_info_basic       *b;
        const mad_type_info             *mti;
    }                   u;
} mad_type_data;

extern imp_mad_state_data       *MADState;

extern const jvm_reg_info       RegList[];
extern const mad_type_data      TypeArray[];

extern mad_status               DisasmInit( void );
extern void                     DisasmFini( void );
extern mad_disasm_control       DisasmControl( mad_disasm_data *, mad_registers * );
extern mad_status               DisasmOne( mad_disasm_data *dd, address *a, int adj );

extern mad_status               RegInit( void );
extern void                     RegFini( void );

#define NUM_ELTS( name ) (sizeof( name ) / sizeof( name[0] ) )
