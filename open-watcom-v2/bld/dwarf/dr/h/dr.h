/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  DWARF reader interface definition.
*
****************************************************************************/


#ifndef DR_H_INCLUDED
#define DR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "watcom.h"
#include "bool.h"
#include "dwarf.h"

typedef enum {
    DR_DEBUG_INFO,
    DR_DEBUG_PUBNAMES,
    DR_DEBUG_ARANGES,
    DR_DEBUG_LINE,
    DR_DEBUG_LOC,
    DR_DEBUG_ABBREV,
    DR_DEBUG_MACINFO,
    DR_DEBUG_STR,
    DR_DEBUG_REF,
    DR_DEBUG_NUM_SECTS          // the number of debug info sections.
} dr_section;

typedef enum {
    DREXCEP_OUT_OF_VM,
    DREXCEP_BAD_DBG_VERSION,
    DREXCEP_BAD_DBG_INFO,
    DREXCEP_OUT_OF_MMEM,        // DWRMALLOC or DWRREALLOC failed
    DREXCEP_DWARF_LIB_FAIL      // bug in the dwarf library
} dr_except;

typedef enum {
    DR_LANG_UNKNOWN,
    DR_LANG_CPLUSPLUS,
    DR_LANG_FORTRAN,
    DR_LANG_C
} dr_language;

typedef enum {
    DR_MODEL_NONE    = 0,
    DR_MODEL_FLAT    = 1,
    DR_MODEL_SMALL   = 2,
    DR_MODEL_MEDIUM  = 3,
    DR_MODEL_COMPACT = 4,
    DR_MODEL_LARGE   = 5,
    DR_MODEL_HUGE    = 6,
} dr_model;

// NOTE: these have to correspond to equivalent #defines in dwarf.h!

typedef enum {
    DR_ACCESS_PUBLIC = 1,
    DR_ACCESS_PROTECTED = 2,
    DR_ACCESS_PRIVATE = 3
} dr_access;

typedef enum {
    DR_VIRTUALITY_NONE = 0,
    DR_VIRTUALITY_VIRTUAL = 1
} dr_virtuality;

#define DRMEM_HDL_VOID  ((drmem_hdl)(pointer_uint)-1)       /* special handle denoting void type */
#define DRMEM_HDL_NULL  ((drmem_hdl)(pointer_uint)0)

#if defined( USE_VIRTMEM )
typedef unsigned long   drmem_hdl;
#else
typedef char            *drmem_hdl;
#endif

struct WDRRoutines {                                        /* defaults */
/* I/O routines */
    void   (*cli_read)( void *, dr_section, void *, size_t );   // read
    void   (*cli_seek)( void *, dr_section, long );             // lseek
/* memory routines */
    void * (*cli_alloc)( size_t );                              // malloc
    void * (*cli_realloc)( void *, size_t );                    // realloc
    void   (*cli_free)( void * );                               // free
    void   (*cli_except)( dr_except );                          // fatal error handler
};

typedef struct COMPUNIT_INFO    *dr_cu_handle;
typedef struct dr_dbg_info      *dr_dbg_handle;

typedef unsigned                dr_fileidx;

#define DWRSetRtns( __read, __seek, __alloc, __realloc, __free, __except ) \
    struct WDRRoutines DWRRtns = { \
        __read,       \
        __seek,       \
        __alloc,      \
        __realloc,    \
        __free,       \
        __except      \
    };

typedef bool    (*DRWLKBLK)( drmem_hdl, int index, void * );

#include "drinit.h"
#include "drmod.h"
#include "drsrchdf.h"
#include "drsearch.h"
#include "drfdkids.h"
#include "drinfo.h"
#include "drarange.h"
#include "drpubnam.h"
#include "drdecnam.h"
#include "drline.h"
#include "drtype.h"
#include "drloc.h"

extern bool     DRSwap( void );

#ifdef __cplusplus
};
#endif

#endif
