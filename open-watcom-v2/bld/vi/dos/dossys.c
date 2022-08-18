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
* Description:  General DOS system helper routines for VI
*
****************************************************************************/


#include "vi.h"
#include <i86.h>
#include "win.h"
#include "dosx.h"
#include "vibios.h"
#include "pragmas.h"
#include "osidle.h"
#include "int16.h"
#include "int10.h"


extern void UpdateDOSClock( void );

#define PHAR_SCRN_SEL   0x34
extern int PageCnt;

int FileSysNeedsCR( int handle )
{
    /* unused parameters */ (void)handle;

    return( true );
}

void BIOSSetBlinkAttr( unsigned char on )
{
    _BIOSVideoSetBlinkAttr( on );
}

void BIOSGetColorPalette( void _FAR *palette )
{
    _BIOSVideoGetColorPalette( palette );
}

void BIOSSetColorRegister( unsigned short reg, unsigned char r, unsigned char g, unsigned char b )
{
    _BIOSVideoSetColorRegister( reg, r, g, b );
}

uint_32 BIOSGetColorRegister( unsigned short reg )
{
    return( _BIOSVideoGetColorRegister( reg ) );
}

unsigned short BIOSGetCursorPos( unsigned char page )
{
    return( _BIOSVideoGetCursorPos( page ).value );
}

void BIOSSetCursorPos( unsigned char page, unsigned char row, unsigned char col )
{
    int10_cursor_pos    pos;

    pos.s.row = row;
    pos.s.col = col;
    _BIOSVideoSetCursorPos( page, pos );
}


/*
 * NewCursor - change cursor to insert mode type
 */
void NewCursor( window_id wid, cursor_type ct )
{
    int10_cursor_typ    int10ct;

    wid = wid;
    if( EditFlags.Monocolor ) {
        int10ct.s.bot_line = 14 - 1;
    } else {
        int10ct.s.bot_line = 16 - 1;
    }
    int10ct.s.top_line = ( ( int10ct.s.bot_line + 1 ) * ( 100 - ct.height ) ) / 100;
    _BIOSVideoSetCursorTyp( int10ct );

} /* NewCursor */

#if 0

/*
 * noteOn - turn beeper on to a specific frequency
 */
static void noteOn( int freq )
{
    unsigned char   pbstate;

    /*
     * beeper on
     */
    pbstate = In61();
    pbstate |= 3;
    Out61( pbstate );

    /*
     * set note - lsb, then msb
     */
    Out43( 0xb6 );
    Out42( freq & 0xFF );
    Out42( freq / 256 );

} /* noteOn */

/*
 * noteOff - turn beeper off
 */
static void noteOff( void )
{
    unsigned char   pbstate;

    pbstate = In61();
    pbstate &= 0xFC;
    Out61( pbstate );

} /* noteOff */

#endif

/*
 * MyBeep - ring beeper
 */
void MyBeep( void )
{
    int             i;
    int             j = 0;
    unsigned char   pbstate;

    if( EditFlags.BeepFlag ) {
        pbstate = In61();
        pbstate |= 3;
        Out61( pbstate );
        Out43( 0xb6 );
        Out42( 3000 & 0xFF );
        Out42( 3000 / 256 );

        for( i = 1; i < 15000; i++ ) {
            j++;
        }

        pbstate = In61();
        pbstate &= 0xFC;
        Out61( pbstate );
    }

} /* MyBeep */

static void getExitAttr( void )
{
    int10_cursor_pos    pos;

    pos = _BIOSVideoGetCursorPos( VideoPage );
    EditVars.ExitAttr = Scrn[pos.s.row * EditVars.WindMaxWidth + pos.s.col].cinfo_attr;
}

/*
 * ScreenInit - get screen info
 */
void ScreenInit( void )
{
    int10_mode_info mode_info;

    mode_info = _BIOSVideoGetModeInfo();
    EditVars.WindMaxWidth = mode_info.columns;
    VideoPage = mode_info.page;

    /*
     * mode _ get apropos screen ptr
     */
    switch( mode_info.mode ) {
    case 0x00:
    case 0x02:
        EditFlags.BlackAndWhite = true;
        break;
    case 0x07:
        EditFlags.Monocolor = true;
        break;
    default:
        EditFlags.Color = true;
        break;
    }
    ScreenPage( 0 );
    EditVars.WindMaxHeight = _BIOSVideoGetRowCount();
    getExitAttr();

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
#define RSH( x )    (((x) & 0x0002) != 0)
#define LSH( x )    (((x) & 0x0002) != 0)
#define CT( x )     (((x) & 0x0004) != 0)
#define AL( x )     (((x) & 0x0008) != 0)
#define LCT( x )    (((x) & 0x0100) != 0)
#define LAL( x )    (((x) & 0x0200) != 0)
#define RCT( x )    (((x) & 0x0400) != 0)
#define RAL( x )    (((x) & 0x0800) != 0)

    unsigned    x;

    EditFlags.ExtendedKeyboard = false;

    x = _BIOSKeyboardTest( KEYB_STD );
    if( (x & 0xff) == 0xff ) {
        return; /* too many damn keys pressed! */
    }

    if( AL( x ) != (RAL( x ) || LAL( x )) ) {
        return;
    }
    if( CT( x ) != (RCT( x ) || LCT( x )) ) {
        return;
    }
    EditFlags.ExtendedKeyboard = true;

} /* ChkExtendedKbd */

/*
 * MemSize - return amount of dos memory left (in 16 byte paragraphs)
 */
