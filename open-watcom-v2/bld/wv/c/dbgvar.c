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
* Description:  Evaluation and formatting of program variables.
*
****************************************************************************/


#include <limits.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgstk.h"
#include "dbgerr.h"
#include "dbglit.h"
#include "dbgitem.h"
#include "dlgvarx.h"
#include "dbgvar.h"
#include "dbgmem.h"
#include "wspawn.h"
#include "dui.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgexpr4.h"
#include "dbgexpr2.h"
#include "dbgexpr.h"
#include "dbgmain.h"
#include "dbgbrk.h"
#include "dbgprint.h"
#include "dbgparse.h"
#include "dbgprog.h"
#include "dipimp.h"
#include "dipinter.h"
#include "dbgreg.h"
#include "addarith.h"
#include "dbgevent.h"
#include "dbgupdt.h"

#include "clibext.h"

extern void             WndVarNewWindow( char *);
extern void             WndVarInspect( const char *);

bool                    VarError;
type_display            *TypeDisplay;

static int              TargRow;
static long             ExprStackTimeStamp;
static int              CurrRow;
static var_node         *VarFound;
static var_info         *TargVar;
static long             ScopeTimeStamp = 0;

static bool             FindField( sym_handle *field, var_node *vfield );

static var_type_bits    Hide;

/*
 *      This is all junk to remember how we display fields/structs
 */

static void VarNodeDisplayUpdate( var_node *v )
/*********************************************/
{
    var_node    *e;

    if( v->display_type != NULL ) {
        v->display = v->display_type->display;
    }
    VarNodeInvalid( v );
    for( e = v->expand; e != NULL; e = e->next ) {
        VarNodeDisplayUpdate( e );
    }
    for( e = v->old_expand; e != NULL; e = e->next ) {
        VarNodeDisplayUpdate( e );
    }
}

bool VarDisplayIsStruct( var_node *v )
/************************************/
{
    return( v->display_type != NULL && v->display_type->is_struct );
}

static var_type_bits *VarHidePtr( var_node *v )
/*********************************************/
{
    if( v != NULL && v->display_type != NULL ) {
        return( &v->display_type->hide );
    } else {
        return( &Hide );
    }
}

bool VarDisplayIsHidden( var_node *v, var_type_bits bit )
/********************************************************************/
{
    return( ( *VarHidePtr( v ) & bit ) != 0 );
}

void VarDisplaySetHidden( var_node *v, var_type_bits bit, bool on )
/*****************************************************************/
{
    if( on ) {
        *VarHidePtr( v ) |= bit;
    } else {
        *VarHidePtr( v ) &= ~bit;
    }
    DbgUpdate( UP_VAR_DISPLAY );
}

void VarDisplayFlipHide( var_node *v, var_type_bits bit )
/*******************************************************/
{
    var_type_bits       *hide = VarHidePtr( v );
    type_display        *alias;

    if( ( *hide & bit ) != 0 ) {
        *hide &= ~bit;
    } else {
        *hide |= bit;
    }
    if( v->display_type != NULL ) {
        alias = v->display_type;
        do {
            alias->hide = *hide;
            VarDisplayDirty( alias );
            alias = alias->alias;
        } while( alias != v->display_type );
    }
    DbgUpdate( UP_VAR_DISPLAY );
}

void VarDisplayUpdate( var_info *i )
/**********************************/
{
    scope_state *s;
    var_node    *v;

    for( s = i->s; s != NULL; s = s->outer ) {
        for( v = s->v; v != NULL; v = v->next ) {
            VarNodeDisplayUpdate( v );
        }
    }
}

bool VarDisplayShowMembers( var_info *i )
/***************************************/
{
    return( i->members );
}

void VarDisplaySetMembers( var_info *i, bool on )
/***********************************************/
{
    i->members = on;
}

void VarDisplaySetBits( var_node *v )
/***********************************/
{
    if( v->display_type != NULL ) {
        v->display = v->display_type->display;
    }
}

type_display *VarDisplayFindParent( type_display *curr )
/******************************************************/
{
    type_display        *alias;

    if( curr->parent != NULL )
        return( curr->parent );
    for( alias = curr->alias; alias != curr; alias = alias->alias ) {
        if( alias->parent != NULL ) {
            return( alias->parent );
        }
    }
    return( NULL );
}

void VarDisplayDirty( type_display *curr )
/****************************************/
{
    while( curr != NULL ) {
        if( curr->dirty )
            return;
        curr->dirty = true;
        curr = VarDisplayFindParent( curr );
    }
}

void VarDisplayInit( void )
/*************************/
{
    TypeDisplay = NULL;
}

bool VarDisplayedOnTop( var_node *v )
/***********************************/
{
    if( v->display_type == NULL )
        return( false );
    return( v->display_type->on_top );
}

void VarDisplayOnTop( var_node *v, bool on )
/******************************************/
{
    type_display *type = v->display_type;
    type_display *parent;
    type_display *alias;
    bool        has_top;

    if( type != NULL ) {
        VarDisplayDirty( type );
        type->on_top = on;
        parent = type->parent;
        if( parent != NULL ) {
            has_top = false;
            for( type = parent->fields; type != NULL; type = type->next ) {
                if( type->on_top ) {
                    has_top = true;
                }
            }
            parent->has_top = has_top;
            for( alias = parent->alias; alias != parent; alias = alias->alias ) {
                alias->has_top = has_top;
            }
        }
    }
    DbgUpdate( UP_VAR_DISPLAY );
}

static void VarDisplayFree( type_display *junk )
/**********************************************/
{
    type_display        *prev;

    for( prev = junk; prev->alias != junk; prev = prev->alias )
        ;
    prev->alias = junk->alias;
    DbgFree( junk );
}

void VarDisplayFini( void )
/*************************/
{
    type_display *curr, *next, *alias;
    type_display *fcurr, *fnext;

    for( curr = TypeDisplay; curr != NULL; curr = next ) {
        next = curr->next;
        for( fcurr = curr->fields; fcurr != NULL; fcurr = fnext ) {
            fnext = fcurr->next;
            VarDisplayFree( fcurr );
        }
        for( alias = curr->alias; alias != curr; alias = alias->alias ) {
            alias->fields = NULL; // so we don't free twice
        }
        VarDisplayFree( curr );
    }
    TypeDisplay = NULL;
}


static type_display *VarDisplayAddType( type_display **owner, const char *name )
/******************************************************************************/
{
    type_display        *new;

    new = DbgAlloc( sizeof( *new ) + strlen( name ) );
    memset( new, 0, sizeof( *new ) );
    strcpy( new->name, name );
    while( *owner != NULL )
        owner = &((*owner)->next);
    *owner = new;
    new->alias = new;
    new->hide = Hide;
    return( new );
}

type_display *VarDisplayAddStruct( const char *name )
/***************************************************/
{
    type_display        *new;

    for( new = TypeDisplay; new != NULL; new = new->next ) {
        if( strcmp( new->name, name ) == 0 ) {
            return( new );
        }
    }
    new = VarDisplayAddType( &TypeDisplay, name );
    new->is_struct = true;
    return( new );
}

static char *TagName( symbol_type tag )
/*************************************/
{
    switch( tag ) {
    case ST_STRUCT_TAG:
        return( "struct" );
    case ST_CLASS_TAG:
        return( "class" );
    case ST_UNION_TAG:
        return( "union" );
    case ST_ENUM_TAG:
        return( "enum" );
    default:
        return( NULL );
    }
}

static type_display *VarDisplayAddStructType( type_handle *th )
/*************************************************************/
{
    symbol_type         tag;
    int                 len;

    len = DIPTypeName( th, 0, &tag, TxtBuff, TXT_LEN );
    if( len == 0 ) {
        return( NULL );
    }
    return( VarDisplayAddStruct( TxtBuff ) );
}

type_display *VarDisplayAddField( type_display *parent, const char *name )
/************************************************************************/
{
    type_display        *new;
    type_display        *alias;

    for( new = parent->fields; new != NULL; new = new->next ) {
        if( strcmp( new->name, name ) == 0 ) {
            return( new );
        }
    }
    new = VarDisplayAddType( &parent->fields, name );
    for( alias = parent->alias; alias != parent; alias = alias->alias ) {
        alias->fields = parent->fields;
    }
    new->parent = parent;
    new->is_field = true;
    return( new );
}

static type_display *VarDisplayAddFieldSym( type_display *parent, sym_handle *field )
/***********************************************************************************/
{
    int         len;

    len = DIPSymName( field, NULL, SNT_SOURCE, TxtBuff, TXT_LEN );
    if( len == 0 )
        return( NULL );
    return( VarDisplayAddField( parent, TxtBuff ) );
}

