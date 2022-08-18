/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2016 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Common code detection and optimization.
*
****************************************************************************/


#include "_cgstd.h"
#include "optwif.h"
#include "utils.h"
#include "inslist.h"
#include "block.h"
#include "data.h"
#include "optmain.h"
#include "encode.h"
#include "optutil.h"
#include "optmkins.h"
#include "optcom.h"
#include "optins.h"
#include "optpull.h"


typedef struct common_info {
        ins_entry   *start_com;
        ins_entry   *start_del;
        uint        save;
} common_info;

static  bool    JustMoveLabel( common_info *max, ins_entry *ins )
/***************************************************************/
{
    oc_class            cl;
    ins_entry           *lbl;
    ins_entry           *add;
    ins_entry           *next;

  optbegin
    if( PrevClass( max->start_del ) != OC_LABEL )
        optreturn( false );
    lbl = PrevIns( max->start_del );
    if( _Attr( lbl ) & ATTR_SHORT )
        optreturn( false );
    cl = PrevClass( lbl );
    if( !_TransferClass( cl ) )
        optreturn( false );
    DeleteQueue( lbl );
    InsertQueue( lbl, PrevIns( max->start_com ) );
    add = PrevIns( max->start_del );
    for( ;; ) {
        next = NextIns( add );
        DelInstr( next );
        if( next == ins ) {
            break;
        }
    }
    Untangle( lbl );
    optreturn( true );
}


static  void    TransformJumps( ins_entry *ins, ins_entry *first )
/****************************************************************/
{
    ins_entry           *add;
    ins_entry           *next;
    ins_entry           *lbl;
    oc_class            cl;

  optbegin
    if( _Class( ins ) == OC_RET )
        optreturnvoid;
    lbl = _Label( ins )->ins;
    if( lbl == NULL )
        optreturnvoid;
    add = lbl;
    for( ;; ) {
        if( add == NULL )
            optreturnvoid;
        cl = _Class( add );
        if( _TransferClass( cl ) )
            break;
        if( cl == OC_LABEL ) {
            if( _Attr( add ) & ATTR_SHORT )
                optreturnvoid;
            _ClrStatus( _Label( add ), SHORTREACH );
        }
        add = NextIns( add );
    }
    if( add == first || add == ins )
        optreturnvoid;
    if( FindShort( first, lbl ) )
        optreturnvoid;
    for( ;; ) {
        next = PrevIns( add );
        DeleteQueue( add );
        InsertQueue( add, first );
        if( add == lbl )
            break;
        add = next;
    }
    DeleteQueue( first );
    InsertQueue( first, next );
    Untangle( NextIns( first ) );
  optend
}



static  bool    CommonInstr( ins_entry *old, ins_entry *add )
/***********************************************************/
{
  optbegin
    if( _IsModel( NO_OPTIMIZATION ) )
        optreturn( false );
    if( _ClassInfo( add ) != _ClassInfo( old ) )
        optreturn( false );
    if( _InsLen( add ) != _InsLen( old ) )
        optreturn( false );
    if( _ObjLen( add ) != _ObjLen( old ) )
        optreturn( false );
    switch( _Class( old ) ) {
    case OC_BDATA:
        /* User may be doing stupid stuff like stringing together a bunch
         * of '__asm _emit x' or '__asm db x' statements which aren't complete
         * instructions. If we try to optimize and split them, things are
         * going to go boom. So let's just not touch those - it's extremely
         * unlikely that we'd be missing any real optimization opportunities.
         */
        /* fall through */
    case OC_DEAD:
        optreturn( false );
    case OC_JMP:
    case OC_CALL:
    case OC_LABEL:
    case OC_LREF:
        if( _Label( old ) != _Label( add ) )
            optreturn( false );
        break;
    case OC_JCOND:
        if( _JmpCond( old ) != _JmpCond( add ) )
            optreturn( false );
        if( _Label( old ) != _Label( add ) )
            optreturn( false );
#if _TARGET & _TARG_RISC
        if( old->oc.oc_jcond.index != add->oc.oc_jcond.index )
            return( false );
#endif
        break;
    case OC_RET:
        if( _RetPop( old ) != _RetPop( add )
          || (_Attr( old ) & ATTR_NORET) != (_Attr( add ) & ATTR_NORET) )
            optreturn( false );
        break;
    default:
        if( !Equal( &add->oc.oc_entry.data, &old->oc.oc_entry.data, _InsLen( add ) - offsetof( oc_entry, data ) ) )
            optreturn( false );
        break;
    }
    optreturn( true );
}


