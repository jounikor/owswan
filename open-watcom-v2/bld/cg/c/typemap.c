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


#include "_cgstd.h"
#include "coderep.h"
#include "types.h"
#include "procdef.h"
#include "typemap.h"
#include "maptypes.h"


static cg_type  Types[] = {
    #define pick(e,t) t,
    #include "typcldef.h"
    #undef pick
    TY_DEFAULT,
    TY_UNKNOWN
};


type_def    *TypeOfTypeClass( type_class_def type_class )
/*******************************************************/
{
    return( TypeAddress( Types[type_class] ) );
}


type_class_def  ReturnTypeClass( type_def *tipe, call_attributes attr )
/*********************************************************************/
{
    switch( tipe->refno ) {
    case TY_INT_1:
    case TY_UINT_1:
    case TY_INT_2:
    case TY_UINT_2:
    case TY_INT_4:
    case TY_UINT_4:
    case TY_INT_8:
    case TY_UINT_8:
        return( MapIntReturn( tipe->refno ) );
    case TY_NEAR_POINTER:
    case TY_NEAR_CODE_PTR:
    case TY_HUGE_POINTER:
    case TY_LONG_CODE_PTR:
    case TY_LONG_POINTER:
        return( MapPointer( tipe->refno ) );
    case TY_SINGLE:
    case TY_DOUBLE:
    case TY_LONG_DOUBLE:
        return( MapFloat( tipe->refno, attr ) );
    default:
        return( MapStruct( tipe->length, attr ) );
    }
}


type_class_def  TypeClass( type_def *tipe )
/*****************************************/
{
    switch( tipe->refno ) {
    case TY_INT_1:
        return( I1 );
    case TY_UINT_1:
        return( U1 );
    case TY_INT_2:
        return( I2 );
    case TY_UINT_2:
        return( U2 );
    case TY_INT_4:
        return( I4 );
    case TY_UINT_4:
        return( U4 );
    case TY_INT_8:
        return( I8 );
    case TY_UINT_8:
        return( U8 );
    default:
        return( ReturnTypeClass( tipe, EMPTY ) );
    }
}
