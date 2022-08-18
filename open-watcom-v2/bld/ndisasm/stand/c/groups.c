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


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "dis.h"
#include "global.h"
#include "groups.h"
#include "formasm.h"
#include "buffer.h"
#include "print.h"
#include "main.h"


static int      useComma;

return_val      DumpASMGroupName( const char *name, bool fasm )
{
    assert( name );

    useComma = 0;

    if( fasm ) {
        BufferQuoteName( name );
        BufferConcat( "\t\tGROUP\t" );
    } else {
        BufferConcat( "GROUP: " );
        BufferQuoteText( name, '\'' );
        BufferConcatChar( ' ' );
    }
    return( RC_OKAY );
}


return_val      DumpASMGroupMember( const char *name )
{
    assert( name );

    if( useComma ) {
        BufferConcatChar( ',' );
    }
    BufferQuoteName( name );
    useComma = 1;
    return( RC_OKAY );
}


return_val      DumpASMGroupFini( void )
{
    BufferConcatNL();
    BufferPrint();
    useComma = 0;
    return( RC_OKAY );
}
