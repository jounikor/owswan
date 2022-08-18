/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Low level console support for Win32.
*
****************************************************************************/


#include <conio.h>
#include <stdio.h>
#include "uidef.h"
#include <windows.h>
#include "uicurshk.h"
#include "uiintern.h"


static MONITOR ui_data = {
    50,
    80,
    M_VGA,
    NULL,
    NULL,
    NULL,
    NULL,
    4,
    1
};

HANDLE              OutputHandle;
HANDLE              InputHandle;
volatile bool       BrkPending;

static COORD        BSize;
static DWORD        oldInputMode;
static HANDLE       oldOutputHandle;

/*
 * consoleHandler - handle console ctrl c
 */
static BOOL WINAPI consoleHandler( DWORD type )
{
    /* unused parameters */ (void)type;

    BrkPending = true;
    return( true );

} /* consoleHandler */


bool UIAPI uiset80col( void )
{
    return( true );
}

bool intern initbios( void )
{
    CONSOLE_SCREEN_BUFFER_INFO  sbi;

    InputHandle = CreateFile( "CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    GetConsoleMode( InputHandle, &oldInputMode );
    SetConsoleMode( InputHandle, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS );

    oldOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    OutputHandle = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL );
    SetConsoleMode( OutputHandle, 0 );
    SetConsoleActiveScreenBuffer( OutputHandle );
    SetConsoleCtrlHandler( consoleHandler, true );

    if( UIData == NULL ) {
        UIData = &ui_data;
    }

    GetConsoleScreenBufferInfo( OutputHandle, &sbi );

    BSize.X = UIData->width  = sbi.dwMaximumWindowSize.X;
    BSize.Y = UIData->height = sbi.dwMaximumWindowSize.Y;
    UIData->colour = M_VGA;

    UIData->screen.origin = (LP_PIXEL)uimalloc( UIData->width * UIData->height * sizeof( PIXEL ) );
    UIData->screen.increment = UIData->width;
    uiinitcursor();
    initkeyboard();
    UIData->mouse_acc_delay = uiclockdelay( 250 /* ms */ );
    UIData->mouse_rpt_delay = uiclockdelay( 100 /* ms */ );
    UIData->mouse_clk_delay = uiclockdelay( 250 /* ms */ );
    UIData->tick_delay      = uiclockdelay( 500 /* ms */ );
    UIData->mouse_speed = 8;

    return( true );
}

void intern finibios( void )
{
    SetConsoleMode( InputHandle, oldInputMode );
    SetConsoleActiveScreenBuffer( oldOutputHandle );
    uifinicursor();
    finikeyboard();
    uifree( (void *)UIData->screen.origin );
    SetConsoleCtrlHandler( consoleHandler, false );
}

void intern physupdate( SAREA *area )
{
    SMALL_RECT  sr;
    COORD       bcoord;

    bcoord.Y = sr.Top = area->row;
    bcoord.X = sr.Left = area->col;
    sr.Bottom = sr.Top + area->height - 1;
    sr.Right = sr.Left + area->width - 1;
    if( sr.Left < 0 || sr.Right >= UIData->width ||
        sr.Top < 0 || sr.Bottom >= UIData->height ) {
        // WriteConsoleOutput crashes if the area is not on the screen
        return;
    }
    WriteConsoleOutput( OutputHandle, (PCHAR_INFO)UIData->screen.origin, BSize, bcoord, &sr );
#if 0
    {
        int     i,j;
        char    buff[256];


        for( j = sr.Top; j <= sr.Bottom; j++ ) {
            for( i = sr.Left; i <= sr.Right; i++ ) {
                buff[i - sr.Left] = UIData->screen.origin[j * UIData->width + i].ch;
            }
            buff[i - sr.Left] = 0;
            printf( "%s\n", buff );
        }
    }

    printf( "Top=%d,Bottom=%d,Left=%d, Right=%d!\n",
        sr.Top,sr.Bottom, sr.Left, sr.Right );
#endif
}
