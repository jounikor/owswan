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
* Description:  Execution sampler for Windows mainline.
*
****************************************************************************/


#include <stdio.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "commonui.h"
#include "sample.h"
#include "smpstuff.h"
#include "wmsg.h"
#include "sampwin.h"
#include "wsample.rh"
#include "wreslang.h"
#include "wclbproc.h"
#include "di386cli.h"


#define TMPSLEN 256
#define NOT_OK 1
#define A_OK 2

static char     sampleClass[] = "WsamplewClass";

/*
 * About - about dialog message handler
 */
static BOOL FAR PASCAL About( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    lparam = lparam;

    switch( message ) {
    case WM_INITDIALOG:
        return (TRUE);

    case WM_COMMAND:
        if( wparam == IDOK || wparam == IDCANCEL) {
            EndDialog( hwnd, TRUE );
            return( TRUE );
        }
        break;
    }
    return( FALSE );

} /* About */

/*
 * WinMessage - display a message in a message box
 */
static void WinMessage( char *str, ... )
{
    char        st[256];
    va_list     args;

    va_start( args, str );
    vsprintf( st, str, args );
    va_end( args );
    MessageBox( NULL, st, "Open Watcom Sampler", MB_OK | MB_ICONHAND | MB_TASKMODAL );

} /* WinMessage */

/*
 * StartOutput - set up output listbox
 */
static BOOL StartOutput( short x, short y )
{

    OutputWindow = CreateWindow(
        "LISTBOX",              /* class */
        "Messages",             /* caption */
        WS_CHILD | WS_CAPTION | WS_HSCROLL | WS_VSCROLL | WS_BORDER, /* style */
        15,                     /* init. x pos */
        20,                     /* init. y pos */
        3*(x/4),                    /* init. x size */
        (3*y)/4,                    /* init. y size */
        MainWindowHandle,       /* parent window */
        NULL,                   /* menu handle */
        InstanceHandle,         /* program handle */
        NULL                    /* create parms */
        );

    if( !OutputWindow )
        return( FALSE );
    ShowWindow( OutputWindow, SW_SHOWNORMAL );
    UpdateWindow( OutputWindow );
    return( TRUE );

} /* StartOutput */

/*
 * MainDriver - message handler for sampler
 */
static long FAR PASCAL MainDriver( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    DLGPROC     dlgproc;

    switch (message) {
    case WM_TIMER:
        FlushSamples( (3*MAX_SAMPLES)/4 ); /* KLUDGE ALERT - flush
                                             tolerence should be done for
                                             real! */
        break;

    case WM_CLOSE:
        if( SharedMemory->ShopClosed ) {
            KillTimer( MainWindowHandle, TIMER_ID );
            DestroyWindow( MainWindowHandle );
            return( 0 );
        }
        OutputMsgNL( MSG_SAMPLE_16 );
        return( 0 );
        break;
    case WM_COMMAND:
        switch( wparam ) {
        case MSG_ABOUT:
            dlgproc = MakeProcInstance_DLG( About, InstanceHandle );
            DialogBox(InstanceHandle, ResName( "AboutBox" ), hwnd, dlgproc);
            FreeProcInstance_DLG(dlgproc);
            break;
        case MSG_OPT:
            Usage();
            break;
        default:
            return( DefWindowProc( hwnd, message, wparam, lparam ) );
        }
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        return( DefWindowProc( hwnd, message, wparam, lparam ) );
    }
    return( 0 );

} /* MainDriver */

