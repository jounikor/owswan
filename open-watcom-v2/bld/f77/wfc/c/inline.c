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


//
// INLINE  : inline pragamas used to optimize certain operations
//

#include "ftnstd.h"
#include "global.h"
#include "cgdefs.h"
#include "wf77aux.h"
#include "wf77prag.h"
#include "wf77defs.h"
#include "fcgbls.h"
#include "cgflags.h"
#include "cpopt.h"
#include "inline.h"
#include "types.h"
#include "fctypes.h"
#include "rstutils.h"
#include "wf77info.h"
#include "fcgmain.h"
#include "cgswitch.h"
#include "cgprotos.h"


#if _CPU == 386 || _CPU == 8086

#if _CPU == 386

// Space optimized pragmas (take an awful beating on a pentium)

// edi  - destination pointer
// esi  - source pointer
// ecx  - number of characters to move
static  char    __RTIStrBlastEqOS[] =  { "aux __RTIStrBlastEq           \
                                        parm    reverse                 \
                                                [edi] [esi] [ecx] =     \
                                        \"rep   movsb\"                 \
                                " };

// edi  - destination pointer
// eax  - number of spaces to append
// esi  - source pointer
// ecx  - number of characters to move
static  char    __RTIStrBlastNeOS[] =  { "aux __RTIStrBlastNe           \
                                        parm    reverse                 \
                                                [edi] [eax] [esi] [ecx]=\
                                        \"rep   movsb\"                 \
                                        \"mov   ecx, eax\"              \
                                        \"mov   eax, 0x20202020\"       \
                                        \"rep   stosb\"                 \
                                " };

// Time Optimized pragmas

// edi  - destination pointer
// esi  - source pointer
// ecx  - number of 4 character tuples to move (strlen >> 2)
// eax  - number of characters left over after initial 4-tuple move (strlen & 3)
static  char    __RTIStrBlastEqOT[] =  { "aux __RTIStrBlastEq             \
                                        parm    reverse                 \
                                                [edi] [esi] [ecx] [eax] =\
                                        \"rep   movsd\"                 \
                                        \"mov   ecx, eax\"              \
                                        \"rep   movsb\"                 \
                                " };

// edi  - destination pointer
// eax  - number of spaces to append
// esi  - source pointer
// ecx  - number of characters to move
static  char    __RTIStrBlastNeOT[] =  { "aux __RTIStrBlastNe             \
                                        parm    reverse                 \
                                                [edi] [edx] [esi] [eax] \
                                        modify  [ecx] =                 \
                                        \"mov   ecx, eax\"              \
                                        \"shr   ecx, 2\"                \
                                        \"rep   movsd\"                 \
                                        \"mov   ecx, eax\"              \
                                        \"and   ecx, 3\"                \
                                        \"rep   movsb\"                 \
                                        \"mov   ecx, edx\"              \
                                        \"shr   ecx, 2\"                \
                                        \"mov   eax, 0x20202020\"       \
                                        \"rep   stosd\"                 \
                                        \"mov   ecx, edx\"              \
                                        \"and   ecx, 3\"                \
                                        \"rep   stosb\"                 \
                                " };

#elif _CPU == 8086

// Space Optimized pragmas

// es di - destination pointer
// ds si - source pointer
// cx    - number of characters to move
static  char    __RTIStrBlastEqOS[] =  { "aux __RTIStrBlastEq           \
                                        parm    reverse                 \
                                                [es di] [ds si] [cx]    \
                                        modify  exact [di si cx] =      \
                                        \"rep   movsb\"                 \
                                " };

// es di - destination pointer
// ax    - number of spaces to append
// ds si - source pointer
// cx    - number of characters to move
static  char    __RTIStrBlastNeOS[] =  { "aux __RTIStrBlastNe           \
                                        parm    reverse                 \
                                                [es di] [ax] [ds si] [cx]\
                                        modify  exact [di ax si cx] =   \
                                        \"rep   movsb\"                 \
                                        \"mov   cx, ax\"                \
                                        \"mov   ax, 0x2020\"            \
                                        \"rep   stosb\"                 \
                                " };

