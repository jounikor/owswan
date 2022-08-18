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


// NOTE: Until we find a way of waking a thread that has blocked on a
// GetKey call we will busy wait for input.

#include <dos.h>
//#include <procdefs.h>
#include "uidef.h"
#include "uishift.h"
#include "uinlm.h"


extern int ScreenHandle;        // from uibios.c

/* Don't dereference this pointer, only pass it as a paramter to functions. */
/* It points into the OS data area and dereferencing could interfere with the */
/* Novell Labs Certification. */
static struct ScreenStruct  *ScreenPointer = NULL;
static unsigned             shift_state = 0;
static bool                 BlockedOnKeyboard = false;

static struct {
    bool    inUse;
    BYTE    keyType;
    BYTE    keyValue;
    BYTE    keyStatus;
    BYTE    scanCode;
} SavedKey = { false };

unsigned char UIAPI uicheckshift( void )
/**************************************/
{
    return( shift_state );
}

static bool netwaregetkey( BYTE *keyType, BYTE *keyValue, BYTE *keyStatus, BYTE *scanCode )
/*****************************************************************************************/
{
    if( SavedKey.inUse ) {
        *keyType = SavedKey.keyType;
        *keyValue = SavedKey.keyValue;
        *keyStatus = SavedKey.keyStatus;
        *scanCode = SavedKey.scanCode;
        SavedKey.inUse = false;
        return( true );
    } else if( CheckKeyStatus( ScreenPointer ) ) {
        GetKey( ScreenPointer, keyType, keyValue, keyStatus, scanCode, 0 );
        return( true );
    } else {
        return( false );
    }
}

void intern flushkey( void )
/**************************/
{
    BYTE dummy;

    while( netwaregetkey( &dummy, &dummy, &dummy, &dummy ) );
}

void intern kbdspawnstart( void )
/*******************************/
{
}

void intern kbdspawnend( void )
/*****************************/
{
}


ui_event intern keyboardevent( void )
/***********************************/
{
    BYTE        scan;
    BYTE        ascii;
    ui_event    ui_ev;
    BYTE        type;
    BYTE        status;

    /* We should check to see if the shift keys have been raised or    */
    /* pressed and send the appropriate events, but we don't, for now. */

    if( !netwaregetkey( &type, &ascii, &status, &scan ) ) {
        return( EV_NO_EVENT );
    } /* end if */

    switch( type ) {
    case NORMAL_KEY:
        if( ascii == 0 || ascii == 0xe0 ) {
            ui_ev = 0x100 + scan;
        } else {
            if( ( status & ALT_KEY ) && ( ascii == ' ' ) ) {
                ui_ev = EV_ALT_SPACE;
            } else if( ascii + 0x100 == EV_TAB_FORWARD ) {
                ui_ev = EV_TAB_FORWARD;
            } else if( ascii + 0x100 == EV_ESCAPE ) {
                ui_ev = EV_ESCAPE;
            } else if( ascii + 0x100 == EV_ENTER ) {
                ui_ev = EV_ENTER;
            } else if( ascii + 0x100 == EV_RUB_OUT ) {
                ui_ev = EV_RUB_OUT;
            } else {
                ui_ev = ascii;
            } /* end if */
        } /* end if */
        break;
    case FUNCTION_KEY:
        if( status & ALT_KEY ) {
            ui_ev = EV_ALT_F1 + ( ascii - 30 - 1 );
        } else if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_F1 + ( ascii - 20 - 1 );
        } else if( status & ( RIGHT_SHIFT_KEY | LEFT_SHIFT_KEY ) ) {
            ui_ev = EV_SHIFT_F1 + ( ascii - 10 - 1 );
        } else {
            ui_ev = EV_F1 + ( ascii - 1 );
        } /* end if */
        break;
    case ENTER_KEY:
        ui_ev = EV_ENTER;
        break;
    case ESCAPE_KEY:
        ui_ev = EV_ESCAPE;
        break;
    case BACKSPACE_KEY:
        ui_ev = EV_RUB_OUT;
        break;
    case DELETE_KEY:
        ui_ev = EV_DELETE;
        break;
    case INSERT_KEY:
        ui_ev = EV_INSERT;
        break;
    case CURSOR_UP_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_CURSOR_UP;
        } else {
            ui_ev = EV_CURSOR_UP;
        }
        break;
    case CURSOR_DOWN_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_CURSOR_DOWN;
        } else {
            ui_ev = EV_CURSOR_DOWN;
        }
        break;
    case CURSOR_RIGHT_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_CURSOR_RIGHT;
        } else {
            ui_ev = EV_CURSOR_RIGHT;
        }
        break;
    case CURSOR_LEFT_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_CURSOR_LEFT;
        } else {
            ui_ev = EV_CURSOR_LEFT;
        }
        break;
    case CURSOR_HOME_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_HOME;
        } else {
            ui_ev = EV_HOME;
        }
        break;
    case CURSOR_END_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_END;
        } else {
            ui_ev = EV_END;
        }
        break;
    case CURSOR_PUP_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_PAGE_UP;
        } else {
            ui_ev = EV_PAGE_UP;
        }
        break;
    case CURSOR_PDOWN_KEY:
        if( status & CONTROL_KEY ) {
            ui_ev = EV_CTRL_PAGE_DOWN;
        } else {
            ui_ev = EV_PAGE_DOWN;
        }
        break;
    case 0xff:
        // 0xff is not a valid key type so this must have come from
        // the uiwakethread call below.
        ui_ev = EV_NO_EVENT;
        break;
    default:
        break;
    } /* end switch */

    if( !iskeyboardchar( ui_ev ) ) {
        ui_ev = EV_NO_EVENT;
    } /* end if */

    return( ui_ev );

}

bool intern initkeyboard( void )
/******************************/
{
    ScreenPointer = (struct ScreenStruct *)__GetScreenID( ScreenHandle );
    return( true );
}

void intern finikeyboard( void )
/******************************/
{
    ScreenPointer = NULL;
}

void intern waitforevent( void )
/******************************/
{
    if( !SavedKey.inUse ) {
        BlockedOnKeyboard = true;
        GetKey( ScreenPointer, &SavedKey.keyType, &SavedKey.keyValue,
                    &SavedKey.keyStatus, &SavedKey.scanCode, 0 );
        BlockedOnKeyboard = false;
        SavedKey.inUse = true;
    }
}

void UIAPI uiwakethread( void )
/******************************/
/* This function is called from a thread other than the UI thread to wake */
/* the thread from the GetKey call. */
{
    // NOTE: 0xff is not a valid key type for a GetKey call and we test for
    // this case above.
    UngetKey( ScreenPointer, 0xff, 0, 0, 0 );
}

bool intern kbdisblocked( void )
/******************************/
/* This function may be called on a thread other that the UI thead */
{
    return( BlockedOnKeyboard );
}
