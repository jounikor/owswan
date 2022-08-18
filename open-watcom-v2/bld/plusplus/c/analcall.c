/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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


#include "plusplus.h"
#include "memmgr.h"
#include "cgfront.h"
#include "fnovload.h"
#include "calldiag.h"
#include "name.h"
#include "defarg.h"
#include "vfun.h"
#include "fnbody.h"
#include "objmodel.h"


static CNV_DIAG diagArgConv =   // DIAGNOSIS FOR ARGUMENT CONVERSION
    { ERR_PARM_IMPOSSIBLE
    , ERR_PARM_AMBIGUOUS
    , ERR_CALL_WATCOM
    , ERR_PARM_PRIVATE
    , ERR_PARM_PROTECTED
    };

static CNV_DIAG diagEllConv =   // DIAGNOSIS FOR ELLIPSIS ARGUMENT CONVERSION
    { ERR_ELLIPSIS_IMPOSSIBLE
    , ERR_ELLIPSIS_AMBIGUOUS
    , ERR_CALL_WATCOM
    , ERR_PARM_PRIVATE
    , ERR_PARM_PROTECTED
    };

static SYMBOL_DIAG diagAccess = // DIAGNOSIS FOR CALLER ACCESS
    { ERR_BARE_FUNCTION_ACCESS
    , ERR_EXTRA_THIS_FOR_FUNCTION
    , ERR_ENCLOSING_THIS_FUNCTION
    };

typedef struct {
    CGOP        cgop;           // operator name
    char        *name;          // name of intrinsic function
} intrinsic_mapping;

static intrinsic_mapping mapIntrinsic[] = {
#include "ppopint.h"
{ CO_NOP, NULL }
};


PTREE NodeGetCallExpr(          // POINT AT CALL EXPRESSION
    PTREE expr )                // - result from AnalyseCall
{
    PTREE call_expr;            // - NULL or call expression

    if( expr->op == PT_ERROR ) {
        call_expr = NULL;
    } else {
        if( PTreeOpFlags( expr ) & PTO_RVALUE ) {
            expr = expr->u.subtree[0];
        }
        call_expr = expr;
    }
    return( call_expr );
}


static PTREE makeCall(          // MAKE A CALL OR INDIRECT CALL
    PTREE proc,                 // - procedure
    TYPE type,                  // - return type
    PTREE args,                 // - arguments
    bool direct_call )          // - true ==> do a direct call
{
    PTREE node;                 // - new node

    if( direct_call ) {
        node = NodeUnaryCopy( CO_CALL_SETUP, proc );
        node = NodeBinary( CO_CALL_EXEC, node, args );
    } else {
        node = VfunSetupCall( proc );
        node = NodeBinary( CO_CALL_EXEC_IND, node, args );
    }
    return( NodeSetType( node, type, PTF_MEANINGFUL | PTF_SIDE_EFF ) );
}


PTREE NodeMakeCall(             // FABRICATE A FUNCTION CALL EXPRESSION
    SYMBOL proc,                // - procedure
    TYPE type,                  // - return type
    PTREE args )                // - arguments
{
    DbgVerify( (PointerTypeEquivalent( type ) == NULL)
               == (PointerTypeEquivalent( SymFuncReturnType( proc ) ) == NULL)
             , "NodeMakeCall -- return type mismatch" );
    return( makeCall( NodeMakeCallee( proc ), type, args, true ) );
}


PTREE NodeReverseArgs(          // REVERSE CALL ARGUMENTS
    unsigned *arg_count,        // - # args
    PTREE arg )                 // - start of arg. list (to be reversed)
{
    PTREE last;                 // - last argument
    PTREE right;                // - right argument
    unsigned count;             // - counts arguments

    for( last = NULL
         , count = 0
       ;   arg != NULL
       ;   ++ count
         , right = arg->u.subtree[0]
         , arg->u.subtree[0] = last
         , last = arg
         , arg = right
       );
    if( arg_count != NULL ) {
        *arg_count = count;
    }
    return( last );
}


void NodeBuildArgList(          // BUILD ARGUMENT LIST FROM CALLER ARG.S
    arg_list *alist,            // - argument structure
    PTREE *ptlist,              // - list of parse tree nodes
    PTREE arg,                  // - arguments
    unsigned num_args )         // - number of arguments
{
    TYPE *aptr;                 // - addr( current TYPE in arg. list )

    alist->num_args = num_args;
    aptr = alist->type_list;
    while( num_args-- > 0 ) {
        arg->type = BindTemplateClass( arg->type, &arg->locn, true );
        if( (arg->flags & PTF_LVALUE) && NodeReferencesTemporary( arg->u.subtree[1] ) ) {
            // temporaries may only be bound to const references
            if( NULL == TypeReference( arg->type ) ) {
                arg->type = MakeConstReferenceTo( arg->type );
            }
            *aptr = arg->type;
            aptr++;
        } else {
            *aptr++ = NodeType( arg );
        }
        arg->u.subtree[1]->type = BindTemplateClass( arg->u.subtree[1]->type, &arg->u.subtree[1]->locn, true );
        *ptlist++ = arg->u.subtree[1];
        arg = arg->u.subtree[0];
    }
}


bool NodeConvertArgument(       // CONVERT AN ARGUMENT VALUE
    PTREE *a_expr,              // - addr( argument value )
    TYPE proto )                // - prototype type
{
    bool ok;                    // - return: true ==> conversion ok

    if( NULL != ArrayType( proto ) ) {
        proto = PointerTypeForArray( proto );
    } else {
        // ( const int ) prototype should be ( int ) at this point
        proto = TypedefModifierRemoveOnly( proto );
    }
    *a_expr = CastImplicit( *a_expr, proto, CNV_FUNC_ARG, &diagArgConv );
    ok = ( (*a_expr)->op != PT_ERROR );
    return( ok );
}


