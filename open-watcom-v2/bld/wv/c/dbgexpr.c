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
* Description:  Top layer of expression evaluator.
*
****************************************************************************/


#include <limits.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbglit.h"
#include "dbgstk.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "ldsupp.h"
#include "madinter.h"
#include "i64.h"
#include "dbgscan.h"
#include "dbgexpr4.h"
#include "dbgexpr3.h"
#include "dbgexpr2.h"
#include "dbgexpr.h"
#include "dbgloc.h"
#include "dbgovl.h"
#include "dbgreg.h"
#include "addarith.h"
#include "dbglkup.h"


extern void             *DupType( void * );
extern void             FreeType( void * );

static stack_entry ExprBOS = {
    NULL, NULL,
    NULL,
    { 0 },
    SF_END_PURGE
};

stack_entry *ExprSP = &ExprBOS;

void InitLC( location_context *new, bool use_real_regs )
{
    memset( new, 0, sizeof( *new ) );
    new->regs = DbgRegs;
    new->have_stack = true;
    new->maybe_have_frame = true;
    new->maybe_have_object = true;
    new->use = 1;
    if( use_real_regs ) {
        new->execution = GetRegIP();
        new->frame = GetRegBP();
        new->stack = GetRegSP();
    } else {
        if( Context.up_stack_level )
            new->regs = NULL;
        new->execution = Context.execution;
        new->frame = Context.frame;
        new->stack = Context.stack;
        new->maybe_have_frame = Context.maybe_have_frame;
        new->have_frame = Context.have_frame;
    }
}

void CreateLC( stack_entry *entry )
{
    location_context    *new;

    if( entry->lc == NULL ) {
        _ChkAlloc( new, sizeof( *new ), LIT_ENG( ERR_NO_MEMORY_FOR_EXPR ) );
        InitLC( new, false );
        entry->lc = new;
    }
}

void FreeLC( stack_entry *entry )
{
    if( entry->lc != NULL ) {
        entry->lc->use--;
        if( entry->lc->use == 0 ) {
            _Free( entry->lc );
            entry->lc = NULL;
        }
    }
}

void MoveLC( stack_entry *src, stack_entry *dst )
{
    if( src != dst ) {
        FreeLC( dst );
        dst->lc = src->lc;
        src->lc = NULL;
    }
}

void DupLC( stack_entry *entry )
{
    if( entry->lc != NULL ) {
        entry->lc->use++;
    }
}

/*
 * CreateEntry - create a new stack entry
 */

void CreateEntry( void )
{
    stack_entry *new;
    unsigned    size;

    size = sizeof( stack_entry ) + type_SIZE + sym_SIZE;
    _ChkAlloc( new, size, LIT_ENG( ERR_NO_MEMORY_FOR_EXPR ) );
    memset( new, 0, size );
    new->up = ExprSP;
    new->dn = ExprSP->dn;
    if( new->dn != NULL )
        new->dn->up = new;
    if( new->up != NULL )
        new->up->dn = new;
    ExprSP = new;
}

bool AllocatedString( stack_entry *stk )
{
    if( stk->ti.kind != TK_STRING )
        return( false );
    if( stk->flags & SF_LOCATION )
        return( false );
    return( stk->v.string.allocated != NULL );
}


static void FreeEntry( stack_entry *old )
{
    if( AllocatedString( old ) )
        _Free( old->v.string.allocated );
    FreeLC( old );
    _Free( old );
}


/*
 * DeleteEntry - delete a stack entry
 */

void DeleteEntry( stack_entry *old )
{
    if( old == ExprSP )
        ExprSP = old->up;
    if( old->up != NULL )
        old->up->dn = old->dn;
    if( old->dn != NULL )
        old->dn->up = old->up;
    FreeEntry( old );
}


/*
 * StkEntry - find a given stack entry
 */

stack_entry *StkEntry( int amount )
{
    stack_entry *new;

    new = ExprSP;
    for( ; amount > 0; --amount ) {
        if( new == &ExprBOS )
            Error( ERR_LOC+ERR_INTERNAL, LIT_ENG( ERR_STK_OVERFL ) );
        new = new->up;
    }
    for( ; amount < 0; ++amount ) {
        new = new->dn;
        if( new == NULL ) {
            Error( ERR_LOC+ERR_INTERNAL, LIT_ENG( ERR_STK_UNDERFL ) );
        }
    }
    return( new );
}

