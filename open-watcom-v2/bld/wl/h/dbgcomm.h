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


/* items used in the debugging information generation files */

#include "pushpck1.h"

typedef struct {
    unsigned_16         linnum;
    unsigned_16         off;
} ln_off_286;

typedef struct {
    unsigned_16         linnum;
    unsigned_32         off;
} ln_off_386;

typedef union {
    ln_off_286  _286;
    ln_off_386  _386;
} ln_off_pair;

#include "poppck.h"

#define LINE_IS_32BIT   1       // since lines always even, can use bottom bit

typedef struct lineinfo {
    struct lineinfo *   next;
    segdata *           seg;
    size_t              size;
    char                data[1];
} lineinfo;

typedef void line_walk_fn( lineinfo * );

extern virt_mem DBIAlloc( virt_mem_size );
extern void     DBIModGlobal( void * );
extern void     DBIAddrInfoScan( seg_leader *,
                      void (*)(segdata *, void *),
                      void (*)(segdata *, offset, offset, void *, bool),
                      void * );
extern void     DBILineWalk( lineinfo *, line_walk_fn * );
extern unsigned DBICalcLineQty( lineinfo * );

#define MOD_NOT_DEBUGGABLE(mod) ( ((mod)->modinfo & MOD_NEED_PASS_2) == 0 || \
                                    ((mod)->modinfo & MOD_IMPORT_LIB) )

extern group_entry *    DBIGroups;