static PTREE arg_fillout(       // FILL OUT A CONVERTED ARGUMENT
    PTREE arg )                 // - the argument
{
    PTREE right;                // - converted tree

    right = arg->u.subtree[1];
    arg->type = right->type;
    arg->flags = (arg->flags & PTF_ARGS) | right->flags;
    return( arg );
}


static PTREE arg_finish(        // FINISH A CONVERTED VALUE
    PTREE converted,            // - converted value
    PTREE arg )                 // - the argument node
{
    arg->u.subtree[1] = converted;
    return( arg_fillout( arg ) );
}


static bool arg_convert(        // CONVERT AN ARGUMENT
    PTREE arg,                  // - argument node
    TYPE proto )                // - prototype type
{
    bool ok;                    // - return: true ==> ok

    if( NodeConvertArgument( &arg->u.subtree[1], proto ) ) {
        arg_fillout( arg );
        ok = true;
    } else {
        PTreeErrorNode( arg );
        ok = false;
    }
    return( ok );
}


static bool passStructOnStack(  // PASS A STRUCT/CLASS ON STACK
    PTREE arg,                  // - argument (CO_LIST)
    MSG_NUM warning )           // - internal-data warning
{
    PTREE right;                // - right operand
    TYPE type;                  // - class type

    right = NodeRvalue( arg->u.subtree[1] );
    type = right->type;
    if( right->flags & PTF_CLASS_RVREF ) {
        if( right->op != PT_ERROR ) {
            PTREE temp = NodeTemporary( type );
            right = ClassDefaultCopyDiag( temp, right, &diagEllConv );
            if( right->op != PT_ERROR ) {
                right = NodeDtorExpr( right, temp->u.symcg.symbol );
                if( right->op != PT_ERROR ) {
                    right->type = type;
                    right = NodeFetch( right );
                    right->flags &= ~PTF_LVALUE;
                }
            }
        }
        if( right->op == PT_ERROR ) {
            arg->u.subtree[1] = right;
            PTreeErrorNode( arg );
            return( false );
        }
    }
    arg_finish( right, arg );
    if( TypeHasSpecialFields( type ) ) {
        PTreeWarnExpr( arg, warning );
    }
    return( true );
}


static bool convertEllipsisArg( // CONVERT AN ELLIPSIS (...) ARGUMENT
    PTREE arg )                 // - argument
{
    bool ok;                    // - return: true ==> ok
    PTREE right;                // - argument
    PTREE afun;                 // - &[ function ]
    TYPE type;                  // - node type

    switch( NodeAddrOfFun( PTreeOpRight( arg ), &afun ) ) {
    case ADDR_FN_MANY :
    case ADDR_FN_MANY_USED :
        PTreeErrorExpr( arg->u.subtree[1], ERR_ELLIPSE_ADDR_OVERLOAD );
        ok = false;
        break;
    default :
        right = NodeRvalue( arg->u.subtree[1] );
        arg->u.subtree[1] = right;
        type =  TypedefModifierRemove( right->type );
        switch( type->id ) {
        case TYP_CHAR :
        case TYP_SCHAR :
        case TYP_UCHAR :
        case TYP_SSHORT :
        case TYP_WCHAR :
        case TYP_USHORT :
            type = TypeUnArithResult( type );
            right = NodeConvert( type, right );
            arg_finish( right, arg );
            ok = true;
            break;
        case TYP_FLOAT :
            type = GetBasicType( TYP_DOUBLE );
            right = NodeConvert( type, right );
            arg_finish( right, arg );
            ok = true;
            break;
        case TYP_ARRAY :
            type = PointerTypeForArray( right->type );
            right = NodeConvert( type, right );
            arg_finish( right, arg );
            ok = true;
            break;
        case TYP_MEMBER_POINTER :
            ConvertMembPtrConst( &arg->u.subtree[1] );
            arg_fillout( arg );
            ok = true;
            break;
        case TYP_POINTER :
            if( NULL == FunctionDeclarationType( type->of ) ) {
                type_flag def_flags;
                type_flag act_flags;
                type_flag arg_flags;
                TYPE base_type;
                PTREE cnv;
                base_type = TypeGetActualFlags( type->of, &arg_flags );
                act_flags = arg_flags & TF1_MEM_MODEL;
                def_flags = DefaultMemoryFlag( type->of );
                if( ( (def_flags & TF1_FAR) && ( (act_flags & (TF1_HUGE | TF1_FAR)) == 0 ) )
                  || ( (def_flags & TF1_HUGE) && ( (act_flags & TF1_HUGE) == 0 ) )
                  ) {
                    type = MakeModifiedType( base_type, (arg_flags & ~TF1_MEM_MODEL) | def_flags );
                    type = MakePointerTo( type );
                    cnv = CastImplicit( arg->u.subtree[1]
                                      , type
                                      , CNV_EXPR
                                      , NULL );
                    arg->u.subtree[1] = cnv;
                    DbgVerify( PT_ERROR != cnv->op, "convertEllipsisArg -- failed ptr.cnv" );
                    arg_fillout( arg );
                    ok = true;
                } else {
                    arg_fillout( arg );
                    ok = true;
                }
            } else {
                arg_fillout( arg );
                ok = true;
            }
            break;
        case TYP_CLASS :
            ok = passStructOnStack( arg, WARN_ELLIPSIS_CLASS_ARG );
            break;
        default :
            arg_fillout( arg );
            ok = true;
            break;
        }
        break;
    }
    return( ok );
}


