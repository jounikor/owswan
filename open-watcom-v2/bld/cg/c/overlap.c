/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Determine whether two names overlap in storage.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "regset.h"
#include "namelist.h"
#include "overlap.h"


static  bool    Conflicts( type_length start,  type_length end,
                           type_length start2, type_length end2 )
/***************************************************************/
{
    if( ( start2 >= start && start2 <= end ) ||
        ( end2 >= start && end2 <= end ) ) {
        return( true );
    }
    return( false );
}

static  bool    ovNo( name *op1, name *op2 )
/******************************************/
{
    /* unused parameters */ (void)op1; (void)op2;

    return( false );
}

static  bool    ovYes( name *op1, name *op2 )
/*******************************************/
{
    /* unused parameters */ (void)op1; (void)op2;

    return( true );
}

static  bool    ovTemp( name *op1, name *op2 )
/********************************************/
{
    if( DeAlias( op1 ) == DeAlias( op2 ) ) {
        return( Conflicts( op1->v.offset, op1->v.offset + op1->n.size,
                    op2->v.offset, op2->v.offset + op2->n.size ) );
    }
    return( false );
}

static  bool    ovReg( name *op1, name *op2 )
/*******************************************/
{
    return( HW_Ovlap( op1->r.reg, op2->r.reg ) );
}

static  bool    ovIndex( name *op1, name *op2 )
/*********************************************/
{
    /* unused parameters */ (void)op1; (void)op2;

    /* this is overly pessimistic, but we shouldn't see mem->mem moves anyway */
    return( true );
}

static  bool    ovUses( name *op1, name *index )
/**********************************************/
{
    return( Overlaps( index->i.index, op1 ) ||
            Overlaps( index->i.base, op1 ) );
}

static  bool    (*OverlapTable[N_CLASS_MAX][N_CLASS_MAX])( name *, name * ) = {
/* result       op ->   N_CONST N_MEM   N_TEMP  N_REG   N_INDEX */
/* N_CONSTANT   */   {  ovNo,   ovNo,   ovNo,   ovNo,   ovNo },
/* N_MEMORY     */   {  ovNo,   ovYes,  ovNo,   ovNo,   ovNo },
/* N_TEMP       */   {  ovNo,   ovNo,   ovTemp, ovNo,   ovUses },
/* N_REGISTER   */   {  ovNo,   ovNo,   ovNo,   ovReg,  ovUses },
/* N_INDEX      */   {  ovNo,   ovNo,   ovNo,   ovNo,   ovIndex } };

bool    Overlaps( name *result, name *op )
/****************************************/
{
    if( result == NULL || op == NULL )
        return( false );
    if( result == op )
        return( true );
    return( OverlapTable[result->n.class][op->n.class]( result, op ) );
}

bool    CondOverlaps( name *result, name *ccop )
/*******************************************************
    returns true if modifying "result" could cause "ccop"
    to be modified as well.
*/
{
    if( result == NULL || ccop == NULL )
        return( true );
    if( result == ccop )
        return( true );
    return( OverlapTable[result->n.class][ccop->n.class]( result, ccop ) );
}
