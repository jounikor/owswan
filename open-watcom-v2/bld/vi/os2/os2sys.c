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
* Description:  OS/2 specific system interface functions.
*
****************************************************************************/


#include "vi.h"
#include "win.h"
#include "dosx.h"
#include "vibios.h"
#include <stddef.h>


#if defined(_M_I86)
    #define SEG16
    #define STUPID_UINT     unsigned short
#else
    #define SEG16   _Seg16
    #define STUPID_UINT     unsigned long
#endif

extern int      PageCnt;

int FileSysNeedsCR( int handle )
{
    /* unused parameters */ (void)handle;

    return( true );
}

/*
 * NewCursor - change cursor to insert mode type
 */
void NewCursor( window_id wid, cursor_type ct )
{
    unsigned char   base,nbase;
    VIOCURSORINFO   vioCursor;

    wid = wid;
    VioGetCurType( &vioCursor, 0 );
    base = vioCursor.cEnd;
    nbase = ( (unsigned)base * ( 100 - ct.height ) ) / 100;
    BIOSSetCursorTyp( nbase, base - 1 );

} /* NewCursor */

/*
 * MyBeep - ring beeper
 */
void MyBeep( void )
{
    if( EditFlags.BeepFlag ) {
    }

} /* MyBeep */

/*
 * ScreenInit - get screen info
 */
void ScreenInit( void )
{
    unsigned short              solvb;
    struct _VIOCONFIGINFO       config;
    struct _VIOMODEINFO         vioMode;
    void * SEG16                ptr;

    /* Set the cb member of VIOMODEINFO/VIOCONFIGINFO to smaller values
     * in order to be backward compatible with old OS/2 versions.
     */
    vioMode.cb = offsetof( VIOMODEINFO, buf_addr );
    if( VioGetMode( &vioMode, 0 ) != 0 ) {
        StartupError( ERR_WIND_INVALID );
    }
    EditVars.WindMaxWidth = vioMode.col;
    EditVars.WindMaxHeight = vioMode.row;

    config.cb = offsetof( VIOCONFIGINFO, Configuration );
    if( VioGetConfig( 0, &config, 0 ) != 0 ) {
        StartupError( ERR_WIND_INVALID );
    }
    if( config.display == 3 ) {
        EditFlags.BlackAndWhite = true;
    } else {
        if( config.adapter == 0 ) {
            EditFlags.Monocolor = true;
        } else {
            EditFlags.Color = true;
        }
    }

    VioGetBuf( (PULONG) &ptr, (PUSHORT) &solvb, 0);
    Scrn = ptr;
    ScreenPage( 0 );

} /* ScreenInit */

/*
 * ScreenFini
 */
void ScreenFini( void )
{
} /* ScreenFini */

/*
 * ChkExtendedKbd - look for extended keyboard type
 */
void ChkExtendedKbd( void )
{
    EditFlags.ExtendedKeyboard = true;

} /* ChkExtendedKbd */

/*
 * MemSize - return amount of dos memory left (in 16 byte paragraphs)
 */
long MemSize( void )
{
    return( 0 );

} /* MemSize */

/*
 * ScreenPage - set the screen page to active/inactive
 */
void ScreenPage( int page )
{
    PageCnt += page;

} /* ScreenPage */

/*
 * ShiftDown - test if shift key is down
 */
bool ShiftDown( void )
{
    KBDINFO     ki;

    ki.cb = sizeof( KBDINFO );
    KbdGetStatus( &ki, 0 );
    if( ki.fsState & KEY_SHIFT ) {
        return( true );
    }
    return( false );

} /* ShiftDown */

static bool hadCapsLock;

/*
 * TurnOffCapsLock - switch off caps lock
 */
void TurnOffCapsLock( void )
{
    KBDINFO     ki;

    ki.cb = sizeof( KBDINFO );
    KbdGetStatus( &ki, 0 );
    if( ki.fsState & KEY_CAPS_LOCK ) {
        hadCapsLock = true;
        ki.fsMask |= 0x10;
        ki.fsState &= ~KEY_CAPS_LOCK;
        KbdSetStatus( &ki, 0 );  /* OS/2 2.0 FUCKS UP IF YOU DO THIS */
    } else {
        hadCapsLock = false;
    }

} /* TurnOffCapsLock */

/*
 * DoGetDriveType - get the type of drive A-Z
 */
drive_type DoGetDriveType( int drv )
{
    STUPID_UINT disk;
    ULONG       map;
    int         i;

    DosQCurDisk( &disk, &map );
    for( i = 'A'; i <= 'Z'; i++ ) {
        if( drv == i ) {
            if( map & 1 ) {
                return( DRIVE_TYPE_IS_FIXED );
            } else {
                return( DRIVE_TYPE_NONE );
            }
        }
        map >>= 1;
    }
    return( DRIVE_TYPE_NONE ); // to quiet the compiler

} /* DoGetDriveType */

/*
 * MyDelay - delay a specified number of milliseconds
 */
void MyDelay( int ms )
{
    DosSleep( ms );

} /* MyDelay */

/*
 * SetCursorBlinkRate - set the current blink rate for the cursor
 */
void SetCursorBlinkRate( int cbr )
{
    EditVars.CursorBlinkRate = cbr;

} /* SetCursorBlinkRate */

vi_key GetKeyboard( void )
{
    unsigned    code;
    unsigned    scan;
    bool        shift;

    code = BIOSGetKeyboard( &scan );
    shift = ShiftDown();
    code &= 0xff;
    if( code == 0xE0 && scan != 0 ) {
        code = 0;
    }
    return( GetVIKey( code, scan, shift ) );
}

bool KeyboardHit( void )
{
    return( BIOSKeyboardHit() );
}

void MyVioShowBuf( size_t offset, unsigned nchars )
{
    BIOSUpdateScreen( offset, nchars );
}

