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
#include "dc.h"

/*
 * ResizeWindow - give a window a new size
 */
vi_rc ResizeWindow( window_id wid, windim x1, windim y1, windim x2, windim y2, bool scrflag )
{
    window      *w;
//    int         bt, k;
//    char        *txt, *tptr;
//    char        *ot;
//    int         i, j;

    w = WINDOW_FROM_ID( wid );
    AccessWindow( w );

    if( !ValidDimension( x1, y1, x2, y2, w->has_border ) ) {
        ReleaseWindow( w );
        return( ERR_WIND_INVALID );
    }
    RestoreOverlap( wid, scrflag );

    AccessWindow( AllocWindow( wid, x1, y1, x2, y2, w->has_border, w->has_gadgets,
            w->border_color1, w->border_color2, w->text_color, w->background_color ) );
    MarkOverlap( wid );

    /*
     * display the new text
     */
    ClearWindow( wid );
    if( w->title != NULL ) {
        WindowTitle( wid, w->title );
    } else {
        DrawBorder( wid );
    }
    DCResize( CurrentInfo );
    DCDisplayAllLines();
    DCUpdate();

    FreeWindow( w );
    ReleaseWindow( WINDOW_FROM_ID( wid ) );

    return( ERR_NO_ERR );

} /* ResizeWindow */

/*
 * ResizeWindowRelative - resize current window with relative shifts
 */
vi_rc ResizeWindowRelative( window_id wid, windim x1, windim y1, windim x2, windim y2, bool scrflag )
{
    window      *w;

    w = WINDOW_FROM_ID( wid );
    return( ResizeWindow( wid, w->area.x1 + x1, w->area.y1 + y1, w->area.x2 + x2, w->area.y2 + y2, scrflag ) );

} /* ResizeWindowRelative */

/*
 * getMinSlot - find a minimize slot
 */
static int getMinSlot( void )
{
    int i;

    for( i = 0; i < MAX_MIN_SLOTS; i++ ) {
        if( !MinSlots[i] ) {
            MinSlots[i] = true;
            return( i + 1 );
        }
    }
    return( 0 );

} /* getMinSlot */

#define MIN_WIN_WIDTH   11

/*
 * MinimizeCurrentWindow - put next window into next minimize slot
 */
vi_rc MinimizeCurrentWindow( void )
{
    int     i, j;
    windim  minx1, miny1;
    vi_rc   rc;

    i = getMinSlot();
    if( !i ) {
        return( ERR_NO_ERR );
    }
    miny1 = EditVars.WindMaxHeight - 8;
    minx1 = 0;
    for( j = 1; j < i; j++ ) {
        minx1 += MIN_WIN_WIDTH;
        if( minx1 >= EditVars.WindMaxWidth - MIN_WIN_WIDTH ) {
            miny1 -= 3;
            if( miny1 < 0 ) {
                miny1 = EditVars.WindMaxHeight - 7;
            }
            minx1 = 0;
        }
    }
    rc = ResizeCurrentWindow( minx1, miny1, minx1 + MIN_WIN_WIDTH - 1, miny1 + 2 );
    WindowAuxUpdate( current_window_id, WIND_INFO_MIN_SLOT, i );
    return( rc );

} /* MinimizeCurrentWindow */

/*
 * MaximizeCurrentWindow - make current window full screen
 */
vi_rc MaximizeCurrentWindow( void )
{
    windim  x1, x2;

    x1 = editw_info.area.x1;
    x2 = editw_info.area.x2;
    if( EditFlags.LineNumbers ) {
        if( EditFlags.LineNumsOnRight ) {
            x2 -= EditVars.LineNumWinWidth;
        } else {
            x1 += EditVars.LineNumWinWidth;
        }
    }
    return( ResizeCurrentWindow( x1, editw_info.area.y1, x2, editw_info.area.y2 ) );

} /* MaximizeCurrentWindow */