void VarDisplayAlias( type_display *type, type_display *to )
/**********************************************************/
{
    type_display        *curr;

    // if already there, do nothing
    if( to == type || to == NULL )
        return;
    for( curr = type->alias; curr != type; curr = curr->alias ) {
        if( curr == to ) {
            return;
        }
    }
    // ok, it's a new one - hook it in
    curr = type->alias;
    type->alias = to->alias;
    to->alias = curr;
    type->fields = to->fields;
    type->has_top = to->has_top;
    type->is_struct = true;
}

static void VarDisplayAliasNode( var_node *v, type_display *to )
/**************************************************************/
{
    if( v->display_type == NULL ) {
        v->display_type = to;
    } else {
        VarDisplayAlias( v->display_type, to );
    }
}

char *VarDisplayType( var_node *v, char *buff, size_t buff_len )
/**************************************************************/
{
    size_t      len;
    size_t      tag_len;
    symbol_type tag;
    char        *tag_name;

    if( v->node_type == NODE_INHERIT ) {
        if( VarNodeExpr( v )[0] != NULLCHAR ) {
            return( StrCopy( VarNodeExpr( v ), buff ) );
        } else {
            StrCopy( LIT_ENG( Unknown_type ), buff );
            return( NULL );
        }
    }
    if( !v->have_type || ( len = DIPTypeName( v->th, 0, &tag, buff, buff_len ) ) == 0 ) {
        StrCopy( LIT_ENG( Unknown_type ), buff );
        return( NULL );
    }
    tag_name = TagName( tag );
    if( tag_name != NULL ) {
        tag_len = strlen( tag_name );
        if( len + tag_len < buff_len ) {
            memmove( buff+tag_len+1, buff, len+1 );
            *StrCopy( tag_name, buff ) = ' ';
        }
    }
    buff += strlen( buff );
    switch( VarGetGadget( v ) ) {
    case VARGADGET_BADPOINTS:
    case VARGADGET_UNPOINTS:
    case VARGADGET_POINTS:
        *buff++ = ' ';
        *buff++ = '*';
        *buff = NULLCHAR;
        break;
    }
    return( buff );
}

/*
   All the following confusion is so that we don't have to re-evaluate
   expressions from the root each time we want to get a window
   piece. between the calls to VarBegPaint and VarEndPaint, we know that
   nothing is going on except the painting of this window, so ExprSP is
   left where it is after VarGetLine.
*/


static  void    CheckExprStackTimeStamp( var_node *v, long exprsp_timestamp )
/*
        The var_node may be a saved structure from something that
        was expanded, collapsed, and expanded again. If it is, set it's
        fields to false so that it will be re-evaluated.
*/
{
    if( v->exprsp_timestamp != exprsp_timestamp ) {
        v->exprsp_timestamp = exprsp_timestamp;
        v->pushed = false;
        v->popped = false;
        v->buried = false;
    }
}


/*
    The expression processor likes to Suicide on us.  These routines
    prevent that
*/
static  int             VarErrStateCount;

bool            VarErrState( void )
{
    _SwitchOn( SW_ERR_IN_TXTBUFF );
    _SwitchOn( SW_AMBIGUITY_FATAL );
    _SwitchOn( SW_CALL_FATAL );
    _SwitchOn( SW_ERROR_PRESERVES_RADIX );
    ++VarErrStateCount;
    FreezeInpStack();
    return( true );
}

bool            VarOldErrState( void )
{
    if( --VarErrStateCount == 0 ) {
        _SwitchOff( SW_ERR_IN_TXTBUFF );
        _SwitchOff( SW_AMBIGUITY_FATAL );
        _SwitchOff( SW_CALL_FATAL );
        _SwitchOff( SW_ERROR_PRESERVES_RADIX );
    }
    PopInpStack();
    return( false );
}

static void     CheckRValue( void )
{
    DupStack();
    ExprValue( ExprSP );
    PopEntry();
}

static void     PushExpr( var_node *v )
{
    const char      *old;

    if( v->is_sym_handle ) {
        SetUpExpr( 1 );
        PushSymHandle( VarNodeHdl( v ) );
        CheckRValue();
    } else {
        old = ReScan( VarNodeExpr( v ) );
        EvalLValExpr( 1 );
        CheckRValue();
        ReScan( old );
    }
}


static void     PushField( sym_handle *field )
{
    DupStack();
    DoGivenField( field );
    CheckRValue();
}

void VarNodeInvalid( var_node *v )
{
    v->value_valid = false;
    v->gadget_valid = false;
    v->on_top_valid = false;
}


static var_node *NewNode( var_info *i, unsigned len )
/*
    Allocate a new var_node
*/
{
    var_node    *new;
    int         size;

    i->mem_lock = true;
    size = sizeof( var_node ) + len;
    new = DbgAlloc( size + type_SIZE );
    i->mem_lock = false;
    if( new == NULL ) {
        ErrorRet( ERR_NONE, LIT_ENG( ERR_NO_MEMORY_FOR_WINDOW ) );
        return( NULL );
    }
    memset( new, 0, size );
    new->th = (type_handle*)((char *)new + size);
    new->value = NULL;
    VarNodeInvalid( new );
    new->exprsp_timestamp = i->exprsp_timestamp;
    return( new );
}


static void VarNodeSetBits( sym_handle *sh, var_node *v )
{
    sym_info    sinfo;

    DIPSymInfo( sh, NULL, &sinfo );
    v->bits = 0;
    if( sinfo.kind == SK_CODE || sinfo.kind == SK_PROCEDURE )
        v->bits |= VARNODE_CODE;
    if( sinfo.is_private )
        v->bits |= VARNODE_PRIVATE;
    if( sinfo.is_protected )
        v->bits |= VARNODE_PROTECTED;
    if( sinfo.compiler )
        v->bits |= VARNODE_COMPILER;
    if( sinfo.is_static && sinfo.is_member ) {
        v->bits |= VARNODE_STATIC;
    }
}

typedef struct {
    var_node    *expand;
    var_node    *vfield;
    var_node    *parent;
    sym_handle  *field;
    bool        ok;
} find_field_info;

extern int              TargRow;

static walk_result CheckOneField( sym_walk_info swi, sym_handle *sh, void *_d )
{
    find_field_info *d = _d;
    switch( swi ) {
    case SWI_SYMBOL:
        if( d->expand == d->vfield ) {
            memcpy( d->field, sh, sym_SIZE );
            d->ok = true;
            return( WR_STOP );
        } else {
            d->expand = d->expand->next;
        }
        break;
    case SWI_INHERIT_START:
        d->parent = d->expand;
        d->expand = d->expand->expand;
        break;
    case SWI_INHERIT_END:
        d->expand = d->parent;
        d->parent = d->expand->parent;
        d->expand = d->expand->next;
    }
    return( WR_CONTINUE );
}

static walk_result DoPushFirstField( sym_walk_info swi, sym_handle *sh, void *pdone )
{
    if( swi == SWI_SYMBOL ) {
        PushField( sh );
        *(bool *)pdone = true;
        return( WR_STOP );
    }
    return( WR_CONTINUE );
}

static void     PushFirstField( void *th )
{
    bool        done;

    done = false;
    DIPWalkSymList( SS_TYPE, th, DoPushFirstField, &done );
    if( !done ) {
        Suicide();
    }
}

typedef struct {
    bool        done;
    char        *name;
} dot_named_field_info;

static walk_result DoDotNamedField( sym_walk_info swi, sym_handle *sh, void *_info )
{
    dot_named_field_info *info = _info;

    if( swi != SWI_SYMBOL )
        return( WR_CONTINUE );
    DIPSymName( sh, NULL, SNT_SOURCE, TxtBuff, TXT_LEN );
    if( strcmp( TxtBuff, info->name ) != 0 )
        return( WR_CONTINUE );
    DoGivenField( sh );
    CheckRValue();
    info->done = true;
    return( WR_STOP );
}

static void     DotNamedField( void *th, void *name )
/*******************************************************/
{
    dot_named_field_info        info;

    info.done = false;
    info.name = name;
    DIPWalkSymList( SS_TYPE, th, DoDotNamedField, &info );
    if( !info.done ) {
        Suicide();
    }
}

static void     PushSubScript( long index )
/*****************************************/
{
    DupStack();
    StartSubscript();
    PushNum( index );
    AddSubscript();
    EndSubscript();
    CheckRValue();
}


static void     PointStack( void )
{
    DoPoints( TK_NONE );
    CheckRValue();
}


static void     PushPointStackFirstField( void )
{
    DupStack();
    DoPoints( TK_NONE );
    for( ;; ) {
        ExprValue( ExprSP );
        if( ExprSP->ti.kind != TK_STRUCT )
            break;
        PushFirstField( ExprSP->th );
    }
}


