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


//
// IFMOD        : remainder function for INTEGER arguments
//

#include "ftnstd.h"
#include "ifenv.h"
#include "errcod.h"
#include "fmthcode.h"


intstar4 MOD( intstar4 arg1, intstar4 arg2 ) {
//============================================

// Return arg1 mod arg2.

    if( arg2 == 0 ) {
        return( __imath2err( FP_FUNC_MOD | M_DOMAIN | V_ZERO, &arg1, &arg2 ) );
    }
    return( arg1 % arg2 );
}

intstar4 XMOD( intstar4 *arg1, intstar4 *arg2 ) {
//===============================================

    return( MOD( *arg1, *arg2 ) );
}
