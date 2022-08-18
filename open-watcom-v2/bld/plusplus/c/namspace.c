/****************************************************************************
*
*                            Open Watcom Project
*
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
#include "ptree.h"
#include "name.h"
#include "namspace.h"
#include "cgfront.h"
#include "template.h"
#include "ring.h"
#include "class.h"

typedef enum {
    FNS_LEXICAL = 0x01,
    FNS_NULL    = 0x00
} find_ns;

static SYMBOL injectNameSpaceSym( SYMBOL sym
                                , NAME_SPACE *ns
                                , SCOPE scope
                                , NAME name
                                , TOKEN_LOCN *locn )
{
    SYMBOL ret_sym;

    if( sym == NULL ) {
        sym = AllocSymbol();
    }
    sym->id = SYMC_NAMESPACE;
    sym->u.ns = ns;
    sym->sym_type = MakeNamespaceType();
    SymbolLocnDefine( locn, sym );
    ret_sym = ScopeInsert( scope, sym, name );
    DbgAssert(( ret_sym == sym )||( ret_sym == NULL )||( ret_sym->id != SYMC_NAMESPACE ));
    return( ret_sym );
}

static SYMBOL previousNSSym( NAME name, SCOPE host_scope, find_ns control )
{
    SEARCH_RESULT *result;
    SYMBOL_NAME sym_name;
    SYMBOL sym;

    if( control & FNS_LEXICAL ) {
        result = ScopeFindLexicalNameSpace( host_scope, name );
    } else {
        result = ScopeContainsNaked( host_scope, name );
    }
    if( result != NULL ) {
        sym_name = result->sym_name;
        ScopeFreeResult( result );
        sym = sym_name->name_type;
        if( sym != NULL ) {
            if( SymIsNameSpace( sym ) ) {
                return( sym );
            }
        }
    }
    return( NULL );
}

static void openNameSpaceSym( NAME name, TOKEN_LOCN *locn )
{
    NAME ns_name;
    SCOPE inject_scope;
    SCOPE scope;
    SYMBOL ns_sym;

    if( name == NULL ) {
        ns_name = ScopeUnnamedNamespaceName( locn );
    } else {
        ns_name = name;
    }
    inject_scope = GetCurrScope();
    if( ScopeId( inject_scope ) != SCOPE_FILE ) {
        CErr1( ERR_NAMESPACE_MUST_BE_GLOBAL );
        inject_scope = ScopeNearestFile( inject_scope );
    }
    ns_sym = previousNSSym( ns_name, inject_scope, FNS_NULL );
    if( ns_sym != NULL ) {
        SCOPE ns_scope = ns_sym->u.ns->scope;
        ScopeOpen( ns_scope );
        ScopeRestoreUsing( ns_scope, false );
    } else {
        ns_sym = AllocSymbol();
        scope = ScopeOpenNameSpace( name, ns_sym );
        injectNameSpaceSym( ns_sym, scope->owner.ns, inject_scope, ns_name, locn );
    }
}

void NameSpaceUnnamed( TOKEN_LOCN *locn )
/***************************************/
{
    SCOPE old_curr;
    SCOPE save_curr;

    old_curr = GetCurrScope();
    openNameSpaceSym( NULL, locn );
    save_curr = GetCurrScope();
    SetCurrScope( old_curr );
    ScopeAddUsing( save_curr, old_curr );
    SetCurrScope( save_curr );
}

void NameSpaceNamed( PTREE id )
/*****************************/
{
    NAME name;

    name = id->u.id.name;
    openNameSpaceSym( name, &(id->locn) );
    PTreeFree( id );
}

void NameSpaceClose( void )
/*************************/
{
    ScopeEnd( SCOPE_FILE );
}

static SCOPE getSearchScope( PTREE id, PTREE *find_id, find_ns *pcontrol )
{
    PTREE scope_tree;
    SCOPE find_scope;
    SCOPE test_scope;
    find_ns control;

    find_scope = GetCurrScope();
    control = FNS_LEXICAL;
    *find_id = id;
    if( id->op == PT_BINARY ) {
        DbgAssert( id->cgop == CO_STORAGE );
        *find_id = id->u.subtree[1];
        scope_tree = id->u.subtree[0];
        DbgAssert( scope_tree != NULL );
        scope_tree = scope_tree->u.subtree[1];
        if( scope_tree != NULL ) {
            DbgAssert( scope_tree->op == PT_ID );
            test_scope = scope_tree->u.id.scope;
            if( test_scope != NULL ) {
                find_scope = test_scope;
            }
        } else {
            find_scope = GetFileScope();
        }
        control = FNS_NULL;
    }
    *pcontrol = control;
    DbgAssert( find_scope != NULL );
    return( find_scope );
}

