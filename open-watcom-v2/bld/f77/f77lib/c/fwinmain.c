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
* Description:  FORTRAN 77 run-time mainline for Windows environments
*
****************************************************************************/

#include "ftnstd.h"
#ifdef __WINDOWS__
  #include <windows.h>
#elif defined( __NT__ )
  // The mechanism used for mangling the runtime library conflicts with the
  // NT header files on one definition.  This should be avoided.
  #include <windows.h>
  #ifdef SetForm
    #undef SetForm
  #endif
#endif
#include "rtspawn.h"
#include "rt_init.h"
#include "fwinmain.h"


// Leave this forward declaration to avoid polluting "ftextfun.h"
// with Windows headers just for one function

static  int             RetCode;
static  HANDLE          PrevHandle;
static  LPSTR           CmdLine;
static  int             CmdShow;
static  HANDLE          PgmHandle;


static  void    CallFWINMAIN( void )
//==================================
{
    RetCode = FWINMAIN( PgmHandle, PrevHandle, CmdLine, CmdShow );
}


int PASCAL  WinMain( HINSTANCE thishandle, HINSTANCE prevhandle, LPSTR cmdline, int cmdshow )
//===========================================================================================
{
    PgmHandle = thishandle;
    PrevHandle = prevhandle;
    CmdLine = cmdline;
    CmdShow = cmdshow;
    RTSysInit();
    RTSpawn( &CallFWINMAIN );

    return( RetCode );
}