static void     PushPoints( void )
{
    DupStack();
    PointStack();
}


static bool     CheckPointerValid( void )
{
    bool        ok;

    FreezeStack();
    ok = Spawn( PushPointStackFirstField ) == 0;
    UnFreezeStack( true );
    return( ok );
}


static type_kind        TypeKind( type_handle *th )
{
    dig_type_info       ti;

    DIPTypeInfo( th, NULL, &ti );
    return( ti.kind );
}


static var_node *VarFindParentStruct( var_node *v )
/************************************************/
{
    while( v->parent != NULL ) {
        if( v->parent->node_type != NODE_INHERIT )
            return( v->parent );
        v = v->parent;
    }
    return( NULL );
}

static bool             FindField( sym_handle *field, var_node *vfield )
{
    find_field_info     d;
    var_node            *vstruct;

    vstruct = VarFindParentStruct( vfield );
    if( TypeKind( vstruct->th ) != TK_STRUCT )
        return( false );
    d.expand = vstruct->expand;
    d.vfield = vfield;
    d.field = field;
    d.ok = false;
    DIPWalkSymList( SS_TYPE, vstruct->th, &CheckOneField, &d );
    return( d.ok );
}

static void     VarNodeFini( var_node *v )
{
    var_node    *junk;
    int         count;
//    var_node    *parent;

    count = 0;
//    parent = NULL;
    while( v != NULL ) {
//        parent = v->parent;
        VarNodeFini( v->old_expand );
        VarNodeFini( v->expand );
        v->have_type = false;
        v->pushed = false;
        v->popped = false;
        v->buried = false;
        junk = v;
        v = v->next;
        ++count;
        DbgFree( junk->value );
        DbgFree( junk );
    }
}


#define MAX_EXPAND_ARRAY_ELEMENTS 100000

static int ExpandArray( var_info *i, var_node *v,
                         long elts, long start, long end )
/*
    Allocate and fill in var_nodes for an array expansion
*/
{
    var_node            *new;
    int                 count;
    unsigned long       element;
    var_node            **owner;
    char                buff[CMD_LEN];

    owner = &v->expand;
    count = 0;
    if( elts > MAX_EXPAND_ARRAY_ELEMENTS ) {
        elts = MAX_EXPAND_ARRAY_ELEMENTS;
        Format( buff, LIT_ENG( WARN_ONLY_MAX_EXPAND_ARRAY_ELEMENTS ), elts );
        Warn( buff );
    }
    for( element = start; element <= end; ++element ) {
        if( elts == 0 )
            break;
        --elts;
        new = NewNode( i, 0 );
        if( new == NULL )
            break;
        new->node_type = NODE_SUBSCR;
        new->parent = v;
        new->s = v->s;
        new->element = element;
        *owner = new;
        owner = &new->next;
        ++count;
    }
    return( count );
}



void    VarKillExprSPCache( var_info *i )
{
    ExprPurge();
    i->exprsp_timestamp++;
    i->exprsp_cache = NULL;
    i->exprsp_cacherow = VAR_NO_ROW;
    i->name_end_row = -1;
}



typedef struct {
    var_info    *i;
    var_node    **owner;
    int         count;
    var_node    *v;
    var_node    *inherit;
} alloc_field_info;

static walk_result AllocOneField( sym_walk_info swi, sym_handle *sh, void *_d )
{
    var_node            *new;
    int                 len;
    symbol_type         tag;
    DIPHDL( type, th );
    alloc_field_info *d = _d;

    switch( swi ) {
    case SWI_SYMBOL:
        new = NewNode( d->i, 0 );
        if( new == NULL )
            return( WR_STOP );
        VarNodeSetBits( sh, new );
        new->node_type = NODE_FIELD;
        new->parent = d->v;
        new->s = d->v->s;
        *(d->owner) = new;
        d->owner = &new->next;
        break;
    case SWI_INHERIT_START:
        len = 0;
        if( sh != NULL && DIPSymType( sh, th ) == DS_OK ) {
            len = DIPTypeName( th, 0, &tag, NULL, 0 );
        }
        new = NewNode( d->i, len );
        if( new == NULL )
            return( WR_STOP );
        if( sh != NULL ) {
            VarNodeSetBits( sh, new );
        }
        new->node_type = NODE_INHERIT;
        new->display |= VARDISP_INHERIT_CLOSED;
        if( len != 0 ) {
            DIPTypeName( th, 0, &tag, VarNodeExpr( new ), len+1 );
        }
        new->parent = d->v;
        new->s = d->v->s;
        *(d->owner) = new;
        d->owner = &new->expand;
        d->v = new;
        break;
    case SWI_INHERIT_END:
        new = d->v;
        d->v = d->v->parent;
        d->owner = &new->next;
        return( WR_CONTINUE );
    default:
        return( WR_CONTINUE );
    }
    return( WR_CONTINUE );
}

static bool PointerToChar( void )
/*******************************/
{
    DIPHDL( type, th );

    switch( ExprSP->ti.kind ) {
    case TK_POINTER:
    case TK_ARRAY:
        DIPTypeBase( ExprSP->th, th, NULL, NULL );
        if( TypeKind( th ) == TK_CHAR )
            return( true );
        break;
    }
    return( false );
}

static bool PointerToStruct( void )
/*********************************/
{
    if( !CheckPointerValid() )
        return( false );

    if( Spawn( PushPoints ) != 0 )
        return( false );
    ExprValue( ExprSP );
    if( ExprSP->ti.kind != TK_STRUCT ) {
        PopEntry();
        return( false );
    }
    /* ok - it's a good pointer to struct - commit indirection */
    SwapStack( 1 );
    PopEntry();
    return( true );
}

bool    VarExpand( var_info *i, var_node *v, long start, long end )
/*
    Expand out node 'v' in the structure. If it is an array, start/end
    indicate the chunk we're really interested in. There's a little
    extra gear at the front of the routine to cause a pointer to struct
    to expand out to the struct in one step rather than taking two
    user interactions.
*/
{
    bool                ok;
    unsigned long       elts;
    alloc_field_info    d;
    array_info          ainfo;

    if( v->old_expand != NULL )
        VarNodeFini( v->old_expand );
    v->old_expand = NULL;
    ExprValue( ExprSP );
    if( ExprSP->th == NULL )
        return( false );
    ok = true;
    switch( ExprSP->ti.kind ) {
    case TK_STRUCT:
        ok = SpawnP( PushFirstField, ExprSP->th ) == 0;
        if( ok )
            PopEntry();
        break;
    case TK_POINTER:
    case TK_ARRAY:
        ok = CheckPointerValid();
        break;
    }
    if( !ok ) {
        VarKillExprSPCache( i );
        return( false );
    }
    ExprValue( ExprSP );
    HDLAssign( type, v->th, ExprSP->th );
    switch( ExprSP->ti.kind ) {
    case TK_POINTER:
        if( end < start ) {
            var_node *new;
            new = NewNode( i, 0 );
            if( new != NULL ) {
                new->node_type = NODE_POINTS;
                new->parent = v;
                new->s = v->s;
                v->expand = new;
                if( PointerToStruct() ) { // auto expand struct
                    new->display_type = v->display_type;
                    new->bits = v->bits;
                    d.count = 0;
                    d.owner = &new->expand;
                    d.v = new;
                    d.i = i;
                    DIPWalkSymList( SS_TYPE, ExprSP->th, &AllocOneField, &d );
                }
            }
        } else {
            elts = end - start + 1;
            ExpandArray( i, v, elts, start, end );
            v->fake_array = true;
        }
        break;
    case TK_STRUCT:
        d.count = 0;
        d.owner = &v->expand;
        d.v = v;
        d.i = i;
        DIPWalkSymList( SS_TYPE, ExprSP->th, &AllocOneField, &d );
        break;
    case TK_ARRAY:
        if( end < start ) {
            DIPTypeArrayInfo( ExprSP->th, ExprSP->lc, &ainfo, NULL );
            elts = ainfo.num_elts;
        } else {
            elts = end - start + 1;
            v->fake_array = true;
        }
        ExpandArray( i, v, elts, start, end );
        break;
    }
    return( true );
}

static void ArrayParms( var_node *v, array_info *ainfo )
{
    DIPHDL( type, th );
    dig_type_info       ti;

    if( TypeKind( v->th ) == TK_ARRAY ) {
        DIPTypeArrayInfo( v->th, ExprSP->lc, ainfo, NULL );
    } else {
        DIPTypeBase( v->th, th, NULL, NULL );
        ainfo->low_bound = 0;
        DIPTypeInfo( th, ExprSP->lc, &ti );
        ainfo->stride = ti.size;
    }
}

static var_node *VarFirstNodeInScope( var_info *i, scope_state *s );

