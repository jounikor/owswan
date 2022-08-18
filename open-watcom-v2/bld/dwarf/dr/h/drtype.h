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



typedef enum {
    DR_PTR_none,
    DR_PTR_near16,
    DR_PTR_far16,
    DR_PTR_huge16,
    DR_PTR_near32,
    DR_PTR_far32
} dr_ptr;

typedef enum {
    DR_TYPEK_NONE,
    DR_TYPEK_DATA,
    DR_TYPEK_CODE,
    DR_TYPEK_ADDRESS,
    DR_TYPEK_VOID,
    DR_TYPEK_BOOL,
    DR_TYPEK_ENUM,
    DR_TYPEK_CHAR,
    DR_TYPEK_INTEGER,
    DR_TYPEK_REAL,
    DR_TYPEK_COMPLEX,
    DR_TYPEK_STRING,
    DR_TYPEK_POINTER,
    DR_TYPEK_REF,
    DR_TYPEK_STRUCT,
    DR_TYPEK_UNION,
    DR_TYPEK_CLASS,
    DR_TYPEK_ARRAY,
    DR_TYPEK_FUNCTION,
    DR_TYPEK_LAST
} dr_typek;

typedef enum {
    DR_STORE_NONE     = 0,
    DR_STORE_CONST    = 0x01<<0,
    DR_STORE_VOLATILE = 0x01<<1,
    DR_STORE_PACKED   = 0x01<<2,
} dr_store;

typedef enum {
    DR_MOD_NONE,
    DR_MOD_ADDR,
    DR_MOD_BASE
} dr_mod;

enum {
    DR_WLKBLK_STRUCT = 5,
    DR_WLKBLK_ARRSIB = 3,
    DR_WLKBLK_ENUMS  = 2,
};

typedef enum {
    DR_ARRAY_NONE          = 0,
    DR_ARRAY_ORDERING      = 1<<0,
    DR_ARRAY_BYTE_SIZE     = 1<<1,
    DR_ARRAY_STRIDE_SIZE   = 1<<2,
    DR_ARRAY_COUNT         = 1<<3,
} dr_array_stat;

typedef struct {
    dr_typek kind;
    uint_32  size;
    dr_store acc;
    dr_mod   mclass;
    union {
        dr_ptr   ptr;
        bool     sign;
        uint_32  beg_scope;
    } modifier;
} dr_typeinfo;

typedef struct {
    uint_32    byte_size;
    uint_32    stride_size;
    uint_32    count;
    dw_ord     ordering;
    drmem_hdl  child;
} dr_array_info;

typedef enum {
    DR_VAL_NOT,
    DR_VAL_INT,
    DR_VAL_REF,
} dr_val_class;

typedef struct {
    dr_val_class val_class;
    union{
        drmem_hdl ref;
        uint_32   s;
    } val;
} dr_val32;

typedef struct {
    dr_val32 low;
    dr_val32 high;
    dr_val32 count;
} dr_subinfo;

typedef struct {
    dr_val32 byte_size;
    dr_val32 bit_offset;
    dr_val32 bit_size;
} dr_bitfield;

extern bool             DRGetTypeInfo( drmem_hdl entry, dr_typeinfo *info );
extern drmem_hdl        DRGetTypeAT( drmem_hdl entry );
extern drmem_hdl        DRSkipTypeChain( drmem_hdl entry );
extern bool             DRWalkStruct( drmem_hdl mod, const DRWLKBLK *wlks, void *d );
extern bool             DRWalkArraySibs( drmem_hdl mod, const DRWLKBLK *wlks, void *d );
extern bool             DRWalkEnum( drmem_hdl mod,  DRWLKBLK wlk, void *d );
extern bool             DRConstValAT( drmem_hdl var, uint_32 *ret );
extern dr_array_stat    DRGetArrayInfo( drmem_hdl array, dr_array_info *info );
extern void             DRGetSubrangeInfo( drmem_hdl sub, dr_subinfo *info );
extern int              DRGetBitFieldInfo( drmem_hdl mem, dr_bitfield *info );
extern dr_ptr           DRGetAddrClass( drmem_hdl entry );
