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
* Description:  process statement functions
*
****************************************************************************/


#include "ftnstd.h"
#include "errcod.h"
#include "namecod.h"
#include "global.h"
#include "stmtsw.h"
#include "fmemmgr.h"
#include "recog.h"
#include "ferror.h"
#include "utility.h"
#include "dsname.h"
#include "stmtfunc.h"
#include "symtab.h"
#include "rstmgr.h"
#include "fcodes.h"
#include "gflow.h"
#include "brseinfo.h"


void    SFPrologue( void ) {
//====================

// Generate code for statement function prologue.

    sym_id      sym;
    itnode      *func_node;
    itnode      *arg_list;
    sf_parm     **parm;

    StmtSw |= SS_SF_REFERENCED;
    CkTypeDeclared();
    SFSymId = CITNode->sym_ptr;
    if( ( SFSymId->u.ns.u1.s.typ == FT_CHAR ) && ( SFSymId->u.ns.xt.size == 0 ) ) {
        Error( SF_ILL_CHAR_LEN );
    } else if( SFSymId->u.ns.u1.s.typ == FT_STRUCTURE ) {
        Error( SF_ILL_TYPE );
    }
    GStartSF();
    SFSymId->u.ns.flags = SY_USAGE | SY_TYPE | SY_SUBPROGRAM | SY_STMT_FUNC;
    CITNode->flags = SFSymId->u.ns.flags;
    func_node = CITNode;
    AdvanceITPtr();
    ReqOpenParen();
    SFSymId->u.ns.si.sf.header = FMemAlloc( sizeof( sf_header ) );
    SFSymId->u.ns.si.sf.header->ref_count = 1;
    parm = &SFSymId->u.ns.si.sf.header->parm_list;
    *parm = NULL;
    if( RecNOpn() ) {
        AdvanceITPtr();
    } else {
        for( ;; ) {
            if( ReqName( NAME_SF_DUMMY ) ) {
                sym = LkSym();
                sym->u.ns.u1.s.xflags |= SY_DEFINED;
                CkTypeDeclared();
                if( ( (sym->u.ns.flags & SY_CLASS) == SY_VARIABLE ) &&
                    ( (sym->u.ns.flags & SY_SUBSCRIPTED) == 0 ) &&
                    ( ( sym->u.ns.u1.s.typ != FT_CHAR ) ||
                      ( sym->u.ns.xt.size != 0 ) ) &&
                    ( sym->u.ns.u1.s.typ != FT_STRUCTURE ) ) {
                    if( sym->u.ns.flags & SY_SPECIAL_PARM ) {
                        Error( SF_DUPLICATE_DUMMY_PARM );
                    } else {
                        *parm = FMemAlloc( sizeof( sf_parm ) );
                        (*parm)->link = NULL;
                        (*parm)->actual = sym;
                        (*parm)->shadow = STShadow( sym );
                        parm = &((*parm)->link);
                    }
                } else {
                    Error( SF_ILL_DUMMY_PARM );
                }
            }
            AdvanceITPtr();
            if( !RecComma() ) {
                break;
            }
        }
    }
    ReqCloseParen();
    ReqNOpn();
    arg_list = func_node->link;
    func_node->link = CITNode->link;
    CITNode->link = NULL;
    CITNode = func_node;
    FreeITNodes( arg_list );
}


void    SFEpilogue( void ) {
//====================

// Generate code for statement function epilogue.

    sf_parm     *parm;

    GEndSF();
    for( parm = SFSymId->u.ns.si.sf.header->parm_list; parm != NULL; parm = parm->link ) {
        if( parm->shadow->u.ns.flags & SY_REFERENCED ) {
            parm->actual->u.ns.flags |= SY_REFERENCED;
        }
        STUnShadow( parm->actual );
    }
    BIEndSF( SFSymId ); // end statement function browsing information
}
