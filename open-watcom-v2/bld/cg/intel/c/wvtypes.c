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
#include "wvdbg.h"
#include "data.h"
#include "types.h"
#include "objout.h"
#include "wvtypes.h"
#include "dbsyms.h"
#include "x86dbsup.h"
#include "dbsupp.h"
#include "wvsupp.h"


#define MAX_TYPE_SIZE   (1024 * 16)

static  void            NewType( temp_buff *temp, uint ty_def );
static  void            EndType( bool check_too_big );

static dbg_patch        CueInfoOffset;

static  byte    GetScalar( cg_type tipe ) {
/*****************************************/

    byte        scalar;
    type_def    *tipe_addr;

    tipe_addr = TypeAddress( tipe );
    if( tipe_addr->refno == TY_DEFAULT ) {
        return( SCALAR_VOID );
    }
    scalar = tipe_addr->length - 1;
    if( tipe_addr->attr & TYPE_FLOAT ) {
        scalar |= SCALAR_FLOAT;
    } else if( tipe_addr->attr & TYPE_SIGNED ) {
        scalar |= SCALAR_INT;
    } else {
        scalar |= SCALAR_UNSIGNED;
    }
    return( scalar );
}


static  uint    SignedSizeClass( signed_32 num ) {
/************************************************/

    uint        class;

    if( num >= -128 && num <= 127 ) {
        class = 0;
    } else if( num >= -32768 && num <= 32767 ) {
        class = 1;
    } else {
        class = 2;
    }
    return( class );
}

static  uint    SignedSizeClass64( signed_64 val ) {
/************************************************/

    uint        class;

    if( val.u._32[I64HI32] == 0 || val.u._32[I64HI32] == -1 ){
        class = SignedSizeClass( val.u._32[I64LO32] );
    }else{
        class = 3;
    }
    return( class );
}

void    WVSrcCueLoc( void  )
/**************************/
// Leave a dword to be back patched with the offset of line info
{
    temp_buff   temp;

    BuffStart( &temp, WT_NAME + NAME_CUEINFO );
    BuffForward( &CueInfoOffset ); /* does a  0 word */
    BuffWord( 0 ); /* another 0 word */
    EndType( false );
}

void    WVTypesEof( void )
/************************/
// Leave Eof indicator for types
{
    temp_buff   temp;

    BuffStart( &temp, WT_NAME + NAME_EOF );
    EndType( false );
}

dbg_type        WVFtnType( const char *name, dbg_ftn_type tipe )
/**************************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_NAME + NAME_SCALAR );
    BuffByte( tipe );
    BuffWSLString( name );
    EndType( true );
    return( TypeIdx );
}


dbg_type        WVScalar( const char *name, cg_type tipe )
/********************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_NAME + NAME_SCALAR );
    BuffByte( GetScalar( tipe ) );
    BuffWSLString( name );
    EndType( true );
    return( TypeIdx );
}


dbg_type        WVScope( const char *name )
/*****************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_NAME + NAME_SCOPE );
    BuffWSLString( name );
    EndType( true );
    return( TypeIdx );
}


void    WVDumpName( dbg_name name, dbg_type tipe )
/************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_NAME + NAME_NAME );
    name->refno = TypeIdx;
    if( _IsModel( DBG_TYPES ) ) {
        BuffIndex( name->scope );
        if( tipe == DBG_FWD_TYPE ) {
            BuffForward( &name->patch );
        } else {
            BuffIndex( tipe );
        }
        BuffString( name->len, name->name );
    }
    EndType( true );
}

void WVBackRefType( dbg_name name, dbg_type tipe )
/************************************************/
{
    offset      here;
    segment_id  old_segid;

    old_segid = SetOP( name->patch.segid );
    here = AskLocation();
    SetLocation( name->patch.offset );
    DataShort( 0x80 | (tipe >> 8) | (tipe << 8) );
    SetLocation( here );
    SetOP( old_segid );
}

dbg_type        WVCharBlock( unsigned_32 len )
/********************************************/
{
    temp_buff   temp;
    int         class;

    class = SignedSizeClass( len );
    NewType( &temp, WT_CHAR_BLOCK + NAME_CHAR_BYTE + class );
    BuffValue( len, class );
    EndType( true );
    return( TypeIdx );
}

