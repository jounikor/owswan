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
* Description:  Internal data types used by standalone disassembler.
*
****************************************************************************/


#ifndef WDIS_TYPES_INCLUDED
#define WDIS_TYPES_INCLUDED

#include "orl.h"

// label list

typedef unsigned_32                 label_number;
typedef unsigned_32                 label_id;
typedef unsigned_16                 list_size;
typedef unsigned_16                 num_errors;

typedef unsigned_32                 dis_sec_size;
typedef unsigned_32                 dis_sec_offset;
typedef signed_32                   dis_sec_addend;

typedef enum {
    LTYP_EXTERNAL_NAMED,
    LTYP_NAMED,
    LTYP_SECTION,
    LTYP_UNNAMED,
    LTYP_FUNC_INFO,
    LTYP_GROUP,
    LTYP_ABSOLUTE
} label_type;

typedef enum {
    RFLAG_DEFAULT       = 0x0000,
    RFLAG_IS_IMMED      = 0x0001,
    RFLAG_NO_FRAME      = 0x0002
} ref_flags;

typedef struct label_entry_struct       label_entry_struct;
typedef label_entry_struct              *label_entry;

struct label_entry_struct {
    orl_sec_handle      shnd;
    dis_sec_offset      offset;
    label_type          type;
    orl_symbol_binding  binding;
    union {
        char            *name;
        label_number    number;
    } label;
    label_entry         next;
};

typedef struct label_list_struct        label_list_struct;
typedef label_list_struct               *label_list;

struct label_list_struct {
    label_entry         first;
    label_entry         last;
};

typedef struct label_list_ptr_struct    label_list_ptr_struct;
typedef label_list_ptr_struct           *label_list_ptr;

struct label_list_ptr_struct {
    label_list          list;
    label_list_ptr      next;
};

typedef struct publics_struct           publics_struct;

struct publics_struct {
    label_list_ptr      label_lists;
    label_entry *       public_symbols;
    list_size           number;
};

// reference list

typedef struct reference_entry_struct   ref_entry_struct;
typedef ref_entry_struct                *ref_entry;

struct reference_entry_struct {
    label_entry         label;
    dis_sec_offset      offset;
    orl_reloc_type      type;
    dis_sec_addend      addend;
    ref_entry           next;
    bool                has_val;
    const char          *frame;
};

typedef struct ref_list_struct          ref_list_struct;
typedef ref_list_struct                 *ref_list;

struct ref_list_struct {
    ref_entry           first;
    ref_entry           last;
    list_size           size;
};

typedef struct externs_struct           externs_struct;
typedef struct externs_struct           *externs;

struct externs_struct {
    ref_entry *         extern_refs;
    list_size           number;
};

// others

#define MAX_SYM_LEN  7800   /* C++ old len 1024 */

#define TAB_WIDTH 8

typedef enum {
    RC_OUT_OF_MEMORY,
    RC_ERROR,
    RC_OKAY
} return_val;

typedef enum {
    SECTION_TYPE_TEXT = 0,
    SECTION_TYPE_DATA,
    SECTION_TYPE_BSS,
    SECTION_TYPE_PDATA,
    SECTION_TYPE_DRECTVE,
    SECTION_TYPE_SYM_TABLE,
    SECTION_TYPE_DYN_SYM_TABLE,
    SECTION_TYPE_NUM_RECOGNIZED,
    SECTION_TYPE_LINES,
    SECTION_TYPE_UNKNOWN,
    SECTION_TYPE_RELOCS                 // used for OMF relocs section
} section_type;

typedef enum {
    NONE                = 0,
    FORM_ASM            = 0x01,
    PRINT_PUBLICS       = 0x01<<1,
    PRINT_EXTERNS       = 0x01<<2,
    PRINT_FPU_EMU_FIXUP = 0x01<<3,
    NODEMANGLE_NAMES    = 0x01<<4,
    METAWARE_COMPATIBLE = 0x01<<5,
} wd_options;

typedef int_16 buffer_position;

typedef struct section_struct       section_struct;
typedef section_struct              *section_ptr;

typedef struct scantab_struct       scantab_struct;
typedef scantab_struct              *scantab_ptr;

struct section_struct {
    const char          *name;
    orl_sec_handle      shnd;
    section_type        type;
    section_ptr         next;
    scantab_ptr         scan;
};

struct scantab_struct {
    scantab_ptr         next;
    dis_sec_offset      start;
    dis_sec_offset      end;
};

typedef struct section_list_struct  section_list_struct;

struct section_list_struct {
    section_ptr         first;
    section_ptr         last;
};

typedef struct unnamed_label_return_struct  unnamed_label_return_struct;
typedef unnamed_label_return_struct         *unnamed_label_return;

struct unnamed_label_return_struct {
    label_entry         entry;
    return_val          error;
};

struct sa_disasm_struct {
    uint_8              *data;
    dis_sec_offset      offs;
    dis_sec_offset      last;
};

typedef struct sa_disasm_struct     sa_disasm_struct;
typedef sa_disasm_struct            *sa_disasm;

// hash table definitions
typedef struct {
    union {
//        const void                  *handle;
        const void                  *sec_handle;
        const void                  *sym_handle;
        const char                  *string;
    } u;
} hash_key;

typedef struct {
    union {
//        const void                  *handle;
        ref_list                    sec_ref_list;
        label_list                  sec_label_list;
        section_type                sec_type;
        section_ptr                 section;
        const char                  *string;
        label_entry                 lab_entry;
    } u;
} hash_data;

typedef unsigned_32                 hash_value;

typedef enum {
    HASH_STRING,
    HASH_STRING_IGNORECASE,
    HASH_HANDLE
} hash_table_type;

typedef struct hash_entry_struct    hash_entry_struct;
typedef struct hash_table_struct    hash_table_struct;
typedef struct hash_entry_data      hash_entry_data;

struct hash_entry_data {
    hash_key                        key;
    hash_data                       data;
};

struct hash_entry_struct {
    hash_entry_data                 entry;
    hash_entry_struct               *next;
};

struct hash_table_struct {
    hash_value                      size;
    hash_value                      (*hash_func)(hash_value,hash_key);
    bool                            (*compare_func)(hash_key,hash_key);
    hash_entry_struct               **table;
};

typedef hash_table_struct           *hash_table;

#endif
