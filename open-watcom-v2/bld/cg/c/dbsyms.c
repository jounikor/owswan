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
* Description:  Emit debug information for debugging locals.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "optmain.h"
#include "zoiks.h"
#include "cgaux.h"
#include "data.h"
#include "types.h"
#include "makeins.h"
#include "objout.h"
#include "dbsyms.h"
#ifndef NDEBUG
#include "echoapi.h"
#endif
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
#include "wvsyms.h"
#endif
#include "dw.h"
#include "dfsyms.h"
#include "cvdbg.h"
#include "cvsyms.h"
#include "namelist.h"
#include "makeblk.h"
#include "dbsupp.h"
#include "feprotos.h"
#include "cgprotos.h"


cue_ctl     LineInfo;
fname_ctl   DBFiles;

static const opcode_entry     DbgInfo[] = {
/*           op1   op2   res   eq      verify          reg           gen             fu  */
_OE( _None(                         ), V_NO,           RG_,          G_DEBUG,        FU_NO )
};

static  void    SrcFileNoInit( void )
/***********************************/
{
    DBFiles.lst = NULL;
    DBFiles.count = 0;
}

static  void    DBSrcFileFini( void )
/***********************************/
{
    fname_lst   *curr;

    while( (curr = DBFiles.lst) != NULL ) {
        DBFiles.lst = curr->next;
        CGFree( curr );
    }
}

uint    _CGAPI DBSrcFile( cchar_ptr fname )
/*****************************************/
{
    uint        index;
    size_t      len;
    fname_lst   *curr;
    fname_lst   **lnk;

#ifndef NDEBUG
    EchoAPI( "DBSrcFile( %c )", fname );
#endif
    len = strlen( fname ) + 1;
    index = 0;
    for( lnk = &DBFiles.lst; (curr = *lnk) != NULL; lnk = &curr->next ) {
        if( memcmp( fname, curr->fname, len ) == 0 ) {
#ifndef NDEBUG
            EchoAPI( " -> %i\n", index );
#endif
            return( index );
        }
        ++index;
    }
    curr = CGAlloc( sizeof( *curr ) - 1 + len );
    curr->len = len;
    curr->next = NULL;
    memcpy( curr->fname, fname, len );
    ++DBFiles.count;
    *lnk = curr;
#ifndef NDEBUG
    EchoAPI( " -> %i\n", index );
#endif
    return( index );
}

const char *SrcFNoFind( uint fno )
/********************************/
{
    uint        index;
    fname_lst   *curr;

    index = 0;
    for( curr  = DBFiles.lst; curr != NULL; curr = curr->next ) {
        if( index == fno ) {
            return( curr->fname );
        }
        ++index;
    }
    return( "unknown" );
}


static void  AddCueBlk( cue_ctl *ctl )
/************************************/
{
    cue_blk *new;

    new = CGAlloc( sizeof( *new ) );
    new->next = NULL;
    *ctl->lnk = new;
    ctl->curr = new;
    ctl->lnk = &new->next;
    ctl->next = &new->info[0];
    ctl->end = &new->info[CUES_PER_BLK];
}


static void  SourceCueInit( cue_ctl *ctl )
/****************************************/
{
    ctl->head = NULL;
    ctl->lnk  = &ctl->head;
    ctl->next = &ctl->start[0];
    ctl->end  = &ctl->start[1];
    ctl->state.cue = 0;
    ctl->state.fno = -1; /* force change */
    ctl->state.line = 0;
    ctl->state.col =  0;
    ctl->count = 0;
}