PTREE NodeConvertCallArgList(   // CONVERT CALL ARGUMENT LIST, AS REQ'D
    PTREE call_expr,            // - call expression (for errors only)
    unsigned num_args_caller,   // - # args, caller
    TYPE type,                  // - function type
    PTREE *args )               // - addr( caller argument nodes )
{
    PTREE arg;                  // - caller argument nodes
    arg_list *plist;            // - prototype arguments
    unsigned i;                 // - # args, processed
    unsigned num_args_prototype;// - # args, prototype
    TYPE *pptr;                 // - prototype type ptr.
    TYPE proto;                 // - prototype arg. type
    bool extern_c_fun;          // - true ==> extern "C" function
    TEMP_TYPE old;              // - old default class for temp.s

    if( call_expr != NULL && call_expr->op != PT_ERROR && num_args_caller > 0 ) {
        old = TemporaryClass( TEMP_TYPE_EXPR );
        plist = TypeArgList( type );
        pptr = plist->type_list;
        num_args_prototype = plist->num_args;
        type = FunctionDeclarationType( type );
        if( TypeHasEllipsisArg( type ) ) {
            for( i = 1; i <= num_args_caller; ++i ) {
                arg = PTreeOp( args );
                if( i < num_args_prototype ) {
                    if( !arg_convert( arg, *pptr++ ) ) {
                        PTreeErrorNode( call_expr );
                        break;
                    }
                } else if( !convertEllipsisArg( arg ) ) {
                    PTreeErrorNode( call_expr );
                    break;
                }
                args = &arg->u.subtree[0];
            }
        } else {
            if( type->flag & TF1_PLUSPLUS ) {
                extern_c_fun = false;
            } else {
                extern_c_fun = true;
            }
            for( i = 1; i <= num_args_caller; ++i ) {
                TYPE cl_type;
                arg = PTreeOp( args );
                proto = *pptr++;
                if( !arg_convert( arg, proto ) ) {
                    PTreeErrorNode( call_expr );
                    break;
                }
                cl_type = ClassType( proto );
                if( NULL != cl_type ) {
                    if( extern_c_fun ) {
                        if( !passStructOnStack( arg, WARN_EXTERN_C_CLASS_ARG ) ) {
                            PTreeErrorNode( call_expr );
                            break;
                        }
                    } else if( OMR_CLASS_VAL == ObjModelArgument( cl_type ) ) {
                        passStructOnStack( arg, ERR_CALL_WATCOM );
                    }
                }
                args = &arg->u.subtree[0];
            }
        }
        TemporaryClass( old );
    }
    return( call_expr );
}


#if _CPU == _AXP
static bool canCoaxVAStartSym( PTREE *parg )
{
    PTREE orig_arg;
    PTREE arg;

    /*
        The reason we need this complexity is that there exists C++ code
        that uses class templates instantiated on reference types but the
        class contains member functions that have ", T v0, ... )" so we
        have to forge ahead generating (possibly incorrect code) because
        the inline function must be compiled but will never be called!
        see COOL\PROPERTY.CPP
    */
    orig_arg = PTreeOp( parg );
    arg = orig_arg;
    if( NodeIsUnaryOp( arg, CO_FETCH ) ) {
        arg = arg->u.subtree[0];
    }
    if( NodeIsUnaryOp( arg, CO_RARG_FETCH ) ) {
        arg = arg->u.subtree[0];
    }
    if( arg->op != PT_SYMBOL ) {
        return( false );
    }
    *parg = NodeComma( orig_arg, PTreeAssign( NULL, arg ) );
    return( true );
}

static PTREE convertVaStart(    // CONVERT va_start function call
    PTREE expr,                 // - expression
    arg_list *alist,            // - arguments for caller
    TYPE type )                 // - function type
{
    PTREE arg;
    arg_list *plist;            // - prototype arguments
    PTREE right;                // - right operand

    if( alist->num_args != 3 ) {
        return( expr );
    }
    plist = TypeArgList( type );
    // va_list symbol
    arg = expr->u.subtree[1];
    if( ! arg_convert( arg, plist->type_list[0] ) ) {
        PTreeErrorNode( expr );
        return( expr );
    }
    // second argument -- must be parameter to current function
    // - here we check for symbol
    // - we check that it is a parameter in transformVaStart
    arg = arg->u.subtree[0];
    right = PTreeOp( &arg->u.subtree[1] );
    if( right->op != PT_SYMBOL ) {
        if( ! canCoaxVAStartSym( &arg->u.subtree[1] ) ) {
            PTreeErrorExpr( expr, ERR_INVALID_VASTART_SYMBOL );
            return( expr );
        }
    }
    // third argument -- must be 0 or one
    arg = arg->u.subtree[0];
    right = PTreeOp( &arg->u.subtree[1] );
    if( right->op != PT_INT_CONSTANT ) {
        PTreeErrorExpr( expr, ERR_INVALID_VASTART_SYMBOL );
        return( expr );
    }
    return( expr );
}


