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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <drgetref.h>
#include "module.h"
#include "brmem.h"
#include "funcsym.h"

const int POOLSIZE = 32;

#pragma disable_message( 549 )      // sizeof contains compiler genned info.
MemoryPool FunctionSym::_pool( sizeof( FunctionSym ), "FunctionSym", POOLSIZE );
#pragma enable_message( 549 )

struct FuncSearchData {
    FunctionSym *   me;
    WVList *        list;
};

void * FunctionSym::operator new( size_t )
//----------------------------------------
{
    return( _pool.alloc() );
}

void FunctionSym::operator delete( void * mem )
//---------------------------------------------
{
    _pool.free( mem );
}

static bool FunctionSym::memberHook( dr_sym_type symtype, drmem_hdl drhdl,
                                char * name, drmem_hdl drhdl_prt, void * info )
//---------------------------------------------------------------------------
{
    FuncSearchData * data = ( FuncSearchData * ) info;
    Symbol *         sym;

    sym = defineSymbol( symtype, drhdl, drhdl_prt, data->me->getModule(), name );
    data->list->add( sym );

    return true;
}


void FunctionSym::localVars( WVList & list )
//------------------------------------------
{
    FuncSearchData data;
    data.me = this;
    data.list = &list;
    DRKidsSearch( getHandle(), DR_SEARCH_VARIABLES, &data, memberHook );
}

struct callSearchData {
    FunctionSym * me;
    WVList *      list;
};

void FunctionSym::callees( WVList & list )
//----------------------------------------
{
    callSearchData data;

    data.me = this;
    data.list = &list;

    DRRefersTo( getHandle(), &data, callHook );
}

void FunctionSym::callers( WVList & list )
//----------------------------------------
{
    int i;

    getModule()->findRefSyms( &list, this );

    for( i = 0; i < list.count(); i += 1 ) {
        Symbol * sym = (Symbol *) list[ i ];
        if( sym->symtype() != DR_SYM_FUNCTION ) {
            list.removeAt( i );
        }
    }
}

static bool FunctionSym::callHook( drmem_hdl, dr_ref_info * ref, char * name,
                                   void * info )
//---------------------------------------------------------------------------
{
    Symbol *         sym;
    callSearchData * data = (callSearchData *) info;
    drmem_hdl        other;
    dr_sym_type      stype;

    other = ref->dependent;
    stype = DRGetSymType( other );
    if( stype == DR_SYM_FUNCTION ) {
        sym = Symbol::defineSymbol(stype, other, DRMEM_HDL_NULL, data->me->getModule(),name);
        data->list->add( sym );
    } else {
        WBRFree( name );
    }
    return true;    // continue
}