static var_node *DoVarNextNode( var_info *i, var_node *v )
{
    if( v->next != NULL )
        return( v->next );
    if( v->node_type == NODE_ROOT && v->s != NULL && v->s->outer != NULL ) {
        return( VarFirstNodeInScope( i, v->s->outer ) );
    }
    return( NULL );
}


static bool VarNodeHidden( var_info *i, var_node *v )
/***************************************************/
{
    var_type_bits       hide;
    var_node            *vstruct;

    /* unused parameters */ (void)i;

    vstruct = VarFindParentStruct( v );
    if( vstruct != NULL && vstruct->display_type != NULL ) {
        hide = vstruct->display_type->hide;
    } else {
        hide = Hide;
    }

    return( (v->bits & hide) != 0 );
}

static var_node *VarNextNode( var_info *i, var_node *v )
/*
    Find the next var node (not going down expansions).
*/
{
    while( (v = DoVarNextNode( i, v )) != NULL ) {
        if( !VarNodeHidden( i, v ) ) {
            break;
        }
    }
    return( v );
}

static bool VarInheritOpen( var_node *v )
/***************************************/
{
    var_node *vstruct = VarFindParentStruct( v );
    if( vstruct != NULL && !VarDisplayIsHidden( vstruct, VARNODE_INHERIT ) ) {
        return( true );
    }
    return( (v->display & VARDISP_INHERIT_CLOSED) == 0 );
}

var_node *VarExpandNode( var_node *v )
/*******************************************/
{
    if( v->node_type == NODE_INHERIT && !VarInheritOpen( v ) )
        return( NULL );
    return( v->expand );
}

var_node *VarFirstExpandNode( var_info *i, var_node *v )
/*
    Find the var's first expansion node.
*/
{
    for( v = VarExpandNode( v ); v != NULL; v = v->next ) {
        if( !VarNodeHidden( i, v ) ) {
            break;
        }
    }
    return( v );
}

static var_node *DoVarFirstNode( scope_state *s )
{
    for( ; s != NULL; s = s->outer ) {
        if( s->v != NULL ) {
            return( s->v );
        }
    }
    return( NULL );
}


static var_node *VarFirstNodeInScope( var_info *i, scope_state *s )
/*
    Find the first var_node associated with the scope s
*/
{
    var_node    *v;

    v = DoVarFirstNode( s );
    if( v == NULL )
        return( v );
    if( !VarNodeHidden( i, v ) )
        return( v );
    return( VarNextNode( i, v ) );
}

var_node *VarFirstNode( var_info *i )
{
    return( VarFirstNodeInScope( i, i->s ) );
}

static  var_node        *DoVarNextRowNode( var_node *v )
{
    var_node    *parent;

    if( VarExpandNode( v ) != NULL )
        return( VarExpandNode( v ) );
    if( v->next != NULL )
        return( v->next );
    for( parent = v->parent; parent != NULL; parent = parent->parent ) {
        if( parent->next != NULL ) {
            return( parent->next );
        }
    }
    if( v->s->outer != NULL )
        return( DoVarFirstNode( v->s->outer ) );
    return( NULL );
}

var_node        *VarNextRowNode( var_info *i, var_node *v )
/*
    Find the var_node that should be associated with a given row of the window
*/
{
    while( (v = DoVarNextRowNode( v )) != NULL ) {
        if( !VarNodeHidden( i, v ) ) {
            break;
        }
    }
    return( v );
}

var_node *VarNextVisibleSibling( var_info *i, var_node *v )
/*********************************************************/
{
    if( v->node_type == NODE_ROOT )
        return( NULL );
    while( (v = v->next) != NULL ) {
        if( !VarNodeHidden( i, v ) ) {
            break;
        }
    }
    return( v );
}

static void     VarScanForward( void *_v )
/*
    Scan forward through our data structure, evaluating the expressions
    as we go until we reach the desired row. This will often be the
    row we've previously evaluated, or just evaluated. The ->pushed
    ->popped and ->buried fields tell us if the expression is at the top of
    ExprSP, already popped off ExprSP, or buried down ExprSP somewhere
*/
{
    bool                have_array_parms;
    array_info          ainfo;
    var_node            *expand;
    type_display        *parent;
    DIPHDL( sym, field );
    var_node            *v = _v;

    have_array_parms = false;
    while( v != NULL ) {
        CheckExprStackTimeStamp( v, ExprStackTimeStamp );
        ++CurrRow;
        expand = VarFirstExpandNode( TargVar, v );
        if( !v->pushed ) {
            switch( v->node_type ) {
            case NODE_ROOT:
                PushExpr( v );
                break;
            case NODE_FIELD:
                if( expand==NULL && CurrRow!=TargRow ) {
                    v->popped = true; // just skip field
                } else {
                    if( !FindField( field, v ) )
                        Suicide();
                    if( v->display_type == NULL ) {
                        DupStack();
                        ExprValue( ExprSP );
                        if( ExprSP->ti.kind == TK_STRUCT ) {
                            parent = VarDisplayAddStructType( ExprSP->th );
                        } else {
                            parent = NULL;
                        }
                        PopEntry();
                        if( parent ) {
                            v->display_type = VarDisplayAddFieldSym( parent, field );
                        }
                    }
                    VarDisplaySetBits( v );
                    PushField( field );
                }
                break;
            case NODE_SUBSCR:
                if( expand==NULL && CurrRow!=TargRow ) {
                    v->popped = true; // just skip array element
                } else {
                    ExprValue( ExprSP );
                    if( !have_array_parms ) {
                        have_array_parms = true;
                        ArrayParms( v->parent, &ainfo );
                    }
                    PushSubScript( v->element + ainfo.low_bound );
                }
                break;
            case NODE_POINTS:
                PushPoints();
                break;
            }
            if( v->node_type != NODE_INHERIT && !v->popped ) {
                v->have_type = false;
                DupStack();
                ExprValue( ExprSP );
                switch( ExprSP->ti.kind ) {
                case TK_STRUCT:
                    HDLAssign( type, v->th, ExprSP->th );
                    v->have_type = true;
                    VarDisplayAliasNode( v, VarDisplayAddStructType( v->th ) );
                    VarDisplaySetBits( v );
                    break;
                case TK_POINTER:
                    DupStack();
                    if( PointerToStruct() ) {
                        VarDisplayAliasNode( v, VarDisplayAddStructType( ExprSP->th ) );
                        VarDisplaySetBits( v );
                    }
                    PopEntry();
                    // fall through here ...
                case TK_ARRAY:
                default:
                    if( ExprSP->th ) {
                        HDLAssign( type, v->th, ExprSP->th );
                        v->have_type = true;
                    }
                    break;
                }
                v->is_string = PointerToChar() && !( v->display & VARDISP_POINTER );
                PopEntry();
            }
            v->pushed = true;
        }
        if( CurrRow == TargRow ) {
            VarFound = v;
            return;
        }
        if( expand != NULL ) {
            v->buried = true;
            VarScanForward( expand );
            if( VarFound != NULL )
                return;
            v->buried = false;
        }
        if( v->node_type != NODE_INHERIT && !v->popped ) {
            PopEntry();
            v->popped = true;
        }
        v = VarNextNode( TargVar, v );
    }
}


var_node        *VarFindRowNode( var_info *i, int row )
/*
        This is used to find a row's node if VarFindRow failed due to
        an error in expression evaluation. This will find the right
        node but NOT put the expression on ExprSP.
*/
{
    var_node    *v;

    for( v = VarFirstNode( i ); v != NULL; v = VarNextRowNode( i, v ) ) {
        if( --row < 0 ) {
            break;
        }
    }
    return( v );
}


int     VarRowTotal( var_info *i )
/*
    Return the total number of rows the scope will use on the screen
*/
{
    int         count;
    var_node    *v;

    count = 0;
    for( v = VarFirstNode( i ); v != NULL; v = VarNextRowNode( i, v ) ) {
        ++count;
    }
    return( count );
}

static int      VarCount( var_info *i, var_node *v )
/*
    Count the number of rows represented by v and all its descendants
*/
{
    int         count;

    count = 0;
    while( v != NULL ) {
        count += VarCount( i, VarFirstExpandNode( i, v ) ) + 1;
        v = VarNextNode( i, v );
    }
    return( count );
}



var_node        *VarFindRoot( var_info *i, int row, int *skipped )
/*
    Find the root expression for a given row of the window, filling
    in how many rows we skipped to get there
*/
{
    int         count;
    var_node    *v,*next;

    *skipped = 0;
    for( v = VarFirstNode( i ); ; v = next ) {
        CheckExprStackTimeStamp( v, i->exprsp_timestamp );
        if( v->pushed )
            break;
        next = VarNextNode( i, v );
        if( next == NULL )
            break;
        count = VarCount( i, VarFirstExpandNode( i, v ) ) + 1;
        if( *skipped + count > row )
            break;
        *skipped += count;
    }
    return( v );
}


