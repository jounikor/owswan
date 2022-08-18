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


#ifndef DWPRIV_H_INCLUDED
#define DWPRIV_H_INCLUDED

#include <assert.h>
#include <string.h>
#include "dw.h"
#include "dwassert.h"
#include "dwarf.h"
#include "dwabort.h"
#include "dwline.h"
#include "dwdie.h"
#include "dwabbrev.h"
#include "dwcarve.h"
#include "dwhandle.h"

#define CLIReloc2( x, __s, __i )       (x)->funcs.cli_reloc( __s, __i )
#define CLIReloc3( x, __s, __i, __v )  (x)->funcs.cli_reloc( __s, __i, __v )
#define CLIReloc4( x, __s, __i, __v1, __v2 ) (x)->funcs.cli_reloc( __s, __i, __v1, __v2 )
#define CLIWrite( x, __s,  __b, __l )  (x)->funcs.cli_write( __s, __b, __l )
#define CLISeek( x, __s, __o, __t )    (x)->funcs.cli_seek( __s, __o, __t )
#define CLITell( x, __s )              (x)->funcs.cli_tell( __s )
#define CLIAlloc( x, __size )          (x)->funcs.cli_alloc( __size )
#define CLIFree( x, __ptr )            (x)->funcs.cli_free( __ptr )


struct handles_private {
    uint_32             num_handles;
    uint_32             forward;  // number of forward refs out
    struct handle_blk * block_head[MAX_HANDLE_HEIGHT];
    struct handle_blk **block_tail[MAX_HANDLE_HEIGHT];
    union handle_extra *extra_list;
    carve_t             extra_carver;
    carve_t             chain_carver;
    uint_8              max_height;
};


struct debug_line_private {
    dw_addr_offset      addr;
    dw_include_stack *  include_stack;
    dw_include *        files;
    dw_linenum          line;
    dw_column           column;
    uint_8              is_stmt : 1;
    uint_8              end_sequence : 1;
};

struct debug_abbrev_private {
    uint_8              emitted[AB_BITVECT_SIZE];
};

struct types_private {
    dw_size_t           byte_size;      // these used in enumerations.
    dw_sect_offs        offset;
};

struct die_private {
    die_tree *          tree;
};

struct debug_loc_private {
    dw_loc_handle       handles;
    carve_t             label_carver;
};

struct decl_private {
    uint                file;
    dw_linenum          line;
    dw_column           column;
    uint_8              changed;
};

struct references_private {
    struct delayed_ref *delayed;
    carve_t             delay_carver;
    uint                scope;
    uint                delayed_file;
    dw_linenum          line;
    dw_column           column;
};

#include "pushpck4.h"
struct dw_client {
    jmp_buf                     exception_handler;
    dw_funcs                    funcs;
    char                        *producer_name;
    uint_8                      compiler_options;
    uint_8                      language;
    dw_out_offset               section_base[DW_DEBUG_MAX];
    struct handles_private      handles;
    struct debug_line_private   debug_line;
    struct debug_abbrev_private debug_abbrev;
    struct types_private        types;
    struct die_private          die;
    struct debug_loc_private    debug_loc;
    struct decl_private         decl;
    struct references_private   references;
    uint_8                      offset_size;
    uint_8                      segment_size;
    dw_handle                   defset;
    dw_sym_handle               abbrev_sym;
    dw_sym_handle               dbg_pch;
};
#include "poppck.h"

#endif