/*
 * WindowsInit - windows-specific initialization
*/
static int WindowsInit( HANDLE inst, int showcmd )
{
    WNDCLASS    wc;
    BOOL        rc;
    WORD        x,y;
    HGLOBAL     handle;

    if( !SetTimer( MainWindowHandle, TIMER_ID, 32000, 0L) ) {
        return( FALSE );
    }
    handle = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, MAX_SAMPLES * sizeof( samp_save ) );
    if( handle == NULL ) {
        KillTimer( MainWindowHandle, TIMER_ID);
        return( FALSE );
    }
    SampSave = _MK_FP( handle, 0 );

    wc.style = 0;
    wc.lpfnWndProc = MainDriver;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon(inst, "ApplIcon" );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(GRAY_BRUSH);
    wc.lpszMenuName =  ResName( "WSampMenu" );
    wc.lpszClassName = sampleClass;

    rc = RegisterClass( &wc );
    if( !rc ) {
        return( FALSE );
    }

    x = GetSystemMetrics( SM_CXSCREEN );
    y = GetSystemMetrics( SM_CYSCREEN );

    InstanceHandle = inst;

    MainWindowHandle = CreateWindow(
        sampleClass,
        "Open Watcom Sampler for Windows",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  /* Window style.                      */
        0,                                      /* Default horizontal position.       */
        0,                                      /* Default vertical position.         */
        x,                                      /* width.                     */
        y,                                      /* height.                    */
        NULL,                                   /* Overlapped windows have no parent. */
        NULL,                                   /* Use the window class menu.         */
        inst,                                   /* This instance owns this window.    */
        NULL                                    /* Pointer not needed.                */
    );

    if( !MainWindowHandle )
        return( FALSE );

    ShowWindow( MainWindowHandle, showcmd );
    UpdateWindow( MainWindowHandle );

    rc = StartOutput( x,y );
    return( rc );

} /* WindowsInit */

/*
 * Output a string to the list box
 */
static void MyOutput( const char *str, ... )
{
    static char tmpStr[TMPSLEN+1];
    static int  tmpOff=0;
    char        buff[256];
    char        c;
    va_list     args;

    va_start( args, str );
    vsprintf( buff, str, args );
    va_end( args );

    str = buff;
    while( (c = *str) != '\0' ) {
        str++;
        if( c == '\r' )
            continue;
        if( c == '\n' ) {
            tmpStr[tmpOff] = 0;
            if( OutputWindow != NULL ) {
                SendMessage( OutputWindow, LB_ADDSTRING, 0, (LONG)(LPSTR)tmpStr );
            }
            tmpOff = 0;
        } else {
            tmpStr[tmpOff++] = c;
            if( tmpOff >= TMPSLEN - 1 ) {
                tmpOff--;
            }
        }
    }

} /* MyOutput */

void Output( const char FAR_PTR *str )
{
    MyOutput( str );
}

void OutputNL( void )
{
    MyOutput( "\r\n" );
}

/*
 * MessageLoop - process any pending messages
 */
int MessageLoop( void )
{
    MSG         msg;
    WORD        rc;

    while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD ) ) {
        rc = GetMessage( &msg, NULL, 0, 0 );
        if( !rc ) {
            return( TRUE );
        }
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    Yield();
    return( FALSE );

} /* MessageLoop */

/*
 * WinMain - main windows entry point
 */
