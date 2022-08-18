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
* Description:  Data types holding object file information.
*
****************************************************************************/


#include "orl.h"
#include "hash.h"


typedef unsigned_16             overlay_ref;

typedef struct file_list        FILE_LIST;
typedef struct path_entry       PATH_ENTRY;
typedef struct mod_entry        MOD_ENTRY;
typedef struct ovl_area         OVL_AREA;
typedef struct section          SECTION;
typedef struct group_entry      GROUP_ENTRY;
typedef struct class_entry      CLASS_ENTRY;
typedef struct segnode          SEGNODE;
typedef struct seg_leader       SEG_LEADER;
typedef struct node             NODE;
typedef struct extnode          EXTNODE;
typedef struct grpnode          GRPNODE;
typedef struct list_of_names    LIST_OF_NAMES;
typedef struct lobject_data     LOBJECT_DATA;
typedef struct outfilelist      OUTFILELIST;
typedef struct infilelist       INFILELIST;
typedef struct member_list      MEMBER_LIST;
typedef struct segdata          SEGDATA;
typedef struct pubdeflist       PUBDEFLIST;
typedef struct ovl_area {
    OVL_AREA    *next_area;
    SECTION     *sections;
} ovl_area;
typedef struct order_class      ORDER_CLASS;
typedef struct order_segment    ORDER_SEGMENT;

typedef struct section {
    SECTION             *next_sect;
    FILE_LIST           *files;
    pHTable             modFilesHashed;
    MOD_ENTRY           *mods;
    CLASS_ENTRY         *classlist;
    ORDER_CLASS         *orderlist;     // Link to data for ordering, if used
    targ_addr           sect_addr;
    overlay_ref         ovlref;
    OVL_AREA            *areas;
    SECTION             *parent;
    unsigned_32         relocs;
    unsigned_32         size;
    void                *reloclist;
    union {
        unsigned_32     file_loc;
        MOD_ENTRY       *dist_mods;
    } u;
    void                *dbg_info;
    OUTFILELIST         *outfile;
} section;

typedef struct path_entry {
    PATH_ENTRY          *next;
    char                name[1];
} path_entry;

typedef struct outfilelist {
    OUTFILELIST         *next;
    char                *fname;     // name of the file to be written to.
    f_handle            handle;
    unsigned long       file_loc;
    char                *buffer;
    unsigned long       bufpos;
    unsigned            ovlfnoff;   // offset of filename from _OVLTAB
    bool                is_exe;     // executable flag (for file permissions)
    unsigned long       origin;
} outfilelist;

typedef enum {
    INSTAT_USE_LIBPATH  = 0x0001,   // use libpath for this file.
    INSTAT_LIBRARY      = 0x0002,   // file is a library
    INSTAT_IOERR        = 0x0004,   // problem reading this file
    INSTAT_IN_USE       = 0x0008,   // file in use.
    INSTAT_OPEN_WARNING = 0x0010,   // only give a warning if can't open
    INSTAT_FULL_CACHE   = 0x0020,   // read entire file.
    INSTAT_PAGE_CACHE   = 0x0040,   // read in "paged"
    INSTAT_GOT_MODTIME  = 0x0080,
    INSTAT_NO_WARNING   = 0x0100
} infile_status;

#define INSTAT_SET_CACHE (INSTAT_FULL_CACHE | INSTAT_PAGE_CACHE)
#define INSTAT_LIB_SEARCH (INSTAT_USE_LIBPATH | INSTAT_LIBRARY)

typedef struct infilelist {
    INFILELIST          *next;
    const PATH_ENTRY    *path_list;
    const char          *prefix;
    void                *cache;  // used when object file cached in mem
    unsigned long       len;     // length of the file.
    unsigned long       currpos; // current position of the file.
    f_handle            handle;
    time_t              modtime;
    name_strtab         name;
    infile_status       status;
} infilelist;

typedef enum {
    /* bits 0..4 reserved for DBI_xxxx symbols */
    DBI_LINE            = 0x0001,
    DBI_TYPE            = 0x0002,
    DBI_LOCAL           = 0x0004,
    DBI_ONLY_EXPORTS    = 0x0008,
    DBI_STATICS         = 0x0010,
} dbi_flags;

#define DBI_ALL         (DBI_LINE | DBI_TYPE | DBI_LOCAL | DBI_STATICS)
#define DBI_MASK        (DBI_ALL | DBI_ONLY_EXPORTS)

