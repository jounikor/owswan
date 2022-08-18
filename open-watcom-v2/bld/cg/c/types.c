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
* Description:  Code generator type system.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "data.h"
#include "types.h"

typedef enum {
        TYPE_DEFINITION,
        TYPE_ALIAS
} type_class;

typedef struct alias {
        struct type_def     *tptr;
        cg_type             refno;
} alias;

typedef union type_alloc {
        struct alias        alias;
        struct type_def     type_def;
} type_alloc;

typedef struct type_list {
        union type_alloc    tipe;
        struct type_list    *link;
        type_class          format;
} type_list;

type_def *PTInteger;
type_def *PTUnsigned;
type_def *PTPointer;
type_def *PTCodePointer;

static    type_list     *TypeList;


/*                          refno           length  attributes */
/*                          ======          ======  ========== */

static type_def TUInt1 = {  TY_UINT_1,       1,      0 };
static type_def TInt1  = {  TY_INT_1,        1,      TYPE_SIGNED };
static type_def TUInt2 = {  TY_UINT_2,       2,      0 };
static type_def TInt2  = {  TY_INT_2,        2,      TYPE_SIGNED };
static type_def TUInt4 = {  TY_UINT_4,       4,      0 };
static type_def TInt4  = {  TY_INT_4,        4,      TYPE_SIGNED };
static type_def TUInt8 = {  TY_UINT_8,       8,      0 };
static type_def TInt8  = {  TY_INT_8,        8,      TYPE_SIGNED };
static type_def TSingle     = {  TY_SINGLE,       4,      TYPE_SIGNED + TYPE_FLOAT };
static type_def TDouble     = {  TY_DOUBLE,       8,      TYPE_SIGNED + TYPE_FLOAT };
static type_def TLongDouble = {  TY_LONG_DOUBLE,  10,     TYPE_SIGNED + TYPE_FLOAT };
static type_def TBool       = {  TY_BOOLEAN,      0,      0 };
static type_def TNull       = {  TY_DEFAULT,      0,      0 };
static type_def TPascal     = {  TY_PROC_PARM,    4,      0 };

type_def        *TypeAddress( cg_type tipe )
/*******************************************
    given a type refno, "tipe" which is known to the front end, return a
    pointer to the appropriate "type_def".
*/
{
    type_list   *list;

    switch( tipe ) {

    case TY_UINT_1:
        return( &TUInt1 );
    case TY_INT_1:
        return( &TInt1 );
    case TY_UINT_2:
        return( &TUInt2 );
    case TY_INT_2:
        return( &TInt2 );
    case TY_UINT_4:
        return( &TUInt4 );
    case TY_INT_4:
        return( &TInt4 );
    case TY_UINT_8:
        return( &TUInt8 );
    case TY_INT_8:
        return( &TInt8 );
    case TY_NEAR_CODE_PTR:
        return( &TNearCP );
    case TY_LONG_CODE_PTR:
        return( &TLongCP );
    case TY_NEAR_POINTER:
        return( &TNearP );
    case TY_LONG_POINTER:
        return( &TLongP );
    case TY_HUGE_POINTER:
        return( &THugeP );
    case TY_SINGLE:
        return( &TSingle );
    case TY_DOUBLE:
        return( &TDouble );
    case TY_LONG_DOUBLE:
        return( &TLongDouble );
    case TY_BOOLEAN:
        return( &TBool );
    case TY_DEFAULT:
        return( &TNull );
    case TY_PROC_PARM:
        return( &TPascal );
    case TY_INTEGER:
        return( PTInteger );
    case TY_UNSIGNED:
        return( PTUnsigned );
    case TY_POINTER:
        return( PTPointer );
    case TY_CODE_PTR:
        return( PTCodePointer );
    default:
        for( list = TypeList; list != NULL; list = list->link ) {
            if( list->format == TYPE_DEFINITION ) {
                if( list->tipe.type_def.refno == tipe ) {
                    return( &list->tipe.type_def );
                }
            } else {    /* TYPE_ALIAS */
                if( list->tipe.alias.refno == tipe ) {
                    return( list->tipe.alias.tptr );
                }
            }
        }
        return( NULL );
    }
}


type_length     TypeLength( cg_type tipe )
/*****************************************
    Return the size of a given type "tipe".
*/
{
    type_def    *t;

    t = TypeAddress( tipe );
    return( t->length );
}


type_def        *TypeAlias( cg_type define, cg_type existing )
/*************************************************************
    cause refno "define" to become an alias for existing type "existing"
*/
{
    type_def    *t;
    type_list   *list;

    t = TypeAddress( existing );
    if( t->refno == TY_DEFAULT )
        return( t );
    list = CGAlloc( sizeof( type_list ) );
    list->link = TypeList;
    TypeList = list;
    list->format = TYPE_ALIAS;
    list->tipe.alias.refno = define;
    list->tipe.alias.tptr  = t;
    return( t );
}


type_def        *TypeDef( cg_type refno, type_length length, type_length align )
/*******************************************************************************
    Define a structure/array type which will have handle "refno".  The
    type will occupy "length" bytes of storage.
*/
{
    type_list   *list;

    /* unused parameters */ (void)align;

    list = CGAlloc( sizeof( type_list ) );
    list->link = TypeList;
    TypeList = list;
    list->format = TYPE_DEFINITION;
    list->tipe.type_def.refno  =  refno;
    list->tipe.type_def.length = length;
    list->tipe.type_def.attr   = 0;
#if _TARGET & _TARG_RISC
    list->tipe.type_def.align  = align;
#endif
    return( &list->tipe.type_def );
}


void    TypeFini( void )
/***********************
    Finish up the typeing stuff
*/
{
    type_list   *type;
    type_list   *next;

    for( type = TypeList; type != NULL; type = next ) {
        next = type->link;
        CGFree( type );
    }
}

void    TypeInit( void )
/**********************/
{
    TypeList = NULL;

    TargTypeInit();
    TypeProcParm = TypeAddress( TY_PROC_PARM );
    TypeInteger = TypeAddress( TY_INTEGER );
    TypeHugeInteger = TypeAddress( TY_HUGE_INTEGER );
    TypeLongInteger = TypeAddress( TY_LONG_INTEGER );
    TypeLongLongInteger = TypeAddress( TY_INT_8 );   // should be something else perhaps?
    TypeNearInteger = TypeAddress( TY_NEAR_INTEGER );
    TypeUnsigned = TypeAddress( TY_UNSIGNED );
    TypeBoolean = TypeAddress( TY_BOOLEAN );
    TypeNone = TypeAddress( TY_DEFAULT );
    TypePtr = TypeAddress( TY_POINTER );
}