dbg_type    WVIndCharBlock( back_handle len, cg_type len_type, int off )
/**********************************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_CHAR_BLOCK + NAME_CHAR_IND );
    BuffByte( GetScalar( len_type ) );
    BuffBack( len, off );
    EndType( true );
    return( TypeIdx );
}

dbg_type        WVLocCharBlock( dbg_loc loc, cg_type len_type )
/*************************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_CHAR_BLOCK + NAME_CHAR_LOC );
    BuffByte( GetScalar( len_type ) );
    LocDump( LocDupl( loc ) );
    EndType( true );
    return( TypeIdx );
}


dbg_type    WVFtnArray( back_handle dims, cg_type lo_bound_tipe, cg_type num_elts_tipe, int off, dbg_type base )
/**************************************************************************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_ARRAY + FORTRAN_TYPE );
    BuffByte( GetScalar( lo_bound_tipe ) );
    BuffByte( GetScalar( num_elts_tipe ) );
    BuffBack( dims, off );
    BuffIndex( base );
    EndType( true );
    return( TypeIdx );
}


dbg_type        WVArray( dbg_type idx, dbg_type base )
/****************************************************/
{
    temp_buff   temp;

    NewType( &temp, WT_ARRAY + ARRAY_TYPE );
    BuffIndex( idx );
    BuffIndex( base );
    EndType( true );
    return( TypeIdx );
}


dbg_type        WVIntArray( unsigned_32 hi, dbg_type base )
/*********************************************************/
{
    uint        class;
    temp_buff   temp;

    class = SignedSizeClass( hi );
    NewType( &temp, WT_ARRAY + ARRAY_BYTE + class );
    BuffValue( hi, class );
    BuffIndex( base );
    EndType( true );
    return( TypeIdx );
}

dbg_type    WVSubRange( signed_32 lo, signed_32 hi, dbg_type base )
/*****************************************************************/
{
    uint        class_lo;
    uint        class_hi;
    temp_buff   temp;

    class_lo = SignedSizeClass( lo );
    class_hi = SignedSizeClass( hi );
    if( class_lo > class_hi ) {
       class_hi = class_lo;
    }
    NewType( &temp, WT_SUBRANGE + RANGE_BYTE + class_hi );
    BuffValue( lo, class_hi );
    BuffValue( hi, class_hi );
    BuffIndex( base );
    EndType( true );
    return( TypeIdx );
}

#if 0
static  void    ReverseDims( dbg_array ar )
/*****************************************/
{
    dim_any   *curr;
    dim_entry *next;
    dim_entry *head;

    curr = ar->list;
    head = NULL;
    while( curr != NULL ) {
        next = curr->entry.next;
        curr->entry.next = NULL;
        curr->entry.next = head;
        head = curr;
        curr = next;
    }
    ar->list = head;
}
#endif

dbg_type    WVEndArray( dbg_array ar )
/************************************/
{
    dim_any   *dim;
    dbg_type  ret = 0;
    dbg_type  sub;

//  ReverseDims( ar );
    for(;;) {
        dim = ar->list;
        if( dim == NULL )
            break;
        switch( dim->entry.kind ) {
        case DIM_CON:
            sub = WVSubRange( dim->con.lo, dim->con.hi, dim->con.idx );
            ret = WVArray( sub, ar->base );
            ar->base = ret;
            break;
        case DIM_VAR:
            ret = WVFtnArray( dim->var.dims, dim->var.lo_bound_tipe,
                     dim->var.num_elts_tipe, dim->var.off, ar->base );
            ar->base = ret;
            break;

        }
        ar->list = dim->entry.next;
        CGFree( dim  );
    }
    return( ret );
}

static  dbg_type        DbgPtr( cg_type ptr_type, dbg_type base, int adjust,
                                dbg_loc loc_segment ) {
/***************************************************************************/

    temp_buff   temp;

    switch( TypeAddress( ptr_type )->refno ) {
    case TY_NEAR_POINTER:
    case TY_NEAR_CODE_PTR:
        NewType( &temp, WT_POINTER + POINTER_NEAR + adjust );
        break;
    default:
        NewType( &temp, WT_POINTER + POINTER_FAR + adjust );
    }
    BuffIndex( base );
    if( loc_segment != NULL ) {
        LocDump( LocDupl( loc_segment ) );
    }
    EndType( true );
    return( TypeIdx );
}


