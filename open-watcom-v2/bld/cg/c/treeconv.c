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
* Description:  Type conversion folding, tree demotion.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "addrname.h"
#include "tree.h"
#include "treeconv.h"
#include "treeprot.h"
#include "treefold.h"
#include "zoiks.h"
#include "cfloat.h"
#include "utils.h"
#include "makeaddr.h"
#include "procdef.h"
#include "typemap.h"


static  bool    DemoteTree( tn name, type_def *tipe, bool just_test ) {
/*********************************************************************/

    type_def    *frum;
    bool        demote_this_node;
    bool        can_demote;

    frum = name->tipe;
    can_demote = false;
    demote_this_node = false;
    if( TypeClass( frum ) <= I4 ) {
        switch( name->class ) {
        case TN_UNARY: /* go left*/
            switch( name->u2.t.op ) {
            case O_UMINUS:
            case O_COMPLEMENT:
            case O_CONVERT:
            case O_ROUND:
                can_demote = DemoteTree( name->u.left, tipe, just_test );
                demote_this_node = true;
                break;
            case O_POINTS:
#if _TARG_MEMORY == _TARG_LOW_FIRST
                can_demote = true;
                demote_this_node = true;
#endif
                break;
            }
            break;
        case TN_BINARY: /* go left, right*/
            switch( name->u2.t.op ) {
#if _TARGET & ( _TARG_80386 | _TARG_8086 )
            case O_CONVERT:
                 /* Based pointer junk */
                 break;
            case O_DIV:
            case O_MOD:
                if( name->u.left->tipe->length > tipe->length ||
                    name->u2.t.rite->tipe->length > tipe->length )
                    break;
                /* fall throught */
#endif
            case O_TIMES:
            case O_AND:
            case O_OR:
            case O_XOR:
            case O_LSHIFT:
            case O_PLUS:
            case O_MINUS:
                if( name->tipe->refno == TY_HUGE_POINTER )
                    break;
                if( name->u.left->tipe->refno == TY_HUGE_POINTER )
                    break;
                if( name->u2.t.rite->tipe->refno == TY_HUGE_POINTER )
                    break;
                can_demote = DemoteTree( name->u.left, tipe, just_test );
                if( can_demote ) {
                    can_demote = DemoteTree( name->u2.t.rite, tipe, just_test );
                }
                demote_this_node = true;
                break;
            }
            break;
        case TN_COMMA:
            can_demote = DemoteTree( name->u2.t.rite, tipe, just_test );
            break;
        case TN_SIDE_EFFECT:
            can_demote = DemoteTree( name->u.left, tipe, just_test );
            break;
        case TN_FLOW_OUT:
        case TN_CONS:
            can_demote = true;
            demote_this_node = true;
            break;
        }
        if( !just_test && demote_this_node && frum->length > tipe->length ) {
            name->flags |= TF_DEMOTED;
            name->tipe = tipe;
        }
    }
    return( can_demote );
}


void    TGDemote( tn name, type_def *tipe )
/*****************************************/
{
    if( DemoteTree( name, tipe, true ) ) {
        DemoteTree( name, tipe, false );
    }
}


tn      FoldCnvRnd( cg_op op, tn name, type_def *to_tipe )
/********************************************************/
{
    tn              new;
    float_handle    cf;
    float_handle    junk;

    if( name->class == TN_CONS ) {
        if( name->tipe->refno == TY_DEFAULT ) {
            cf = CFCopy( name->u.name->c.value );
        } else {
            cf = CnvCFToType( name->u.name->c.value, name->tipe );
        }
        if( to_tipe->attr & TYPE_FLOAT ) {
            new = TGConst( cf, to_tipe );
        } else if( op == O_CONVERT ) {
            junk = cf;
            cf = CFTrunc( cf );
            CFFree( junk );
            if( to_tipe->refno != TY_DEFAULT ) {
                junk = cf;
                cf = CnvCFToType( cf, to_tipe );
                CFFree( junk );
            }
            new = TGConst( cf, to_tipe );
        } else if( op == O_ROUND ) {
            junk = cf;
            cf = CFRound( cf );
            CFFree( junk );
            if( to_tipe->refno != TY_DEFAULT ) {
                junk = cf;
                cf = CnvCFToType( cf, to_tipe );
                CFFree( junk );
            }
            new = TGConst( cf, to_tipe );
        } else {
            new = NULL;
            Zoiks( ZOIKS_078 ); /* Not supposed to get here, ever */
        }
        BurnTree( name );
    } else {
        TGDemote( name, to_tipe );
        new = NULL;
    }
    return( new );
}
