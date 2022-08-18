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
#include <stddef.h>
#include "walloca.h"
#include "win.h"

/*
 * ResetWindow - close a window an re-create it
 */
vi_rc ResetWindow( window_id *wid )
{
    window      *w;
    char        *tmp;
    vi_rc       rc;

    w = WINDOW_FROM_ID( *wid );
    if( w->title != NULL ) {
        tmp = alloca( strlen( w->title ) + 1 );
        strcpy( tmp, w->title );
    } else {
        tmp = NULL;
    }
    CloseAWindow( *wid );
    rc = NewWindow2( wid, &editw_info );
    if( rc != ERR_NO_ERR ) {
        return( rc );
    }
    SetBorderGadgets( *wid, EditFlags.WindowGadgets );
    if( tmp != NULL ) {
        WindowTitle( *wid, tmp );
    }
    DCDisplayAllLines();
    return( ERR_NO_ERR );

} /* ResetWindow */

/*
 * Valid Dimension - see if a window has a valid dim or not
 */
bool ValidDimension( windim x1, windim y1, windim x2, windim y2, bool has_border )
{
    windim  lb;

    if( !has_border ) {
        lb = 0;
    } else {
        lb = 2;
    }

    if( x2 - x1 < lb || x2 >= EditVars.WindMaxWidth ) {
        return( false );
    }
    if( y2 - y1 < lb || y2 >= EditVars.WindMaxHeight ) {
        return( false );
    }
    if( x1 < 0 || y1 < 0 ) {
        return( false );
    }
    return( true );

} /* ValidDimension */

/*
 * GimmeWindow - find next avaliable window
 */
static window_id GimmeWindow( void )
{
    window_id   wid;

    for( wid = 0; wid < MAX_WINDS; wid++ ) {
        if( WINDOW_FROM_ID( wid ) == NULL ) {
            return( wid );
        }
    }
    return( NO_WINDOW );

} /* GimmeWindow */

/*
 * AllocWindow - allocate a new window
 */
window *AllocWindow( window_id wid, windim x1, windim y1, windim x2, windim y2, bool has_border, bool has_gadgets,
                        vi_color bc1, vi_color bc2, vi_color tc, vi_color bgc )
{
    window      *w;
    windim      width, height;
    int         size;
    int         i;

    width = x2 - x1 + 1;
    height = y2 - y1 + 1;
    size = width * height;

    w = MemAlloc( offsetof( window, overcnt ) + height );
    w->id = wid;
    w->has_gadgets = has_gadgets;
    w->accessed = 0;
    w->isswapped = false;
    w->text = _MemAllocArray( char_info, size );
    w->overlap = _MemAllocArray( window_id, size );
    w->whooverlapping = _MemAllocArray( window_id, size );
    w->area.x1 = x1;
    w->area.x2 = x2;
    w->area.y1 = y1;
    w->area.y2 = y2;
    w->has_border = has_border;
    w->border_color1 = bc1;
    w->border_color2 = bc2;
    w->text_color = tc;
    w->background_color = bgc;
    w->width = width;
    w->height = height;
    w->text_lines = height;
    w->text_cols = width;
    if( has_border ) {
        w->text_lines -= 2;
        w->text_cols -= 2;
        w->vert_scroll_pos = THUMB_START;
    }
    for( i = 0; i < size; ++i ) {
        w->overlap[i] = NO_WINDOW;
        w->whooverlapping[i] = NO_WINDOW;
    }
    for( i = 0; i < height; ++i ) {
        w->overcnt[i] = 0;
    }
    WINDOW_TO_ID( wid, w );
    return( w );

} /* AllocWindow */

/*
 * NewWindow - build a new window
 */
vi_rc NewWindow( window_id *wid, windim x1, windim y1, windim x2, windim y2, bool has_border,
               vi_color bc1, vi_color bc2, type_style *s )
{
    window_id   new_wid;
    bool        has_mouse;

    if( !ValidDimension( x1, y1, x2, y2, has_border ) ) {
        return( ERR_WIND_INVALID );
    }

    new_wid = GimmeWindow();
    if( BAD_ID( new_wid ) ) {
        return( ERR_WIND_NO_MORE_WINDOWS );
    }

    has_mouse = DisplayMouse( false );

    AllocWindow( new_wid, x1, y1, x2, y2, has_border, false, bc1, bc2, s->foreground, s->background );

    MarkOverlap( new_wid );

    ClearWindow( new_wid );
    DrawBorder( new_wid );

    *wid = new_wid;
    DisplayMouse( has_mouse );
    return( ERR_NO_ERR );

} /* NewWindow */

/*
 * NewFullWindow - build a new full window
 */
vi_rc NewFullWindow( window_id *wid, bool has_border, vi_color bc1, vi_color bc2, type_style *s )
{
    return( NewWindow( wid, 0, 0, 79, 24, has_border, bc1, bc2, s ) );

} /* NewFullWindow */

/*
 * NewWindow2 - build a new window, using window_info struct
 */
vi_rc NewWindow2( window_id *wid, window_info *wi )
{
    return( NewWindow( wid, wi->area.x1, wi->area.y1, wi->area.x2, wi->area.y2,
                       wi->has_border, wi->border_color1,
                       wi->border_color2, &wi->text_style ) );

} /* NewWindow2 */

/*
 * FreeWindow - free data associated with a window
 */
void FreeWindow( window *w )
{
    MemFree( w->text );
    MemFree( w->overlap );
    MemFree( w->whooverlapping );
    MemFree( w->title );
    MemFree( w->borderdata );
    MemFree( w );

} /* FreeWindow */

/*
 * CloseAWindow - close down specified window
 */
void CloseAWindow( window_id wid )
{
    window      *w;

    w = WINDOW_FROM_ID( wid );

    RestoreOverlap( wid, true );
    if( w->min_slot ) {
        MinSlots[w->min_slot - 1] = false;
    }

    FreeWindow( w );

    WINDOW_TO_ID( wid, NULL );

} /* CloseAWindow */
