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
#include "data.h"
#include "savings.h"
#include "targetin.h"
#include "namelist.h"
#include "optab.h"
#include "temps.h"
#include "split.h"
#include "regsave.h"
#include "x86temps.h"


static  void    AssignPushLocals( void );

void    PushLocals( void )
/*************************
    Assign locals which can have their initial value pushed onto the stack
*/
{
    if( BlockByBlock )
        return;
    if( Save.store_cost[WD] <= Save.push_cost[WD] )
        return;
    AssignPushLocals();
}

static  void    AssignPushLocals( void ) {
/***********************************
    Scan through HeadBlock to see if there are any leading instructions of
    the form MOV REG => temp, where temp is on the stack. We can eliminate
    the instruction (call DoNothing to mark it as unnecessary to generate)
    and then the guy that generates the prolog can come along later and
    generate PUSH REG, and adjust the SUB SP,n instruction appropriately.
    this replaces a move with a less expensive push, and if all locals
    turn out to be push locals, eliminates the SUP SP,n instruction as well
*/

    instruction *ins;
    name        *src;
    name        *dst;
    type_length curr_offset;

    curr_offset = 0;
    for( ins = HeadBlock->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        if( CurrProc->prolog_state & GENERATED_PROLOG )
            break;
        if( DoesSomething( ins ) ) {
            if( ins->head.opcode != OP_MOV )
                break;
            if( UnChangeable( ins ) )
                break;
            src = ins->operands[0];
            dst = ins->result;
            if( src->n.class != N_REGISTER )
                break;
            if( _IsFloating( src->n.type_class ) )
                break;
            if( dst->n.class != N_TEMP )
                break;
#if _TARGET & _TARG_8086
            if( dst->n.size != 2 && dst->n.size != 4 )
                break;
#else
            if( dst->n.size != 4 )
                break;
#endif
            curr_offset -= PushSize( dst->n.size );/* assume it will be pushed*/
            if( DeAlias( dst ) != dst )
                break;
            if( dst->v.usage & HAS_MEMORY ) {
                if( dst->t.location != curr_offset ) {
                    break;
                }
            } else {
                CurrProc->targ.push_local_size += PushSize( dst->n.size );
                dst->t.location = - CurrProc->targ.push_local_size;
                dst->v.usage |= HAS_MEMORY;
                PropLocal( dst );
            }
            ins->head.state = OPERANDS_NEED_WORK;
            DoNothing( ins );
        }
    }
}


void    AdjustPushLocal( name *temp )
/**********************************************
    Adjust the location field of a push local, and all its aliases.
    Before this routine is called, the location field just contains
    the offset relative to the first push local in the routine, not BP.
    We adjust here because the adjustment depends upon CurrProc->locals.base
    which is set by I86PROC.C, and cannot be known until we know just
    what kind of prolog we're generating.
*/
{
    name        *scan;

    if( (temp->t.temp_flags & PUSH_LOCAL) == 0 ) { /* if not already adjusted*/
        temp->t.location += -CurrProc->locals.size - CurrProc->locals.base;
        for( scan = temp->t.alias; scan != temp; scan = scan->t.alias ) {
            scan->t.location = temp->t.location;
            scan->v.usage |= HAS_MEMORY;
            scan->t.temp_flags |= PUSH_LOCAL;
        }
    }
}


void    SetTempLocation( name *temp, type_length size )
/******************************************************
    Snag a temporary with length "size" off the stack and set it's location
*/
{
    CurrProc->locals.size += size;
    temp->t.location = - CurrProc->locals.size
                      - CurrProc->locals.base;
}


void    RelocParms( void )
/*****************************
    Relocate parameter locations based on what type of prolog we generated,
    how many registers were pushed on the stack, and all that other stuff
    figured out by GenProlog
*/
{
    name        *name;

    for( name = Names[N_TEMP]; name != NULL; name = name->n.next_name ) {
        if( ( name->v.usage & HAS_MEMORY ) && ( name->t.temp_flags & STACK_PARM ) ) {
            name->t.location += CurrProc->parms.base;
        }
    }
}


bool    TempAllocBefore( void *t1, void *t2 )
/*******************************************/
{
    if( (((name *)t1)->t.temp_flags & USED_AS_FD) && (((name *)t2)->t.temp_flags & USED_AS_FD) == 0 ) {
        if( CurrProc->targ.sp_align )
            return( false );
        return( true );
    }
    if( (((name *)t2)->t.temp_flags & USED_AS_FD) && (((name *)t1)->t.temp_flags & USED_AS_FD) == 0 ) {
        if( CurrProc->targ.sp_align )
            return( true );
        return( false );
    }
    if( ((name *)t1)->n.size < ((name *)t2)->n.size )
        return( true );
    if( ((name *)t1)->n.size != ((name *)t2)->n.size )
        return( false );
    /* allocate big pointers the right way 'round for LES/LDS*/
    if( ((name *)t1)->t.v.id == ((name *)t2)->t.v.id && ((name *)t1)->v.offset > ((name *)t2)->v.offset )
        return( true );
    return( false );
}