void NameSpaceAlias( PTREE to_id, PTREE from_id )
/***********************************************/
{
    NAME alias_name;
    PTREE id;
    SCOPE find_scope;
    NAME_SPACE *ns;
    NAME_SPACE *prev_ns;
    SYMBOL ns_sym;
    SYMBOL prev_ns_sym;
    find_ns control;

    find_scope = getSearchScope( from_id, &id, &control );
    ns_sym = previousNSSym( id->u.id.name, find_scope, control );
    if( ns_sym != NULL ) {
        ns = ns_sym->u.ns;
        alias_name = to_id->u.id.name;
        prev_ns_sym = previousNSSym( alias_name, GetCurrScope(), FNS_NULL );
        if( prev_ns_sym == NULL ) {
            injectNameSpaceSym( NULL, ns, GetCurrScope(), alias_name, &(to_id->locn) );
        } else {
            prev_ns = prev_ns_sym->u.ns;
            if( ns != prev_ns ) {
                PTreeErrorExpr( from_id, ERR_NAMESPACE_ALIAS_DIFFERENT );
            }
        }
    } else {
        PTreeErrorExpr( from_id, ERR_NAME_DOESNT_REF_NAMESPACE );
    }
    PTreeFreeSubtrees( to_id );
    PTreeFreeSubtrees( from_id );
}

void NameSpaceUsingDirective( PTREE ns_id )
/*****************************************/
{
    PTREE id;
    SCOPE find_scope;
    SYMBOL ns_sym;
    find_ns control;

    find_scope = getSearchScope( ns_id, &id, &control );
    ns_sym = previousNSSym( id->u.id.name, find_scope, control );
    if( ns_sym != NULL ) {
        ScopeAddUsing( ns_sym->u.ns->scope, NULL );
    } else {
        PTreeErrorExpr( ns_id, ERR_NAME_DOESNT_REF_NAMESPACE );
    }
    PTreeFreeSubtrees( ns_id );
}

static void nameSpaceUsingDecl( SYMBOL ns_sym, TOKEN_LOCN *locn )
{
    injectNameSpaceSym( NULL
                      , ns_sym->u.ns
                      , GetCurrScope()
                      , ns_sym->name->name
                      , locn );
}

static SEARCH_RESULT *lookupUsingId( PTREE using_id )
{
    SCOPE scope;
    SCOPE disambig;
    PTREE left;
    PTREE right;
    NAME name;
    SEARCH_RESULT *result;

    if( NodeIsBinaryOp( using_id, CO_COLON_COLON ) ) {
        left = using_id->u.subtree[0];
        if( left == NULL ) {
            scope = GetFileScope();
        } else {
            DbgAssert( left->op == PT_TYPE );
            scope = left->u.type.scope;
            DbgAssert( scope != NULL );
        }
        right = using_id->u.subtree[1];
    } else {
        DbgAssert( using_id->op == PT_ID );
        right = using_id;
        scope = GetFileScope();
    }
    disambig = scope;
    DbgAssert( right != NULL && right->op == PT_ID );
    name = right->u.id.name;
    DbgAssert( scope != NULL && name != NULL );
    if( ScopeId( scope ) == SCOPE_CLASS ) {
        if( right->cgop == CO_NAME_CONVERT ) {
            result = ScopeFindScopedMemberConversion( scope
                                                    , disambig
                                                    , right->type
                                                    , TF1_NULL );
        } else {
            result = ScopeFindScopedMember( scope, disambig, name );
        }
    } else {
        DbgAssert( ScopeId( scope ) == SCOPE_FILE );
        result = ScopeFindScopedNaked( scope, disambig, name );
    }
    if( result == NULL ) {
        PTreeErrorExprName( using_id, ERR_UNDECLARED_SYM, name );
        PTreeFreeSubtrees( using_id );
    }
    return( result );
}

static bool verifyUsingDecl( SCOPE scope )
{
    bool error_occurred;

    if( scope == NULL ) {
        return( true );
    }
    error_occurred = false;
    if( ScopeId( GetCurrScope() ) == SCOPE_CLASS ) {
        if( ScopeId( scope ) != SCOPE_CLASS ) {
            CErr1( ERR_MEMBER_USING_DECL_REFS_NON_MEMBER );
            error_occurred = true;
        } else {
            derived_status is_a_base = DERIVED_YES;
            if( ScopeDerived( GetCurrScope(), scope ) == DERIVED_NO ) {
                is_a_base = DERIVED_NO;
            } else if( GetCurrScope() == scope ) {
                is_a_base = DERIVED_NO;
            }
            if( is_a_base == DERIVED_NO ) {
                CErr2p( ERR_USING_DECL_NOT_A_BASE_CLASS, ScopeClass( scope ) );
                error_occurred = true;
            }
        }
    } else {
        DbgAssert( ScopeId( GetCurrScope() ) == SCOPE_FILE || ScopeId( GetCurrScope() ) == SCOPE_BLOCK );
        if( ScopeId( scope ) == SCOPE_CLASS ) {
            CErr1( ERR_USING_DECL_REFS_MEMBER );
            error_occurred = true;
        } else {
            if( GetCurrScope() == scope ) {
                CErr1( ERR_USING_DECL_NAME_SAME );
                error_occurred = true;
            }
        }
    }
    return( error_occurred );
}