static var_node *DoVarFindRow( var_info *i, int row )
/*
        See VarFindRow
*/
{
    int         skipped;
    var_node    *v;

    TargVar = i;
    TargRow = row;
    CurrRow = -1;
    VarFound = NULL;
    ExprStackTimeStamp = i->exprsp_timestamp;
    if( VarFirstNode( i ) == NULL )
        return( NULL );
    if( row >= VarRowTotal( i ) )
        return( NULL );
    v = VarFindRoot( i, row, &skipped );
    TargRow -= skipped;
    if( SpawnP( VarScanForward, v ) != 0 ) {
        VarError = true;
        return( NULL );
    }
    return( VarFound );
}


var_node        *VarFindRow( var_info *i, int row )
/*
        Find the var_node for a given row of the window, leaving the
        LValue of the expression on the top of ExprSP. We try
        real hard not to re-evaluate anything we don't have to because
        the expression interpreter can be expensive
*/
{
    var_node    *found;

    VarError = false;
    if( row < 0 )
        return( NULL );
    found = DoVarFindRow( i, row );
    if( found != NULL && ( found->popped || found->buried ) ) {
        VarKillExprSPCache( i );
        VarError = false;
        found = DoVarFindRow( i, row );
    }
    if( found == NULL ) {
        VarKillExprSPCache( i );
    }
    return( found );
}


void VarDoneRow( var_info *i )
/*
        Indicate that we're done with the ExprSP for the row we just
        got using VarFindRow
*/
{
    if( !i->ok_to_cache_exprsp ) {
        VarKillExprSPCache( i );
    }
}


static void ChkTxtBuffOverflow( char *newbuff )
/*
    See VarBuildName
*/
{
    if( newbuff - TxtBuff >= TXT_LEN ) {
        Error( ERR_NONE, LIT_ENG( ERR_EXPR_TOO_LONG ) );
    }
}


static void InsertTxt( char *buff, const char *txt, size_t len )
/*
    See VarBuildName
*/
{
    char        *src;
    char        *dst;

    if( len == 0 ) {
        len = strlen( txt );
    }
    src = buff + strlen( buff );
    dst = src + len;
    do {
        *dst-- = *src--;
    } while( src >= buff );
    memcpy( buff, txt, len );
}


static char AddToName( tokens token, const char *fldname, int namelen, char pprio )
/*
    See VarBuildName
*/
{
    const char  *tstr;
    size_t      tlen;
    char        *buff, *end;
    char        prio;

    SetTokens( true );
    TokenName( token, &tstr, &tlen );
    SetTokens( false );
    prio = *tstr++;
    if( pprio < prio && TxtBuff[0] != NULLCHAR ) {
        AddToName( T_SSL_SPEC_PAREN, NULL, 0, 127 );
    }
    buff = TxtBuff;
    end = TxtBuff + strlen( TxtBuff );
    for( --tlen; tlen > 0; tlen--, tstr++ ) {
        if( *tstr == 'x' ) {
            buff = end;
        } else if( *tstr == 'y' ) {
            ChkTxtBuffOverflow( end + namelen );
            InsertTxt( buff, fldname, namelen );
            buff += namelen;
            end += namelen;
        } else {
            ChkTxtBuffOverflow( end + 1 );
            InsertTxt( buff, tstr, 1 );
            ++buff;
            ++end;
        }
    }
    return( prio );
}


void VarBaseName( var_node *v )
/*****************************/
{
    TxtBuff[0] = NULLCHAR;
    if( v->is_sym_handle ) {
        if( DIPSymName( VarNodeHdl( v ), NULL, SNT_SCOPED, TxtBuff, TXT_LEN ) )
            return;
        DIPSymName( VarNodeHdl( v ), NULL, SNT_SOURCE, TxtBuff, TXT_LEN );
    } else {
        strcpy( TxtBuff, VarNodeExpr( v ) );
    }
}

void    VarBuildName( var_info *info, var_node *v, bool just_end_bit )
/*****************************************************************/
/*
    Given a var_node, build up its name in TxtBuff in a form
    that can be fed back into the expression processor if need
    be. We must consult the SSL files for the proper syntax
    for indirection, etc. If "just_end_bit", rather than appending,
    we end up with just "->foo" instead of "x->y->foo".
*/
{
    char                *name;
    size_t              len;
    char                buff[20];
    char                *end;
    char                prio;
    bool                delay_indirect;
    array_info          ainfo;
    DIPHDL( sym, field );

    /* unused parameters */ (void)info;

    if( v->node_type == NODE_INHERIT ) {
        name = StrCopy( ": ", TxtBuff );
        if( v->bits & VARNODE_PROTECTED ) {
            StrCopy( "protected", name );
        } else if( v->bits & VARNODE_PRIVATE ) {
            StrCopy( "private", name );
        } else {
            StrCopy( "public", name );
        }
        return;
    }
    prio = 127;
    v->path = NULL;
    while( v->parent != NULL ) {
        v->parent->path = v;
        v = v->parent;
    }
    VarBaseName( v );
    delay_indirect = false;
    while( v->path != NULL ) {
        switch( v->path->node_type ) {
        case NODE_FIELD:
            if( VarError || !FindField( field, v->path ) ) {
                name = LIT_ENG( field );
                len = strlen( LIT_ENG( field ) );
            } else {
                len = DIPSymName( field, NULL, SNT_SOURCE, NULL, 0 );
                _AllocA( name, len+1 );
                DIPSymName( field, NULL, SNT_SOURCE, name, len+1 );
            }
            if( delay_indirect ) {
                prio = AddToName( T_SSL_SPEC_POINTER_FIELD, name, len, prio );
                delay_indirect = false;
            } else {
                prio = AddToName( T_SSL_SPEC_FIELD_SELECT, name, len, prio );
            }
            if( just_end_bit ) {
                TxtBuff[0] = NULLCHAR;
                InsertTxt( TxtBuff, name, len );
            }
            break;
        case NODE_SUBSCR:
            if( delay_indirect ) {
                prio = AddToName( T_SSL_SPEC_POINTER_IND, NULL, 0, prio );
                delay_indirect = false;
            }
            ArrayParms( v, &ainfo );
            end = CnvLongDec( v->path->element + ainfo.low_bound, buff, sizeof( buff ) );
            if( just_end_bit ) {
                *TxtBuff = NULLCHAR;
            }
            prio = AddToName( T_SSL_SPEC_ARRAY, buff, end - buff, prio );
            break;
        case NODE_POINTS:
            if( delay_indirect ) {
                prio = AddToName( T_SSL_SPEC_POINTER_IND, NULL, 0, prio );
                delay_indirect = false;
            }
            delay_indirect = true;
            break;
        }
        v = v->path;
    }
    if( delay_indirect ) {
        AddToName( T_SSL_SPEC_POINTER_IND, NULL, 0, prio );
    }
}

static bool     VarFreeOldExpansion( var_node *v )
{
    bool        freed;

    freed = false;
    while( v != NULL ) {
        if( v->old_expand != NULL ) {
            freed = true;
            VarNodeFini( v->old_expand );
            v->old_expand = NULL;
        }
        if( VarFreeOldExpansion( v->expand ) ) {
            freed = true;
        }
        v = v->next;
    }
    return( freed );
}

void  VarDeExpand( var_node *v )
/*
    Get rid of the expansion for a given var_node
*/
{
    VarFreeOldExpansion( v );
    if( v->expand != NULL ) {
        VarNodeFini( v->expand );
        v->expand = NULL;
    }
    v->fake_array = false;
}

static var_node *MakeNewNode( var_info *i, const void *name, unsigned len )
{
    var_node    *v;

    v = NewNode( i, len );
    if( v == NULL )
        return( v );
    v->parent = NULL;
    v->node_type = NODE_ROOT;
    v->s = i->s;
    memcpy( &v->u, name, len );
    return( v );
}

void VarAllNodesInvalid( var_info *i )
{
    var_node    *v;

    for( v = DoVarFirstNode( i->s ); v != NULL; v = DoVarNextRowNode( v ) ) {
        VarNodeInvalid( v );
    }
}

var_node *VarAdd1( var_info *i, const void *name,
                             unsigned len, bool expand,
                             bool is_sym_handle )
{
    var_node    **owner;
    var_node    *v;
    var_node    *v2;

    owner = &i->s->v;
    while( *owner != NULL ) {
        owner = &(*owner)->next;
    }
    v = MakeNewNode( i, name, len );
    if( is_sym_handle ) {
        VarNodeSetBits( VarNodeHdl( v ), v );
    } else {
        VarNodeExpr( v )[len] = NULLCHAR;
    }
    v->is_sym_handle = is_sym_handle;
    *owner = v;
    VarErrState();
    if( expand ) {
        v2 = VarFindRow( i, VarRowTotal( i ) - 1 );
        if( v2 != NULL ) {
            VarExpand( i, v, 0, -1 );
        }
        VarDoneRow( i );
    }
    VarOldErrState();
    return( v );
}


