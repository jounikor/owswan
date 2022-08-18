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
* Description:  Literal list
*
****************************************************************************/


#include "ftnstd.h"
#include "global.h"
#include "stmtsw.h"
#include "fmemmgr.h"
#include "rstlit.h"


sym_id  STLit( byte *string, size_t len )
//=======================================
// Search the symbol a literal. If the literal is not in the
// symbol table, add it to the symbol table.
{
    sym_id      sym;

    for( sym = LList; sym != NULL; sym = sym->u.lt.link ) {
        if( sym->u.lt.length == len ) {
            if( memcmp( string, &sym->u.lt.value, len ) == 0 ) {
                if( StmtSw & SS_DATA_INIT ) {
                    sym->u.lt.flags |= LT_DATA_STMT;
                } else {
                    sym->u.lt.flags |= LT_EXEC_STMT;
                }
                return( sym );
            }
        }
    }
    sym = FMemAlloc( sizeof( literal ) - 1 + len );
    memcpy( &sym->u.lt.value, string, len );
    sym->u.lt.length = len;
    if( StmtSw & SS_DATA_INIT ) {
        sym->u.lt.flags = LT_DATA_STMT;
    } else {
        sym->u.lt.flags = LT_EXEC_STMT;
    }
    sym->u.lt.address = NULL;
    sym->u.lt.link = LList;
    LList = sym;
    return( sym );
}