cue_idx CueAdd( int fno, int line, int col )
/******************************************/
{
    enum {
        EQUAL    = 0x00,
        LINE_NO  = 0x01,
        COL_NO   = 0x02,
        FNO_NO   = 0x04,
    } cmp;
    cue_ctl     *ctl;
    int         diff;

    if( fno == 0 && col == 1 && line < PRIMARY_RANGE ) {
        return( line );
    }
    ctl = &LineInfo;
    cmp = EQUAL;
    if( fno != ctl->state.fno ) {
        cmp |= FNO_NO;
    }
    if( col != ctl->state.col ) {
        cmp |= COL_NO;
    }
    diff = line - ctl->state.line;
    if( diff != 0 ) {
       cmp |= LINE_NO;
    }
    if( cmp == LINE_NO && 0 <= diff && diff < MAX_LINE_DELTA  ) {
        ctl->state.line = line;
        ctl->state.cue += diff;
        cmp = EQUAL;
    }
    if( cmp ) {
        if( ctl->next >= ctl->end ) {
            AddCueBlk( ctl );
        }
        ++ctl->state.cue;
        ctl->state.fno = fno;
        ctl->state.line = line;
        ctl->state.col = col;
        *ctl->next = ctl->state;
        ++ctl->next;
        ++ctl->count;
    }
    return( ctl->state.cue + PRIMARY_RANGE );
}

bool CueFind( cue_idx cue, cue_state *ret )
/*****************************************/
{
    cue_ctl     *ctl;
    cue_blk     *blk;
    cue_state   *hi;
    int         diff;

    ctl = &LineInfo;
    if( ctl->count == 0 || cue < PRIMARY_RANGE ) {
        ret->fno = 0;
        ret->line = cue;
        ret->col = 1;
        return( true );
    }
    cue -= PRIMARY_RANGE;
    if( cue < ctl->start[0].cue ) {
        Zoiks( ZOIKS_078 ); /* lower than low */
        return( false );
    }
    hi = &ctl->start[1];
    for( blk = ctl->head; blk != NULL; blk = blk->next ) {
        if( cue < blk->info[0].cue )
            break;
        hi = &blk->info[CUES_PER_BLK];
    }
    if( blk == NULL ) {
        /* if ctl->head == NULL then next is == ctl->start[1] */
        /* if cue in last blk next is end of entries in blk */
        hi = ctl->next;
        if( cue > ctl->state.cue ) { /* high than high */
            Zoiks( ZOIKS_078 );
            return( false );
        }
    }
    do {
        --hi;
    } while( ( diff = cue - hi->cue ) < 0 );
    *ret = *hi;
    ret->line += diff;
    return( true );
}

#if 0
static void CueLen( cue_ctl *ctl )
/********************************/
//Set map number to #cues with entry
{
    cue_ctl     *ctl;
    cue_blk     *blk;
    cue_state   *curr, *last;
    int         diff;

    if( ctl->count > 0 ) {
        last = &ctl->start[0];
    }
    for( blk = ctl->head; blk != NULL; blk = blk->next ) {
        if( blk->next == NULL ) {
            end = ctl->next;
        } else {
            end = &blk->info[CUES_PER_BLK];
        }
        for( curr = &blk->info[0]; curr != end; ++curr ) {
            last->map = curr->cue - last->cue;
        }
    }
    last->map = ctl->state.cue- last->cue;
}

// the only time we can do this is if
// the cue guy has seen all the cues and no cue numbers have been
// released
static cue_idx  CueFMap( cue_ctl *ctl, int fno, cue_idx map )
/***********************************************************/
//Map cues with same file together
{
    cue_ctl    *ctl;
    cue_blk    *blk;
    cue_state   *base, *curr, *last;
    cue_idx    map;
    cue_idx    len;

    if( ctl->count > 0 ) {
        curr = &ctl->start[0];
        if( curr->fno == fno ) {
            len =  curr->map;
            curr->map = map;
            map += len;
        }
    }
    for( blk = ctl->head; blk != NULL; blk = blk->next ) {
        if( blk->next == NULL ) {
            end = ctl->next;
        } else {
            end = &blk->info[CUES_PER_BLK];
        }
        for( curr = &blk->info[0]; curr != end; ++curr ) {
            if( curr->fno == fno ) {
                len =  curr->map;
                curr->map = map;
                map += len;
            }
        }
    }
    return( map );
}

