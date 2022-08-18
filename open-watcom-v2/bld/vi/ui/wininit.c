/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2016 The Open Watcom Contributors. All Rights Reserved.
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


#include "vi.h"
#include "win.h"

/*
 * allocImage - allocate screen image
 */
static void allocImage( void )
{
    size_t  i;
    size_t  size;

    size = EditVars.WindMaxWidth * EditVars.WindMaxHeight;
    ScreenImage = _MemAllocArray( window_id, size );
    for( i = 0; i < size; i++ ) {
        ScreenImage[i] = NO_WINDOW;
    }

} /* allocImage */

static void initAllWindows( void )
{
    window_id   wid;

    for( wid = 0; wid < MAX_WINDS; wid++ ) {
        WINDOW_TO_ID( wid, NULL );
    }
    WINDOW_TO_ID( MAX_WINDS, NULL );
}

/*
 * StartWindows - begin windows session
 */
void StartWindows( void )
{
    allocImage();
    initAllWindows();
    ClearScreen();
    EditFlags.WindowsStarted = true;

} /* StartWindows */

/*
 * FinishWindows - done with windows; close down
 */
void FinishWindows( void )
{
    cursor_type ct;
    window_id   wid;

    // Close Down All Straggling Windows
    for( wid = 0; wid < MAX_WINDS; wid++ ) {
        if( WINDOW_FROM_ID( wid ) != NULL ) {
            // CloseAWindow( wid );
        }
    }

    // Close down the windowing system.

    if( EditFlags.ZapColors ) {
        size_t  j, total;

        if( !EditFlags.Quiet && Scrn != NULL ) {
            total = EditVars.WindMaxWidth * EditVars.WindMaxHeight;
            for( j = 0; j < total; j++ ) {
                Scrn[j].cinfo_attr = EditVars.ExitAttr;
            }
#ifdef __VIO__
            MyVioShowBuf( 0, EditVars.WindMaxWidth * EditVars.WindMaxHeight );
#endif
        }
    }
    FiniColors();
    ct.height = 7;
    ct.width = 100;
    NewCursor( NO_WINDOW, ct );
    MemFree( ScreenImage );

} /* FinishWindows */