static PTREE transformVaStart   // TRANSFORM TO CO_VASTART OPCODE
    ( PTREE expr )              // - va_start expression
{
    SYMBOL pre_ellipsis_sym;
    SYMBOL stop;
    SYMBOL curr;
    SYMBOL fn;
    SCOPE caller_arg_scope;
    unsigned offset;
    target_size_t arg_size;
    PTREE arg1;
    PTREE arg2;
    PTREE arg3;
    PTREE valist;

    // second argument -- must be pre-... parameter
    arg1 = expr->u.subtree[1];
    arg2 = arg1->u.subtree[0];
    arg3 = arg2->u.subtree[0];
    pre_ellipsis_sym = PTreeOp( &arg2->u.subtree[1] )->u.symcg.symbol;
    caller_arg_scope = ScopeFunctionScopeInProgress();
    fn = ScopeFunction( caller_arg_scope );
    offset = 0;
    if( ObjModelFunctionReturn( fn->sym_type ) == OMR_CLASS_VAL ) {
        offset += TARGET_PACKING;
    }
    if( SymIsThisMember( fn ) ) {
        offset += TARGET_PACKING;
        if( SymCDtorExtraParm( fn ) ) {
            offset += TARGET_PACKING;
        }
    }
    stop = ScopeOrderedStart( caller_arg_scope );
    for( curr = NULL; (curr = ScopeOrderedNext( stop, curr )) != NULL; ) {
        if( ObjModelArgument( curr->sym_type ) == OMR_CLASS_REF ) {
            arg_size = TARGET_PACKING;
        } else {
            arg_size = CgMemorySize( curr->sym_type );
            arg_size = _RoundUp( arg_size, TARGET_PACKING );
        }
        offset += arg_size;
        if( curr == pre_ellipsis_sym ) {
            break;
        }
    }
    if( curr == NULL || ScopeOrderedNext( stop, curr ) != NULL ) {
        PTreeErrorExpr( expr, ERR_INVALID_VASTART_SYMBOL );
        return( expr );
    }
    // third argument -- va_list symbol
    valist = arg1->u.subtree[1];
    arg1->u.subtree[1] = NULL;
    if( arg3->u.subtree[1]->u.int_constant == 0 ) {
        // compensate for "void *__alist;" arg in <varargs.h>
        offset -= TARGET_PACKING;
    }
    NodeFreeDupedExpr( expr );
    expr = NodeBinary( CO_VASTART, valist, NodeOffset( offset ) );
    return( expr );
}
#endif


static bool adjustForVirtualCall(   // ADJUSTMENTS FOR POSSIBLE VIRTUAL CALL
    PTREE *node_this,           // - addr[ "this" node ]
    PTREE *routine,             // - routine to be called
    SEARCH_RESULT *result )     // - search result for routine
{
    SYMBOL sym;                 // - symbol for call
    bool ok;                    // - return: true ==> adjusted for virtual
    TYPE this_type;             // - target type for "this"
    PTREE expr;                 // - transformed expression
    bool exact_call;            // - true ==> this node is exact

    expr = *node_this;
    this_type = NodeType( expr );
    this_type = ClassType( this_type );
    if( this_type != NULL ) {
        if( OMR_CLASS_VAL == ObjModelArgument( this_type ) ) {
            expr = NodeAssignTemporary( this_type, expr );
        } else {
            expr = NodeConvert( MakePointerTo( expr->type ), expr );
        }
        *node_this = expr;
    }
    sym = (*routine)->u.symcg.symbol;
    this_type = TypeThisForCall( expr, sym );
    /* virtual calls don't have to check for NULL pointers when they convert */
    expr->flags |= PTF_PTR_NONZERO;
    exact_call = ( (expr->flags & PTF_MEMORY_EXACT) != 0 );
    NodeConvertToBasePtr( node_this, this_type, result, true );
    sym = SymDefaultBase( sym );
    if( SymIsVirtual( sym ) && ((*routine)->flags & PTF_COLON_QUALED) == 0 && !exact_call ) {
        expr = AccessVirtualFnAddress( NodeDupExpr( node_this )
                                     , result
                                     , sym );
        expr->type = MakePointerTo( expr->type );
        *routine = NodeReplace( *routine, expr );
        ok = true;
    } else {
        NodeFreeSearchResult( *routine );
        ok = false;
    }
    return( ok );
}


static intrinsic_mapping* intrinsicMapping // GET INTRINSIC MAPPING
    ( SYMBOL sym )
{
    NAME name;
    intrinsic_mapping *curr;

    name = sym->name->name;
    for( curr = mapIntrinsic; ; ++curr ) {
        const char* iname = curr->name;
        if( iname == NULL ) {
            curr = NULL;
            break;
        }
        /* it would be nice to do a pointer compare but we would
           need to convert the strings to NAMEs first (AFS) */
        if( strcmp( NameStr( name ), iname ) == 0 ) {
            break;
        }
    }
    return( curr );
}


static PTREE insertArg(         // INSERT ARGUMENT BEFORE ARGUMENT SEQUENCE
    PTREE seq,                  // - argument sequence
    PTREE arg )                 // - argument
{
    if( seq == NULL ) {
        seq = arg;
    } else if( arg != NULL ) {
        arg->u.subtree[0] = seq;
        seq = arg;
    }
    return( seq );
}


static PTREE appendArg(         // APPEND ARGUMENT AFTER ARGUMENT SEQUENCE
    PTREE seq,                  // - argument sequence
    PTREE arg )                 // - argument
{
    PTREE last;                 // - last arg in sequence

    if( seq == NULL ) {
        seq = arg;
    } else if( arg != NULL ) {
        for( last = seq
           ; last->u.subtree[0] != NULL
           ; last = last->u.subtree[0] );
        last->u.subtree[0] = arg;
    }
    return( seq );
}


