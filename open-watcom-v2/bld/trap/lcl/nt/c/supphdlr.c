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
* Description:  Supplemental request handler.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "stdnt.h"

/* Do not like this! */

extern BOOL                  Supporting8ByteBreakpoints;
extern BOOL                  SupportingExactBreakpoints;

trap_retval TRAP_CAPABILITIES( get_8b_bp )( void )
{
    capabilities_get_8b_bp_req  *req;
    capabilities_get_8b_bp_ret  *ret;

    req = GetInPtr( 0 );
    ret = GetOutPtr( 0 );

    ret->err = 0;
    ret->status = 1;            /* This signals we support 8 byte breakpoints */
    return( sizeof( *ret ) );
}

trap_retval TRAP_CAPABILITIES( set_8b_bp )( void )
{
    capabilities_set_8b_bp_req  *req;
    capabilities_set_8b_bp_ret  *ret;

    req = GetInPtr( 0 );
    ret = GetOutPtr( 0 );

    Supporting8ByteBreakpoints = req->status ? 1 : 0;

    ret->err = 0;
    ret->status = Supporting8ByteBreakpoints ? 1 : 0;   /* And are we supporting it? */
    return( sizeof( *ret ) );
}

trap_retval TRAP_CAPABILITIES( get_exact_bp )( void )
{
    capabilities_get_exact_bp_req  *req;
    capabilities_get_exact_bp_ret  *ret;

    req = GetInPtr( 0 );
    ret = GetOutPtr( 0 );

    ret->err = 0;
    ret->status = 1;            /* This signals we support exact breakpoints */
    return( sizeof( *ret ) );
}

trap_retval TRAP_CAPABILITIES( set_exact_bp )( void )
{
    capabilities_set_exact_bp_req  *req;
    capabilities_set_exact_bp_ret  *ret;

    req = GetInPtr( 0 );
    ret = GetOutPtr( 0 );

    SupportingExactBreakpoints = req->status ? 1 : 0;

    ret->err = 0;
    ret->status = SupportingExactBreakpoints ? 1 : 0;
    return( sizeof( *ret ) );
}