void VarExpandRow( var_info *i, var_node *v, int row )
/*
        Expand or Collapse a row in the window. If we want to remember
        all the old nestings so that collapsing an entire structure
        the re-opening leaves any sub-expansions as they were.
*/
{
    /* unused parameters */ (void)row;

    if( v->node_type == NODE_INHERIT ) {
        v->display ^= VARDISP_INHERIT_CLOSED;
        VarNodeDisplayUpdate( v );
    } else if( v->expand == NULL ) {
        if( v->old_expand != NULL ) {
            v->expand = v->old_expand;
            v->old_expand = NULL;
        } else {
            VarExpand( i, v, 0, -1 );
            if( v->expand != NULL && VarFirstExpandNode( i, v ) == NULL ) {
                Warn( LIT_ENG( No_Visible_Members ) );
            }
        }
    } else {
        v->old_expand = v->expand;
        v->expand = NULL;
    }
}

void VarExpandRowNoCollapse( var_info *i, var_node *v, int row )
{
    /* unused parameters */ (void)row;

    if( v->node_type == NODE_INHERIT ) {
        v->display ^= VARDISP_INHERIT_CLOSED;
        VarNodeDisplayUpdate( v );
    } else if( v->expand == NULL ) {
        if( v->old_expand != NULL ) {
            v->expand = v->old_expand;
            v->old_expand = NULL;
        } else {
            VarExpand( i, v, 0, -1 );
            if( v->expand != NULL && VarFirstExpandNode( i, v ) == NULL ) {
                Warn( LIT_ENG( No_Visible_Members ) );
            }
        }
    }
}


static bool     Followable( type_kind class )
{
    if( class == TK_POINTER ) {
        return( CheckPointerValid() );
    }
    return( false );
}


bool    VarGetStackClass( type_kind *class )
{
    bool        followable;

    DupStack();
    ExprValue( ExprSP );
    *class = ExprSP->ti.kind;
    followable = Followable( *class );
    PopEntry();
    return( followable );
}


bool    VarExpandable( type_kind class )
{
    switch( class ) {
    case TK_STRUCT:
    case TK_ARRAY:
        return( true );
    }
    return( false );
}


bool    VarIsPointer( type_kind class )
{
    switch( class ) {
    case TK_ARRAY:
    case TK_POINTER:
    case TK_ADDRESS:
        return( true );
    }
    return( false );
}

bool VarPrintText( var_info *i, char *buff, wspawn_func *rtn, int len )
{
    StartPrintBuff( buff, len );
    if( Spawn( rtn ) == 0 ) {
        EndPrintBuff();
        return( true );
    } else {
        VarKillExprSPCache( i );
        return( false );
    }
}


static bool PrintAString( var_info *i, char *buff, unsigned buff_len, bool force )
{
    size_t      len;
    bool        ok;

    FreezeStack();
    DupStack();
    ok = VarPrintText( i, buff + 1,
                    force ? ForcePrintString : PrintString, buff_len - 2 );
    UnFreezeStack( true );
    if( !ok )
        return( false );
    buff[0] = '"';
    len = strlen( buff );
    buff[len++] = '"';
    buff[len] = NULLCHAR;
    return( true );
}


void VarDelete( var_info *i, var_node *v )
{
    var_node    **owner;

    owner = &i->s->v;
    while( *owner != v ) {
        owner = &(*owner)->next;
    }
    *owner = v->next;
    DbgFree( v->value );
    DbgFree( v );
}


static void VarSetType( var_node *v, int hex, int decimal, int character, int string, int pointer )
{
    type_display        *alias;
    if( hex ) {
        v->display |= VARDISP_HEX;
    } else {
        v->display &= ~VARDISP_HEX;
    }
    if( decimal ) {
        v->display |= VARDISP_DECIMAL;
    } else {
        v->display &= ~VARDISP_DECIMAL;
    }
    if( character ) {
        v->display |= VARDISP_CHARACTER;
    } else {
        v->display &= ~VARDISP_CHARACTER;
    }
    if( string ) {
        v->display |= VARDISP_STRING;
    } else {
        v->display &= ~VARDISP_STRING;
    }
    if( pointer ) {
        v->display |= VARDISP_POINTER;
    } else {
        v->display &= ~VARDISP_POINTER;
    }
    if( v->display_type != NULL ) {
        v->display_type->display = v->display;
        for( alias = v->display_type->alias; alias != v->display_type; alias = alias->alias ) {
            alias->display = v->display;
        }
        VarDisplayDirty( v->display_type );
    }
    DbgUpdate( UP_VAR_DISPLAY );
}

bool VarDisplayIsHex( var_node *v )
{
    return( ( v->display & VARDISP_HEX ) != 0 );
}

void VarDisplaySetHex( var_node *v )
{
    VarSetType( v, CurrRadix != 16, 0, 0, 0, 0 );
}

void VarDisplaySetArrayHex( var_node *v )
{
    var_node    *next_v;

    if( NULL == v )
        return;

    for( next_v = v->expand; next_v != NULL; next_v = next_v->next ) {
        VarSetType( next_v, CurrRadix != 16, 0, 0, 0, 0 );
    }
}

bool VarDisplayIsDecimal( var_node *v )
{
    return( ( v->display & VARDISP_DECIMAL ) != 0 );
}

void VarDisplaySetDecimal( var_node *v )
{
    VarSetType( v, 0, CurrRadix != 10, 0, 0, 0 );
}

void VarDisplaySetArrayDec( var_node *v )
{
    var_node    *next_v;

    if( NULL == v )
        return;

    for( next_v = v->expand; next_v != NULL; next_v = next_v->next ) {
        VarSetType( next_v, 0, CurrRadix != 10, 0, 0, 0 );
    }
}

bool VarDisplayIsChar( var_node *v )
{
    return( ( v->display & VARDISP_CHARACTER ) != 0 );
}

void VarDisplaySetChar( var_node *v )
{
    VarSetType( v, 0, 0, 1, 0, 0 );
}

bool VarDisplayIsString( var_node *v )
{
    return( ( v->display & VARDISP_STRING ) != 0 );
}

void VarDisplaySetString( var_node *v )
{
    VarSetType( v, 0, 0, 0, 1, 0 );
}

bool VarDisplayIsPointer( var_node *v )
{
    return( ( v->display & VARDISP_POINTER ) != 0 );
}

void VarDisplaySetPointer( var_node *v )
{
    VarSetType( v, 0, 0, 0, 0, 1 );
}

void VarBreakOnWrite( var_info *i, var_node *v )
{
    char                *name;

    VarBuildName( i, v, false );
    name = DupStr( TxtBuff );
    SpawnP( BreakOnExprSP, name );
    DbgFree( name );
}

void VarAddWatch( var_info *i, var_node *v )
{
    VarBuildName( i, v, false );
    WndVarInspect( TxtBuff );
}

void VarAddNodeToScope( var_info *i, var_node *v, const char *buff )
{
    var_node    **owner;
    var_node    *new;

    owner = &i->s->v;
    while( *owner != v ) {
        owner = &(*owner)->next;
    }
    new = MakeNewNode( i, buff, strlen( buff ) + 1 );
    new->next = *owner;
    *owner = new;
    if( v != NULL ) {
        VarDeExpand( v );
        VarDelete( i, v );
    }
}


int VarFindRootRow( var_info *i, var_node *v, int row )
{
    var_node    *sibling;
    int         new_row;

    if( v->parent == NULL )
        return( row );
    new_row = row-1;
    for( sibling = VarFirstExpandNode( i, v->parent ); sibling != v; sibling = VarNextNode( i, sibling ) ) {
        new_row -= VarCount( i, VarFirstExpandNode( i, sibling ) ) + 1;
    }
    return( new_row );
}

mad_radix VarNewDisplayRadix( var_display_bits display )
/******************************************************/
{
    if( ( display & VARDISP_HEX ) )
        return( NewCurrRadix( 16 ) );
    if( ( display & VARDISP_DECIMAL ) )
        return( NewCurrRadix( 10 ) );
    return( CurrRadix );
}

mad_radix VarNewCurrRadix( var_node *v )
{
    return( VarNewDisplayRadix( v->display ) );
}

bool VarGetOnTop( var_node *v )
{
    if( v->on_top_valid )
        return( v->on_top );
    return( VarDisplayedOnTop( v ) );
}