long MemSize( void )
{
    short       x;

    x = DosMaxAlloc();
#ifdef _M_I86
    return( 16L * (long)x );
#else
    return( 4096L * (long)x );
#endif

} /* MemSize */

/*
 * ScreenPage - set the screen page to active/inactive
 */
void ScreenPage( int page )
{
#if defined( _M_I86 )
    unsigned long       a;
    unsigned long       b;

    if( !EditFlags.Monocolor ) {
        Scrn = (char_info _FAR *)0xb8000000;
    } else {
        Scrn = (char_info _FAR *)0xb0000000;
    }
    a = *(unsigned short _FAR *)_MK_FP( 0x40, 0x4e ) / sizeof( char_info );
    Scrn += a;
    PageCnt += page;
    if( PageCnt > 0 ) {
        b = (unsigned long)( ( EditVars.WindMaxWidth + 1 ) * ( EditVars.WindMaxHeight + 1 ) );
        if( a + b < 0x8000L / sizeof( char_info ) ) {
            Scrn += b;
        }
        EditFlags.NoSetCursor = true;
    } else {
        EditFlags.NoSetCursor = false;
    }
#elif defined( __4G__ )
    unsigned long       a;
    unsigned long       b;

    if( !EditFlags.Monocolor ) {
        Scrn = (char_info _FAR *)0xb8000;
    } else {
        Scrn = (char_info _FAR *)0xb0000;
    }
    a = *(unsigned short _FAR *)0x44e / sizeof( char_info );
    Scrn += a;
    PageCnt += page;
    if( PageCnt > 0 ) {
        b = (unsigned long)( ( EditVars.WindMaxWidth + 1 ) * ( EditVars.WindMaxHeight + 1 ) );
        if( a + b < 0x8000L / sizeof( char_info ) ) {
            Scrn += b;
        }
        EditFlags.NoSetCursor = true;
    } else {
        EditFlags.NoSetCursor = false;
    }
#else
    unsigned long       a;
    unsigned long       b;
    unsigned long       c;

    if( !EditFlags.Monocolor ) {
        c = 0xb8000;
    } else {
        c = 0xb0000;
    }
    a = *(unsigned short _FAR *)_MK_FP( PHAR_SCRN_SEL, 0x44e );
    c += a;
    PageCnt += page;
    if( PageCnt > 0 ) {
        b = (unsigned long)( ( EditVars.WindMaxWidth + 1 ) * ( EditVars.WindMaxHeight + 1 ) * sizeof( char_info ) );
        if( a + b < 0x8000L ) {
            c += b;
        }
        EditFlags.NoSetCursor = true;
    } else {
        EditFlags.NoSetCursor = false;
    } /* if */
    Scrn = _MK_FP( PHAR_SCRN_SEL, c );
#endif

} /* ScreenPage */

#if defined( _M_I86 )
    #define KEY_PTR (char _FAR *)0x00400017;
#elif defined( __4G__ )
    #define KEY_PTR (char _FAR *)0x00000417;
#else
    #define KEY_PTR _MK_FP( PHAR_SCRN_SEL, 0x417 );
#endif

/*
 * ShiftDown - test if shift key is down
 */
bool ShiftDown( void )
{
    char _FAR   *kptr;

    kptr = KEY_PTR;
    if( kptr[0] & KEY_SHIFT ) {
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
    char _FAR   *kptr;

    kptr = KEY_PTR;

    if( kptr[0] & KEY_CAPS_LOCK ) {
        hadCapsLock = true;
        kptr[0] &= ~KEY_CAPS_LOCK;
    } else {
        hadCapsLock = false;
    }

} /* TurnOffCapsLock */

/*
 * DoGetDriveType - get the type of drive A-Z
 */
drive_type DoGetDriveType( int drv )
{
    return( (drive_type)CheckRemovable( (unsigned char)( drv - 'A' + 1 ) ) );

} /* DoGetDriveType */

/*
 * MyDelay - delay a specified number of milliseconds
 */
void MyDelay( int ms )
{
    int         final_ticks;

    final_ticks = ClockTicks + ((ms * 182L + 5000L) / 10000L);
    do {
        ReleaseVMTimeSlice();
    } while( ClockTicks < final_ticks );

} /* MyDelay */

/*
 * SetCursorBlinkRate - set the current blink rate for the cursor
 */
void SetCursorBlinkRate( int cbr )
{
    EditVars.CursorBlinkRate = cbr;

} /* SetCursorBlinkRate */

/*
 * KeyboardHit - test for keyboard hit
 */
bool KeyboardHit( void )
{
    bool        rc;

    rc = _BIOSKeyboardHit( ( EditFlags.ExtendedKeyboard ) ? KEYB_EXT : KEYB_STD );
    if( !rc ) {
#if !( defined( _M_I86 ) || defined( __4G__ ) )
        UpdateDOSClock();
#endif
        ReleaseVMTimeSlice();
    }
    return( rc );

} /* KeyboardHit */

/*
 * GetKeyboard - get a keyboard char
 */
vi_key GetKeyboard( void )
{
    unsigned    code;
    unsigned    scan;
    bool        shift;

    code = _BIOSKeyboardGet( ( EditFlags.ExtendedKeyboard ) ? KEYB_EXT : KEYB_STD );
    shift = ShiftDown();
    scan = code >> 8;
    code &= 0xff;
    if( code == 0xE0 && scan != 0 ) {
        code = 0;
    }
    return( GetVIKey( code, scan, shift ) );

} /* GetKeyboard */
