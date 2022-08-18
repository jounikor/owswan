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
* Description:  Dummy stack check routines for non-x86 Win32 platforms.
*
****************************************************************************/


#include "variety.h"
#include <alloca.h>
#include <windows.h>
#include "rtstack.h"
#include "stacklow.h"
#include "exitwmsg.h"
#include "thread.h"
#include "stkoverf.h"
#include "rtinit.h"


_WCRTLINK unsigned stackavail( void )
{
#if defined( _M_IX86 )
    return( _SP() - _RWD_stacklow );
#else
    unsigned    _SP;

    _SP = (unsigned)&_SP;
    return( _SP - _RWD_stacklow );
#endif
}

#if !defined( _M_IX86 )
_WCRTLINK _WCNORETURN void __STKOVERFLOW( void )
{
    __fatal_runtime_error( "stack overflow", -1 );
    // never return
}

static void _init_stk( void )
{
}

AXI( _init_stk, INIT_PRIORITY_LIBRARY )
#endif