/*
 * MoveSP - change expression stack pointer
 */

void MoveSP( int amount )
{
    ExprSP = StkEntry( amount );
}


/*
 * SwapStack - exchange two stack elements
 */

void SwapStack( int entry )
{
    stack_entry *other, *temp;

    other = StkEntry( entry );

    if( (temp = ExprSP->up) == other )
        temp = ExprSP;
    ExprSP->up = other->up;
    other->up = temp;

    if( (temp = other->dn) == ExprSP )
        temp = other;
    other->dn = ExprSP->dn;
    ExprSP->dn = temp;

    if( ExprSP->up != NULL )
        ExprSP->up->dn = ExprSP;
    if( ExprSP->dn != NULL )
        ExprSP->dn->up = ExprSP;

    if( other->up != NULL )
        other->up->dn = other;
    if( other->dn != NULL )
        other->dn->up = other;

    ExprSP = other;
}


void FreezeStack( void )
{
    stack_entry *save_sp;

    save_sp = ExprSP;
    while( ExprSP->dn != NULL ) {
        ExprSP = ExprSP->dn;
    }
    CreateEntry();
    ExprSP->flags = SF_END_PURGE;
    ExprSP->v.save_sp = save_sp;
}


void UnFreezeStack( bool nuke_top )
{
    stack_entry *sp;
    if( nuke_top ) {
        while( ExprSP->dn != NULL ) {
            ExprSP = ExprSP->dn;
        }
        while( (ExprSP->flags & SF_END_PURGE) == 0 ) {
            DeleteEntry( ExprSP );
        }
        sp = ExprSP->v.save_sp;
        DeleteEntry( ExprSP );
        ExprSP = sp;
    } else {
        sp = ExprSP;
        if( sp->flags & SF_END_PURGE ) {
            ExprSP = ExprSP->v.save_sp;
        }
        while( (sp->flags & SF_END_PURGE) == 0 ) {
            sp = sp->up;
        }
        DeleteEntry( sp );
    }
}


char *DupStringVal( stack_entry *stk )
{
    char *dest;

    if( stk->ti.size == 0 )
        return( NULL );
    if( stk->ti.size >= UINT_MAX )
        Error( ERR_NONE, LIT_ENG( ERR_NO_MEMORY_FOR_EXPR ) );
    _ChkAlloc( dest, stk->ti.size, LIT_ENG( ERR_NO_MEMORY_FOR_EXPR ) );
    memcpy( dest, stk->v.string.loc.e[0].u.p, stk->ti.size );
    return( dest );
}


/*
 * DupStack - duplicate a stack entry
 */

void DupStack( void )
{
    stack_entry *old, *down, *link;

    old = link = ExprSP;
    while( old->flags & SF_END_PURGE )
        old = old->up;
    CreateEntry();
    down = ExprSP->dn;
    memcpy( ExprSP, old, sizeof( *old ) + type_SIZE + sym_SIZE );
    ExprSP->up = link;
    ExprSP->dn = down;
    DupLC( ExprSP );
    if( old->th != NULL )
        SET_TH( ExprSP );
    if( old->flags & SF_SYM )
        SET_SH( ExprSP );
    if( AllocatedString( old ) ) {
        ExprSP->v.string.allocated = DupStringVal( old );
        ExprSP->v.string.loc.e[0].u.p = ExprSP->v.string.allocated;
    }
}


/*
 * PushName -- push a name onto the stack
 */

void PushName( lookup_item *li )
{
    CreateEntry();
    ExprSP->flags = SF_NAME;
    ExprSP->v.li = *li;
}


/*
 * PushNum -- push an integer number
 */

void PushNum( long val )
{
    CreateEntry();
    I32ToI64( val, &ExprSP->v.sint );
    ClassNum( ExprSP );
}


/*
 * PushNum64 -- push an 64-bit integer number
 */

void PushNum64( unsigned_64 val )
{
    CreateEntry();
    ExprSP->v.sint = val;
    ClassNum( ExprSP );
}


/*
 * PushRealNum -- push a real number
 */