// Time Optimized pragmas

// es di - destination pointer
// ds si - source pointer
// cx    - number of 2 character tuples to move (strlen >> 21
// ax    - number of characters left over after initial 2-tuple move (strlen & 1)
static  char    __RTIStrBlastEqOT[] =  { "aux __RTIStrBlastEq             \
                                        parm    reverse                 \
                                                [es di] [ds si] [cx] [ax]\
                                        modify  exact [cx si di] =      \
                                        \"rep   movsw\"                 \
                                        \"mov   cx, ax\"                \
                                        \"rep   movsb\"                 \
                                " };

// es di - destination pointer
// ax    - number of spaces to append
// ds si - source pointer
// cx    - number of characters to move
static  char    __RTIStrBlastNeOT[] =  { "aux __RTIStrBlastNe             \
                                        parm    reverse                 \
                                                [es di] [dx] [ds si] [ax]\
                                        modify  exact [di dx si ax cx] =\
                                        \"mov   cx,ax\"                 \
                                        \"shr   cx,1\"                  \
                                        \"rep   movsw\"                 \
                                        \"adc   cx,0\"                  \
                                        \"rep   movsb\"                 \
                                        \"mov   cx,dx\"                 \
                                        \"mov   ax,0x2020\"             \
                                        \"shr   cx,1\"                  \
                                        \"rep   stosw\"                 \
                                        \"adc   cx,0\"                  \
                                        \"rep   stosb\"                 \
                                " };

// Windows pragmas (can't use DS as an argument since DS is pegged)

// Space Optimized pragmas

// es di - destination pointer
// si bx - source pointer
// cx    - number of characters to move
static  char    __RTIStrBlastEqOSWin[] =  { "aux __RTIStrBlastEq        \
                                        parm    reverse                 \
                                                [es di] [si bx] [cx]    \
                                        modify  exact [di si cx] =      \
                                        \"push  ds\"                    \
                                        \"mov   ds,si\"                 \
                                        \"mov   si,bx\"                 \
                                        \"rep   movsb\"                 \
                                        \"pop   ds\"                    \
                                " };

// es di - destination pointer
// ax    - number of spaces to append
// si bx - source pointer
// cx    - number of characters to move
static  char    __RTIStrBlastNeOSWin[] =  { "aux __RTIStrBlastNe        \
                                        parm    reverse                 \
                                                [es di] [ax] [si bx] [cx]\
                                        modify  exact [di ax si cx] =   \
                                        \"push  ds\"                    \
                                        \"mov   ds,si\"                 \
                                        \"mov   si,bx\"                 \
                                        \"rep   movsb\"                 \
                                        \"mov   cx, ax\"                \
                                        \"mov   ax, 0x2020\"            \
                                        \"rep   stosb\"                 \
                                        \"pop   ds\"                    \
                                " };

// Time Optimized pragmas

// es di - destination pointer
// si bx - source pointer
// cx    - number of 2 character tuples to move (strlen >> 21
// ax    - number of characters left over after initial 2-tuple move (strlen & 1)
static  char    __RTIStrBlastEqOTWin[] =  { "aux __RTIStrBlastEq          \
                                        parm    reverse                 \
                                                [es di] [si bx] [cx] [ax]\
                                        modify  exact [cx si di] =      \
                                        \"push  ds\"                    \
                                        \"mov   ds,si\"                 \
                                        \"mov   si,bx\"                 \
                                        \"rep   movsw\"                 \
                                        \"mov   cx,ax\"                 \
                                        \"rep   movsb\"                 \
                                        \"pop   ds\"                    \
                                " };

// es di - destination pointer
// ax    - number of spaces to append
// si bx - source pointer
// cx    - number of characters to move
static  char    __RTIStrBlastNeOTWin[] =  { "aux __RTIStrBlastNe          \
                                        parm    reverse                 \
                                                [es di] [dx] [si bx] [ax]\
                                        modify  exact [di dx si ax cx] =\
                                        \"push  ds\"                    \
                                        \"mov   ds,si\"                 \
                                        \"mov   si,bx\"                 \
                                        \"mov   cx,ax\"                 \
                                        \"shr   cx,1\"                  \
                                        \"rep   movsw\"                 \
                                        \"adc   cx,0\"                  \
                                        \"rep   movsb\"                 \
                                        \"mov   cx,dx\"                 \
                                        \"mov   ax,0x2020\"             \
                                        \"shr   cx,1\"                  \
                                        \"rep   stosw\"                 \
                                        \"adc   cx,0\"                  \
                                        \"rep   stosb\"                 \
                                        \"pop   ds\"                    \
                                " };

