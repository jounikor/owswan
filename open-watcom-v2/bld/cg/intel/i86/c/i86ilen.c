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
* Description:  Instuction size calculation and NOP padding.
*
****************************************************************************/


#include "_cgstd.h"
#include "optwif.h"
#include "model.h"
#include "inslist.h"
#include "encode.h"
#include "x86nopli.h"


static const byte   NopList[] = {
    2,                  /* objlen of first NOP pattern */
    0x89, 0xc0,         /* MOV AX,AX */
    0xfc                /* CLD */
};

const byte * const  NopLists[2] = { NopList, NopList };

static const byte   InsSize[4][OC_DEST_FAR + 1] = {
/*      OC_DEST_SHORT   OC_DEST_NEAR    OC_DEST_CHEAP   OC_DEST_FAR */
{       0,              3,              4,              5 },    /* CALL */
{       2,              3,              0,              5 },    /* JMP */
{       2,              5,              0,              0 },    /* JCOND */
{       2,              4,              0,              0 },    /* JCOND,386 */
};


obj_length  OptInsSize( oc_class class, oc_dest_attr attr )
/*********************************************************/
{
    obj_length  i;

    switch( class ) {
    default:
    case OC_CALL:
        i = 0;
        break;
    case OC_JMP:
        i = 1;
        break;
    case OC_LREF:
        return( 2 );
    case OC_JCOND:
        i = _CPULevel( CPU_386 ) ? 3 : 2;
        break;
    }
    return( InsSize[i][attr] );
}
