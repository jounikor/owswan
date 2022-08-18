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


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "zoiks.h"
#include "cgauxinf.h"
#include "wvdbg.h"
#include "data.h"
#include "types.h"
#include "objout.h"
#include "wvsyms.h"
#include "objio.h"
#include "x86dbsup.h"
#include "dbsupp.h"
#include "wvtypes.h"
#include "x86objd.h"
#include "x86obj.h"
#include "wvsupp.h"
#include "feprotos.h"
#include "cgprotos.h"


static  offset          CodeOffset;

static  void    DumpDbgBlkStart( dbg_block *blk, offset lc ) {
/************************************************************/

    block_patch *bpatch;
    offset      off;
    segment_id  old_segid;

    old_segid = AskOP();
    while( blk->patches != NULL ) {
        bpatch = blk->patches;
        blk->patches = bpatch->link;
        SetOP( bpatch->patch.segid );
        off = AskLocation();
        SetLocation( bpatch->patch.offset );
        DataShort( off );
        SetLocation( off );
        CGFree( bpatch );
    }
    SetOP( old_segid );
    BuffOffset( blk->start - CodeOffset );
    BuffOffset( lc - blk->start );
}


static  void    DumpParentPtr( dbg_block *blk ) {
/***********************************************/

    block_patch *bpatch;

    blk = blk->parent;
    if( blk == NULL ) {
        BuffWord( 0 );
   } else {
        bpatch = CGAlloc( sizeof( block_patch ) );
        bpatch->link = blk->patches;
        blk->patches = bpatch;
        BuffForward( &bpatch->patch );
    }
}

static  void    DumpDbgBlk( dbg_block *blk, offset lc ) {
/*******************************************************/

    DumpDbgBlkStart( blk, lc );
    DumpParentPtr( blk );
}

static  void    DumpLocals( dbg_local *local ) {
/*************************************************/


    dbg_local   *junk;
    temp_buff   temp;
    dbg_type    tipe;

    while( local != NULL ) {
        tipe = FEDbgType( local->sym );
        BuffStart( &temp, SYM_VARIABLE + VAR_LOCAL );
        LocDump( local->loc );
        BuffIndex( (uint) tipe );
        BuffWSLString( FEName( local->sym ) );
        BuffEnd( DbgLocals );
        junk = local;
        local = local->link;
        CGFree( junk );
    }
}

void    WVInitDbgInfo( void )
/***************************/
{
    TypeIdx   = 0;
    DbgLocals = 0;
    DbgTypes  = 0;
}

void    WVObjInitDbgInfo( void )
/******************************/
// Called right after define seg's in Obj
{
    WVSrcCueLoc();
}

void    WVFiniDbgInfo( void )
/***************************/
{
}

void    WVObjFiniDbgInfo( void )
/******************************/
{
}

void    WVGenStatic( cg_sym_handle sym, dbg_loc loc )
/***************************************************/
{
    temp_buff   temp;
    dbg_type    tipe;

    tipe = FEDbgType( sym );
    if( LocSimpStatic( loc ) == sym ) {
        BuffStart( &temp, SYM_VARIABLE + VAR_MODULE );
        BuffAddr( sym );
    } else {
        BuffStart( &temp, SYM_VARIABLE + VAR_MODULE_LOC );
        LocDump( LocDupl( loc ) );
    }
    BuffIndex( (uint) tipe );
    BuffWSLString( FEName( sym ) );
    BuffEnd( DbgLocals );
}

void    WVSetBase( void )
/***********************/
{
    temp_buff   temp;
    back_handle bck;

    if( _IsModel( DBG_LOCALS ) && NeedBaseSet() ) {
        bck = BENewBack( NULL );
        bck->segid = AskOP();
        OutLabel( bck->lbl );
        CodeOffset = AskAddress( bck->lbl );
        BuffStart( &temp, NEW_BASE+SET_BASE );
        BuffBack( bck, 0 );
        BuffEnd( DbgLocals );
        BEFreeBack( bck );
        SetUpObj( false );
    }
}

void    WVObjectPtr(  cg_type ptr_type )
/**************************************/
{
    switch( TypeAddress( ptr_type )->refno ) {
    case TY_NEAR_POINTER:
    case TY_NEAR_CODE_PTR:
        CurrProc->targ.debug->obj_ptr_type = POINTER_NEAR;
        break;
    default:
        CurrProc->targ.debug->obj_ptr_type = POINTER_FAR;
        break;
    }
}

/**/
/* Going into optimizer queue*/
/**/



/**/
/* Coming out of optimizer queue*/
/**/



void    WVBlkEnd( dbg_block *blk, offset lc )
/*******************************************/
{
    temp_buff   temp;

    BuffStart( &temp, SYM_CODE + CODE_BLOCK );
    DumpDbgBlk( blk, lc );
    BuffEnd( DbgLocals );
    DumpLocals( blk->locals );
}


void    WVRtnEnd( dbg_rtn *rtn, offset lc )
/*****************************************/
{
    uint                count;
    dbg_local           *parm;
    dbg_local           *junk;
    temp_buff           temp;
    cg_sym_handle       sym;
    dbg_type            tipe;
    offset              off;
    segment_id          old_segid;

    off = 0;
    if( rtn->obj_type != DBG_NIL_TYPE ) {
        /* is a member function */
        old_segid = SetOP( DbgLocals );
        off = AskLocation();
        BuffStart( &temp, SYM_CODE + CODE_MEMBER_SCOPE );
        DumpParentPtr( rtn->blk );
        BuffIndex( (uint) rtn->obj_type );
        if( rtn->obj_loc != NULL ) {
            BuffByte( rtn->obj_ptr_type ); /* 'this' pointer type */
            LocDump( rtn->obj_loc );
        }
        BuffEnd( DbgLocals );
        SetOP( old_segid );
    }
    sym = AskForLblSym( CurrProc->label );
    tipe = FEDbgType( sym );
    if( *(call_class *)FindAuxInfoSym( sym, CALL_CLASS ) & FAR_CALL ) {
        BuffStart( &temp, SYM_CODE + CODE_FAR_RTN );
    } else {
        BuffStart( &temp, SYM_CODE + CODE_NEAR_RTN );
    }
    DumpDbgBlkStart( rtn->blk, lc );
    if( off != 0 ) {
        BuffWord( off );
    } else {
        DumpParentPtr( rtn->blk );
    }
    BuffByte( rtn->pro_size );
    BuffByte( lc - rtn->epi_start );
    BuffOffset( rtn->ret_offset );
    BuffIndex( (uint) tipe );
    if( rtn->reeturn == NULL ) {
        BuffByte( 0 ); /* no return location */
    } else {
        LocDump( rtn->reeturn );
    }
    count = 0;
    parm = rtn->parms;
    while( parm != NULL ) {
        ++count;
        parm = parm->link;
    }
    BuffByte( count );
    parm = rtn->parms;
    while( parm != NULL ) {
        LocDump( parm->loc );
        junk = parm;
        parm = parm->link;
        CGFree( junk );
    }
    BuffWSLString( FEName( AskForLblSym( CurrProc->label ) ) );
    BuffEnd( DbgLocals );
    DumpLocals( rtn->blk->locals );
}