// Small memory pragmas.

// Space Optimized pragmas

// di   - destination pointer
// si   - source pointer
// cx   - number of characters to move
static  char    __RTIStrBlastEqOSS[] =  { "aux __RTIStrBlastEq          \
                                        parm    reverse                 \
                                                [di] [si] [cx]          \
                                        modify  [es] =                  \
                                        \"push  ds\"                    \
                                        \"pop   es\"                    \
                                        \"rep   movsb\"                 \
                                " };

// di   - destination pointer
// ax   - number of spaces to append
// si   - source pointer
// cx   - number of characters to move
static  char    __RTIStrBlastNeOSS[] =  { "aux __RTIStrBlastNe          \
                                        parm    reverse                 \
                                                [di] [ax] [si] [cx]     \
                                        modify  [es] =                  \
                                        \"push  ds\"                    \
                                        \"pop   es\"                    \
                                        \"rep   movsb\"                 \
                                        \"mov   cx, ax\"                \
                                        \"mov   ax, 0x2020\"            \
                                        \"rep   stosb\"                 \
                                " };

// Time Optimized pragmas

// di   - destination pointer
// si   - source pointer
// cx   - number of 2 character tuples to move (strlen >> 21
// ax   - number of characters left over after initial 2-tuple move (strlen & 1)
static  char    __RTIStrBlastEqOTS[] =  { "aux __RTIStrBlastEq            \
                                        parm    reverse                 \
                                                [di] [si] [cx] [ax]     \
                                        modify  [es] =                  \
                                        \"push  ds\"                    \
                                        \"pop   es\"                    \
                                        \"rep   movsw\"                 \
                                        \"mov   cx, ax\"                \
                                        \"rep   movsb\"                 \
                                " };

// di   - destination pointer
// ax   - number of spaces to append
// si   - source pointer
// cx   - number of characters to move
static  char    __RTIStrBlastNeOTS[] =  { "aux __RTIStrBlastNe            \
                                        parm    reverse                 \
                                                [di] [dx] [si] [ax]     \
                                        modify  [cx es] =               \
                                        \"push  ds\"                    \
                                        \"pop   es\"                    \
                                        \"mov   cx, ax\"                \
                                        \"shr   cx, 1\"                 \
                                        \"rep   movsw\"                 \
                                        \"adc   cx, 0\"                 \
                                        \"rep   movsb\"                 \
                                        \"mov   cx, dx\"                \
                                        \"mov   ax, 0x2020\"            \
                                        \"shr   cx, 1\"                 \
                                        \"rep   stosw\"                 \
                                        \"adc   cx, 0\"                 \
                                        \"rep   stosb\"                 \
                                " };
#endif

typedef struct inline_rtn {
    const char  *name;
    const char  *pragma;
    cg_type     typ;
    sym_id      sym_ptr;
    aux_info    *info;
} inline_rtn;

static inline_rtn  OptTimeInlineTab[] = {
    #define pick(e,n,s1,s2,s3,s4,s5,s6)  #n, n##s1, TY_INTEGER, NULL, NULL,
    #include "_inline.h"
    #undef pick
};

static inline_rtn  OptSpaceInlineTab[] = {
    #define pick(e,n,s1,s2,s3,s4,s5,s6)  #n, n##s2, TY_INTEGER, NULL, NULL,
    #include "_inline.h"
    #undef pick
};

#if _CPU == 8086
static inline_rtn  OptTimeWinInlineTab[] = {
    #define pick(e,n,s1,s2,s3,s4,s5,s6)  #n, n##s3, TY_INTEGER, NULL, NULL,
    #include "_inline.h"
    #undef pick
};