void CueMap( cue_ctl *ctl, cue_state *base )
/******************************************/
//Add a map number so cues from the same file
//can be re-written on a continueum
{
    cue_ctl     *ctl;
    cue_blk     *blk;
    cue_state   *base, *curr, *last;
    int         fno;
    cue_idx     curr_idx;

    ctl = &LineInfo;
    CueLen( ctl ); /* add lengths */
    curr_idx = 0;
    for( fno = 0; fno <  DBFiles.count; ++fno ) {
        curr_idx = CueFMap( ctl, fno, curr_idx );
    }
    return( true );
}
#endif
#if 0
void DmpCue( cue_idx cue  )
{
    cue_state   state;
    const char  *fname;

    if( CueFind( cue,  &state ) ) {
        fname = SrcFNoFind( state.fno );
        printf( "out %s %d %d\n" , fname, state.line, state.col );
    } else {
        printf( "bad cue %d\n", cue );
    }
}
#endif
static void SourceCueFini( cue_ctl *ctl )
/***************************************/
{
    cue_blk *list;
    cue_blk *next;

    for( list = ctl->head; list != NULL; list = next ) {
        next = list->next;
        CGFree( list );
    }
    ctl->head = NULL;
}

void    InitDbgInfo( void )
/*************************/
{
//    cue_idx     idx;
    uint        fno;

    SrcFileNoInit();
    DBNested( false ); /* set nesting */
    SourceCueInit( &LineInfo );
    fno = DBSrcFile( FEAuxInfo( NULL, SOURCE_NAME ) );
//    idx = CueAdd( fno, 1, 1 );
    CueAdd( fno, 1, 1 );
    SrcLine = 1;
    if( _IsModel( DBG_DF ) ) {
        DFInitDbgInfo();
    } else if( _IsModel( DBG_CV ) ) {
        CVInitDbgInfo();
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    } else {
        WVInitDbgInfo();
#endif
    }
}


void    FiniDbgInfo( void )
/*************************/
{
    DBSrcFileFini();
    SourceCueFini( &LineInfo );
    if( _IsModel( DBG_DF ) ) {
        DFFiniDbgInfo();
    } else if( _IsModel( DBG_CV ) ) {
        CVFiniDbgInfo();
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    } else {
        WVFiniDbgInfo();
#endif
    }
}


void    _CGAPI DBLineNum( uint no )
/*********************************/
{
#ifndef NDEBUG
    EchoAPI( "\nDBLineNum( %i )\n", no );
#endif
    SrcLine = no;
}

void _CGAPI     DBSrcCue( uint fno, uint line, uint col )
/*******************************************************/
{
    cue_idx     idx;
    bool        hasxcue;
//    const char  *fname;

#ifndef NDEBUG
    EchoAPI( "\nDBsrcCue( %i, %i, %i )\n", fno, line, col );
#endif
//  fname = SrcFNoFind( fno );
//  printf( "in %s %d %d\n", fname, line, col );
    hasxcue =  _IsntModel( DBG_TYPES ); // Just OMF line nums
    if( hasxcue ) {
        if( fno == 0 && col == 1 ) {
            DBLineNum( line );
        }
    } else {
        idx = CueAdd( fno, line, col );
        SrcLine = idx;
    }
}

void _CGAPI DBGenStMem( cg_sym_handle sym, dbg_loc loc )
/******************************************************/
{
#ifndef NDEBUG
    EchoAPI( "DBGenStMem( %s,%i)\n", sym, loc );
#endif
    if( _IsModel( DBG_DF ) ) {
        DFGenStatic( sym, loc );
    } else if( _IsModel( DBG_CV ) ) {
        CVGenStatic( sym, loc, true );
    } else {
    }
}

static  void    AddLocal( dbg_local **owner, dbg_local *lcl )
/***********************************************************/
{
    dbg_local *curr;

    while( (curr = *owner) != NULL ) {
        owner = &curr->link;
    }
    lcl->link = NULL;
    *owner = lcl;
}

static  dbg_block *MkBlock( void )
/********************************/
{
    dbg_block   *blk;

    blk = CGAlloc( sizeof( dbg_block ) );
    blk->parent = CurrProc->targ.debug->blk;
    CurrProc->targ.debug->blk = blk;
    blk->locals = NULL;
    blk->patches = NULL;
    return( blk );
}


