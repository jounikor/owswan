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
* Description:  Default Windowing - Start/exit Windows Win16, WIN386 and Win32
*
****************************************************************************/

#include "variety.h"
#include "widechar.h"
#include <stdio.h>
#ifdef __NT__
    #include <ctype.h>
#endif
#define INCLUDE_COMMDLG_H
#include <wwindows.h>
#include "win.h"
#include "strdup.h"
#include "initarg.h"
#include "defwin.h"
#include "wclbproc.h"
#include "initwin.h"
#include "wintitle.h"
#include "winmenu.rh"


#ifdef __WINDOWS__
#pragma aux __init_default_win "*";
char    __init_default_win;
#endif

#ifdef DEFAULT_WINDOWING

static char *mainClass;

static BOOL firstInstance( HANDLE );
static int windowsInit( HANDLE, int );
static void windowsFini( void );


#if defined( __NT__ )

_WCRTLINK void  __InitDefaultWin( void )
{
    char        *str;
    HANDLE      inst;

    str = __clib_strdup( GetCommandLine() );
    while( !isspace( (unsigned char)*str ) && *str != '\0' )
        str++;
    while( isspace( (unsigned char)*str ) )
        str++;
    inst = GetModuleHandle( NULL );
    if( !firstInstance( inst ) )
//        return( FALSE );
        return;
    if( !windowsInit( inst, SW_SHOWDEFAULT ) )
//        return( FALSE );
        return;
    _InitFunctionPointers();
//    return( TRUE );
}

_WCRTLINK void  __FiniDefaultWin( void )
{
    windowsFini();
}

#endif

/*
 * DefaultWinMain - main windows entry point
 */
int PASCAL __export DefaultWinMain( HINSTANCE inst, HINSTANCE previnst,
        LPSTR cmd, int show, int (*pmain)( int, char ** ) )
{
    int rc;

    previnst = previnst;
    cmd = cmd;
    if( !firstInstance( inst ) )
        return( FALSE );
    if( !windowsInit( inst, show ) )
        return( FALSE );
    _InitFunctionPointers();

    rc = pmain( ___Argc, ___Argv );

    windowsFini();
    _WindowsExit();
    return( rc );

} /* DefaultWinMain */


/*
 * firstInstance - initialization at startup
 */
static BOOL firstInstance( HANDLE inst)
{
    char        tmp[128];
    BOOL        rc;
    WNDCLASS    wc;
    HMENU       smf,smh;

    /*
     * set up class names
     */
    sprintf( tmp,"WATCLASS%d", inst );
    mainClass = malloc( strlen( tmp ) + 1 );
    if( mainClass == NULL )
        return( FALSE );
    strcpy( mainClass, tmp );
    sprintf( tmp,"WATSUBCLASS%d", inst );
    _ClassName = malloc( strlen( tmp ) + 1 );
    if( _ClassName == NULL )
        return( FALSE );
    strcpy( _ClassName, tmp );

    /*
     * make a menu (this way, we don't need resources)
     */
    smf = CreateMenu();
    if( smf == NULL )
        return( FALSE );
    AppendMenu( smf, MF_ENABLED, MSG_WRITE, "&Save As ..." );
    AppendMenu( smf, MF_ENABLED, MSG_SETCLEARINT,
                        "Set &Lines Between Auto-Clears ..." );
    AppendMenu( smf, MF_SEPARATOR, 0,NULL );
    AppendMenu( smf, MF_ENABLED, MSG_EXIT, "E&xit" );

    smh = CreateMenu();
    if( smh == NULL )
        return( FALSE );
    AppendMenu( smh, MF_ENABLED, MSG_ABOUT, "&About..." );

    _SubMenuEdit = CreateMenu();
    if( _SubMenuEdit == NULL )
        return( FALSE );
    AppendMenu( _SubMenuEdit, MF_ENABLED, MSG_FLUSH, "&Clear" );
    AppendMenu( _SubMenuEdit, MF_ENABLED, MSG_COPY, "&Copy" );

    _SubMenuWindows = CreateMenu();

    _MainMenu = CreateMenu();
    if( _MainMenu == NULL )
        return( FALSE );
    AppendMenu( _MainMenu, MF_POPUP, (UINT) smf, "&File" );
    AppendMenu( _MainMenu, MF_POPUP, (UINT) _SubMenuEdit, "&Edit" );
    AppendMenu( _MainMenu, MF_POPUP, (UINT) _SubMenuWindows, "&Windows" );
    AppendMenu( _MainMenu, MF_POPUP, (UINT) smh, "&Help" );

    /*
     * register window classes
     */
    wc.style = 0;
    wc.lpfnWndProc = GetWndProc( _MainDriver );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon( (HINSTANCE)NULL, IDI_APPLICATION );
    wc.hCursor = LoadCursor( (HINSTANCE)NULL, IDC_ARROW );
    wc.hbrBackground = GetStockObject( GRAY_BRUSH );
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = mainClass;

    rc = RegisterClass( &wc );
    if( !rc )
        return( FALSE );

    wc.style = 0;
    wc.lpfnWndProc = GetWndProc( _MainDriver );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon( (HINSTANCE)NULL, IDI_APPLICATION );
    wc.hCursor = LoadCursor( (HINSTANCE)NULL, IDC_ARROW );
    wc.hbrBackground = GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = _ClassName;

    rc = RegisterClass( &wc );
    if( !rc )
        return( FALSE );
    return( TRUE );

} /* firstInstance */

/*
 * windowsInit - windows-specific initialization
*/
static int windowsInit( HANDLE inst, int showcmd )
{
    LOGFONT     logfont;
    WORD        x,y;

    /*** Create a font to use ***/
#ifdef _MBCS
    if( __IsDBCS ) {
        _FixedFont = GetStockObject( SYSTEM_FONT );
    } else {
        _FixedFont = GetStockObject( SYSTEM_FIXED_FONT );
    }
#else
    _FixedFont = GetStockObject( SYSTEM_FIXED_FONT );
#endif
    GetObject( _FixedFont, sizeof(LOGFONT), (LPSTR) &logfont );
    _FixedFont = CreateFontIndirect( &logfont );

    x = GetSystemMetrics( SM_CXSCREEN );
    y = GetSystemMetrics( SM_CYSCREEN );

    _InitMainWindowData( inst );

    _MainWindow = CreateWindow(
        mainClass,                      /* our class                  */
        __WinTitleBar,                  /* Text for window title bar  */
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  /* Window style.      */
        0,                              /* horizontal position.       */
        0,                              /* vertical position.         */
        x,                              /* width.                     */
        y,                              /* height.                    */
        (HWND)NULL,                     /* parent                     */
        _MainMenu,                      /* menu handle                */
        inst,                           /* owner of window            */
        NULL                            /* extra data pointer         */
    );

    if( !_MainWindow ) {
        return( FALSE );
    }

    /*
     * display the window
     */
    ShowWindow( _MainWindow, showcmd );
    UpdateWindow( _MainWindow );

    /*
     * create standard IO window - takes output from stdout, stderr and
     *                             input from stdin
     */
    _NewWindow( "Standard IO", stdin->_handle, stdout->_handle, stderr->_handle, -1 );
    return( TRUE );

} /* windowsInit */

/*
 * windowsFini - windows-specific initialization
*/
static void windowsFini( void )
{
    _FiniMainWindowData();

} /* windowsFini */

#endif
