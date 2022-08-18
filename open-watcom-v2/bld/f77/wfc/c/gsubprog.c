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
// GSUBPROG  : Subprogram code generation routines
//

#include "ftnstd.h"
#include "global.h"
#include "wf77aux.h"
#include "fcodes.h"
#include "opr.h"
#include "opn.h"
#include "fcgbls.h"
#include "prmcodes.h"
#include "cpopt.h"
#include "recog.h"
#include "emitobj.h"
#include "types.h"
#include "utility.h"
#include "rstutils.h"
#include "parmcode.h"
#include "symtab.h"
#include "cgmagic.h"
#include "gstring.h"
#include "gsubprog.h"
#include "gflow.h"
#include "gtypes.h"


/* Forward declarations */
static  int     DumpArgInfo( itnode *node );
static  void    SetArgAddrs( void );
static  void    FinishCALL( itnode *sp );
void    GNullRetIdx( void );



void    GBegCall( itnode *itptr ) {
//=================================

// Initialize for subprogram invocation.

    sym_id      sp;
    obj_ptr     curr_obj;
    int         num_args;

    sp = itptr->sym_ptr;
#if _CPU == 386
    CheckFar16Call( sp );
#endif
    EmitOp( FC_CALL );
    OutPtr( itptr->sym_ptr );
    curr_obj = ObjTell();
    OutU16( 0 );
    if( (Options & OPT_DESCRIPTOR) == 0 ) {
        if( (sp->u.ns.flags & SY_SUBPROG_TYPE) == SY_FUNCTION ) {
            if( (sp->u.ns.flags & SY_INTRINSIC) == 0 ) {
                if( sp->u.ns.u1.s.typ == FT_CHAR ) {
                    OutPtr( GTempString( sp->u.ns.xt.size ) );
                }
            }
        }
    }
    num_args = DumpArgInfo( itptr->list );
    curr_obj = ObjSeek( curr_obj );
    OutU16( num_args );
    ObjSeek( curr_obj );
    if( (sp->u.ns.flags & SY_SUBPROG_TYPE) == SY_FUNCTION ) {
        if( sp->u.ns.u1.s.typ == FT_CHAR ) {
            if( (Options & OPT_DESCRIPTOR) || (sp->u.ns.flags & SY_INTRINSIC) ) {
                OutPtr( GTempString( sp->u.ns.xt.size ) );
            }
        }
    }
}


void    GEndCall( itnode *itptr, int num_stmts ) {
//================================================

// Finish off a subprogram invocation.

    itnode      *arg;

    if( num_stmts > 0 ) {
        EmitOp( FC_ALT_RET );
        OutU16( num_stmts );
        arg = itptr->list;
        for(;;) {
            if( (arg->opn.us & USOPN_WHAT) == USOPN_STN ) {
                GStmtAddr( arg->sym_ptr );
                num_stmts--;
            }
            arg = arg->link;
            if( num_stmts == 0 ) {
                break;
            }
        }
    } else if( (itptr->sym_ptr->u.ns.flags & SY_SUBPROG_TYPE) == SY_SUBROUTINE ) {
        EmitOp( FC_EXPR_DONE );
    }
    SetOpn( itptr, USOPN_SAFE );
}


void    GArg( void ) {
//==============

// Generate an argument for subprogram, subscript, or substring.

    if( (CITNode->opn.us & USOPN_WHERE) == USOPN_SAFE ) {
        if( (CITNode->opn.us & USOPN_FLD) &&
            ((CITNode->opn.us & USOPN_WHAT) == USOPN_ARR) &&
            (CITNode->typ == FT_CHAR) ) {
            EmitOp( FC_PASS_FIELD_CHAR_ARRAY );
            OutPtr( CITNode->value.st.field_id );
            OutPtr( GTempString( 0 ) );
        }
        return;
    }
    if( (CITNode->opn.us & USOPN_WHAT) == USOPN_SSR ) {
        EmitOp( FC_PUSH_SCB_LEN );
    } else if( (CITNode->opn.us & USOPN_WHAT) == USOPN_CON ) {
        PushOpn( CITNode );
    } else if( (CITNode->opn.us & USOPN_WHAT) == USOPN_ARR ) {
        PushOpn( CITNode );
        if( CITNode->typ == FT_CHAR ) {
            EmitOp( FC_PASS_CHAR_ARRAY );
            SymRef( CITNode );
            OutPtr( GTempString( 0 ) );
        }
    } else {
        PushOpn( CITNode );
    }
}


int     GParms( itnode *sp ) {
//============================

// Process argument list.

    int         num_stmts;

    /* unused parameters */ (void)sp;

    num_stmts = 0;
    for(;;) {
        if( !RecNOpn() ) {  // consider f()
            if( CITNode->opn.us == USOPN_STN ) {
                num_stmts++;
            }
        }
        AdvanceITPtr();
        if( RecCloseParen() || RecColon() ) {
            break;
        }
    }
    return( num_stmts );
}


