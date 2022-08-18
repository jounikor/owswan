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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <dos.h>
#include "tinyio.h"
#include "stdwin.h"
#include "dbgrmsg.h"


static redirect_stdin_ret       *rdRet;
static BOOL                     isInput;
static char                     *fileName;

/*
 * ExecuteRedirect - code that the debugee executes to cause file redirection
 */
void ExecuteRedirect( void )
{
    tiny_ret_t          rc;
    tiny_handle_t       std_hndl;
    tiny_handle_t       *var;

    rdRet->err = 0;
    if( isInput ) {
        std_hndl = TINY_IN;
        var = &SaveStdIn;
    } else {
        std_hndl = TINY_OUT;
        var = &SaveStdOut;
    }
    if( *fileName == '\0' ) {
        if( *var != NIL_HANDLE ) {
            rc = TinyDup2( *var, std_hndl );
            if( TINY_ERROR( rc ) ) {
                rdRet->err = 1;
            } else {
                TinyClose( *var );
                *var = NIL_HANDLE;
            }
        }
    } else {
        if( *var == NIL_HANDLE ) *var = TinyDup( std_hndl );
        if( isInput ) {
            rc = TinyOpen( fileName, TIO_READ );
        } else {
            rc = TinyCreate( fileName, TIO_NORMAL );
        }
        if( TINY_ERROR( rc ) ) {
            rdRet->err = 1;
        } else {
            TinyDup2( TINY_INFO( rc ), std_hndl );
            TinyClose( TINY_INFO( rc ) );
        }
    }
} /* ExecuteRedirect */

/*
 * doRedirect:
 *
 * do file redirection by swapping to the debugee, and having him run
 * the ExecuteRedirect code.
 */
static trap_elen doRedirect( BOOL isin )
{
    rdRet = GetOutPtr( 0 );
    isInput = isin;
    fileName = GetInPtr( sizeof( redirect_stdin_req ) );
    DebuggerWaitForMessage( RUNNING_DEBUGEE, TaskAtFault, RUN_REDIRECT );
    return( sizeof( redirect_stdin_ret ) );
} /* doRedirect */

trap_retval TRAP_CORE( Redirect_stdin )( void  )
{
    return( doRedirect( TRUE ) );
}

trap_retval TRAP_CORE( Redirect_stdout )( void )
{
    return( doRedirect( FALSE ) );
}
