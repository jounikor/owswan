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
* Description:  DDE Spy main module.
*
****************************************************************************/


#include "wddespy.h"
#include "jdlg.h"
#include "ddemem.h"


#define MAIN_CLASS      "WDDE_MAIN_CLASS"

/*
 * firstInstInit - register classes and do other initializiation that
 *                 is only done by the first instance of the spy
 */
static bool firstInstInit( void )
{
    WNDCLASS    wc;

    /* main window */
    wc.style = 0L;
    wc.lpfnWndProc = DDEMainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof( LONG_PTR );
    wc.hInstance = Instance;
    wc.hIcon = LoadIcon( Instance, "APPLICON" );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszMenuName = "DDEMENU";
    wc.lpszClassName = MAIN_CLASS;
    if( !RegisterClass( &wc ) ) {
        return( false );
    }

    /* tracking windows */
    wc.style = 0L;
    wc.lpfnWndProc = DDETrackingWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof( LONG_PTR );
    wc.hInstance = Instance;
    wc.hIcon = NULL /* LoadIcon( Instance, "HEAPICON" ) */;
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TRACKING_CLASS;
    if( !RegisterClass( &wc ) ) {
        return( FALSE );
    }
    RegPushWin( Instance );
    return( true );

} /* firstInstInit */

/*
 * everyInstInit - do initialization required by every instance of the spy
 */
static bool everyInstInit( int cmdshow )
{
    JDialogInit();
    ReadConfig();
#ifndef NOUSE3D
    CvrCtl3DInit( Instance );
    CvrCtl3dRegister( Instance );
    CvrCtl3dAutoSubclass( Instance );
#endif
    HintWndInit( Instance, NULL, 0 );

    DDEMainWnd = CreateWindow(
        MAIN_CLASS,             /* Window class name */
        AppName,                /* Window caption */
        WS_OVERLAPPEDWINDOW,    /* Window style */
        MainWndConfig.xpos,     /* Initial X position */
        MainWndConfig.ypos,     /* Initial Y position */
        MainWndConfig.xsize,    /* Initial X size */
        MainWndConfig.ysize,    /* Initial Y size */
        NULL,                   /* Parent window handle */
        NULL,                   /* Window menu handle */
        Instance,               /* Program instance handle */
        NULL );                 /* Create parameters */

    if( DDEMainWnd == NULL ) {
        return( false );
    }
    if( !CreateTrackWnd() ) {
        return( false );
    }
    InitTrackWnd( DDEMainWnd );

    ShowWindow( DDEMainWnd, cmdshow );
    UpdateWindow( DDEMainWnd );
    return( true );

} /* everyInstInit */

/*
 * WinMain - entry point for the application
 */
int PASCAL WinMain( HINSTANCE currinst, HINSTANCE previnst, LPSTR cmdline, int cmdshow )
{
    MSG         msg;
    int         rc;

    /* unused parameters */ (void)cmdline;
    Instance = currinst;
    SetInstance( Instance );

    rc = 1;
    MemOpen();
    if( rc && !InitGblStrings() ) {
        MessageBox( NULL, "Unable to find string resources", AppName, MB_OK );
        rc = 0;
    }
    if( rc && previnst == NULL ) {
        if( !firstInstInit() ) {
            rc = 0;
        }
    }
    if( rc && !everyInstInit( cmdshow ) ) {
        rc = 0;
    }

    if( rc ) {
        while( GetMessage( &msg, NULL, 0, 0 ) ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        DdeUninitialize( DDEInstId );
        JDialogFini();
#ifndef NOUSE3D
        CvrCtl3dUnregister( Instance );
        CvrCtl3DFini( Instance );
#endif
    }
    MemClose();
    return( rc );

} /* WinMain */
