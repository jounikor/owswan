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
 * reDisplayWindow - redisplay the saved window text
 */
static void reDisplayWindow( window_id wid )
{
    window              *w;
    char_info           *txt;
    window_id           *over;
    char_info           _FAR *scr;
    size_t              oscr;
    int                 j, i;

    if( EditFlags.Quiet ) {
        return;
    }
    w = WINDOW_FROM_ID( wid );

    /*
     * re-display text area
     */
    AccessWindow( w );
    txt = w->text;
    over = w->overlap;
    for( j = w->area.y1; j <= w->area.y2; j++ ) {
        oscr = w->area.x1 + j * EditVars.WindMaxWidth;
        scr = &Scrn[oscr];
        for( i = w->area.x1; i <= w->area.x2; i++ ) {
            if( BAD_ID( *over ) ) {
                WRITE_SCREEN( *scr, *txt );
            }
            over++;
            scr++;
            txt++;
        }
#ifdef __VIO__
        MyVioShowBuf( oscr, w->width );
#endif
    }

    DrawBorder( wid );
    ReleaseWindow( w );

} /* reDisplayWindow */

/*
 * MoveWindowToFront - bring a window forward
 */
void MoveWindowToFront( window_id wid )
{
    if( !TestOverlap( wid ) ) {
        return;
    }
    MoveWindowToFrontDammit( wid, true );

} /* MoveWindowToFront */

/*
 * MoveWindowToFrontDammit - bring a window forward
 */
void MoveWindowToFrontDammit( window_id wid, bool scrflag )
{
    window      *w;

    if( BAD_ID( wid ) ) {
        return;
    }
    w = WINDOW_FROM_ID( wid );

    RestoreOverlap( wid, scrflag );
    ResetOverlap( w );
    MarkOverlap( wid );
    reDisplayWindow( wid );

} /* MoveWindowToFrontDammit */

/*
 * InactiveWindow - display a window as inactive
 */
void InactiveWindow( window_id wid )
{
    window      *w;
    vi_color    c;

    if( BAD_ID( wid ) ) {
        return;
    }

    w = WINDOW_FROM_ID( wid );
    if( w == NULL ) {
        return;
    }

    if( !w->has_border ) {
        return;
    }

    /*
     * change the border color
     */
    c = w->border_color1;
    w->border_color1 = EditVars.InactiveWindowColor;
    DrawBorder( wid );
    w->border_color1 = c;

} /* InactiveWindow */

void ActiveWindow( window_id wid )
{
    /* unused parameters */ (void)wid;
}

/*
 * WindowTitleAOI - set the title of a window, active or inactive
 */
void WindowTitleAOI( window_id wid, const char *title, bool active )
{
    window      *w;

    w = WINDOW_FROM_ID( wid );
    MemFree( w->title );
    if( title == NULL ) {
        w->title = NULL;
    } else {
        w->title = DupString( title );
    }
    if( active ) {
        DrawBorder( wid );
    } else {
        InactiveWindow( wid );
    }

} /* WindowTitleAOI */


/*
 * WindowTitle - set window title, active border
 */
void WindowTitle( window_id wid, const char *name )
{
    WindowTitleAOI( wid, name, true );

} /* WindowTitle */

/*
 * WindowTitleInactive - set window title, inactive border
 */
void WindowTitleInactive( window_id wid, const char *name )
{
    WindowTitleAOI( wid, name, false );

} /* WindowTitleInactive */

/*
 * ClearWindow - do just that
 */
void ClearWindow( window_id wid )
{
    window              *w;
    window_id           *over;
    char_info           *txt;
    char_info           _FAR *scr;
    size_t              oscr;
    int                 j, i, shift, addr;
    char_info           what = {0, 0};

    if( EditFlags.Quiet ) {
        return;
    }
    w = WINDOW_FROM_ID( wid );

    /*
     * clear text area
     */
    AccessWindow( w );
    what.cinfo_char = ' ';
    what.cinfo_attr = MAKE_ATTR( w, w->text_color, w->background_color );
    shift = 0;
    addr = 0;
    if( w->has_border ) {
        shift = 1;
        addr = w->width + shift;
    }
    for( j = w->area.y1 + shift; j <= w->area.y2 - shift; j++ ) {
        oscr = w->area.x1 + shift + j * EditVars.WindMaxWidth;
        scr = &Scrn[oscr];
        txt = &(w->text[addr]);
        over = &(w->overlap[addr]);
        for( i = w->area.x1 + shift; i <= w->area.x2 - shift; i++ ) {
            WRITE_SCREEN_DATA( *txt++, what );
            if( BAD_ID( *over ) ) {
                WRITE_SCREEN( *scr, what );
            }
            over++;
            scr++;
        }
#ifdef __VIO__
        MyVioShowBuf( oscr, w->width - 2 * shift ); // inside of window only
#endif
        addr += w->width;
    }

    ReleaseWindow( w );

} /* ClearWindow */

/*
 * InsideWindow - test if coordinates are in window or on border
 */
bool InsideWindow( window_id wid, int x, int y )
{
    window      *w;

    w = WINDOW_FROM_ID( wid );
    if( !w->has_border ) {
        return( true );
    }
    if( x == 0 || y == 0 ) {
        return( false );
    }
    if( x == w->width - 1 ) {
        return( false );
    }
    if( y == w->height - 1 ) {
        return( false );
    }
    return( true );

} /* InsideWindow */
