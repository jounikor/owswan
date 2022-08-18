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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "wdeglbl.h"
#include "wdemain.h"
#include "wdemsgbx.h"
#include "rcstr.grh"
#include "ldstr.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

/* routine to create a message box */
void WdeDisplayMsgBox( const char *msg )
{
    char        *title;

    title = WdeAllocRCString( WDE_MSGBOXTITLE );

    if( !MessageBox( (HWND)NULL, msg, title,
                     MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL ) ) {
        MessageBeep( (UINT)-1 );
    }

    if( title != NULL ) {
        WdeFreeRCString( title );
    }
}

char *WdeAllocRCString( msg_id id )
{
    return( AllocRCString( id ) );
}

void WdeFreeRCString( char *str )
{
    FreeRCString( str );
}

int WdeCopyRCString( msg_id id, char *buf, int bufsize )
{
    return( CopyRCString( id, buf, bufsize ) );
}

void WdeInitDisplayError( HINSTANCE inst )
{
    SetInstance( inst );
}

void WdeDisplayErrorMsg( msg_id msg )
{
    char        *title;

    title = WdeAllocRCString( WDE_MSGBOXTITLE );

    if( !RCMessageBox( (HWND)NULL, msg, title, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL ) ) {
        MessageBeep( (UINT)-1 );
    }

    if( title != NULL ) {
        WdeFreeRCString( title );
    }
}