typedef enum {
    /* bits 0..4 reserved for DBI_xxxx symbols are also stored here */
    /* bits 5..max available (bits 5..7 reserved for FMT_xxxx symbols, not used here) */
    STAT_HAS_CHANGED    = 0x0020,
    STAT_OMF_LIB        = 0x0040,
    STAT_AR_LIB         = 0x0080,
    STAT_LAST_SEG       = 0x0100,    // set by newsegment option
    STAT_TRACE_SYMS     = 0x0200,
    STAT_LIB_FIXED      = 0x0400,
    STAT_OLD_LIB        = 0x0800,
    STAT_LIB_USED       = 0x1000,
    STAT_SEEN_LIB       = 0x2000,
    STAT_HAS_MEMBER     = 0x4000,
    STAT_USER_SPECD     = 0x8000
} file_flags;

#define STAT_IS_LIB     (STAT_AR_LIB | STAT_OMF_LIB)

/*
 * overlay manager library priority         0
 * default library priority                 1-8 (OMF)
 * default library priority                 8   (coff/elf)
 * compiler user specified library priority 9
 * WLINK user specified library priority    10
 */
typedef enum {
    LIB_PRIORITY_MIN    = 0,
    LIB_PRIORITY_MID    = 5,
    LIB_PRIORITY_MAX    = 10
} lib_priority;

typedef struct file_list {
    FILE_LIST           *next_file;
    infilelist          *infile;
    union {
        union dict_entry    *dict;
        MEMBER_LIST         *member;
    } u;
    char                *strtab;    /* for AR format */
    file_flags          flags;
    lib_priority        priority;   /* for libraries */
    overlay_ref         ovlref;     /* for fixed libraries */
    unsigned                     : 0;
} file_list;

typedef enum {
    /* bits 0..4 reserved for DBI_xxxx symbols are also stored here */
    /* bits 5..7 reserved for FMT_xxxx symbols are also stored here */
    /* bits 8..max available */
    MOD_DBI_SEEN        = CONSTU32( 0x00000100 ),   // true if dbi segment seen in this mod.
    MOD_FIXED           = CONSTU32( 0x00000200 ),   // true if mod must stay in spec'd section
    MOD_VISITED         = CONSTU32( 0x00000400 ),   // true if visited in call graph analysis.
    MOD_NEED_PASS_2     = CONSTU32( 0x00000800 ),   // true if pass 2 needed for this module.
    MOD_LAST_SEG        = CONSTU32( 0x00001000 ),   // true if this module should end a group
    MOD_GOT_NAME        = CONSTU32( 0x00002000 ),   // true if already got a source file name
    MOD_IMPORT_LIB      = CONSTU32( 0x00004000 ),   // ORL: true if this is an import lib.
    MOD_KILL            = CONSTU32( 0x00008000 ),   // module should be removed from list
    MOD_FLATTEN_DBI     = CONSTU32( 0x00010000 ),   // flatten DBI found in this mod.
    MOD_DONE_PASS_1     = CONSTU32( 0x00020000 ),   // module been through pass 1 already.
    MOD_IS_FREE         = CONSTU32( 0x80000000 ),   // used for marking carve free blocks
} module_flags;

#define MOD_CLEAR_ON_INC    /* flags to clear when incremental linking. */ \
    (MOD_DONE_PASS_1)

typedef struct member_list {
    MEMBER_LIST         *next;
    module_flags        flags;      //dbi & newseg flags to be xferred to mod entry
    char                name[1];
} member_list;

#define NO_ARCS_YET     ((overlay_ref)-1)

/*
   NOTE: this is an entry for the kludge of the year award, 1993.
   Since I need to keep symbol *'s and modules in the same pointer,
   I tell the difference by checking test to see if it is less than 8K
   (the max. # of distributed modules).
*/

typedef union {
    symbol              *sym;
    unsigned_16         mod;
    unsigned_32         test;
} dist_arc;

// fields used only in distributing libs are marked dist:
// remember to change DIST_ONLY_SIZE if you remove or add a "dist" field!

typedef struct {
    overlay_ref         ovlref;     // dist: # of the module
    unsigned_16         numarcs;    // dist: of arcs in the list
    dist_arc            arcs[1];    // dist: the actual arcs.
} arcdata;

#define DIST_ONLY_SIZE (2 * sizeof( unsigned_16 ) + sizeof( dist_arc ))

typedef struct obj_name_list {
    struct obj_name_list    *next;
    size_t                  len;
    unsigned_32             num;
    name_strtab             name;          // NYI: make this vbl length again.
} obj_name_list;