PC_CALLIMPL PcCallImpl(         // OBTAIN PC CALL IMPLEMENTATION
    TYPE ftype )                // - function type
{
    PC_CALLIMPL retn;           // - type of call implemented

    ftype = TypedefModifierRemove( ftype );
    if( TypeHasReverseArgs( ftype ) ) {
        if( ftype->flag & TF1_PLUSPLUS ) {
            retn = CALL_IMPL_REV_CPP;
        } else {
            retn = CALL_IMPL_REV_C;
        }
    } else if( TypeHasEllipsisArg( ftype ) ) {
        if( ftype->flag & TF1_PLUSPLUS ) {
            retn = CALL_IMPL_ELL_CPP;
        } else {
            retn = CALL_IMPL_ELL_C;
        }
    } else {
        if( ftype->flag & TF1_PLUSPLUS ) {
            retn = CALL_IMPL_CPP;
        } else {
            retn = CALL_IMPL_C;
        }
    }
    return( retn );
}


static PTREE insertRetnCopy(    // INSERT COPY OF RETURN ON STACK
    PTREE callexpr,             // - call expression
    PTREE temp )                // - optional temporary
{
    if( temp != NULL ) {
        PTreeOpLeft( callexpr )->flags |= PTF_MEMORY_EXACT;
        callexpr->flags |= PTF_MEMORY_EXACT;
        callexpr = NodeCopyClassObject( temp, callexpr );
    }
    return( callexpr );
}


static PTREE insertCppRetnArg(  // INSERT C++ RETURN ARGUMENT
    PTREE arglist,              // - argument list
    PTREE retnnode,             // - return node
    OMR return_kind )           // - kind of return
{
    if( return_kind == OMR_CLASS_REF ) {
        retnnode->flags |= PTF_ARG_RETURN;
        arglist = insertArg( arglist, NodeArg( retnnode ) );
    }
    return( arglist );
}


static PTREE insertCppRetnCopy( // INSERT C++ RETURN COPY
    PTREE callnode,             // - node for call
    PTREE retnnode,             // - return node
    OMR return_kind )           // - kind of return
{
    switch( return_kind ) {
    case OMR_CLASS_VAL :
        callnode = insertRetnCopy( callnode, retnnode );
        break;
    case OMR_CLASS_REF :
        callnode->flags |= PTF_CLASS_RVREF;
        break;
    }
    return( callnode );
}


static PTREE insertCDtor(       // INSERT CDTOR NODE INTO CALL LIST
    PTREE seq,                  // - argument sequence
    PTREE arg )                 // - argument
{
    PTREE* r_val;               // - reference[ arg value ]
    PTREE val;                  // - arg value

    if( arg != NULL ) {
        r_val = PTreeRefRight( arg );
        val = *r_val;
        if( val->op == PT_INT_CONSTANT ) {
            PTREE new_val = NodeIcUnsigned( IC_CDARG_VAL, val->u.uint_constant );
            new_val->type = val->type;
            *r_val = new_val;
            PTreeFree( val );
        }
    }
    return( insertArg( seq, arg ) );
}


PTREE CallArgsArrange(          // ARRANGE CALL ARGUMENTS
    TYPE ftype,                 // - function type
    PTREE callnode,             // - node for call
    PTREE userargs,             // - user arguments
    PTREE node_this,            // - this node
    PTREE cdtor,                // - cdtor node (ignored when node_this==NULL)
    PTREE retnnode )            // - return node (for struct return)
{
    PTREE arglist;              // - argument list under construction
    OMR return_kind;            // - type of return

    if( node_this != NULL ) {
        node_this->flags |= PTF_ARG_THIS;
    }
    arglist = NULL;
    return_kind = ObjModelFunctionReturn( ftype );
    switch( PcCallImpl( ftype ) ) {
    case CALL_IMPL_REV_C :
        return_kind = OMR_CLASS_VAL;
        /* fall through */
    case CALL_IMPL_REV_CPP :
        arglist = insertCDtor( node_this, cdtor );
        arglist = insertCppRetnArg( arglist, retnnode, return_kind );
        arglist = appendArg( userargs, arglist );
        break;
    case CALL_IMPL_ELL_C :
    case CALL_IMPL_C :
        return_kind = OMR_CLASS_VAL;
        /* fall through */
    case CALL_IMPL_ELL_CPP :
    case CALL_IMPL_CPP :
        arglist = insertCppRetnArg( userargs, retnnode, return_kind );
        arglist = insertCDtor( arglist, cdtor );
        arglist = insertArg( arglist, node_this );
        break;
    }
    callnode->u.subtree[1] = arglist;
    callnode = insertCppRetnCopy( callnode, retnnode, return_kind );
    return( callnode );
}

static void outputCallTriggeredWarning( PTREE expr, SYMBOL sym )
{
    MSG_NUM msg;
    NAME name;

    msg = ERR_NULL;
    name = sym->name->name;
    if( name == CppSpecialName( SPECIAL_NAME_BEGINTHREAD ) ) {
        msg = WARN_MUST_BE_MULTITHREADED;
    } else if( name == CppSpecialName( SPECIAL_NAME_BEGINTHREADEX ) ) {
        msg = WARN_MUST_BE_MULTITHREADED;
    }
    if( msg != ERR_NULL ) {
        switch( msg ) {
        case WARN_MUST_BE_MULTITHREADED:
            if( !CompFlags.bm_switch_used ) {
                PTreeErrorExprName( expr, msg, name );
            }
            break;
        DbgDefault( "unexpected message number" );
        }
    }
}

