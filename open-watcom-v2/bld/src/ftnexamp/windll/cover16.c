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


/*
 *  COVER16.C
 */

#include <stdio.h>
#include <windows.h>    /* required for all Windows applications */
#include "cover16.h"


long (FAR PASCAL * DLL_1)(long,long,long);
long (FAR PASCAL * DLL_2)(long,long);

long FAR PASCAL __export Function1( long var1,
                                     long var2,
                                     long var3 )
{
    return( DLL_1( var1, var2, var3 ) );
}

long FAR PASCAL __export Function2( long var1, long var2 )
{
    return( DLL_2( var1, var2 ) );
}

#pragma off (unreferenced);
extern BOOL FAR PASCAL LibMain( HINSTANCE hInstance, WORD wDataSegment, WORD wHeapSize, LPSTR lpszCmdLine )
#pragma on (unreferenced);
{
    HINSTANCE hlib;

    /* Do our DLL initialization */
    hlib = LoadLibrary( "vbdll32.dll" );
    if( (UINT)hlib < 32 ) {
        MessageBox( (HWND)NULL,
                    "Make sure your PATH contains VBDLL32.DLL",
                    "COVER16", MB_OK | MB_ICONEXCLAMATION );
        return( FALSE );
    }
    DLL_1 = (long(FAR PASCAL *)(long,long,long))GetProcAddress( hlib, "DLL1" );
    DLL_2 = (long(FAR PASCAL *)(long,long))GetProcAddress( hlib, "DLL2" );
    return( TRUE );
}
