/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  compile-time exception handling
*
****************************************************************************/


#include "ftnstd.h"
#include <signal.h>
#include <float.h>
#include "errcod.h"
#include "ferror.h"
#include "rtfpesig.h"
#include "cptraps.h"


#if !defined( __UNIX__ )

static __sigfpe_func        WFC_FPEHandler;

static void _WFC_FPEHandler( int fpe_type )
//=========================================
{
    if( fpe_type == FPE_OVERFLOW ) {
        Warning( KO_FOVERFLOW );
    } else if( fpe_type == FPE_UNDERFLOW ) {
        Warning( KO_FUNDERFLOW );
    } else if( fpe_type == FPE_ZERODIVIDE ) {
        Warning( KO_FDIV_ZERO );
    }
}


static  void    WFC_FPEHandler( int sig_num, int fpe_type )
//=========================================================
{
    /* unused parameters */ (void)sig_num;

    // reset the signal so we can get subsequent signals
    SET_SIGFPE( WFC_FPEHandler );
    _WFC_FPEHandler( fpe_type );
}

#endif /* !defined( __UNIX__ ) */

void    FTrapInit( void )
//=======================
{
#if !defined( __UNIX__ )
    SET_SIGFPE( WFC_FPEHandler );
#endif
}


void    FTrapFini( void )
//=======================
{
}