var_gadget_type VarGetGadget( var_node *v )
{
    type_kind   class;
    var_node *vstruct;

    if( v->gadget_valid )
        return( v->gadget );
    if( v->node_type == NODE_INHERIT ) {
        vstruct = VarFindParentStruct( v );
        if( vstruct != NULL && !VarDisplayIsHidden( vstruct, VARNODE_INHERIT ) ) {
            return( VARGADGET_NONE );
        } else if( v->display & VARDISP_INHERIT_CLOSED ) {
            return( VARGADGET_INHERIT_CLOSED );
        } else {
            return( VARGADGET_INHERIT_OPEN );
        }
    }
    class = ExprSP->ti.kind;
    if( class == TK_POINTER ) {
        if( !CheckPointerValid() ) {
            return( VARGADGET_BADPOINTS );
        } else if( v->expand != NULL ) {
            return( VARGADGET_UNPOINTS );
        } else {
            return( VARGADGET_POINTS );
        }
    } else if( v->expand != NULL ) {
        return( VARGADGET_OPEN );
    } else if( VarExpandable( class ) && !VarError ) {
        return( VARGADGET_CLOSED );
    }
    return( VARGADGET_NONE );
}

void VarSetGadget( var_node *v, var_gadget_type gadget )
{
    v->gadget = gadget;
    v->gadget_valid = true;
}

void VarSetOnTop( var_node *v, bool on_top )
{
    v->on_top = on_top;
    v->on_top_valid = true;
}

static void VarPrintValue( char *buff, unsigned len, var_info *i,
                            var_display_bits display, bool is_string )
/********************************************************************/
{
    char        buff2[TXT_LEN];
    mad_radix   old_radix;

    old_radix = VarNewDisplayRadix( display );
    if( ( display & VARDISP_STRING ) ) {
        PrintAString( i, buff, len, true );
    } else if( is_string ) {
        VarPrintText( i, buff2, PrintValue, len );
        if( !PrintAString( i, buff, len, false ) ) {
            strcpy( buff, buff2 );
        }
    } else if( ( display & VARDISP_CHARACTER ) ) {
        VarPrintText( i, buff, PrintChar, len );
    } else {
        VarPrintText( i, buff, PrintValue, len );
    }
    NewCurrRadix( old_radix );
}

static char *Append( char *end, char *p, char *str )
/**************************************************/
{
    for( ; p < end; ++p ) {
        *p = *str;
        if( *str == NULLCHAR )
            break;
        ++str;
    }
    return( p );
}

static char *VarDisplayTop( char *p, char *end, var_info *i, type_display *type )
/********************************************************************************/
{
    bool        comma, dotted;

    p = Append( end, p, "{ " );
    comma = false;
    for( type = type->fields; type != NULL; type = type->next ) {
        if( type->on_top ) {
            if( comma )
                p = Append( end, p, ", " );
            FreezeStack();
            DupStack();
            dotted = ( SpawnPP( DotNamedField, ExprSP->th, type->name ) == 0 );
            if( !dotted ) {
                UnFreezeStack( true );
                continue;
            }
            if( type->has_top ) {
                p = VarDisplayTop( p, end, i, type );
            } else if( !type->is_struct ) {
                ExprValue( ExprSP );
                VarPrintValue( p, end - p, i, type->display, PointerToChar() );
                p += strlen( p );
            }
            UnFreezeStack( true );
            comma = true;
        }
    }
    return( Append( end, p, " }" ) );
}

char *VarGetValue( var_info *i, var_node *v )
{
    mad_radix           old_radix;
    char                *value;
    char                buff[TXT_LEN];
    char                *p, *end;

    if( v->value_valid )
        return( v->value );
    if( v->node_type == NODE_INHERIT ) {
        p = StrCopy( "(", TxtBuff );
        if( VarNodeExpr( v )[0] != NULLCHAR ) {
            p = StrCopy( VarNodeExpr( v ), p );
        } else {
            p = StrCopy( LIT_ENG( inherited_members ), p );
        }
        p = StrCopy( ")", p );
        return( TxtBuff );
    }
    old_radix = VarNewCurrRadix( v );
    if( VarError ) {
        value = LIT_ENG( Quest_Marks );
    } else {
        value = " ";
        switch( ExprSP->ti.kind ) {
        case TK_POINTER:
            FreezeStack();
            DupStack();
            if( v->display_type != NULL &&
                v->display_type->has_top && PointerToStruct() ) {
                VarDisplayTop( buff, buff + TXT_LEN, i, v->display_type );
                strcpy( TxtBuff, buff );
                value = TxtBuff;
            } else {
                value = TxtBuff;
                VarPrintValue( TxtBuff, TXT_LEN, i, v->display, v->is_string );
            }
            UnFreezeStack( true );
            break;
        case TK_STRUCT:
            if( v->display_type != NULL && v->display_type->has_top ) {
                VarDisplayTop( buff, buff + TXT_LEN, i, v->display_type );
                strcpy( TxtBuff, buff );
                value = TxtBuff;
            } else {
                end = TxtBuff+TXT_LEN;
                p = TxtBuff;
                p = Append( end, p, "(" );
                p = VarDisplayType( v, p, TXT_LEN-2 );
                if( p == NULL ) {
                    value = LIT_ENG( Struct );
                } else {
                    p = Append( end, p, ")" );
                    value = TxtBuff;
                }
            }
            break;
        case TK_ARRAY:
            if( ( v->display & VARDISP_STRING ) ) {
                value = TxtBuff;
                PrintAString( i, TxtBuff, TXT_LEN, true );
            } else if( v->is_string ) {
                value = TxtBuff;
                if( !PrintAString( i, TxtBuff, TXT_LEN, false ) ) {
                    value = LIT_ENG( Array );
                }
            } else {
                value = LIT_ENG( Array );
            }
            break;
        default:
            value = TxtBuff;
            VarPrintValue( TxtBuff, TXT_LEN, i, v->display, v->is_string );
            break;
        }
    }
    NewCurrRadix( old_radix );
    return( value );
}


void VarSetValue( var_node *v, const char *value )
{
    if( v->value_valid )
        return;
    DbgFree( v->value );
    v->value = DupStr( value );
    v->value_valid = true;
}


bool SameScope( scope_block *scope, scope_state *s )
{
    if( s->unmapped )
        return( false );
    if( AddrComp( scope->start, s->scope.addr ) != 0 )
        return( false );
    if( scope->len != s->scope_len )
        return( false );
    if( scope->unique != s->scope_unique )
        return( false );
    return( true );
}


static scope_state **FindScope( var_info *i, scope_block *scope, mod_handle mod )
{
    scope_state         **owner;
    scope_state         *s;

    for( owner = &i->s; (s = *owner) != NULL; owner = &s->next ) {
        if( mod == s->mod && SameScope( scope, s ) ) {
            return( owner );
        }
    }
    return( NULL );
}


static scope_state *NilScope( void )
{
    scope_state *s;

    s = DbgMustAlloc( sizeof( scope_state ) );
    memset( s, 0, sizeof( *s ) );
    InitMappableAddr( &s->scope );
    s->scope.addr = NilAddr;
    s->mod = NO_MOD;
    s->unmapped = false;
    return( s );
}


typedef struct add_new_var_info {
    var_node    *v;
    var_info    *i;
} add_new_var_info;

static walk_result AddNewVar( sym_walk_info swi, sym_handle *sym, void *_d )
{
    add_new_var_info    *d = _d;
    var_node            *new;
    sym_info            sinfo;

    switch( swi ) {
    case SWI_SYMBOL:
        DIPSymInfo( sym, NULL, &sinfo );
        if( !sinfo.is_member && sinfo.kind != SK_TYPE ) {
            if( d->v == NULL ) {
                DIPSymName( sym, NULL, SNT_SOURCE, TxtBuff, TXT_LEN );
                // nyi - use SymInfo when Brian implements the "this" indicator
                if( stricmp( TxtBuff, "this" ) == 0 ) {
                    new = VarAdd1( d->i, sym, sym_SIZE, d->i->members, true );
                    new->bits |= VARNODE_THIS;
                } else {
                    new = VarAdd1( d->i, sym, sym_SIZE, false, true );
                }
            } else {
                new = d->v;
                HDLAssign( sym, VarNodeHdl( d->v ), sym );
                d->v = d->v->next;
            }
        }
        break;
    }
    return( WR_CONTINUE );
}

