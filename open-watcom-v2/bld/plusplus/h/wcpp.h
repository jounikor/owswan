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


#ifndef __WCPP_H__
#define __WCPP_H__

// WCPP.H -- common definitions between compile, library
//
// 91/11/07 -- J.W.Welch        -- defined

#include <stddef.h>
#include "watcom.h"

// run-time constants for destructor's flag parm
enum
{   DTOR_COMPONENT      = 0x01          // be a component dtor
,   DTOR_DELETE_THIS    = 0x02          // delete the element
,   DTOR_DELETE_VECTOR  = 0x04          // delete the array
,   DTOR_DELETE_OBJECT  = 0x08          // delete the object
,   DTOR_IGNORE_COMPS   = 0x10          // do not destruct components
,   DTOR_NULL           = 0x00
};

// run-time constants for constructor's flag parm
enum
{   CTOR_COMPONENT      = DTOR_COMPONENT    // be a component ctor
,   CTOR_NULL           = 0x00
};

// run-time constants for RTTI leap control word
#define RL_NULL         0x00
#define RL_VIRTUAL      0x01    // use 'vb_index' before 'offset'
#define RL_BAD_BASE     0x02    // cannot convert to this base class from most-derived
#define RL_LAST         0x04    // last leap in list
#define RL_HAS_VBASE    0x08    // host class has virtual bases (only in first leap)
#define RL_RUNTIME      0x0f    // keep these bits for runtime structure
                                // compile-time use only
#define RL_AMBIGUOUS    0x80    // more than one copy of the base class
#define RL_PUBLIC       0x40    // base is publically accesible

// run-time constants for RTTI adjustor control word
#define RA_NULL         0x00
#define RA_CDISP        0x01    // RA_CDISP -- apply ctor-disp adjustment
#define RA_RUNTIME      0x01    // keep these bits for runtime structure

#define MAX_SET_SV  32                  // max. static set_sv commands

typedef enum {
    #define pick(a,p) DTC_ ## a,
    #include "_dtcdefs.h"
    #undef pick
    MAX_DTC_DEF
} DTC_KIND;

typedef uint_8 DTOR_CMD_CODE;

enum                                    // registration codes
{   DTRG_FUN                            // - function
,   DTRG_OBJECT                         // - object
,   DTRG_ARRAY                          // - array
// remove following on next object-model change
,   DTRG_STATIC_INITFS_not_used         // - static initialization, file scope
,   DTRG_STATIC_INITLS                  // - static initialization, local
,   DTRG_COUNT                          // # registrations
};

enum                                    // CATEGORIES OF OBJECTS THROWN/CAUGHT
{   THROBJ_CLASS        = 0x00          // - class (no virtual bases)
,   THROBJ_PTR_CLASS    = 0x01          // - pointer to class
,   THROBJ_REFERENCE    = 0x02          // - reference
,   THROBJ_SCALAR       = 0x03          // - scalar
,   THROBJ_CLASS_VIRT   = 0x04          // - class (has virtual bases)
,   THROBJ_ANYTHING     = 0x05          // - ... (catch only)
,   THROBJ_VOID_STAR    = 0x06          // - void* (with optional const,volatile)
,   THROBJ_PTR_SCALAR   = 0x07          // - pointer to scalar
,   THROBJ_PTR_FUN      = 0x08          // - pointer to function
,   THROBJ_MAX                          // - number of categories
};
typedef uint_8 THROBJ;

enum {                                  // TYPE-SIG flags
    TSIG_FLAGS_NONE     = 0x00,         // - none
    TSIG_FLAGS_INDIRECT = 0x01          // - indirect
};

typedef struct ts_hdr TS_HDR;
#include "pushpck1.h"
struct ts_hdr                           // TYPE-SIG HEADER
{   uint_8  hdr_actual;                 // - actual entry
    uint_8  hdr_ptr;                    // - T* entry
    uint_8  hdr_ref;                    // - T& entry
    uint_8  flags;                      // - flags
};
#include "poppck.h"

#define __STR( __s ) #__s                               // quoted string
#define __PASTE( __s1, __s2 )        __s1##__s2         // paste tokens
#define __PASTE3( __s1, __s2, __s3 ) __s1##__s2##__s3   // paste tokens

// There are __wcpp_* names that are internal to the runtime library.
// These names should be changed to something like __wint_* so that
// they are not confused with external compiler entry points.
//
#define CPPLIB( name )      __wcpp_4_##name##_          // name of run-time function
#define CPPLIBDATA( name )  __wcpp_4_data_##name##_     // name of data

#endif
