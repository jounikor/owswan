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
// CSREMBLK  : processing for remote blocks and EXECUTE
//

#include "ftnstd.h"
#include "errcod.h"
#include "namecod.h"
#include "global.h"
#include "recog.h"
#include "ferror.h"
#include "insert.h"
#include "utility.h"
#include "compstmt.h"
#include "proctbl.h"
#include "csutls.h"
#include "symtab.h"
#include "cgmagic.h"
#include "fcodes.h"
#include "gflow.h"
#include "brseinfo.h"


#define RB_FLAGS (SY_USAGE | SY_SUBPROGRAM | SY_REMOTE_BLOCK)

static bool BlockName( unsigned_16 rb_defined )
{
    sym_id      sym_ptr;
    bool        rb_name;

    CSExtn();
    rb_name = false;
    if( ReqName( NAME_REM_BLOCK ) ) {
        sym_ptr = LkSym();
        if( (sym_ptr->u.ns.flags & ~SY_REFERENCED) == 0 ) {
            sym_ptr->u.ns.si.rb.entry = NextLabel();
            sym_ptr->u.ns.flags = RB_FLAGS;
        }
        if( ( (sym_ptr->u.ns.flags & ~(SY_RB_DEFINED | SY_REFERENCED)) == RB_FLAGS ) &&
            ( (sym_ptr->u.ns.flags & rb_defined) == 0 ) ) {
            sym_ptr->u.ns.flags |= rb_defined;
            rb_name = true;
        } else {
            IllName( sym_ptr );
        }
    }
    return( rb_name );
}

void CpRemBlock( void )
{
    sym_id      rb;

    if( !EmptyCSList() ) {
        StmtErr( SP_BLK_IN_STRUCTURE );
    }
    AddCSNode( CS_REMOTEBLOCK );
    CSHead->bottom = NextLabel();
    CSHead->branch = NextLabel();
    GBranch( CSHead->branch );
    if( BlockName( SY_RB_DEFINED ) ) {
        rb = CITNode->sym_ptr;
        if( (rb->u.ns.flags & SY_REFERENCED) == 0 ) {
            rb->u.ns.si.rb.ref_count = 0;
        }
        rb->u.ns.si.rb.ref_count++;
        CSHead->cs_info.rb = rb;
        GStartBlock();
        BIStartRBorEP( rb );
    }
    AdvanceITPtr();
    ReqEOS();
    StNumbers.in_remote = true;
    ClearRem();
}

void CpEndBlock( void )
{
    CheckCSList( CS_REMOTEBLOCK );
    if( CSHead->typ == CS_REMOTEBLOCK ) {
        GLabel( CSHead->bottom );
        FreeLabel( CSHead->bottom );
        // Make sure REMOTEBLOCK statement was valid.
        if( CSHead->cs_info.rb != NULL ) {
            GEndBlock();
        }
        GLabel( CSHead->branch );
        FreeLabel( CSHead->branch );
        DelCSNode();
    } else {
        Match();
    }
    CSNoMore();
    StNumbers.in_remote = false;
    ClearRem();
    CSNoMore();
    BIEndRBorEP();
}

static void CkRemBlkRec( void )
{
    csnode      *node;

    node = CSHead;
    while( node->typ != CS_REMOTEBLOCK ) {
        node = node->link;
    }
    if( CmpNode2Str( CITNode, &node->label ) ) {
        Error( SP_RECURSE );
    }
}

void CpExecute( void )
{
    sym_id      rb;

    if( StNumbers.in_remote ) {
        CkRemBlkRec();
    }
    if( BlockName( 0 ) ) {
        rb = CITNode->sym_ptr;
        if( (rb->u.ns.flags & (SY_RB_DEFINED | SY_REFERENCED)) == 0 ) {
            rb->u.ns.si.rb.ref_count = 0;
        }
        rb->u.ns.si.rb.ref_count++;
        BIOutSymbol( rb );              // reference and or declare the sucker
        rb->u.ns.flags |= SY_REFERENCED;
        GExecute();
    }
    AdvanceITPtr();
    ReqEOS();
}