PTREE AnalyseCall(              // ANALYSIS FOR CALL
    PTREE expr,                 // - call expression
    CALL_DIAG *diagnostic )     // - diagnostics used for function problems
{
    PTREE *r_args;              // - reference( arguments )
    PTREE *r_func;              // - reference( function )
    PTREE *ptlist;              // - nodes for arguments
    PTREE left;                 // - left operand ( the function )
    PTREE right;                // - right operand ( the arguments )
    PTREE node_this;            // - node for "this" computation
    PTREE deref_args;           // - member pointer dereference args
    PTREE last_arg;             // - last argument
    PTREE static_fn_this;       // - "this" for a static member
    PTREE templ_args;           // - explicit template arguments
    SYMBOL sym;                 // - function symbol
    SYMBOL caller_sym;          // - function that is doing the call
    TYPE type;                  // - temporary type
    TYPE fn_type;               // - function type
    type_flag fn_mod;           // - function modifier flags
    unsigned num_args;          // - # args, caller
    arg_list *alist;            // - arg_list for caller
    intrinsic_mapping *intr_map;// - mapping for intrinsic function
    SEARCH_RESULT *result;      // - searching result
    bool membptr_deref;         // - true ==> member pointer dereference
//    bool has_ellipsis;          // - true ==> ellipsis in argument list
    bool virtual_call;          // - true ==> virtual call
    TEMP_PT_LIST default_list;  // - default PTREE list
    TEMP_ARG_LIST default_args; // - default arg_list
    FNOV_DIAG fnov_diag;        // - diagnosis information;

    r_args = PTreeRefRight( expr );
    last_arg = *r_args;
    right = NodeReverseArgs( &num_args, last_arg );
    *r_args = right;
    r_func = PTreeRefLeft( expr );
    left = *r_func;
    membptr_deref = false;
    node_this = NULL;
    intr_map = NULL;
    static_fn_this = NULL;
    virtual_call = false;
    switch( left->cgop ) {
    case CO_DOT:
    case CO_ARROW:
        node_this = left->u.subtree[0];
        left->u.subtree[0] = NULL;
        left = NodePruneTop( left );
        *r_func = left;
        r_func = PTreeRefLeft( expr );
        left = *r_func;
        if( ( left->op == PT_ID ) && ( left->cgop == CO_NAME_DTOR ) ) {
            /* dtor of a non-class type */
            left = NodePruneTop( *r_func );
            /* NYI: verify dtor call has no arguments */
            expr->u.subtree[0] = NULL;
            NodeFreeDupedExpr( expr );
            expr = NodeConvert( GetBasicType( TYP_VOID ), node_this );
            expr = NodeComma( expr, left );
            return( expr );
        }
        break;
    case CO_CALL_EXEC_IND:
        if( left->flags & PTF_CALLED_ONLY ) {
            /* member pointer dereference being called */
            deref_args = left->u.subtree[1];
            node_this = NodeDupExpr( &(deref_args->u.subtree[1]) );
            membptr_deref = true;
        }
        break;
    }
    alist = ArgListTempAlloc( &default_args, num_args );
    ptlist = PtListAlloc( default_list, num_args );
    NodeBuildArgList( alist, ptlist, right, num_args );
    if( node_this == NULL ) {
        alist->qualifier = FunctionThisQualifier();
    } else {
        alist->qualifier = BaseTypeClassFlags( NodeType( node_this ) );
    }

    if( NodeIsBinaryOp( left, CO_TEMPLATE ) ) {
        DbgAssert( left->u.subtree[0]->op == PT_SYMBOL );

        templ_args = left->u.subtree[1];

        left->u.subtree[1] = NULL;
        left = NodePruneTop( left );
        *r_func = left;
        r_func = PTreeRefLeft( expr );
        left = *r_func;
    } else {
        templ_args = NULL;
    }

    if( left->op == PT_SYMBOL ) {
        FNOV_RESULT ovret;
        SYMBOL orig;        // - original symbol
        sym = left->u.symcg.symbol;
        orig = sym;
        if( left->cgop == CO_NAME_CONVERT ) {
            ovret = UdcOverloadedDiag( &sym
                                 , left->u.symcg.result
                                 , sym
                                 , SymFuncReturnType( sym )
                                 , alist->qualifier
                                 , &fnov_diag );
        } else {
            ovret = FuncOverloadedDiag( &sym
                                   , left->u.symcg.result
                                   , sym
                                   , alist
                                   , ptlist
                                   , templ_args
                                   , &fnov_diag );
        }

        switch( ovret ) {
        case FNOV_AMBIGUOUS :
            CallDiagAmbiguous( expr, diagnostic->msg_ambiguous, &fnov_diag );
            NodeFreeDupedExpr( node_this );
            ArgListTempFree( alist, num_args );
            PtListFree( ptlist, num_args );
            return( expr );
        case FNOV_NO_MATCH :
            if( node_this == NULL ) {
                if( SymIsThisFuncMember( orig ) ) {
                    node_this = NodeThisCopyLocation( left );
                }
            }
            if( node_this != NULL ) {
                if( ( ! SymIsCtor( orig ) )
                  &&( ! SymIsDtor( orig ) )
                  &&( CNV_OK != AnalysePtrCV
                                ( node_this
                                , TypeThisSymbol( orig, (node_this->flags & PTF_LVALUE) != 0 )
                                , NodeType( node_this )
                                , CNV_FUNC_THIS ) ) ) {
                    PTreeErrorNode( expr );
                    InfSymbolDeclaration( orig );
                    NodeFreeDupedExpr( node_this );
                    ArgListTempFree( alist, num_args );
                    PtListFree( ptlist, num_args );
                    return( expr );
                }
            }
            CallDiagNoMatch( expr
                           , diagnostic->msg_no_match_one
                           , diagnostic->msg_no_match_many
                           , node_this
                           , orig
                           , &fnov_diag );
            NodeFreeDupedExpr( node_this );
            ArgListTempFree( alist, num_args );
            PtListFree( ptlist, num_args );
            return( expr );
        }
        FnovFreeDiag( &fnov_diag );
        left->u.symcg.symbol = sym;
        result = left->u.symcg.result;
        if( node_this == NULL ) {
            if( SymIsThisFuncMember( sym ) ) {
                if( result->use_this ) {
                    node_this = NodeThisCopyLocation( left );
                    if( node_this == NULL ) {
                        PTreeErrorExpr( expr, ERR_INVALID_NONSTATIC_ACCESS );
                        InfSymbolDeclaration( sym );
                        ArgListTempFree( alist, num_args );
                        PtListFree( ptlist, num_args );
                        return( expr );
                    }
                } else {
                    PTreeErrorExpr( expr, ERR_BARE_FUNCTION_ACCESS );
                    InfSymbolDeclaration( sym );
                    ArgListTempFree( alist, num_args );
                    PtListFree( ptlist, num_args );
                    return( expr );
                }
            }
        }
        if( ! AnalyseSymbolAccess( expr, left, node_this, &diagAccess ) ) {
            NodeFreeDupedExpr( node_this );
            ArgListTempFree( alist, num_args );
            PtListFree( ptlist, num_args );
            return( expr );
        }
        type = sym->sym_type;
        fn_type = TypeGetActualFlags( type, &fn_mod );
        if( fn_type->flag & TF1_INTRINSIC ) {
            intr_map = intrinsicMapping( sym );
            if( intr_map == NULL ) {
                outputCallTriggeredWarning( expr, sym );
            }
        }
        if( fn_mod & TF1_FAR16 ) {
            /* we are calling a far16 function */
            caller_sym = ScopeFunctionInProgress();
            caller_sym->flag |= SYMF_FAR16_CALLER;
        }
        left->type = type;
        if( node_this == NULL ) {
            if( SymIsThisFuncMember( sym ) ) {
                node_this = NodeThisCopyLocation( left );
            }
        } else {
            if( SymIsStaticFuncMember( sym ) ) {
#ifdef OLD_STATIC_MEMBER_ACCESS
                NodeFreeDupedExpr( node_this );
#else
                static_fn_this = node_this;
#endif
                node_this = NULL;
            }
        }
        if( node_this != NULL ) {
            TYPE pted;
            pted = TypePointedAtModified( node_this->type );
            if( pted == NULL ) {
                pted = node_this->type;
            }
            if( TypeTruncByMemModel( pted ) ) {
                if( SymIsCtor( sym ) ) {
                    PTreeErrorExpr( node_this, ERR_CTOR_OBJ_MEM_MODEL );
                } else if( SymIsDtor( sym ) ) {
                    PTreeErrorExpr( node_this, ERR_DTOR_OBJ_MEM_MODEL );
                } else {
                    PTreeErrorExpr( node_this, ERR_THIS_OBJ_MEM_MODEL );
                }
                InfSymbolDeclaration( sym );
                PTreeErrorNode( expr );
                NodeFreeDupedExpr( node_this );
                ArgListTempFree( alist, num_args );
                PtListFree( ptlist, num_args );
                NodeFreeDupedExpr( static_fn_this );
                return( expr );
            }
            if( adjustForVirtualCall( &node_this, r_func, result ) ) {
                virtual_call = true;
                expr->cgop = CO_CALL_EXEC_IND;
                left = VfunSetupCall( expr->u.subtree[0] );
                left = VfnDecorateCall( left, sym );
            } else {
                expr->cgop = CO_CALL_EXEC;
                left = NodeUnaryCopy( CO_CALL_SETUP, expr->u.subtree[0] );
                SymMarkRefed( sym );
            }
        } else {
            NodeFreeSearchResult( left );
            expr->cgop = CO_CALL_EXEC;
            left = NodeUnaryCopy( CO_CALL_SETUP, expr->u.subtree[0] );
            SymMarkRefed( sym );
        }
    } else {
        if( ! membptr_deref ) {
            /* i.e, p->foo() where foo is a pointer to a function */
            NodeFreeDupedExpr( node_this );
            node_this = NULL;
        }
        sym = NULL;
        left = expr->u.subtree[0];
        type = TypedefModifierRemove( left->type );
        if( type->id == TYP_POINTER ) {
            type = type->of;
        }
        fn_type = TypeGetActualFlags( type, &fn_mod );
        if( fn_mod & TF1_FAR16 ) {
            /* we are calling a far16 function */
            caller_sym = ScopeFunctionInProgress();
            caller_sym->flag |= SYMF_FAR16_CALLER;
        }
        if( ! TypeHasNumArgs( type, num_args ) ) {
            PTreeErrorExpr( expr, ERR_PARM_COUNT_MISMATCH_POINTER );
            CErr2p( INF_FUNCTION_TYPE, type );
            ArgListTempFree( alist, num_args );
            PtListFree( ptlist, num_args );
            NodeFreeDupedExpr( static_fn_this );
            return( expr );
        }
        expr->cgop = CO_CALL_EXEC_IND;
        left = VfunSetupCall( left );
    }
    expr->u.subtree[0] = left;
#if _CPU == _AXP
    if( intr_map != NULL && intr_map->cgop == CO_VASTART ) {
        expr = convertVaStart( expr, alist, type );
    } else {
        expr = NodeConvertCallArgList( expr, num_args, type, &expr->u.subtree[1] );
    }
#else
    expr = NodeConvertCallArgList( expr, num_args, type, &expr->u.subtree[1] );
#endif
    if( expr->op != PT_ERROR ) {
        TYPE ftype;             // - function type
        PTREE cdtor;            // - CDTOR node
        PTREE callnode;         // - call node
        PTREE retnnode;         // - return node (for struct return)

        callnode = expr;
        cdtor = NULL;
        if( node_this != NULL ) {
            node_this = NodeArg( node_this );
            if( virtual_call ) {
                node_this->flags |= PTF_ARG_THIS_VFUN;
            }
            if( sym != NULL && SymIsDtor( sym ) ) {
                cdtor = MakeNodeCDtorArg( DTOR_NULL );
            }
        }
        ftype = type;
        type = TypedefModifierRemove( type );
//        has_ellipsis = TypeHasEllipsisArg( type );
        type = type->of;
        {
            TYPE tgt = TypeReference( type );
            if( tgt == NULL ) {
                expr->type = type;
            } else {
                expr->type = tgt;
                expr->flags |= PTF_LVALUE;
            }
        }
        if( sym != NULL ) {
            if( ! AddDefaultArgs( sym, expr ) ) {
                NodeFreeDupedExpr( cdtor );
                NodeFreeDupedExpr( node_this );
                ArgListTempFree( alist, num_args );
                PtListFree( ptlist, num_args );
                return( expr );
            }
        }
        if( NULL != TypeReference( type ) ) {
            expr->flags |= PTF_LVALUE;
        }
        if( OMR_CLASS_REF == ObjModelArgument( type ) ) {
            retnnode = NodeTemporary( type );
            retnnode = PTreeCopySrcLocation( retnnode, expr );
        } else {
            retnnode = NULL;
        }
        expr = CallArgsArrange( ftype
                              , callnode
                              , callnode->u.subtree[1]
                              , node_this
                              , cdtor
                              , retnnode );
        if( retnnode != NULL ) {
            expr = NodeDtorExpr( expr, retnnode->u.symcg.symbol );
            if( SymRequiresDtoring( retnnode->u.symcg.symbol ) ) {
                expr = PtdCtoredExprType( expr, NULL, type );
            }
        }
        type = ClassType( type );
        if( type != NULL && ! TypeDefined( type ) ) {
            PTreeErrorExpr( expr, ERR_RETURN_UNDEFD_TYPE );
        }
        if( intr_map != NULL && expr->op != PT_ERROR ) {
#if _CPU == _AXP
            if( intr_map->cgop == CO_VASTART ) {
                expr = transformVaStart( expr );
            } else {
                expr = PTreeIntrinsicOperator( expr, intr_map->cgop );
            }
#else
            expr = PTreeIntrinsicOperator( expr, intr_map->cgop );
#endif
            expr->flags |= PTF_MEANINGFUL | PTF_SIDE_EFF;
        }
    }
    if( static_fn_this != NULL ) {
        expr = NodeCommaIfSideEffect( static_fn_this, expr );
    }
    ArgListTempFree( alist, num_args );
    PtListFree( ptlist, num_args );
    return( expr );
}