dbg_type        WVDereference( cg_type ptr_type, dbg_type base )
/**************************************************************/
{
    return( DbgPtr( ptr_type, base, DEREF_NEAR - POINTER_NEAR, NULL ) );
}

dbg_type        WVPtr( cg_type ptr_type, dbg_type base )
/******************************************************/
{
    return( DbgPtr( ptr_type, base, 0, NULL ) );
}

dbg_type    WVBasedPtr( cg_type ptr_type, dbg_type base, dbg_loc loc_segment )
/****************************************************************************/
{
    return( DbgPtr( ptr_type, base, 0, loc_segment ) );
}

static  void    AddField( field_any **owner, field_any *field  ){
/*** Sort according to WV(brian)***********************************/

    field_any     *curr;
    unsigned      strt;
    offset        off;

    strt = field->member.b_strt;
    off  = field->member.u.off;
    for(;;) {
        curr = *owner;
        if( curr == NULL )
            break;
        if( curr->member.entry.field_type == FIELD_OFFSET ) {
            if( (off == curr->member.u.off) && (strt >= curr->member.b_strt) )
                break;
            if( off >= curr->member.u.off ) {
                break;
            }
        }
        owner = &curr->member.entry.next;
    }
    field->entry.next = curr;
    *owner = field;
}

static  void    SortFields( dbg_struct st )
/*****************************************/
{
    field_any   *curr;
    field_any   *next;
    field_any   *head;

    curr = st->list;
    head = NULL;
    while( curr != NULL ) {
        next = curr->entry.next;
        curr->entry.next = NULL;
        if( curr->entry.field_type == FIELD_OFFSET ){
            AddField(  &head, curr );
        }else{
            curr->entry.next = head;
            head = curr;
        }
        curr = next;
    }
    st->list = head;
}


dbg_type        WVEndStruct( dbg_struct st )
/******************************************/
{
    field_any   *field;
    uint        class;
    temp_buff   temp;

    NewType( &temp, WT_STRUCTURE + STRUCT_LIST );
    BuffWord( st->num );
    BuffDWord( st->size );
    EndType( true );
    SortFields( st );
    for(;;) {
        field = st->list;
        if( field == NULL )
            break;
        switch( field->entry.field_type ) {
        case FIELD_INHERIT:
            BuffStart( &temp, WT_STRUCTURE + STRUCT_INHERIT );
            LocDump( field->bclass.u.adjustor );
            BuffIndex( field->bclass.base );
            break;
        case FIELD_LOC:
            if( field->member.b_len == 0 ) {
                BuffStart( &temp, WT_STRUCTURE + STRUCT_F_LOC );
                BuffByte( field->member.attr );
                LocDump( field->member.u.loc );
            } else {
                BuffStart( &temp, WT_STRUCTURE + STRUCT_BF_LOC );
                BuffByte( field->member.attr );
                LocDump( field->member.u.loc );
                BuffByte( field->member.b_strt );
                BuffByte( field->member.b_len );
            }
            BuffIndex( field->member.base );
            BuffString( field->member.len, field->member.name );
            break;
        case FIELD_OFFSET:
            if( field->member.u.off <= 0x00ff ) {
                class = 0;
            } else if( field->member.u.off <= 0xffff ) {
                class = 1;
            } else {
                class = 2;
            }
            if( field->member.b_len == 0 ) {
                BuffStart( &temp, WT_STRUCTURE + STRUCT_F_BYTE + class );
                BuffValue( field->member.u.off, class );
            } else {
                BuffStart( &temp, WT_STRUCTURE + STRUCT_BF_BYTE + class );
                BuffValue( field->member.u.off, class );
                BuffByte( field->member.b_strt );
                BuffByte( field->member.b_len );
            }
            BuffIndex( field->member.base );
            BuffString( field->member.len, field->member.name );
            break;
        case FIELD_METHOD:
            break;
        case FIELD_NESTED:
            break;
        case FIELD_VFUNC:
            break;
        default:
            break;
        }
        EndType( false );
        st->list = field->entry.next;
        CGFree( field  );
    }
    return( TypeIdx );
}