void _CGAPI DBGenSym( cg_sym_handle sym, dbg_loc loc, int scoped )
/****************************************************************/
{
    fe_attr     attr;
    dbg_local   *lcl;

#ifndef NDEBUG
    EchoAPI( "DBGenSym( %s, %i, %i )\n", sym, loc, scoped );
#endif
    if( _IsModel( DBG_LOCALS ) ) {
        attr = FEAttr( sym );
        if( (attr & FE_IMPORT) == 0 ) {
            if( attr & FE_PROC ) {
                CurrProc->state.attr |= ROUTINE_WANTS_DEBUGGING;
                CurrProc->targ.debug = CGAlloc( sizeof( dbg_rtn ) );
                memset( CurrProc->targ.debug, 0, sizeof( dbg_rtn ) );
                CurrProc->targ.debug->parms = NULL;
                CurrProc->targ.debug->reeturn = LocDupl( loc );
                CurrProc->targ.debug->obj_type = DBG_NIL_TYPE;
                CurrProc->targ.debug->obj_loc = NULL;
                CurrProc->targ.debug->rtn_blk = MkBlock();
            } else if( scoped ) {
                lcl = CGAlloc( sizeof( dbg_local ) );
                lcl->sym = sym;
                lcl->loc = LocDupl( loc );
                lcl->kind = DBG_SYM_VAR;
                AddLocal( &CurrProc->targ.debug->blk->locals, lcl );
            } else {
                if( _IsModel( DBG_DF ) ) {
                    DFGenStatic( sym, loc );
                } else if( _IsModel( DBG_CV ) ) {
                    CVGenStatic( sym, loc, false );
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
                } else {
                    WVGenStatic( sym , loc );
#endif
                }
            }
        }
    }
}


void    _CGAPI DBModSym( cg_sym_handle sym, cg_type indirect )
/************************************************************/
{
    fe_attr     attr;
    dbg_loc     loc;

    /* unused parameters */ (void)indirect;

#ifndef NDEBUG
    EchoAPI( "DBModSym( %s, %t )\n",  sym, indirect );
#endif
    if( _IsModel( DBG_LOCALS ) ) {
        attr = FEAttr( sym );
        if( (attr & FE_IMPORT) == 0 ) {
            if( attr & FE_PROC ) {
                loc = NULL;
            } else {
                loc = DBLocInit();
                loc = DBLocSym( loc, sym );
            }
            DBGenSym( sym, loc, false );
            DBLocFini( loc );
        }
    }
}


void _CGAPI DBObject( dbg_type tipe, dbg_loc loc, cg_type ptr_type )
/******************************************************************/
{
    /* unused parameters */ (void)ptr_type;

#ifndef NDEBUG
    EchoAPI( "DBObject( %i, %i, %t )\n", tipe, loc, ptr_type );
#endif
    CurrProc->targ.debug->obj_type = tipe;
    CurrProc->targ.debug->obj_loc = LocDupl( loc );
    if( _IsModel( DBG_DF ) ) {
        //
    } else if( _IsModel( DBG_CV ) ) {
        //
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    } else {
        WVObjectPtr( ptr_type );
#endif
    }
}



void    DBAllocReg( name *reg, name *temp )
/*****************************************/
{
    /* unused parameters */ (void)reg; (void)temp;
}

void _CGAPI DBTypeDef( cchar_ptr nm, dbg_type tipe )
/**************************************************/
{
#ifndef NDEBUG
    EchoAPI( "DBTypeDef( %c, %t )\n", nm, tipe );
#endif
    if( _IsModel( DBG_DF ) ) {
        DFTypedef( nm, tipe );
    } else if( _IsModel( DBG_CV ) ) {
        CVTypedef( nm, tipe );
    } else {
    }
}

void    _CGAPI DBLocalSym( cg_sym_handle sym, cg_type indirect )
/**************************************************************/
{
    fe_attr     attr;
    dbg_loc     loc;

    /* unused parameters */ (void)indirect;

#ifndef NDEBUG
    EchoAPI( "DBLocalSym( %s, %t )\n", sym, indirect );
#endif
    if( CurrProc->targ.debug != NULL ) {
        attr = FEAttr( sym );
        if( (attr & FE_IMPORT) == 0 ) {
            loc = DBLocInit();
            loc = DBLocSym( loc, sym );
            DBGenSym( sym, loc, true );
            DBLocFini( loc );
        }
    }
}