PTREE AnalyseDtorCall(          // ANALYSIS FOR SPECIAL DTOR CALLS
    TYPE class_type,            // - class to be destructed
    PTREE node_this,            // - expression for address of class
    int dtor_parm_code )        // - constant for extra second parm
{
    SEARCH_RESULT *result;      // - search results
    SYMBOL dtor_sym;            // - DTOR symbol
    PTREE dtor_id;              // - node for dtor symbol
    PTREE expr;                 // - expression under construction
    TYPE return_type;           // - type of return
    bool virtual_call;          // - true ==> virtual dtor
    TOKEN_LOCN err_locn;        // - error location

    /* assumes class_type->u.c.info->needs_dtor is true */
    result = DtorFindResult( class_type );
    PTreeExtractLocn( node_this, &err_locn );
    ScopeResultErrLocn( result, &err_locn );
    dtor_sym = result->sym_name->name_syms;
    ScopeCheckSymbol( result, dtor_sym );
    dtor_id = NodeSymbolCallee( PTreeAlloc(), dtor_sym, result );
    virtual_call = adjustForVirtualCall( &node_this, &dtor_id, result );
    return_type = SymFuncReturnType( dtor_sym );
    expr = makeCall( dtor_id, return_type, NULL, ! virtual_call );
    node_this = NodeArg( node_this );
    if( virtual_call ) {
        node_this->flags |= PTF_ARG_THIS_VFUN;
    }
    expr = CallArgsArrange( dtor_sym->sym_type
                          , expr
                          , NULL
                          , node_this
                          , MakeNodeCDtorArg( dtor_parm_code )
                          , NULL );
    if( virtual_call ) {
        expr->u.subtree[0] = VfnDecorateCall( expr->u.subtree[0], dtor_sym );
    }
    expr->flags |= PTF_MEANINGFUL | PTF_SIDE_EFF;
    return( expr );
}

#if 0 // not ref'ed
PTREE MakeDeleteCall(   // MAKE A CALL TO 'OPERATOR DELETE'
    SYMBOL del_sym,     // - symbol for 'op del'
    PTREE ptr,          // - parm 1 for delete
    TYPE class_parm )   // - if non-NULL, use size as second parm
{
    PTREE size_arg;     // - size argument for call
    PTREE args;         // - arguments for call
    PTREE expr;         // - final expression
    arg_list *del_args; // - op del arguments

    del_args = SymFuncArgList( del_sym );
    args = NULL;
    if( class_parm != NULL ) {
        size_arg = NodeOffset( class_parm->u.c.info->size );
        size_arg = NodeConvert( del_args->type_list[1], size_arg );
        args = NodeArgument( args, size_arg );
    }
    ptr = NodeConvert( del_args->type_list[0], ptr );
    args = NodeArgument( args, ptr );
    expr = NodeMakeCall( del_sym, GetBasicType( TYP_VOID ), NULL );
    expr = CallArgsArrange( del_sym->sym_type
                          , expr
                          , args
                          , NULL
                          , NULL
                          , NULL );
    return( expr );
}
#endif