void PushRealNum( xreal val )
{
    CreateEntry();
    ExprSP->v.real = val;
    ExprSP->ti.kind = TK_REAL;
    ExprSP->ti.size = sizeof( xreal );
    ExprSP->flags = SF_CONST;
}

/*
 * PushSymHandle -- push a symbol handle
 */

void PushSymHandle( sym_handle *sh )
{
    CreateEntry();
    CreateLC( ExprSP );
    ExprSymbol( ExprSP, sh );
}


void ExprSetAddrInfo( stack_entry *stk, bool trunc )
{
    mad_type_info mti;

    stk->ti.kind = TK_ADDRESS;
    stk->ti.modifier = TM_FAR;
    GetMADTypeDefaultAt( stk->v.addr, MTK_ADDRESS, &mti );
    stk->ti.size = BITS2BYTES( mti.b.bits );
    if( trunc ) {
        stk->v.addr.mach.offset &= ~0UL >> ( sizeof( addr48_off ) * 8 - ( mti.b.bits - mti.a.seg.bits ) );
    }
}

/*
 * PushAddr -- push an address on the stack
 */

void PushAddr( address addr )
{
    CreateEntry();
    ExprSP->v.addr = addr;
    AddrFix( &ExprSP->v.addr );
    ExprSetAddrInfo( ExprSP, false );
}

void PushLocation( location_list *ll, dig_type_info *ti )
{
    CreateEntry();
    if( ti != NULL )
        ExprSP->ti = *ti;
    ExprSP->v.loc = *ll;
    ExprSP->flags |= SF_LOCATION;
}


/*
 * CombinEntries -- combine the SF_CONST bits of the left and optional right
 *            entries into the dest entry. Be careful of aliases.
 */

void CombineEntries( stack_entry *dest, stack_entry *l, stack_entry *r )
{
    stack_flags         f;
    stack_entry         *lc_src;

    f = l->flags;
    lc_src = l;
    if( r != NULL ) {
        f &= r->flags;
        if( r->lc != NULL ) {
            lc_src = r;
        }
    }
    MoveLC( lc_src, dest );
    dest->flags &= ~SF_CONST;
    dest->flags |= f & SF_CONST;
    if( l != dest )
        DeleteEntry( l );
    if( r != NULL && r != dest ) {
        DeleteEntry( r );
    }
}


/*
 * MoveTH - move a type handle from one stack entry to another
 */

void MoveTH( stack_entry *old, stack_entry *new )
{
    if( old->th != NULL ) {
        SET_TH( new );
        HDLAssign( type, new->th, old->th );
    }
}


/*
 * PushType -- push a type on the stack
 */

void PushType( type_handle *th )
{
    CreateEntry();
    ExprSP->flags = SF_LOCATION;
    SET_TH( ExprSP );
    HDLAssign( type, ExprSP->th, th );
    ClassifyEntry( ExprSP, &ExprSP->ti );
}

/*
 * PushInt - push an integer constant on the stack
 */

void PushInt( int val )
{
    CreateEntry();
    I32ToI64( val, &ExprSP->v.sint );
    ExprSP->ti.kind = TK_INTEGER;
    ExprSP->ti.modifier = TM_SIGNED;
    ExprSP->ti.size = sizeof( ExprSP->v.sint );
}


static void PushBool( int val )
{
    CreateEntry();
    I32ToI64( val, &ExprSP->v.sint );
    ExprSP->ti.kind = TK_BOOL;
    ExprSP->ti.modifier = TM_NONE;
    ExprSP->ti.size = 1;
}


/*
 * PushString - push the created string onto the stack
 */

void PushString( void )
{
    //NYI: This interface sucks. Hidden static variables. :-(
    CreateEntry();
    ExprSP->ti.kind = TK_STRING;
    ExprSP->ti.size = StringLength;
    ExprSP->v.string.allocated = StringStart;
    LocationCreate( &ExprSP->v.string.loc, LT_INTERNAL, StringStart );
    StringStart = NULL;
    StringLength = 0;
}


/*
 * PopEntry - pop a stack entry
 */

void PopEntry( void )
{
    if( ExprSP == &ExprBOS )
        Error( ERR_LOC+ERR_INTERNAL, LIT_ENG( ERR_STK_UNDERFL ) );
    DeleteEntry( ExprSP );
}