void    _CGAPI DBLocalType( cg_sym_handle sym, bool kind )
/********************************************************/
{
    dbg_local   *lcl;

#ifndef NDEBUG
    EchoAPI( "DBLocalType( %s, %i)\n", sym, kind );
#endif
    if( _IsModel( DBG_LOCALS ) ) {
        if( _IsModel( DBG_CV | DBG_DF ) ) {
            lcl = CGAlloc( sizeof( dbg_local ) );
            lcl->sym = sym;
            lcl->loc = NULL;
            if( kind ) {
                lcl->kind = DBG_SYM_TYPE;
            } else {
                lcl->kind = DBG_SYM_TYPEDEF;
            }
            AddLocal( &CurrProc->targ.debug->blk->locals, lcl );
        }
    }
}

static  void    AddBlockInfo( dbg_block *blk, bool start )
/********************************************************/
{
    instruction *ins;

    ins = MakeNop();
    ins->table = DbgInfo;
    ins->u.gen_table = ins->table;
    ins->flags.nop_flags = NOP_DBGINFO;
    if( start )
        ins->flags.nop_flags |= NOP_DBGINFO_START;
    ins->operands[0] = (name *)blk;
    AddIns( ins );
}


dbg_block *DoDBBegBlock( int fast_codegen )
/*****************************************/
{
    dbg_block   *blk;

    if( CurrProc->targ.debug == NULL )
        return( NULL );
    blk = MkBlock();
    if( !fast_codegen ) {
        /*%%%% stick a NOP in the instruction stream, point it at block*/
        AddBlockInfo( blk, true );
    }
    return( blk );
}

void _CGAPI     DBBegBlock( void )
/********************************/
{
#ifndef NDEBUG
    EchoAPI( "DBBegBlock()\n" );
#endif
    DoDBBegBlock( 0 );
}


void    DoDBEndBlock( int fast_codegen )
/**************************************/
{
    dbg_block   *blk;

#ifndef NDEBUG
    EchoAPI( "DBEndBlock()\n" );
#endif
    if( CurrProc->targ.debug != NULL ) {
        blk = CurrProc->targ.debug->blk;
        if( !fast_codegen ) {
            AddBlockInfo( blk, false );
        }
        CurrProc->targ.debug->blk = blk->parent;
    }
}

void _CGAPI     DBEndBlock( void )
/********************************/
{
    DoDBEndBlock( 0 );
}


void    DbgSetBase( void )
/************************/
{
    if( _IsModel( DBG_DF ) ) {
        /* nothing */
    } else if( _IsModel( DBG_CV ) ) {
        CVSetBase();
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    } else {
        WVSetBase();
#endif
    }
}


void    DbgParmLoc( name *parm, cg_sym_handle sym )
/*************************************************/
// sym is NULL if no front end sym
{
    dbg_local       *lcl;
    dbg_loc         loc;

    if( _IsntModel( DBG_DF ) ) {
        if( parm->n.class != N_REGISTER  ) {
            return;
        }
    }
    lcl = CGAlloc( sizeof( dbg_local ) );
    loc = DBLocInit();
    loc = LocParm( loc, parm );
    lcl->loc = loc;
    lcl->sym = sym;
    lcl->kind = DBG_SYM_VAR;
    AddLocal( &CurrProc->targ.debug->parms, lcl );
}


void    DbgRetLoc( void )
/***********************/
{
    dbg_loc loc;

    if( CurrProc->targ.debug->reeturn == NULL ) {
        loc = DBLocInit();
        loc = LocReg( loc, AllocRegName( CurrProc->state.return_reg ) );
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
        if( CurrProc->targ.return_points == NULL ) {
            /* nothing to do */
        } else if( CurrProc->state.attr & ROUTINE_ALLOCS_RETURN ) {
            loc->class = LOC_IND_REG + IND_RALLOC_NEAR;
        } else {
            loc->class = LOC_IND_REG + IND_CALLOC_NEAR;
        }
#endif
        CurrProc->targ.debug->reeturn = loc;
    }
}


