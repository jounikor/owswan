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
#include "specfuns.h"

typedef struct {
    const char* name;
    unsigned length;
} SPEC_FUNC;

static SPEC_FUNC funs[] = {     // special functions
    #define SPEC_FUN( code, name ) { name, sizeof(name)-1 }
    #include "_spcfuns.h"
    #undef SPEC_FUN
};

#define SPEC_FUN_END &funs[ARRAY_SIZE( funs )]


SPFN SpecialFunction(           // IDENTIFY SPECIAL FUNCTION
    SYMBOL func )               // - potential special function
{
    SPFN retn;                  // - return: code for that function
    SYMBOL_NAME sn;             // - symbol_name structure
    NAME sname;                 // - name of function
    SPEC_FUNC* spp;             // - current definition
    unsigned sname_len;         // - size of name

    DbgVerify( SymIsFunction( func ), "SpecialFunction -- non-function" );
    sn = func->name;
    retn = SPFN_NONE;
    if( sn != NULL && ScopeIsGlobalNameSpace( sn->containing ) != NULL ) {
        sname = sn->name;
        if( sname != NULL ) {
            sname_len = strlen( NameStr( sname ) );
            for( spp = SPEC_FUN_END - 1; spp != funs; --spp ) {
                if( spp->name[0] == NameStr( sname )[0]
                 && sname_len == spp->length
                 && 0 == memcmp( spp->name, NameStr( sname ), sname_len ) ) break;
            }
            retn = spp - funs;
        }
    }
    return( retn );
}