int PASCAL WinMain( HINSTANCE inst, HINSTANCE previnst, LPSTR cmd, int show)
{
    HINSTANCE           newinst;
    HANDLE              h;
    MSG                 msg;
    parm_data           parm;
    command_data        cmddat;
    char                FAR_PTR *cmdline;
    char                filename[_MAX_PATH];
    char                *cmd_line;
    int                 rc;

    /*
     * are we the first? if so, winexec another one of ourselves
     * and then start sampling
     */
    if( !previnst ) {
        SharedMemory = NULL;
        if( CheckWin386Debug() != WGOD_VERSION ) {
            WinMessage( "Could not find WDEBUG.386" );
            return( FALSE );
        }
        WDebug386 = true;
        if( !MsgInit( inst ) )
            fatal();
        cmddat.nCmdShow = SW_NORMAL;
        if( cmd == NULL || cmd[0] == 0 ) {
            if( !GetFileName( inst, show, filename ) ) {
                CloseShop();
                return( FALSE );
            }
            cmdline = filename;
        } else {
            cmddat.nCmdShow = SW_MINIMIZE;
            cmdline = cmd;
        }

        h = GlobalAlloc( GMEM_SHARE | GMEM_FIXED | GMEM_ZEROINIT, sizeof( shared_data ) );
        if( h == NULL ) {
            CloseShop();
            return( FALSE );
        }
        SharedMemory = _MK_FP( h,0 );

        WaitForFirst = TRUE;    /* tell our counterpart to wait for
                               us before starting the timer */
        cmddat.always2= 2;
        parm.wEnvSeg = 0;
        parm.lpCmdLine = (char __far *)"";
        parm.lpCmdShow = (void __far *)&cmddat;
        parm.lpReserved = NULL;
        newinst = LoadModule( "wsamplew.exe", (LPVOID)&parm );
        if( newinst < HINSTANCE_ERROR ) {
            WinMessage( GET_MESSAGE( MSG_SAMPLE_15 ) );
            CloseShop();
            return( FALSE );
        }
        /*
         * wait for our counterpart to initialize - if he fails,
         * then we must die too
         */
        do {
            GetIData( newinst, (void __near *)&IsSecondOK, sizeof( IsSecondOK ) );
            MessageLoop();
        } while( !IsSecondOK );
        if( IsSecondOK == NOT_OK ) {
            WinMessage( GET_MESSAGE( MSG_SAMPLE_15 ) );
            CloseShop();
            return( FALSE );
        }

        /*
         * get data created by our counterpart
         */
        GetIData( newinst, &OutputWindow, sizeof( OutputWindow ) );
        GetIData( newinst, &MainWindowHandle, sizeof( MainWindowHandle ) );
        GetIData( newinst, &SampSave, sizeof( SampSave) );

        SysInit();
        cmd_line = malloc( strlen( cmdline ) + 1 );
        if( cmd_line != NULL ) {
            _fstrcpy( cmd_line, cmdline );
        }

        /*
         * start the sampler - our other half will be re-started
         * once we have loaded the task to be sampled.
         */
        rc = sample_main( cmd_line );

        if( cmd_line != NULL ) {
            free( cmd_line );
        }
        MsgFini();

        CloseShop();
        SendMessage( MainWindowHandle, WM_CLOSE, 0, 0 );
        return( FALSE );

    } else {

        /*
         * we are the second instance (the guy who waits for
         * timer events and then tries to write the sample file)
         * init our windows stuff, then wait for the first instance
         * to start the samplee, set a timer, and go
         */
        PrevInstance = previnst;
        if( !MsgInit( inst ) )
            fatal();
        if( !WindowsInit( inst, show ) ) {
            IsSecondOK = NOT_OK;
            return( FALSE );
        }
        IsSecondOK = A_OK;
        do {
            GetIData( previnst, (void __near *)&WaitForFirst, sizeof( WaitForFirst ) );
            MessageLoop();
        } while( WaitForFirst );
        GetIData( previnst, &Samples, sizeof( Samples ) );
        GetIData( previnst, &SharedMemory, sizeof( SharedMemory ) );
        KillTimer( MainWindowHandle, TIMER_ID );
        SetTimer( MainWindowHandle, TIMER_ID, 4500, 0L); /* 4.5 seconds */
    }

    /*
     * main message loop for the second instance
     */
    while( GetMessage( &msg, NULL, 0, 0 ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return( FALSE );

} /* WinMain */

char *ResName( char *res )
{
    static char buff[80];
    unsigned    len;

    len = strlen( res );
    memcpy( buff, res, len );
    switch( _WResLanguage() ) {
    case RLE_JAPANESE:
        buff[len + 0] = '1';
        break;
    default:
        buff[len + 0] = '0';
        break;
    }
    buff[len + 1] = '\0';
    return( buff );
}