typedef struct odbimodinfo      ODBIMODINFO;    // defd in dbg information hdrs
typedef struct dwarfmodinfo     DWARFMODINFO;
typedef struct cvmodinfo        CVMODINFO;

// OMF debug information formats
typedef enum {
    OMF_DBG_UNKNOWN,
    OMF_DBG_CODEVIEW,
    OMF_DBG_HLL
} omf_dbg_type;

typedef struct mod_entry {
    union {
        MOD_ENTRY       *next_mod;  // regular next pointer
        section         *sect;      // when distributing - section of current mod.
    } n;
    union {
        FILE_LIST       *source;
        name_strtab     fname;
    } f;
    name_strtab         name;
    unsigned_32         location;
    symbol              *publist;
    SEGDATA             *segs;
    time_t              modtime;
    size_t              relocs;
    size_t              sizerelocs;
    module_flags        modinfo;
    void                *lines;
    omf_dbg_type        omfdbg;
    union {
        arcdata         *arclist;   // segment definition data.
        MOD_ENTRY       *next;      // for keeping track of modules when distrib
    } x;
    union {
        ODBIMODINFO     *o;
        DWARFMODINFO    *d;
        CVMODINFO       *cv;
    } d;                        // union used for debugging information
} mod_entry;

typedef enum {
    CLASS_32BIT         = 0x0001,
    CLASS_TRANSFER      = 0x0002,   /* used for PE import transfer code */
    CLASS_MS_TYPE       = 0x0004,
    CLASS_MS_LOCAL      = 0x0008,
    CLASS_DWARF         = 0x000C,
    CLASS_CODE          = 0x0010,
    CLASS_LXDATA_SEEN   = 0x0020,
    CLASS_READ_ONLY     = 0x0040,
    CLASS_STACK         = 0x0080,
    CLASS_IDATA         = 0x0100,
    CLASS_FIXED         = 0x1000,   // Class should load at specified address
    CLASS_COPY          = 0x2000,   // Class should use data from DupClass
    CLASS_NOEMIT        = 0x4000,   // Class should not generate output
} class_status;

#define CLASS_DEBUG_INFO    (CLASS_MS_TYPE | CLASS_MS_LOCAL | CLASS_DWARF)

#define EMIT_CLASS(c)       (((c)->flags & CLASS_NOEMIT) == 0)

typedef struct class_entry {
    CLASS_ENTRY         *next_class;
    SEG_LEADER          *segs;
    name_strtab         name;
    class_status        flags;
    section             *section;
    targ_addr           BaseAddr;   // Fixed location to of this class for loadfile
    CLASS_ENTRY         *DupClass;  // Class to get data from for output
} class_entry;

typedef struct group_entry {
    GROUP_ENTRY         *next_group;
    SEG_LEADER          *leaders;
    symbol              *sym;
    section             *section;
    targ_addr           grp_addr;
    unsigned_16         segflags;
    offset              size;
    offset              totalsize;
    offset              linear;         // preferred base address
    union {
        void            *grp_relocs;    // OS2/ELF only.
        class_entry     *class;         // CV (during addr calc )
    } g;
    union {
        unsigned        qnxflags;       // QNX
        unsigned        os2flags;       // OS/2
        segment         dos_segment;    // DOS/16M: DOS segment value
    } u;
    boolbit             isfree      : 1;
    boolbit             isautogrp   : 1;
    boolbit             isdup       : 1;
    unsigned            num;
} group_entry;

// this is a bit in the segflags field. This is also defined in exeos2.h

#define SEG_DATA        1
#define SEG_READ_ONLY   0x80

// the default value to initialize group flags to. This is the same as
// SEG_LEVEL_3 in exeos2.h.

#define DEFAULT_GRP_FLAGS (0xC00 | SEG_READ_ONLY)

// flags used under OS/2 to indicate special information about a segment

#define SEG_16_ALIAS    1

typedef struct seg_leader {
    SEG_LEADER          *next_seg;
    SEG_LEADER          *grp_next;
    name_strtab         segname;
    SEGDATA             *pieces;
    group_entry         *group;
    class_entry         *class;
    offset              size;               // total size of segment
    offset              vsize;              // total virtual size of segment
    SEG_LEADER          *DupSeg;            // Segment to get data from for output
    unsigned_16         info;
    unsigned_16         align   : 5;        // alignment of seg (power of 2)
    unsigned_16         dbgtype : 3;        // debugging type of seg
    unsigned_16         combine : 2;        // combine val. of seg
    unsigned_32         num;                // # of addrinfos to output (video)
    targ_addr           seg_addr;           // address of segment.
    unsigned_16         segflags;           // format specific segment flags
} seg_leader;