static void injectNameSpaceFns( SYMBOL from, SYMBOL to, TOKEN_LOCN *locn )
{
    SYMBOL curr;

    RingIterBegFrom( from, curr ) {
        SymMakeAlias( curr, locn );
    } RingIterEndTo( curr, to )
}

static void functionUsingDecl( SEARCH_RESULT *result, SYMBOL name_syms, TOKEN_LOCN *locn )
{
    SYM_REGION *head;
    SYM_REGION *curr;

    if( ScopeId( result->scope ) == SCOPE_CLASS ) {
        DbgAssert( ScopeId( GetCurrScope() ) == SCOPE_CLASS );
        return;
    }
    ScopeCheckSymbol( result, name_syms );
    head = result->region;
    if( head != NULL ) {
        RingIterBeg( head, curr ) {
            injectNameSpaceFns( curr->from, curr->to, locn );
        } RingIterEnd( curr )
    } else {
        injectNameSpaceFns( RingFirst( name_syms ), RingLast( name_syms ), locn );
    }
}

static void varUsingDecl( SEARCH_RESULT *result, SYMBOL name_syms, TOKEN_LOCN *locn )
{
    if( ScopeId( result->scope ) == SCOPE_CLASS ) {
        DbgAssert( ScopeId( GetCurrScope() ) == SCOPE_CLASS );
        return;
    }
    if( ScopeCheckSymbol( result, name_syms ) ) {
        return;
    }
    SymMakeAlias( name_syms, locn );
}

void NameSpaceUsingDeclId( PTREE using_id )
/*****************************************/
{
    SCOPE sym_scope;
    SYMBOL_NAME sym_name;
    SEARCH_RESULT *result;
    TOKEN_LOCN id_locn;
    SYMBOL name_type;
    SYMBOL name_syms;

    result = lookupUsingId( using_id );
    if( result == NULL ) {
        /* using_id has been freed for this case */
        return;
    }
    sym_scope = result->scope;
    if( !verifyUsingDecl( sym_scope ) ) {
        PTreeExtractLocn( using_id, &id_locn );
        ScopeResultErrLocn( result, &id_locn );
        sym_name = result->sym_name;
        name_type = sym_name->name_type;
        name_syms = sym_name->name_syms;
        if( name_type != NULL ) {
            switch( name_type->id ) {
            case SYMC_NAMESPACE:
                nameSpaceUsingDecl( name_type, &id_locn );
                break;
            case SYMC_CLASS_TEMPLATE:
                TemplateUsingDecl( name_type, &id_locn );
                break;
            case SYMC_TYPEDEF:
                TypedefUsingDecl( NULL, name_type, &id_locn );
                break;
            DbgDefault( "unexpected storage class" );
            }
            if( name_syms == NULL ) {
                ScopeCheckSymbol( result, name_type );
            }
        }
        if( name_syms != NULL ) {
            if( ScopeId( result->scope ) == SCOPE_CLASS ) {
                DbgAssert( ScopeId( GetCurrScope() ) == SCOPE_CLASS );
                // NYI: full using-decl semantics within a class
                ClassAccessDeclaration( using_id, &id_locn );
                using_id = NULL;
            } else {
                if( SymIsFunction( name_syms ) ) {
                    functionUsingDecl( result, name_syms, &id_locn );
                } else {
                    varUsingDecl( result, name_syms, &id_locn );
                }
            }
        }
    }
    PTreeFreeSubtrees( using_id );
    ScopeFreeResult( result );
}

void NameSpaceUsingDeclType( DECL_SPEC *dspec )
/*********************************************/
{
    SCOPE type_scope;

    type_scope = dspec->scope;
    if( ! verifyUsingDecl( type_scope ) ) {
        TypedefUsingDecl( dspec, NULL, NULL );
    }
    PTypeRelease( dspec );
}

void NameSpaceUsingDeclTemplateName( PTREE tid )
/**********************************************/
{
    SYMBOL_NAME sym_name;
    TOKEN_LOCN id_locn;
    SYMBOL name_type;
//    SYMBOL name_syms;
    PTREE right;

    DbgAssert( NodeIsBinaryOp( tid, CO_STORAGE ) );

    right = tid->u.subtree[1];
    DbgAssert( ( right->op == PT_ID ) );

    PTreeExtractLocn( tid, &id_locn );
    sym_name = tid->sym_name;
    name_type = sym_name->name_type;
//    name_syms = sym_name->name_syms;
    if( name_type != NULL ) {
        switch( name_type->id ) {
        case SYMC_CLASS_TEMPLATE:
            TemplateUsingDecl( name_type, &id_locn );
            break;
        DbgDefault( "unexpected storage class" );
        }
    }

    PTreeFreeSubtrees( tid );
}
