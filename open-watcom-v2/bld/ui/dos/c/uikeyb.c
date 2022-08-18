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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <string.h>
#include <dos.h>
#ifdef __386__
#include <conio.h>
#endif
#include "uidef.h"
#include "uishift.h"
#include "int16.h"


#define NRM_KEY_READ            0x00
#define NRM_KEY_STAT            0x01
#define NRM_KEY_SHFT            0x02
#define NRM_KEY_WRITE           0x05

#define EXT_KEY_READ            0x10
#define EXT_KEY_STAT            0x11
#define EXT_KEY_SHFT            0x12

static unsigned char    ReadReq = KEYB_STD;     /* this will be KEYB_STD or KEYB_EXT */

static shiftkey_event   ShiftkeyEvents[] = {
    EV_SHIFT_PRESS,     EV_SHIFT_RELEASE,
    EV_SHIFT_PRESS,     EV_SHIFT_RELEASE,
    EV_CTRL_PRESS,      EV_CTRL_RELEASE,
    EV_ALT_PRESS,       EV_ALT_RELEASE,
    EV_SCROLL_PRESS,    EV_SCROLL_RELEASE,
    EV_NUM_PRESS,       EV_NUM_RELEASE,
    EV_CAPS_PRESS,      EV_CAPS_RELEASE,
    EV_INSERT_PRESS,    EV_INSERT_RELEASE
};

bool UIAPI uiextkeyboard( void )
/******************************/
{
    return( ReadReq == KEYB_EXT );
}


unsigned intern getkey( void )
/****************************/
{
    return( _BIOSKeyboardGet( ReadReq ) );
}


int intern checkkey( void )
/*************************/
{
    return( _BIOSKeyboardHit( ReadReq ) );
}


void intern flushkey( void )
/**************************/
{
    while( checkkey() ) {
        getkey();
    }
}


unsigned char intern checkshift( void )
/*************************************/
{
    return( _BIOSKeyboardTest( ReadReq ) );
}


unsigned char UIAPI uicheckshift( void )
/***************************************/

{
    return( checkshift() );
}


#define RSH(x)  ( ( ( x ) & 0x0002 ) != 0 )
#define LSH(x)  ( ( ( x ) & 0x0002 ) != 0 )
#define CT(x)   ( ( ( x ) & 0x0004 ) != 0 )
#define AL(x)   ( ( ( x ) & 0x0008 ) != 0 )
#define LCT(x)  ( ( ( x ) & 0x0100 ) != 0 )
#define LAL(x)  ( ( ( x ) & 0x0200 ) != 0 )
#define RCT(x)  ( ( ( x ) & 0x0400 ) != 0 )
#define RAL(x)  ( ( ( x ) & 0x0800 ) != 0 )

bool intern initkeyboard( void )
/******************************/
{
    unsigned x;

    ReadReq = KEYB_STD;
    x = _BIOSKeyboardTest( KEYB_STD );
    if( (x & 0xff) == 0xff )
        return( true ); /* too many damn keys pressed! */
    if( AL( x ) != ( RAL( x ) || LAL( x ) ) )
        return( true );
    if( CT( x ) != ( RCT( x ) || LCT( x ) ) )
        return( true );
    ReadReq = KEYB_EXT;
    return( true );
}


ui_event intern keyboardevent( void )
/***********************************/
{
    unsigned            key;
    unsigned char       scan;
    unsigned char       ascii;
    ui_event            ui_ev;
    unsigned char       newshift;
    unsigned char       changed;

    newshift = checkshift();
    /* checkkey must take precedence over shift change so that  *
     * typing characters by holding the alt key and typing the  *
     * ascii code on the numeric keypad works                   */
    if( checkkey() ) {
        key = getkey();
        scan = (unsigned char)( key >> 8 ) ;
        ascii = (unsigned char)key;
        if( scan != 0 && ascii == 0xe0 ) {  /* extended keyboard */
            ascii = 0;
        }
        ui_ev = scan + 0x100;
        /* ignore shift key for numeric keypad if numlock is not on */
        if( ui_ev >= EV_HOME && ui_ev <= EV_DELETE ) {
            if( (newshift & S_NUM) == 0 ) {
                if( newshift & S_SHIFT ) {
                    ascii = 0;      /* wipe out digit */
                }
            }
        }
        if( ascii != 0 ) {
            ui_ev = ascii;
            if( (newshift & S_ALT) && ( ascii == ' ' ) ) {
                ui_ev = EV_ALT_SPACE;
            } else if( scan != 0 ) {
                switch( ascii + 0x100 ) {
                case EV_RUB_OUT:
                case EV_TAB_FORWARD:
                case EV_ENTER:
                case EV_ESCAPE:
                    ui_ev = ascii + 0x100;
                    break;
                }
            }
        }
#ifdef FD6
        if( !iskeyboardchar( ui_ev ) ) {
            ui_ev = EV_NO_EVENT;
        }
#endif
    } else {
        changed = ( newshift ^ UIData->old_shift );
        if( changed != 0 ) {
            scan = 1;
            for( key = 0; key < sizeof( ShiftkeyEvents ) / sizeof( ShiftkeyEvents[0] ); key++ ) {
                if( changed & scan ) {
                    if( newshift & scan ) {
                        UIData->old_shift |= scan;
                        return( ShiftkeyEvents[key].press );
                    } else {
                        UIData->old_shift &= ~scan;
                        return( ShiftkeyEvents[key].release );
                    }
                }
                scan <<= 1;
            }
        }
        ui_ev = EV_NO_EVENT;
    }
    return( ui_ev );
}

ui_event UIAPI uikeyboardevent( void )
/************************************/
{
    return( keyboardevent() );
}