scope_state *NewScope( var_info *i, scope_block *scope, mod_handle mod, bool *new )
/*
    Our scope just changed so change the variable information in
    the window. Save the old scope's structures so that we can
    present the same expansions when we return to it. This can
    burn up a bit of memory, so we timestamp them all and record
    them for garbage collection if necessary.
*/
{
    scope_state **owner;
    scope_state *s;
    add_new_var_info    info;


    owner = FindScope( i, scope, mod );
    if( owner != NULL ) {
        s = *owner; // bring to front of list
        *owner = s->next;
        s->next = i->s;
        i->s = s;
    } else {
        s = NilScope();
        s->next = i->s;
        i->s = s;
        info.i = i;
        info.v = NULL;
        if( mod == NO_MOD ) {
            DIPWalkSymList( SS_BLOCK, scope, AddNewVar, &info );
        } else {
            DIPWalkSymList( SS_MODULE, &mod, AddNewVar, &info );
        }
        s->scope_timestamp = ++ScopeTimeStamp;
        s->scope.addr = scope->start;
        s->scope_len = scope->len;
        s->scope_unique = scope->unique;
        s->mod = mod;
        *new = true;
    }
    return( s );
}


static void VarFreeScopeList( var_info *i, scope_state *junk )
{
    scope_state **owner;
    scope_state *s;

    owner = &i->s;
    while( *owner != junk ) {
        owner = &((*owner)->next);
    }
    *owner = junk->next;
    VarNodeFini( junk->v );
    FiniMappableAddr( &junk->scope );
    DbgFree( junk->wnd_data );
    DbgFree( junk );
    for( s = i->s; s != NULL; s = s->next ) {
        if( s->outer == junk ) {
            s->outer = NULL;
        }
    }
}


bool VarDeleteAllScopes( var_info *i, void *cookie )
{
    /* unused parameters */ (void)cookie;

    while( i->s != NULL ) {
        VarFreeScopeList( i, i->s );
    }
    i->s = NilScope();
    return( false );
}


bool VarDeleteAScope( var_info *i, void *cookie )
{
    scope_state *s, *oldest, *outer;

    /* unused parameters */ (void)cookie;

    if( i->mem_lock )
        return( false );
    oldest = NULL;
    for( s = i->s->next; s != NULL; s = s->next ) {
        if( oldest == NULL || s->scope_timestamp < oldest->scope_timestamp ) {
            outer = i->s;
            for( outer = i->s; outer != s; outer = outer->outer ) {
                if( outer == NULL ) {
                    oldest = s;
                    break;
                }
            }
        }
    }
    if( oldest != NULL ) {
        VarFreeScopeList( i, oldest );
        return( true );
    } else {
        return( VarFreeOldExpansion( i->s->v ) );
    }
}


bool VarUnMap( var_info *i, void *image )
{
    scope_state *s, *next;
    mod_handle  mod;

    for( s = i->s; s != NULL; s = next ) {
        next = s->next;
        if( s->unmapped )
            continue;
        mod = s->mod;
        if( mod != NO_MOD ) {
            VarFreeScopeList( i, s );
            continue;
        }
        if( DeAliasAddrMod( s->scope.addr, &mod ) == SR_NONE )
            continue;
        if( image != ImageEntry( mod ) )
            continue;
        if( UnMapAddress( &s->scope, image ) ) {
            s->unmapped = true;
        } else {
            VarFreeScopeList( i, s );
        }
    }
    return( false ); // keep_going
}


bool VarReMap( var_info *i, void *image )
{
    scope_state *s, *next;
    add_new_var_info    info;
    scope_block scope;

    for( s = i->s; s != NULL; s = next ) {
        next = s->next;
        if( !s->unmapped )
            continue;
        switch( ReMapImageAddress( &s->scope, image ) ) {
        case REMAP_REMAPPED:
            info.i = i;
            info.v = s->v;
            scope.start = s->scope.addr;
            scope.len = s->scope_len;
            scope.unique = s->scope_unique;
            DIPWalkSymList( SS_BLOCK, &scope, AddNewVar, &info );
            s->unmapped = false;
            break;
        case REMAP_ERROR:
            VarFreeScopeList( i, s );
            break;
        }
    }
    return( false ); // keep_going
}


void VarInitInfo( var_info *i )
{
    i->s = NilScope();
    i->exprsp_cacherow = VAR_NO_ROW;
    i->exprsp_cache = NULL;
    i->exprsp_cache_is_error = false;
    i->exprsp_timestamp = 0;
    i->mem_lock = false;
}

void VarFiniInfo( var_info *i )
{
    ExprPurge();
    while( i->s != NULL ) {
        VarFreeScopeList( i, i->s );
    }
}

void VarOkToCache( var_info *i, bool ok )
{
    i->ok_to_cache_exprsp = ok;
    VarKillExprSPCache( i );
}

bool VarInfoRefresh( var_type vtype, var_info *i, address *addr )
{
    scope_list  *nested, *new;
    scope_state *s, *outer;
    bool        repaint;
    scope_block noscope;
    bool        havescope;

    repaint = false;
    *addr = NilAddr;
    switch( vtype ) {
    case VAR_FILESCOPE:
        if( i->s->mod != ContextMod ) {
            repaint = true;
            noscope.start = NilAddr;
            noscope.len = 0;
            noscope.unique = 0;
            NewScope( i, &noscope, ContextMod, &repaint );
        }
        break;
    case VAR_LOCALS:
        _AllocA( nested, sizeof( *nested ) );
        outer = NULL;
        nested->next = NULL;
        havescope = true;
        noscope.start = NilAddr;
        noscope.len = 0;
        noscope.unique = 0;
        if( DeAliasAddrScope( ContextMod, Context.execution, &nested->scope ) == SR_NONE ) {
            nested->scope = noscope;
            repaint = true;
            havescope = false;
        }
        if( !SameScope( &nested->scope, i->s ) ) {
            repaint = true;
            if( havescope ) {
                for( ;; ) {
                    _AllocA( new, sizeof( *new ) );
                    if( DIPScopeOuter( ContextMod, &nested->scope, &new->scope ) == SR_NONE )
                        break;
                    new->next = nested;
                    nested = new;
                }
            }
            while( nested != NULL ) {
                s = NewScope( i, &nested->scope, NO_MOD, &repaint );
                s->outer = outer;
                outer = s;
                nested = nested->next;
            }
        }
        if( outer != NULL )
            *addr = outer->scope.addr;
        break;
    }
    return( repaint );
}

void VarInspectPointer( void )
/**********************/
{
    ExprValue( ExprSP );
    if( ExprSP->ti.kind == TK_ARRAY ) {
        DUIAddrInspect( ExprSP->v.loc.e[0].u.addr );
    } else {
        DUIAddrInspect( ExprSP->v.addr );
    }
}

void VarInspectMemory( void )
/*********************/
{
    LValue( ExprSP );
    DUIAddrInspect( ExprSP->v.loc.e[0].u.addr );
}

void VarInspectCode( void )
/*******************/
{
    ExprValue( ExprSP );
    DUISrcOrAsmInspect( ExprSP->v.addr );
}

bool VarIsLValue( void )
/**********************/
{
    LValue( ExprSP );
    return( ExprSP->v.loc.num == 1 && ExprSP->v.loc.e[0].type == LT_ADDR );
}

void VarGetDepths( var_info *i, var_node *v, int *pdepth, int *pinherit )
/***********************************************************************/
{
    var_node    *curr;

    /* unused parameters */ (void)i;

    *pinherit = 0;
    *pdepth = 0;
    for( curr = v->parent; curr != NULL; curr = curr->parent ) {
        if( curr->node_type == NODE_INHERIT ) {
            ++*pinherit;
        } else {
            ++*pdepth;
        }
    }
}

void VarDoAssign( var_info *i, var_node *v, const char *value )
/*************************************************************/
{
    char        *p;
    char        buff[TXT_LEN];

    p = StrCopy( GetCmdName( CMD_ASSIGN ), buff );
    p = StrCopy( " ", p );
    VarBuildName( i, v, false );
    p = StrCopy( TxtBuff, p );
    p = StrCopy( "=", p );
    p = StrCopy( value, p );
    if( AdvMachState( ACTION_MODIFY_VARIABLE ) ) {
        _SwitchOn( SW_RECORD_LOCATION_ASSIGN );
        if( Spawn( DoAssign ) == 0 ) {
            RecordEvent( buff );
        }
        _SwitchOff( SW_RECORD_LOCATION_ASSIGN );
    }
    CollapseMachState();
}

bool VarParentIsArray( var_node * v )
{
    var_node            *vparent = v;
    dig_type_info       ti;

    while( vparent->parent != NULL ) {
        if( vparent->parent->node_type != NODE_INHERIT ) {
            vparent = vparent->parent;
            break;
        }
        vparent = vparent->parent;
    }

    if( ( vparent == v ) || ( NULL == vparent ) )
        return( false );

    DIPTypeInfo( vparent->th, NULL, &ti );

    return( ti.kind == TK_ARRAY || vparent->fake_array );
}

