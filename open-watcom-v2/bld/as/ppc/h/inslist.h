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


/* POWER PC specific junk */

#include "ppcfmt.h"

#define INS_NOP         0x60000000      /* ori r0,r0,0 */

typedef enum {
    IF_SETS_OVERFLOW    = 0x01,         /* sets the overflow flag */
    IF_SETS_CC          = 0x02,         /* sets the condition codes */
    IF_SETS_ABSOLUTE    = 0x04,
    IF_SETS_LINK        = 0x08,
} ins_flags;

typedef uint_16 ins_opcode;

typedef uint_16 ins_special;

typedef enum {
    NOTHING             = 0x00,
    RELOC               = 0x01,
    UNNAMED_RELOC       = 0x02,
    CONSTANT            = 0x04
} op_flags;

typedef enum {
    #define PICK( a, b ) OP_##a = b,
    #include "regclass.inc"
    #undef PICK
    OP_IMMED            = (1 << REGCLASS_COUNT),        // an immediate constant (could have reloc with it)
    OP_REG_INDIRECT     = (1 << (REGCLASS_COUNT + 1)),  // register with an immediate attached
    OP_BI               = (1 << (REGCLASS_COUNT + 2)),  // BI field in branch instructions
} op_type;

typedef int_32  op_const;
typedef reg     op_reg;                 /* from chip-specific register.h - must be included before inslist.h */
typedef struct {
    union {
        int_32      label;
        void        *ptr;
    }               target;
    asm_reloc_type  type;
} op_reloc;

#define MAX_OPERANDS    5               /* the dreaded rlwnmx instruction et all */
typedef uint_8 ins_opcount;             /* carrying this abstraction thing just a little too far... */

typedef ppc_template ins_template;

struct ins_symbol;
typedef struct ins_symbol ins_symbol;

typedef struct ins_table {
    const char  *name;
    ins_opcode  primary;
    ins_opcode  secondary;
    ins_special special;                /* extra field for use by the simplified mnemonics */
    ins_template template;              /* used to check operand types and encode the instruction */
    ins_flags   optional;               /* which flags can be specified by programmer */
    ins_flags   required;               /* which flags are always present - an aid to abstraction */
    ins_symbol  *symbols;
} ins_table;

/*
 * This is our entry in the assembler symbol table for a
 * particular instruction - it contains information which is
 * encoded in the instruction name, and a pointer to the
 * table entry above for that particular instruction.
 */
struct ins_symbol {
    ins_symbol  *next;
    ins_table   *table_entry;
    ins_flags   flags;
};

/* end of POWER PC specific junk */

typedef ins_symbol *ins_format;

typedef struct ins_operand {
    op_type     type;
    op_const    constant;
    op_reg      reg;
    op_reloc    reloc;
    op_flags    flags;
} ins_operand;

typedef struct instruction {
    sym_handle  opcode_sym;
    ins_format  format;
    ins_opcount num_operands;
    ins_operand *operands[MAX_OPERANDS];
} instruction;

extern void             AsInsInit( void );
extern instruction      *AsInsCreate( sym_handle );
extern void             AsInsAddOperand( instruction *, ins_operand * );
extern void             AsInsEmit( instruction * );
extern void             AsInsDestroy( instruction * );
extern void             AsInsFini( void );

extern ins_operand      *AsOpImmed( expr_tree * );
extern ins_operand      *AsOpRegister( reg );
extern ins_operand      *AsOpRegIndirect( reg, expr_tree * );
extern ins_operand      *AsOpBI( uint_32, uint_32 );

#ifdef _STANDALONE_
extern void PPCEmit( owl_section_handle, instruction * );
#else
extern void PPCEmit( instruction * );
#endif
extern bool PPCValidate( instruction * );

#if defined( _STANDALONE_ ) && defined( AS_DEBUG_DUMP )
extern void             DumpOperand( ins_operand * );
extern void             DumpIns( instruction * );
extern void             DumpInsTableEntry( ins_table *table_entry );
extern void             DumpInsTables( void );
//extern void             DumpInsEnumMethod( ins_enum_method method );
#endif

#define InsInit                 AsInsInit
#define InsCreate               AsInsCreate
#define InsAddOperand           AsInsAddOperand
#define InsEmit                 AsInsEmit
#define InsDestroy              AsInsDestroy
#define InsFini                 AsInsFini
#define OpImmed                 AsOpImmed
#define OpRegister              AsOpRegister
#define OpRegIndirect           AsOpRegIndirect
#define OpBI                    AsOpBI
