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
* Description:  386 implementation of __CMain().
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdio.h>
#include <stdlib.h>
#include "initarg.h"
#include "rtstack.h"
#include "cmain.h"
#include "cominit.h"


#ifdef __WIDECHAR__
    extern      int     wmain( int, wchar_t ** );

    void __wCMain( void )
    {
        /* allocate alternate stack for F77 */
        __ASTACKPTR = (char *)__alloca( __ASTACKSIZ ) + __ASTACKSIZ;

        __CommonInit();
        exit( wmain( ___wArgc, ___wArgv ) );
        // never return
    }
#else
    extern      int     main( int, char ** );

    void __CMain( void )
    {
        /* allocate alternate stack for F77 */
        __ASTACKPTR = (char *)__alloca( __ASTACKSIZ ) + __ASTACKSIZ;

        __CommonInit();
        exit( main( ___Argc, ___Argv ) );
        // never return
    }
#endif