static inline_rtn  OptSpaceWinInlineTab[] = {
    #define pick(e,n,s1,s2,s3,s4,s5,s6)  #n, n##s4, TY_INTEGER, NULL, NULL,
    #include "_inline.h"
    #undef pick
};

static inline_rtn  OptTimeSmallModelInlineTab[] = {
    #define pick(e,n,s1,s2,s3,s4,s5,s6)  #n, n##s5, TY_INTEGER, NULL, NULL,
    #include "_inline.h"
    #undef pick
};

static inline_rtn  OptSpaceSmallModelInlineTab[] = {
    #define pick(e,n,s1,s2,s3,s4,s5,s6)  #n, n##s6, TY_INTEGER, NULL, NULL,
    #include "_inline.h"
    #undef pick
};
#endif

static inline_rtn  *InlineTab = OptTimeInlineTab;

static bool     CreatedPragmas = false;

#endif

void    InitInlinePragmas( void ) {
//===========================

#if _CPU == 386 || _CPU == 8086
    rtn_ids     i;

    if( !CreatedPragmas ) {
        if( OZOpts & OZOPT_O_SPACE ) {
            InlineTab = OptSpaceInlineTab;
#if _CPU == 8086
            if( CGOpts & CGOPT_M_MEDIUM ) {
                InlineTab = OptSpaceSmallModelInlineTab;
            } else {
                // using large/huge memory model
                if( CGOpts & CGOPT_WINDOWS ) {
                    InlineTab = OptSpaceWinInlineTab;
                }
            }
        } else {
            if( CGOpts & CGOPT_M_MEDIUM ) {
                InlineTab = OptTimeSmallModelInlineTab;
            } else {
                // using large/huge memory model
                if( CGOpts & CGOPT_WINDOWS ) {
                    InlineTab = OptTimeWinInlineTab;
                }
            }
#endif
        }
        for( i = 0; i < INLINETAB_SIZE; i++ ) {
            DoPragma( InlineTab[i].pragma );
        }
        CreatedPragmas = true;
    }
    for( i = 0; i < INLINETAB_SIZE; i++ ) {
        InlineTab[i].sym_ptr = NULL;
    }
#endif
}


call_handle     InitInlineCall( rtn_ids rtn_id )
//==============================================
// Initialize a call to a runtime routine.
{
#if _CPU == 386 || _CPU == 8086
    sym_id              sym;
    inline_rtn          *in_entry;
    size_t              name_len;

    if( !CreatedPragmas ) {
        InitInlinePragmas();
    }
    in_entry = &InlineTab[rtn_id];
    sym = in_entry->sym_ptr;
    if( sym == NULL ) {
        name_len = strlen( in_entry->name );
        sym = STAdd( in_entry->name, name_len );
        sym->u.ns.flags = SY_USAGE | SY_TYPE | SY_SUBPROGRAM | SY_FUNCTION;
        sym->u.ns.u1.s.typ = FT_INTEGER_TARG;
        sym->u.ns.xt.size = TypeSize( sym->u.ns.u1.s.typ );
        sym->u.ns.u3.address = NULL;
        in_entry->sym_ptr = sym;
        in_entry->info = InfoLookup( sym );
    }
    return( CGInitCall( CGFEName( sym, in_entry->typ ), in_entry->typ, in_entry->sym_ptr ) );
#else
    /* unused parameters */ (void)rtn_id;
    return( 0 );
#endif
}


void    FreeInlinePragmas( void ) {
//===========================

// Free symbol table entries for run-time routines.

#if _CPU == 386 || _CPU == 8086
    int         i;
    sym_id      sym;

    if( !CreatedPragmas )
        return;
    for( i = 0; i < INLINETAB_SIZE; i++ ) {
        sym = InlineTab[i].sym_ptr;
        if( sym != NULL ) {
            if( ( CGFlags & CG_FATAL ) == 0 ) {
                if( sym->u.ns.u3.address != NULL ) {
                    BEFreeBack( sym->u.ns.u3.address );
                }
            }
            STFree( sym );
            InlineTab[i].sym_ptr = NULL;
        }
    }
#endif
}
