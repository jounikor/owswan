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
v*  ========================================================================
*
* Description:  MIPS specific assembler internal interface.
*
****************************************************************************/


/* MIPS specific junk */
#include "mipsfmt.h"

#define INS_NOP         0x00000000      /* sll $zero,$zero,0 - neat! */

typedef enum {
    QF_NONE     = 0x00,
    QF_S        = 0x01,
    QF_U        = 0x02,
    QF_V        = 0x04,
    QF_I        = 0x08,
    QF_C        = 0x10,
    QF_M        = 0x20,
    QF_D        = 0x40
} qualifier_flags;

typedef enum {
    #define PICK( a, b ) a,
    #include "insenum.inc"
    #undef PICK
} ins_enum_method;              // All the possible enumeration methods

typedef uint_16 ins_opcode;
typedef uint_16 ins_funccode;

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
    OP_REG_INDIRECT     = (1 << (REGCLASS_COUNT+1)),    // register with an immediate attached
} op_type;

typedef int_32  op_const;
typedef reg     op_reg;                 /* from chip-specific register.h - must be included before inslist.h */

typedef union {
    int_32              label;
    void                *ptr;
} op_reloc_target;

typedef struct {
    op_reloc_target     target;
    asm_reloc_type      type;
} op_reloc;

#define MAX_OPERANDS    3
typedef uint_8 ins_opcount;             /* carrying this abstraction thing just a little too far... */

typedef mips_template ins_template;

struct ins_symbol;
typedef struct ins_symbol ins_symbol;

typedef enum {
    MIPS_ISA1           = 0x01,
    MIPS_ISA2           = 0x02,
    MIPS_ISA3           = 0x03,
    MIPS_ISA4           = 0x04
} ins_level;

typedef struct ins_table {
    const char          *name;
    ins_opcode          opcode;
    ins_funccode        funccode;
    ins_template        template;       /* used to check operand types and encode the instruction */
    ins_enum_method     method;         /* which enumeration method to list all the possible instructions */
    ins_symbol          *symbols;
    ins_level           level;          /* MIPS ISA level required */
} ins_table;

/*
 * This is our entry in the assembler symbol table for a
 * particular instruction - it contains information which is
 * encoded in the instruction name, and a pointer to the
 * table entry above for that particular instruction.
 */
struct ins_symbol {
    ins_symbol          *next;
    ins_table           *table_entry;
    qualifier_flags     flags;
};

/* end of MIPS specific junk */

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

#ifdef _STANDALONE_
extern void MIPSEmit( owl_section_handle, instruction * );
#else
extern void MIPSEmit( instruction * );
#endif
extern bool MIPSValidate( instruction * );
#if defined( _STANDALONE_ ) && defined( AS_DEBUG_DUMP )
extern void             DumpOperand( ins_operand * );
extern void             DumpIns( instruction * );
extern void             DumpInsTableEntry( ins_table *table_entry );
extern void             DumpInsTables( void );
extern void             DumpInsEnumMethod( ins_enum_method method );
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
