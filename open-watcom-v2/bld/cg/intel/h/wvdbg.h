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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/* aligned */


enum {
    #define pick(name,ci,start,len)  REG_##name,
    #include "watdbreg.h"
    #undef pick
};

/* % */
/* % Type constants */
/* % */

#define WT_NAME                 0x10
#define NAME_SCALAR             0x00
#define NAME_SCOPE              0x01
#define NAME_NAME               0x02
#define NAME_CUEINFO            0x03
#define NAME_EOF                0x04

#define WT_ARRAY                0x20
#define ARRAY_BYTE              0x00
#define ARRAY_WORD              0x01
#define ARRAY_LONG              0x02
#define ARRAY_TYPE              0x03
#define FORTRAN_TYPE_286        0x04
#define FORTRAN_TYPE_386        0x05

#define WT_SUBRANGE             0x30
#define RANGE_BYTE              0x00
#define RANGE_WORD              0x01
#define RANGE_LONG              0x02

#define WT_POINTER              0x40
#define POINTER_NEAR_286        0x00
#define POINTER_FAR_286         0x01
#define POINTER_HUGE            0x02
#define DEREF_NEAR_286          0x03
#define DEREF_FAR_286           0x04
#define DEREF_HUGE              0x05
#define POINTER_NEAR_386        0x06
#define POINTER_FAR_386         0x07
#define DEREF_NEAR_386          0x08
#define DEREF_FAR_386           0x09

#define WT_ENUMERATED           0x50
#define ENUM_LIST               0x00
#define ENUM_BYTE               0x01
#define ENUM_WORD               0x02
#define ENUM_LONG               0x03

#define WT_STRUCTURE            0x60
#define STRUCT_LIST             0x00
#define STRUCT_F_BYTE           0x01
#define STRUCT_F_WORD           0x02
#define STRUCT_F_LONG           0x03
#define STRUCT_BF_BYTE          0x04
#define STRUCT_BF_WORD          0x05
#define STRUCT_BF_LONG          0x06
#define STRUCT_F_LOC            0x07
#define STRUCT_BF_LOC           0x08
#define STRUCT_INHERIT          0x09


#define METHOD_VANILLA          0x00
#define METHOD_STATIC           0x01
#define METHOD_FRIEND           0x02
#define METHOD_VIRTUAL          0x03

#define INHERIT_DBASE           0x01
#define INHERIT_VBASE           0x02
#define INHERIT_IVBASE          0x03

#define WT_PROCEDURE            0x70
#define PROC_NEAR_286           0x00
#define PROC_FAR_286            0x01
#define PROC_NEAR_386           0x02
#define PROC_FAR_386            0x03
#define PROC_EXT_PARM_LIST      0x04

#define WT_CHAR_BLOCK           0x80
#define NAME_CHAR_BYTE          0x00
#define NAME_CHAR_WORD          0x01
#define NAME_CHAR_LONG          0x02
#define NAME_CHAR_IND_286       0x03
#define NAME_CHAR_IND_386       0x04
#define NAME_CHAR_LOC           0x05

/*  low nibble contains size of scalar - 1 */
#define SCALAR_INT              0x00
#define SCALAR_UNSIGNED         0x10
#define SCALAR_FLOAT            0x20
#define SCALAR_VOID             0x30
#define SCALAR_COMPLEX          0x40


/* % */
/* % Symbol constants */
/* % */



/*  symbol definitions */
#define SYM_VARIABLE            0x10
#define VAR_MODULE_286          0x00
#define VAR_LOCAL               0x01
#define VAR_MODULE_386          0x02
#define VAR_MODULE_LOC          0x03

#define SYM_CODE                0x20
#define CODE_BLOCK_286          0x00
#define CODE_NEAR_RTN_286       0x01
#define CODE_FAR_RTN_286        0x02
#define CODE_BLOCK_386          0x03
#define CODE_NEAR_RTN_386       0x04
#define CODE_FAR_RTN_386        0x05
#define CODE_MEMBER_SCOPE       0x06

#define NEW_BASE                0x30
#define ADD_PREV_SEG            0x00
#define SET_BASE_286            0x01
#define SET_BASE_386            0x02

#if _TARGET & _TARG_8086
        #define FORTRAN_TYPE    FORTRAN_TYPE_286
        #define POINTER_NEAR    POINTER_NEAR_286
        #define POINTER_FAR     POINTER_FAR_286
        #define DEREF_NEAR      DEREF_NEAR_286
        #define DEREF_FAR       DEREF_FAR_286
        #define PROC_NEAR       PROC_NEAR_286
        #define PROC_FAR        PROC_FAR_286
        #define NAME_CHAR_IND   NAME_CHAR_IND_286
        #define VAR_MODULE      VAR_MODULE_286
        #define SET_BASE        SET_BASE_286
        #define CODE_BLOCK      CODE_BLOCK_286
        #define CODE_NEAR_RTN   CODE_NEAR_RTN_286
        #define CODE_FAR_RTN    CODE_FAR_RTN_286
#elif _TARGET & _TARG_80386
        #define FORTRAN_TYPE    FORTRAN_TYPE_386
        #define POINTER_NEAR    POINTER_NEAR_386
        #define POINTER_FAR     POINTER_FAR_386
        #define DEREF_NEAR      DEREF_NEAR_386
        #define DEREF_FAR       DEREF_FAR_386
        #define PROC_NEAR       PROC_NEAR_386
        #define PROC_FAR        PROC_FAR_386
        #define NAME_CHAR_IND   NAME_CHAR_IND_386
        #define VAR_MODULE      VAR_MODULE_386
        #define SET_BASE        SET_BASE_386
        #define CODE_BLOCK      CODE_BLOCK_386
        #define CODE_NEAR_RTN   CODE_NEAR_RTN_386
        #define CODE_FAR_RTN    CODE_FAR_RTN_386
#endif


