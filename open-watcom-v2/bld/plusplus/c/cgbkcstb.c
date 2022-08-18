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


#include "plusplus.h"
#include "cgfront.h"
#include "cgback.h"
#include "memmgr.h"
#include "codegen.h"
#include "cgbackut.h"
#include "ring.h"
#include "initdefs.h"
#ifndef NDEBUG
#include "pragdefn.h"
#include "togglesd.h"
#endif


static carve_t carve_call_stab; // carve control: CALL_STAB

#ifndef NDEBUG

static void __dump( const char* text, CALL_STAB* cstb )
{
    if( TOGGLEDBG( dump_stab ) ) {
        printf( "CALL_STAB[%p] %s handle(%p) se(%p) has_cd_arg(%d) cd_arg(%x)\n"
              , cstb
              , text
              , cstb->call
              , cstb->se
              , cstb->has_cd_arg
              , cstb->cd_arg );
    }
}

#else

    #define __dump(a,b)

#endif


CALL_STAB* CallStabAlloc(       // ALLOCATE CALL_STAB
    call_handle call,           // - handle for call
    FN_CTL* fctl )              // - function hosting the call
{
    CALL_STAB* cstb;            // - call information

    cstb = RingCarveAlloc( carve_call_stab, &fctl->expr_calls );
    cstb->call = call;
    cstb->se = FstabMarkedPosn();
    cstb->has_cd_arg = false;
    cstb->cd_arg = 0;
    CgCdArgUsed( call );
    __dump( "allocate", cstb );
    return( cstb );
}


static CALL_STAB* callStabEntry(// GET CALL_STAB FOR A HANDLE
    call_handle call )          // - handle for call
{
    FN_CTL* fctl;               // - top function information
    CALL_STAB* curr;            // - call information (current)
    CALL_STAB* retn;            // - call information (returned)

    retn = NULL;
    if( 0 != call ) {
        fctl = FnCtlTop();
        RingIterBeg( fctl->expr_calls, curr ) {
            if( curr->call == call ) {
                retn = curr;
                break;
            }
        } RingIterEnd( curr );
    }
    return( retn );
}


bool CallStabCdArgGet(          // GET CD-ARG FOR A CALL
    call_handle call,           // - handle for call
    unsigned *a_cd_arg )        // - addr[ value for CD-ARG ]
{
    CALL_STAB* cstb;            // - call information
    bool ok;                    // - true ==> have CDTOR arg.

    cstb = callStabEntry( call );
    if( cstb != NULL && cstb->has_cd_arg ) {
        *a_cd_arg = cstb->cd_arg;
        ok = true;
    } else {
        ok = false;
    }
    return( ok );
}


unsigned CallStabCdArgSet(      // SET CD-ARG FOR A CALL
    call_handle call,           // - handle for call
    unsigned cd_arg )           // - value for CD-ARG
{
    CALL_STAB* cstb;            // - call information

    cstb = callStabEntry( call );
    if( cstb != NULL ) {
        cstb->has_cd_arg = true;
        cstb->cd_arg = cd_arg;
    }
    return( cd_arg );
}


void CallStabFree(              // FREE CALL_STAB
    FN_CTL* fctl,               // - function hosting the call
    CALL_STAB* cstb )           // - call information
{
    RingPrune( &fctl->expr_calls, cstb );
    CarveFree( carve_call_stab, cstb );
}


SE* CallStabStateTablePosn(     // GET STATE-TABLE POSITION AT CALL POINT
    call_handle call )          // - handle for inline call
{
    CALL_STAB* curr;            // - current CALL_STAB entry

    curr = callStabEntry( call );
    DbgVerify( curr, "CallStabStateTablePosn -- no active call" );
    __dump( "inline", curr );
    return( curr->se );
}


static void callStabInit(       // INITIALIZATION FOR CALL_STAB
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    carve_call_stab = CarveCreate( sizeof( CALL_STAB ), 16 );
}


static void callStabFini(       // COMPLETION FOR CALL_STAB
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    CarveDestroy( carve_call_stab );
}


INITDEFN( call_stab, callStabInit, callStabFini )