static  int     DumpArgInfo( itnode *node ) {
//===========================================

// Dump argument types.

    int         num_args;
    unsigned_16 arg_info;
    PTYPE       parm_type;
    PCODE       parm_code;
#if _CPU == 386
    aux_info    *info;
#endif

    num_args = 0;
    if( node != NULL ) {
        for(;;) {
            if( node->opr == OPR_COL )
                break;
            if( node->opr == OPR_RBR )
                break;
            if( node->opn.ds == DSOPN_PHI )
                break;
            if( node->opn.us != USOPN_STN ) {
                parm_type = ParmType( node->typ, node->size );
                parm_code = ParmClass( node );
#if _CPU == 386
                if( (parm_code == PC_PROCEDURE) || (parm_code == PC_FN_OR_SUB) ) {
                    info = InfoLookup( node->sym_ptr );
                    if( info->cclass & FAR16_CALL ) {
                        parm_code |= PC_PROC_FAR16;
                    }
                }
#endif
                arg_info = _SetTypeInfo( parm_code, parm_type );
                OutU16( arg_info );
                ++num_args;
            }
            node = node->link;
        }
    }
    return( num_args );
}


void    GSPProlog( void ) {
//===================

// Generate a subprogram prologue.

    SetArgAddrs();
    ReturnValue = SymLookup( "$@RVAL", 6 );
    ReturnValue->u.ns.flags |= SY_REFERENCED;
    ReturnValue->u.ns.u1.s.xflags |= SY_DEFINED;
}


void    GEPProlog( void ) {
//===================

// Generate an entry point prologue.

    sym_id      ep;
    char        *ptr;
    char        name[MAX_SYMLEN+3];

    ep = ArgList->id;
    ep->u.ns.si.sp.u.entry = NextLabel();
    GLabel( ep->u.ns.si.sp.u.entry );
    // by the time we define the label for the entry point, the code that
    // references it will have been executed
    FreeLabel( ep->u.ns.si.sp.u.entry );
    SetArgAddrs();
    if( CommonEntry == NULL ) {
        ptr = name;
        *ptr = '$';
        ptr++;
        ptr = STGetName( SubProgId, ptr );
        *ptr = '.';
        ptr++;
        *ptr = NULLCHAR;
        CommonEntry = SymLookup( name, SubProgId->u.ns.u2.name_len + 2 );
        if( (SubProgId->u.ns.flags & SY_SUBPROG_TYPE) == SY_SUBROUTINE ) {
            CommonEntry->u.ns.flags = SY_USAGE | SY_SUBPROGRAM | SY_SUBROUTINE |
                                    SY_SENTRY | SY_REFERENCED;
        } else {
            CommonEntry->u.ns.flags = SY_USAGE | SY_SUBPROGRAM | SY_FUNCTION |
                                    SY_SENTRY | SY_REFERENCED;
        }
        EPValue = SymLookup( "$@EVAL", 6 );
        EPValue->u.ns.flags |= SY_REFERENCED;
        EPValue->u.ns.u1.s.xflags |= SY_DEFINED;
    }
}


static  void    SetArgAddrs( void ) {
//=============================

// Assign addresses to dummy argument arguments.

    parameter   *d_arg;

    EmitOp( FC_DARG_INIT );
    OutPtr( ArgList->id );
    for( d_arg = ArgList->parms; d_arg != NULL; d_arg = d_arg->link ) {
        if( (d_arg->flags & ARG_STMTNO) == 0 ) {
            OutPtr( d_arg->id );
        }
    }
    OutPtr( NULL );
}


void    GEpilog( void ) {
//=================

// Generate a subprogram epilogue.

    if( (SubProgId->u.ns.flags & SY_SUBPROG_TYPE) == SY_SUBROUTINE ) {
        GNullRetIdx();
    }
    if( EpilogLabel != 0 ) {
        GLabel( EpilogLabel );
        FreeLabel( EpilogLabel );
    }
    EmitOp( FC_EPILOGUE );
    OutPtr( SubProgId );
}


void    GReturn( void ) {
//=================

// Generate a return from the program.

    GEpilog();
}


void    GEndBlockData( void ) {
//=======================

// Terminate a block data subprogram.

    EmitOp( FC_EPILOGUE );
    OutPtr( SubProgId );
}


void    GGotoEpilog( void ) {
//=====================

// Generate a branch to the epilogue.

    if( EpilogLabel == 0 ) {
        EpilogLabel = NextLabel();
    }
    GBranch( EpilogLabel );
}


void    GRetIdx( void ) {
//=================

// Generate an alternate return.

    PushOpn( CITNode );
    EmitOp( FC_ASSIGN_ALT_RET );
    GenType( CITNode );
}


void    GNullRetIdx( void ) {
//=====================

// No alternate return.

    PushConst( 0 );
    EmitOp( FC_ASSIGN_ALT_RET );
    DumpType( FT_INTEGER, TypeSize( FT_INTEGER ) );
}


void    GCallNoArgs( void ) {
//=====================

// Generate a CALL with no arguments.

    GBegCall( CITNode );
    GEndCall( CITNode, 0 );
    FinishCALL( CITNode );
}


void    GCallWithArgs( void ) {
//=======================

// Generate a CALL with arguments.

    PushOpn( CITNode );
    FinishCALL( CITNode );
}


static  void    FinishCALL( itnode *sp ) {
//========================================

    if( (sp->sym_ptr->u.ns.flags & SY_SUBPROG_TYPE) == SY_FUNCTION ) {
        // a FUNCTION invoked in a CALL statement
        EmitOp( FC_EXPR_DONE );
    }
}


void    GArgList( entry_pt *arg_list, uint args, PTYPE typ ) {
//===========================================================

// Dump start of an argument list.

    /* unused parameters */ (void)arg_list; (void)args; (void)typ;
}


void    GArgInfo( sym_id sym, PCODE code, PTYPE typ ) {
//===================================================

// Dump information for an argument.

    /* unused parameters */ (void)sym; (void)code; (void)typ;
}
