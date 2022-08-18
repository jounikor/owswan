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


#include "plusplus.h"
#include "cgfront.h"
#include "cgback.h"
#include "codegen.h"
#include "pragdefn.h"
#include "feprotos.h"
#include "segment.h"
#include "initdefs.h"
#include "dbgsupp.h"

SYMBOL DefaultCodeSymbol;
SYMBOL DefaultDataSymbol;

void DbgSuppSegRequest( void )
/****************************/
{
    if( GenSwitches & ( DBG_TYPES | DBG_LOCALS | DBG_DF ) ) {
        SegmentMarkUsed( SEG_CODE );
        SegmentMarkUsed( SEG_BSS );
    }
}

void DbgSuppInit( dsi_control control )
/*************************************/
{
    TYPE code_type;
    TYPE data_type;
    SYMBOL code_sym;
    SYMBOL data_sym;
    NAME code_name;
    NAME data_name;
    segment_id old_segid;

    code_sym = DefaultCodeSymbol;
    if( code_sym == NULL ) {
        code_type = MakeSimpleFunction( GetBasicType( TYP_VOID ), NULL );
        code_sym = SymMakeDummy( code_type, &code_name );
        code_sym->id = SYMC_STATIC;
        code_sym->flag |= SYMF_INITIALIZED;
        code_sym->segid = SEG_CODE;
        code_sym = InsertSymbol( GetFileScope(), code_sym, code_name );
        DefaultCodeSymbol = code_sym;
    }
    data_sym = DefaultDataSymbol;
    if( data_sym == NULL ) {
        data_type = GetBasicType( TYP_CHAR );
        data_sym = SymMakeDummy( data_type, &data_name );
        data_sym->id = SYMC_STATIC;
        data_sym->flag |= SYMF_INITIALIZED;
        data_sym->segid = SEG_BSS;
        data_sym = InsertSymbol( GetFileScope(), data_sym, data_name );
        DefaultDataSymbol = data_sym;
    }
    if(( control & DSI_ONLY_SYMS ) == 0 ) {
        old_segid = BESetSeg( code_sym->segid );
        CgBackGenLabel( code_sym );
        BESetSeg( data_sym->segid );
        CgBackGenLabel( data_sym );
        BESetSeg( old_segid );
    }
}

void DbgAddrTaken( SYMBOL sym )
/*****************************/
{
    if( sym != NULL ) {
#if 0
        sym->flag |= SYMF_ADDR_TAKEN;
#else
        if( sym->flag & SYMF_INITIALIZED ) {
            sym->flag |= SYMF_ADDR_TAKEN;
        } else if(( sym->flag & SYMF_ADDR_TAKEN ) == 0 ) {
            // first time SYMF_ADDR_TAKEN will be set
            sym->flag |= SYMF_DBG_ADDR_TAKEN | SYMF_ADDR_TAKEN;
        }
#endif
    }
}

static void init(               // INITIALIZATION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    DefaultCodeSymbol = NULL;
    DefaultDataSymbol = NULL;
}

INITDEFN( dbg_supp, init, InitFiniStub );