/***********************************************************************
 *
 *        The info field is used as follows:
 *
 *  n = bit used in segnode only        b = used in both segnode and leader
 *  l = bit used in leader only
 *
 *  b            l b b        b b              n b
 *  x x x x      x x x x      x x x x      x x x x
 *  | | | |      | | |        | |              | |
 *  | | | |      | | |        | |              | +---> seg. is absolute
 *  | | | |      | | |        | |              +-----> seg. is comdat (ORL)
 *  | | | |      | | |        | +--------------------> seg. in ovl. class
 *  | | | |      | | |        +--(leader)------------> generate an addr_info.
 *  | | | |      | | |        +--(node)--------------> segdef dead (terminated)
 *  | | | |      | | +-------------------------------> seg. is code.
 *  | | | |      | +---------------------------------> 32-bit segment.
 *  | | | |      +-(leader)--------------------------> last segment in group
 *  | | | +-(leader)---------------------------------> Segment should load at specified address
 *  | | +---(leader)---------------------------------> Segment should use data copied from DupSeg
 *  | +-----(leader)---------------------------------> Segment should not generate output
 *  +------------------------------------------------> LxDATA seen for this seg.
 ***********************************************************************/

/*
 * bits in "info" field of "seg_entry" struct && the "seg_leader" struct.
 * Note that there are bits in this which are only used in the seg_entry->info
 * field, so the two words have overlapping bits.
 */

enum {
    SEG_ABSOLUTE        = 0x0001,
    SEG_COMDAT          = 0x0002,   /* seg is a comdat */
    SEG_OVERLAYED       = 0x0040,   /* segment belongs to an overlay class */
    MAKE_ADDR_INFO      = 0x0080,   /* set if making an addr info next time*/
    SEG_DEAD            = 0x0080,   /* mark a segdef as being "dead"(pass 2)*/
    SEG_CODE            = 0x0200,   /* segment is a code segment.         */
    USE_32              = 0x0400,   /* segment uses 32 bit addresses      */
    LAST_SEGMENT        = 0x0800,   /* force last segment in a code group */
    SEG_LXDATA_SEEN     = 0x8000,   /* LxDATA rec. seen for this segment */
    SEG_FIXED           = 0x1000,   /* Segment should start at seg_addr, not next addr */
    SEG_NOEMIT          = 0x2000,   /* Segment should not generate output */
    SEG_BOTH_MASK       = 0x8641,   /* flags common to both structures */
};

#define EMIT_SEG(s)     (((s)->segflags & SEG_NOEMIT) == 0)

enum {
    NOT_DEBUGGING_INFO  = 0x0000,
    MS_TYPE             = 0x0001,   /* microsoft type information         */
    MS_LOCAL            = 0x0002,   /* microsoft local symbol information */
    DWARF_DEBUG_INFO    = 0x0003,   /* various types of dwarf debug segments */
    DWARF_DEBUG_ABBREV  = 0x0004,
    DWARF_DEBUG_LINE    = 0x0005,
    DWARF_DEBUG_ARANGE  = 0x0006,
    DWARF_DEBUG_OTHER   = 0x0007
};

enum {
    COMBINE_INVALID     = 0,
    COMBINE_ADD         = 1,
    COMBINE_COMMON      = 2,
    COMBINE_STACK       = 3,
};

#define IS_DBG_DWARF( x ) ((x)->dbgtype >= DWARF_DEBUG_INFO)
#define IS_DBG_INFO( x ) ((x)->dbgtype != NOT_DEBUGGING_INFO)

/*
 * these are used to keep track of each individual contribution to a segment.
 * Any field that is solely used for dead code elimination is marked "dce"
 * any field that is solely used for comdat processing is marked "comdat"
*/