/*
 * FStrCmp -- compare two strings, padding shorter one with spaces aka FORTRAN
 */
static int FStrCmp( char *str1, unsigned len1, char *str2, unsigned len2 )
{
    unsigned long   max, count;
    char            c1, c2;

    max = (len1 > len2)  ?  len1  :  len2;
    for( count = 0; count < max; ++count ) {
        c1 = (count < len1)  ?  *str1++  :  ' ';
        c2 = (count < len2)  ?  *str2++  :  ' ';
        if( c1 != c2 ) {
            return(  c1 - c2 );
        }
    }
    return( 0 );
}



/*
 * TstEQ - test for equality
 */
int TstEQ( int true_value )
{
    stack_entry *left, *rite;
    int temp;

    left = StkEntry( 1 );
    rite = ExprSP;
    BinOp( left, rite );
    switch( left->ti.kind ) {
    case TK_BOOL:
    case TK_ENUM:
    case TK_CHAR:
    case TK_INTEGER:
        temp = (U64Cmp( &left->v.uint, &rite->v.uint ) == 0);
        break;
    case TK_ADDRESS:
    case TK_POINTER:
        temp = (AddrComp(left->v.addr,rite->v.addr) == 0);
        break;
    case TK_REAL:
        temp = (LDCmp( &left->v.real, &rite->v.real ) == 0);
        break;
    case TK_COMPLEX:
        temp = (LDCmp( &left->v.cmplx.re, &rite->v.cmplx.re ) == 0) &&
               (LDCmp( &left->v.cmplx.im, &rite->v.cmplx.im ) == 0);
        break;
    case TK_STRING:
        temp = FStrCmp( left->v.string.loc.e[0].u.p, left->ti.size,
                        rite->v.string.loc.e[0].u.p, rite->ti.size ) == 0;
        break;
    default:
        temp = 0;
        Error( ERR_NONE, LIT_ENG( ERR_ILL_TYPE ) );
        break;
    }
    PushBool( temp ? true_value : 0 );
    CombineEntries( ExprSP, left, rite );
    return( I32FetchTrunc( ExprSP->v.sint ) );
}


/*
 * TstLT - test for less than
 */
int TstLT( int true_value )
{
    stack_entry *left, *rite;
    int temp;

    left = StkEntry( 1 );
    rite = ExprSP;
    BinOp( left, rite );
    switch( left->ti.kind ) {
    case TK_BOOL:
    case TK_ENUM:
    case TK_CHAR:
    case TK_INTEGER:
        if( left->ti.modifier == TM_UNSIGNED ) {
            temp = ( U64Cmp( &left->v.uint, &rite->v.uint ) < 0 );
        } else {
            temp = ( I64Cmp( &left->v.sint, &rite->v.sint ) < 0 );
        }
        break;
    case TK_ADDRESS:
    case TK_POINTER:
        temp = (AddrComp(left->v.addr,rite->v.addr) < 0);
        break;
    case TK_REAL:
        temp = (LDCmp( &left->v.real, &rite->v.real ) < 0);
        break;
    case TK_STRING:
        temp = FStrCmp( left->v.string.loc.e[0].u.p, left->ti.size,
                        rite->v.string.loc.e[0].u.p, rite->ti.size ) < 0;
        break;
    default:
        temp = 0;
        Error( ERR_NONE, LIT_ENG( ERR_ILL_TYPE ) );
        break;
    }
    PushBool( temp ? true_value : 0 );
    CombineEntries( ExprSP, left, rite );
    return( I32FetchTrunc( ExprSP->v.sint ) );
}


/*
 * TstTrue - set to false or true and return result
 */
int TstTrue( int true_value )
{
    PushInt( 0 );
    TstEQ( true_value );
    PushInt( true_value );
    DoXor();
    return( I32FetchTrunc( ExprSP->v.sint ) );
}


/*
 * TstExist - test if a variable exists or not
 */
int TstExist( int true_value )
{
    bool        tst;
    sym_list    *syms;

    if( ExprSP->flags & SF_NAME ) {
        syms = ExprGetSymList( ExprSP, false );
        if( syms != NULL ) {
            PurgeSymHandles();
            tst = true;
        } else {
            tst = false;
        }
    } else {
        tst = true;
    }
    PopEntry();
    PushBool( tst ? true_value : 0 );
    return( I32FetchTrunc( ExprSP->v.sint ) );
}