/**/
/* Going into optimizer queue*/
/**/


void    DbgRetOffset( type_length offset )
/****************************************/
{
    CurrProc->targ.debug->ret_offset = offset;
}


static  void    EmitDbg( oc_class class, pointer ptr )
/****************************************************/
{
    any_oc      oc;

    oc.oc_debug.hdr.class = OC_INFO + class;
    oc.oc_debug.hdr.reclen = sizeof( oc_debug );
    oc.oc_debug.hdr.objlen = 0;
    oc.oc_debug.ptr = ptr;
    InputOC( &oc );
}


void    EmitRtnBeg( void )
/************************/
{
    EmitDbg( INFO_DBG_RTN_BEG, CurrProc->targ.debug );
}


void    EmitProEnd( void )
/************************/
{
    EmitDbg( INFO_DBG_PRO_END, CurrProc->targ.debug );
}


void    EmitDbgInfo( instruction *ins )
/*************************************/
{
    if( ins->flags.nop_flags & NOP_DBGINFO_START ) {
        EmitDbg( INFO_DBG_BLK_BEG, ins->operands[0] );
    } else {
        EmitDbg( INFO_DBG_BLK_END, ins->operands[0] );
    }
}


void    EmitEpiBeg( void )
/************************/
{
    EmitDbg( INFO_DBG_EPI_BEG, CurrProc->targ.debug );
}


void    EmitRtnEnd( void )
/************************/
{
    segment_id      old_segid;

    EmitDbg( INFO_DBG_RTN_END, CurrProc->targ.debug );
    old_segid = SetOP( AskCodeSeg() );
    EmptyQueue();
    SetOP( old_segid );
}


/**/
/* Coming out of optimizer queue*/
/**/


void    DbgRtnBeg( dbg_rtn *rtn,  offset lc )
/*******************************************/
{
    rtn->rtn_blk->start = lc;
    if( _IsModel( DBG_CV ) ) {
        CVRtnBeg( rtn, lc );
    }
}


void    DbgProEnd( dbg_rtn *rtn, offset lc )
/******************************************/
{
    rtn->pro_size = lc - rtn->rtn_blk->start;
    if( _IsModel( DBG_DF ) ) {
        DFProEnd( rtn, lc );
    } else if( _IsModel( DBG_CV ) ) {
        CVProEnd( rtn, lc );
    }
}


void    DbgBlkBeg( dbg_block *blk, offset lc )
/********************************************/
{
    blk->start = lc;
    if( _IsModel( DBG_DF ) ) {
        DFBlkBeg( blk, lc );
    } else if( _IsModel( DBG_CV ) ) {
        CVBlkBeg( blk, lc );
    }
}

void    DbgBlkEnd( dbg_block *blk, offset lc )
/********************************************/
{
    if( _IsModel( DBG_DF ) ) {
        DFBlkEnd( blk, lc );
    } else if( _IsModel( DBG_CV ) ) {
        CVBlkEnd( blk, lc );
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    } else {
        WVBlkEnd( blk, lc );
#endif
    }
    CGFree( blk );
}


void    DbgEpiBeg( dbg_rtn *rtn, offset lc )
/******************************************/
{
    rtn->epi_start = lc;
    if( _IsModel( DBG_DF ) ) {
        DFEpiBeg( rtn, lc );
    } else if( _IsModel( DBG_CV ) ) {
        CVEpiBeg( rtn, lc );
    }
}


void    DbgRtnEnd( dbg_rtn *rtn, offset lc )
/******************************************/
{
    if( _IsModel( DBG_DF ) ) {
        DFRtnEnd( rtn, lc );
    } else if( _IsModel( DBG_CV ) ) {
        CVRtnEnd( rtn, lc );
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    } else {
        WVRtnEnd( rtn, lc );
#endif
    }
    CGFree( rtn->blk );
    CGFree( rtn );
}