typedef struct segdata {
    SEGDATA             *next;
    SEGDATA             *mod_next;      // next segdata in module list.
    offset              length;         // length of segment in current module.
    virt_mem_ptr        u1;             // virtual memory pointer to data for this segment
    virt_mem            vm_data;        // virtual memory pointer to data for class copy data
    union {
        name_strtab     name;           // name of the segment
        seg_leader      *leader;        // leader for the segment.
        SEGDATA         *sdata;         // for explicit comdats
    } u;
    union {
        void            *refs;          // P1dce: list of other seg's this references
        signed_32       delta;          // P2: for calc'ing segment & symbol addrs
    } a;
    union {
        mod_entry       *mod;           // P2CV&DW: pointer to defining module.
        name_strtab     clname;         // INC: class name for segment
    } o;
    unsigned_32         addrinfo;       // P2VIDEO: offset into addrinfo of seg.
    unsigned_16         frame;          // the frame of an absolute segment.
    unsigned            align      : 5;
    unsigned            select     : 3; // comdat: selection type
    unsigned            combine    : 2; // how to combine segment with others
    unsigned            alloc      : 2; // comdat: where to allocate segment.

    boolbit             is32bit    : 1; // true if segment is 32 bits
    boolbit             iscode     : 1; // true if a code segment.
    boolbit             isabs      : 1; // true if this is an absolute segment.
    boolbit             iscdat     : 1; // true if this is a comdat
    boolbit             isuninit   : 1; // true if seg is uninitialized
    boolbit             isidata    : 1; // true if segment is .idata (ORL only)
    boolbit             ispdata    : 1; // true if segment is .pdata
    boolbit             isreldata  : 1; // true if segment is .reldata
    boolbit             visited    : 1; // dce: true if visited in graph search.
    boolbit             isrefd     : 1; // dce: true if this module is referenced.
    boolbit             isdead     : 1; // dce: true if segdata or segdef killed.
    boolbit             isdefd     : 1; // segdata has been defined
    boolbit             isfree     : 1; // segdata is free (used in carver stuff)
    boolbit             isprepd    : 1; // has been prepped for inc linking
    boolbit             canfarcall : 1; // OK to do far call optimization here
    boolbit             hascdatsym : 1; // true if comdat and has a symbol defd
} segdata;

typedef struct {
    void                *next;
    void                *entry;
} node;

#define NOT_IMP_BY_ORDINAL ((ordinal_t)-1)

typedef signed_32       ordinal_t;

typedef struct {
    union {
        obj_name_list   *modnum;        /* # of DLL in imported names table */
        name_strtab     modname;
    } m;
    union {
        obj_name_list   *entry;         /* # of entry in DLL */
        name_strtab     entname;
        ordinal_t       ordinal;
    } u;
    boolbit             isordinal   : 1;
    boolbit             isfree      : 1;
    symbol              *iatsym;        // NT: symbol for address in iat
} dll_sym_info;

typedef enum {
    SEGFLAG_SEGMENT,
    SEGFLAG_CLASS,
    SEGFLAG_CODE,
    SEGFLAG_DATA
} segflag_type;

// this structure used for processing segment flags for various executable types
// structures qnx_seg_flags and os2_seg_flags depend on this declaration
typedef struct xxx_seg_flags {
    struct xxx_seg_flags    *next;
    unsigned_16             flags;
    char                    *name;
    segflag_type            type;   // as above.
} xxx_seg_flags;

typedef struct {
    symbol              *entry;
    orl_symbol_handle   handle;         // ORL: handle for the symbol
    overlay_ref         ovlref;
    boolbit             isweak  : 1;
    boolbit             isdefd  : 1;    // used in ORL
} extnode;

typedef struct {
    GROUP_ENTRY         *entry;
} grpnode;

typedef struct {
    SEGDATA             *entry;
    orl_sec_handle      handle;     // ORL: handle for the segment.
    unsigned_8          *contents;  // ORL: pointer to contents of segment.
    unsigned            info;
} segnode;

typedef struct list_of_names {
    LIST_OF_NAMES       *next_name;
    char                name[1];
} list_of_names;

typedef struct {
    segdata             *seg;
    offset              obj_offset; // pass 1: delta for fixup offsets
    targ_addr           addr;
    unsigned_8          *data;
} lobject_data;

typedef struct {
    size_t              len;
    const char          *name;
} length_name;

typedef struct order_class {
    ORDER_CLASS         *NextClass;
    class_entry         *Ring;  // Used for sorting
    char                *Name;
    char                *SrcName;
    targ_addr           Base;
    ORDER_SEGMENT       *SegList;
    boolbit             FixedAddr   : 1;
    boolbit             NoEmit      : 1;
    boolbit             Copy        : 1;
} order_class;

typedef struct order_segment {
    ORDER_SEGMENT       *NextSeg;
    char                *Name;
    targ_addr           Base;
    boolbit             FixedAddr   : 1;
    boolbit             NoEmit      : 1;
} order_segment;