/*
 * MakeAddr - convert two stack entries into a segment/offset address
 */

void MakeAddr( void )
{
    stack_entry *left;
    addr48_off  offset;

    left = StkEntry( 1 );
    RValue( left );
    ConvertTo( left, TK_INTEGER, TM_UNSIGNED, sizeof( word ) );
    RValue( ExprSP );
    ConvertTo( ExprSP, TK_INTEGER, TM_UNSIGNED, sizeof( addr48_off ) );
    //NYI: 64 bit offsets
    offset = U32FetchTrunc( ExprSP->v.uint );
    /* NYI: address abstraction lost */
    ExprSP->v.addr = NilAddr;
    ExprSP->v.addr.mach.offset = offset;
    ExprSP->v.addr.mach.segment = U32FetchTrunc( left->v.uint );
    AddrFloat( &ExprSP->v.addr );
    ExprSetAddrInfo( ExprSP, true );
    CombineEntries( ExprSP, left, ExprSP );
}



void FreePgmStack( bool freeall )
{
    unsigned    count, newlevel;
    addr_ptr    stk;

    count = ( freeall  ?  NestedCallLevel + 1  :  1 );
    newlevel = ( freeall  ?  0  :
                    ( (NestedCallLevel > 0)  ?  NestedCallLevel - 1  :  0 ) );
    MADRegSpecialGet( MSR_SP, &DbgRegs->mr, &stk );
    for( ; count > 0; --count ) {
        stk.offset += PgmStackUsage[NestedCallLevel];
        PgmStackUsage[NestedCallLevel] = 0;
        --NestedCallLevel;
    }
    MADRegSpecialSet( MSR_SP, &DbgRegs->mr, &stk );
    NestedCallLevel = newlevel;
}



/*
 * ExprPurge - clean up expression stack
 */

void ExprPurge( void )
{
    stack_entry *stk_ptr, *next_ptr;

    for( stk_ptr = ExprSP; stk_ptr->dn != NULL; stk_ptr = stk_ptr->dn )
        ;
    for( ; (stk_ptr->flags & SF_END_PURGE) == 0; stk_ptr = next_ptr ) {
        next_ptr = stk_ptr->up;
        FreeEntry( stk_ptr );
    }
    stk_ptr->dn = NULL;
    ExprSP = stk_ptr;
    if( StringStart != NULL )
        _Free( StringStart );
    FreePgmStack( true );
}

void MarkArrayOrder( bool column_major )
{
    if( column_major ) {
        _SwitchOn( SW_COL_MAJ_ARRAYS );
    } else {
        _SwitchOff( SW_COL_MAJ_ARRAYS );
    }
}

void StartSubscript( void )
{
    LValue( ExprSP );
}

void AddSubscript( void )
{
    stack_entry     *array;
    array_info      ai;
    dig_type_info   ti;
    stack_flags     save_imp;
    DIPHDL( type, th );

    array = StkEntry( 1 );
    save_imp = array->flags & SF_IMP_ADDR;
    switch( array->ti.kind ) {
    case TK_ARRAY:
        DIPTypeArrayInfo( array->th, array->lc, &ai, th );
        PushType( th );
        SwapStack( 1 );
        DoConvert();
        break;
    case TK_POINTER:
        save_imp = 0;
        DIPTypeBase( array->th, th, NULL, NULL );
        DIPTypeInfo( th, array->lc, &ti );
        ai.stride = ti.size;
        ai.low_bound = 0;
        RValue( ExprSP );
        ConvertTo( ExprSP, TK_INTEGER, TM_SIGNED, sizeof( ExprSP->v.sint ) );
        break;
    default:
        Error( ERR_NONE, LIT_ENG( ERR_ILL_TYPE ) );
    }
    PushNum( ai.low_bound );
    DoMinus();
    PushNum( ai.stride );
    DoMul();
    DoPlus();
    DoPoints( TK_NONE );
    ExprSP->flags |= save_imp;
}

void EndSubscript( void )
{
}