dbg_type        WVEndEnum( dbg_enum en )
/**************************************/
{
    const_entry *cons;
    uint        class;
    temp_buff   temp;
    signed_64   val;

    NewType( &temp, WT_ENUMERATED + ENUM_LIST );
    BuffWord( en->num );
    BuffByte( GetScalar( en->tipe ) );
    EndType( true );
    for(;;) {
        cons = en->list;
        if( cons == NULL )
            break;
        val = cons->val;
        class = SignedSizeClass64( val );
        BuffStart( &temp, WT_ENUMERATED + ENUM_BYTE + class );
        if( class == 3 ){
            BuffValue( val.u._32[I64LO32], 2 );
            BuffValue( val.u._32[I64HI32], 2 );
        }else{
            BuffValue( val.u._32[I64LO32], class );
        }
        BuffString( cons->len, cons->name );
        EndType( false );
        en->list = cons->next;
        CGFree( cons );
    }
    return( TypeIdx );
}


dbg_type        WVEndProc( dbg_proc pr )
/**************************************/
{
    parm_entry  *parm;
    temp_buff   temp;
    dbg_type    proc_type;

    if( pr->call == TY_NEAR_CODE_PTR ) {
        NewType( &temp, WT_PROCEDURE + PROC_NEAR );
    } else {
        NewType( &temp, WT_PROCEDURE + PROC_FAR );
    }
    proc_type = TypeIdx;
    BuffIndex( pr->ret );
    BuffByte( pr->num );
    for(;;) {
        parm = pr->list;
        if( parm == NULL )
            break;
        if( BuffLoc() > DB_BUFF_SIZE - 4 ) {
            /* record is getting too big - split it up */
            EndType( true );
            NewType( &temp, WT_PROCEDURE + PROC_EXT_PARM_LIST );
        }
        BuffIndex( parm->tipe );
        pr->list = parm->next;
        CGFree( parm );
    }
    EndType( false );
    return( proc_type );
}



static  void    NewType( temp_buff *temp, uint ty_def ) {
/*******************************************************/

    ++TypeIdx;
    BuffStart( temp, ty_def );
}


static  void    EndType( bool check_too_big ) {
/*********************************************/


    if( _IsModel( DBG_TYPES ) ) {
        if( check_too_big )
            ChkDbgSegSize( MAX_TYPE_SIZE, true );
        BuffEnd( DbgTypes );
    }
}

static void DmpFileInfo( void ){
/*******************************/
    fname_lst *lst;
    unsigned_16 index;

    DataShort( DBFiles.count );
    index = 0;
    for( lst = DBFiles.lst; lst != NULL; lst = lst->next ){
        DataShort( index );
        index += lst->len;
    }
    for( lst = DBFiles.lst; lst != NULL; lst = lst->next ) {
        DataBytes( lst->len, lst->fname );
    }
}

void WVDmpCueInfo( long_offset where )
/************************************/
// Assume here is offset from first dbgtype segment to here
// and we are in our segement for writing
{
    cue_ctl     *ctl;
    cue_blk     *blk;
    cue_state   *curr;
    cue_state   *end;
    segment_id  old_segid;
    offset      here;


    old_segid = SetOP( CueInfoOffset.segid );
    here = AskLocation();
    SetLocation( CueInfoOffset.offset );
    DataLong( where );  // current location in DbgTypes segment
    SetLocation( here );
    SetOP( old_segid );
    ctl = &LineInfo;
    blk = ctl->head;
    DataShort( ctl->count );  // number of entries
    if( ctl->count > 0 ){
        curr = &ctl->start[0];
        DataShort( curr->cue );
        DataShort( curr->fno );
        DataShort( curr->line );
        DataShort( curr->col );
    }
    while( blk != NULL ){
        curr = &blk->info[0];
        if( blk->next == NULL ){
            end = ctl->next;
        }else{
            end = &blk->info[CUES_PER_BLK];
        }
        while( curr != end ){
            DataShort( curr->cue );
            DataShort( curr->fno );
            DataShort( curr->line );
            DataShort( curr->col );
            ++curr;
        }
        blk = blk->next;
    }
    DmpFileInfo();
}