static  void    FindCommon( common_info *c, ins_entry *p1, ins_entry *p2 )
/************************************************************************/
{
  optbegin
    c->save = 0;
    for( ;; ) {
        if( !CommonInstr( p1, p2 ) )
            break;
        c->start_com = p1;
        c->start_del = p2;
        c->save += _ObjLen( p1 );
        for( ;; ) {
            p1 = PrevIns( p1 );
            if( p1 == NULL )
                optreturnvoid;
            if( _Class( p1 ) != OC_LABEL ) {
                break;
            }
        }
        p2 = PrevIns( p2 );
        if( p2 == NULL ) {
            break;
        }
    }
  optend
}

bool    ComTail( ins_entry *list, ins_entry *ins )
/************************************************/
{
    ins_entry           *try;
    ins_entry           *add;
    ins_entry           *next;
    ins_entry           *lbl;
    ins_entry           *first;
    common_info         common;
    common_info         max;
    obj_length          align;

  optbegin
    align = (_Class( list ) == OC_LABEL) ? _ObjLen( list ) : 0;
    try = list;
    first = NULL;
    max.save = 0;
    for( ;; ) {
        if( try != ins ) {
            FindCommon( &common, try, ins );
            if( common.save > max.save ) {
                max = common;
                first = try;
            }
        }
        try = _LblRef( try );
        if( try == NULL ) {
            break;
        }
    }
    if( max.save == 0 )
        optreturn( false );
    if( JustMoveLabel( &max, ins ) )
        optreturn( true );
    if( OptForSize < 25 )
        optreturn( false );
    if( max.save <= OptInsSize( OC_JMP, OC_DEST_NEAR ) )
        optreturn( false );
    TransformJumps( ins, first );
    add = PrevIns( max.start_del );
    try = PrevIns( max.start_com );
    if( FindShort( try, NULL ) )
        align = 0;
    AddNewJump( add, AddNewLabel( PrevIns( max.start_com ), align ) );
    lbl = PrevIns( max.start_com );
    add = NextIns( add );
    for( ;; ) {
        next = NextIns( add );
        DelInstr( next );
        if( next == ins ) {
            break;
        }
    }
    Untangle( lbl );
    IsolatedCode( add );
    optreturn( true );
}


bool    ComCode( ins_entry *jmp )
/*******************************/
{
    ins_entry   *new;
    ins_entry   *com;
    obj_length  align;
    bool        common;

  optbegin
    common = false;
    com = NULL;
    new = _Label( jmp )->ins;
    if( new != NULL ) {
        align = _ObjLen( new );
        for( ;; ) {
            for( ;; ) {
                new = PrevIns( new );
                if( new == NULL )
                    break;
                if( new == jmp )
                    break;
                if( _Class( new ) != OC_LABEL ) {
                    break;
                }
            }
            if( new == NULL )
                break;
            if( new == jmp )
                break;
            com = PrevIns( jmp );
            if( com == NULL )
                break;
            if( !CommonInstr( new, com ) )
                break;
            com = PrevIns( DelInstr( com ) );
            common = true;
            if( _Class( jmp ) == OC_DEAD ) {
                optreturn( common );
            }
        }
        if( common ) {
            if( FindShort( new, NULL ) )
                align = 0;
            ChgLblRef( jmp, AddNewLabel( new, align ) );
            Untangle( NextIns( new ) );
            Untangle( NextIns( jmp ) );
            Untangle( com );
        }
    }
    if( _Class( jmp ) == OC_DEAD )
        optreturn( common );
    common |= ComTail( _Label( jmp )->refs, jmp );
    optreturn( common );
}


void    TraceCommon( ins_entry *lbl_ins )
/***************************************/
{
    ins_entry   *ref;
    ins_entry   *ins;

  optbegin
    for( ;; ) {
        ref = _Label( lbl_ins )->refs;
        for( ;; ) {
            for( ;; ) {
                if( ref == NULL )
                    optreturnvoid;
                ins = ref;
                ref = _LblRef( ref );
                if( _Class( ins ) == OC_JMP ) {
                    break;
                }
            }
            if( ComCode( ins ) ) {
                break;
            }
        }
        if( ref == NULL ) {
            break;
        }
    }
    optreturnvoid;
}
