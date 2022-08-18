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
* Description:  Dump register tree.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "confldef.h"
#include "regset.h"
#include "dumpio.h"
#include "dumpconf.h"
#include "dumpins.h"
#include "dumpregt.h"


static  void    DumpIndent( int i )
/*********************************/
{
    while( --i >= 0 ) {
        DumpChar( ' ' );
    }
}


static  void    DumpRegs( hw_reg_set *regs ) {
/********************************************/

    int     i;
    bool    first;

    DumpLiteral( "Choices " );
    if( regs != NULL ) {
        first = true;
        for( i = REG_COUNT; i > 0; --i ) {
            if( !HW_CEqual( *regs, HW_EMPTY ) ) {
                if( first ) {
                    first = false;
                } else {
                    DumpChar( ',' );
                }
                DumpRegName( *regs );
            }
            ++regs;
        }
    }
}


static  void    DoDump( reg_tree *tree, int indent ) {
/****************************************************/

    DumpIndent( indent );
    DumpRegs( tree->regs );
    DumpNL();
    DumpIndent( indent );
    DumpLiteral( "offset " );
    DumpInt( tree->offset );
    DumpLiteral( " size " );
    DumpInt( tree->size );
    DumpNL();
    if( tree->temp != NULL ) {
        DumpIndent( indent );
        DumpLiteral( "name " );
        DumpPtr( tree->temp );
        DumpChar( ' ' );
        DumpSym( tree->temp );
        DumpNL();
    }
    if( tree->alt != NULL ) {
        DumpIndent( indent );
        DumpLiteral( "alt  " );
        DumpPtr( tree->alt  );
        DumpChar( ' ' );
        DumpSym( tree->alt );
        DumpNL();
    }
    if( tree->hi != NULL ) {
        DumpIndent( indent );
        DumpLiteral( "high " );
        DumpNL();
        DoDump( tree->hi, indent + 4 );
    }
    if( tree->lo != NULL ) {
        DumpIndent( indent );
        DumpLiteral( "low  " );
        DumpNL();
        DoDump( tree->lo, indent + 4 );
    }
}


void    DumpRegTree( reg_tree *tree )
/***********************************/
{
    if( tree != NULL ) {
        DoDump( tree, 0 );
    }
}
