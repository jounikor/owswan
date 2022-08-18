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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdlib.h>
#include <stddef.h>
#include <windows.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgscrn.h"
#include "dbgmain.h"
#include "wndsys.h"
#include "trpsys.h"
#include "dbginit.h"
#include "guigsysh.h"


static HWND     HwndFore = NULL;
static HWND     FirstForeWnd = NULL;


void TellWinHandle( void )
{
    if( _IsOn( SW_POWERBUILDER ) )
        return;
    TRAP_EXTFUNC( InfoFunction )( GUIGetSysHandle( WndGui( WndMain ) ) );
}

void Ring_Bell( void )
{
    MessageBeep( 0 );
}

unsigned ConfigScreen( void )
{
    return( 0 );
}

size_t GetSystemDir( char *buff, size_t buff_len )
{
    buff[0] = NULLCHAR;
    GetWindowsDirectory( buff, buff_len );
    return( strlen( buff ) );
}

void InitScreen( void )
{
    RestoreMainScreen( "WDNT" );
    FirstForeWnd = GetForegroundWindow();
}

bool UsrScrnMode( void )
{
    return( false );
}

void DbgScrnMode( void )
{
}

static enum {
    DEBUG_SCREEN,
    USER_SCREEN,
    UNKNOWN_SCREEN
} ScreenState = UNKNOWN_SCREEN;

void UnknownScreen( void )
{
    ScreenState = UNKNOWN_SCREEN;
}

bool DebugScreen( void )
{
    HWND        hwnd;
    HWND        fore;

    if( ScreenState == DEBUG_SCREEN )
        return( false );
    if( _IsOn( SW_POWERBUILDER ) )
        return( false );
    if( WndMain ) {
        ScreenState = DEBUG_SCREEN;
        hwnd = GUIGetSysHandle( WndGui( WndMain ) );
        fore = GetForegroundWindow();
        if( fore != hwnd ) {
            HwndFore = fore;
        }
        if( GUIIsMinimized( WndGui( WndMain ) ) )
            GUIRestoreWindow( WndGui( WndMain ) );
        if( IsWindow( hwnd ) )
            SetForegroundWindow( hwnd );
        if( _IsOn( SW_POWERBUILDER ) ) {
            WndRestoreWindow( WndMain );
        }
    }
    return( false );
}

bool DebugScreenRecover( void )
{
    return( true );
}


bool UserScreen( void )
{
    if( ScreenState == USER_SCREEN )
        return( false );
    if( _IsOn( SW_POWERBUILDER ) )
        return( false );
    if( WndMain ) {
        ScreenState = USER_SCREEN;
        if( _IsOn( SW_POWERBUILDER ) ) {
            WndMinimizeWindow( WndMain );
        } else {
            if( IsWindow( HwndFore ) ) {
                SetForegroundWindow( HwndFore );
            } else {
                HwndFore = NULL;
            }
        }
    }
    return( false );
}

void SaveMainWindowPos( void )
{
    SaveMainScreen( "WDNT" );
}

void FiniScreen( void )
{
    if( _IsOn( SW_POWERBUILDER ) )
        return;
    if( IsWindow( FirstForeWnd ) ) {
        SetForegroundWindow( FirstForeWnd );
    }
}

void SetNumLines( int num )
{
    num = num;
}

void SetNumColumns( int num )
{
    num = num;
}

bool ScreenOption( const char *start, unsigned len, int pass )
{
    start=start;len=len;pass=pass;
    return( false );
}

void ScreenOptInit( void )
{
}
