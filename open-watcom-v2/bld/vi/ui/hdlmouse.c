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
#include "mouse.h"
#include "menu.h"
#include "win.h"

static bool dragThumb;

/*
 * HandleMouseEvent - handle main editor mouse events
 */
vi_rc HandleMouseEvent( void )
{
    windim      win_x, win_y;
    window_id   wid;
    info        *cinfo;
    window      *w;
    ctl_id      id;
    bool        diff_word;
    vi_rc       rc;

    wid = GetMousePosInfo( &win_x, &win_y );
    if( BAD_ID( wid ) ) {
        return( ERR_NO_ERR );
    }
    w = WINDOW_FROM_ID( wid );
    if( !w->has_border ) {
        win_x += 1;
        win_y += 1;
    }

    if( dragThumb ) {
        if( LastMouseEvent == VI_MOUSE_RELEASE ) {
            dragThumb = false;
        }
        if( wid != current_window_id ) {
            return( ERR_NO_ERR );
        }
        if( win_x == w->width - 1 ) {
            return( PositionToNewThumbPosition( w, win_y ) );
        }
        return( ERR_NO_ERR );
    }

    if( EditFlags.Dragging ) {
        if( LastMouseEvent == VI_MOUSE_DRAG || LastMouseEvent == VI_MOUSE_REPEAT ) {
            UpdateDrag( wid, win_x, win_y );
        } else {
            if( LastMouseEvent == VI_MOUSE_PRESS_R || LastMouseEvent == VI_MOUSE_PRESS ) {
                EditFlags.Dragging = false;
                if( LastMouseEvent == VI_MOUSE_PRESS_R ) {
                    LastMouseEvent = VI_MOUSE_RELEASE_R;
                }
            }
        }
    }

    if( LastMouseEvent == VI_MOUSE_RELEASE_R || LastMouseEvent == VI_MOUSE_DCLICK ) {
        if( wid == current_window_id && InsideWindow( wid, win_x, win_y ) ) {
            diff_word = (LastMouseEvent == VI_MOUSE_DCLICK);
            if( GoToLineRelCurs( LeftTopPos.line + win_y - 1 ) ) {
                return( ERR_NO_ERR );
            }
            win_x += LeftTopPos.column;
            win_x = RealColumnOnCurrentLine( win_x );
            GoToColumnOnCurrentLine( win_x );
            if( diff_word ) {
                InitWordSearch( EditVars.WordAltDefn );
            }
            rc = DoSelectSelection( true );
            if( diff_word ) {
                InitWordSearch( EditVars.WordDefn );
            }
            return( rc );
        }
    }

    /*
     * all kinds of stuff to do if the button was pressed
     */
    if( LastMouseEvent == VI_MOUSE_PRESS || LastMouseEvent == VI_MOUSE_PRESS_R ) {
        if( wid != current_window_id ) {
            /*
             * swap to another window
             */
            for( cinfo = InfoHead; cinfo != NULL; cinfo = cinfo->next ) {
                if( wid == cinfo->current_window_id ) {
                    BringUpFile( cinfo, true );
                    break;
                }
            }
        }
        if( wid == current_window_id ) {
            if( !ShiftDown() ) {
                UnselectRegion();
            }
            if( w->has_border && LastMouseEvent == VI_MOUSE_PRESS ) {
                /*
                 * clicked on menu for window
                 */
                if( win_x == 0 && win_y == 0 ) {
                    return( DoWindowGadgetMenu() );
                }

                /*
                 * check for resize request
                 */
                if( win_x == w->width - 1 && win_y == w->height - 1 ) {
                    return( ResizeCurrentWindowWithMouse() );
                }

                /*
                 * check for move request
                 */
                if( win_y == 0 ) {
                    return( MoveCurrentWindowWithMouse() );
                }
            }

            /*
             * check for locate cursor
             */
            if( InsideWindow( wid, win_x, win_y ) ) {
                if( ShiftDown() ) {
                    EditFlags.Dragging = true;
                }
                if( GoToLineRelCurs( LeftTopPos.line + win_y - 1 ) ) {
                    return( ERR_NO_ERR );
                }
                win_x += LeftTopPos.column;
                win_x = RealColumnOnCurrentLine( win_x );
                GoToColumnOnCurrentLine( win_x );
                if( ShiftDown() ) {
                    EditFlags.Dragging = false;
                } else {
                    InitSelectedRegion();
                }
                return( ERR_NO_ERR );
            }
        }
        if( EditFlags.Menus && wid == menu_window_id ) {
            id = GetMenuIdFromCoord( win_x - 1 );
            if( id != NO_ID ) {
                return( SetToMenuId( id ) );
            }
        }
    }

    /*
     * allow double click to close window
     */
    if( wid == current_window_id && LastMouseEvent == VI_MOUSE_DCLICK ) {
        if( win_y == 0 && win_x == 0 ) {
            return( NextFile() );
        }
    }

    /*
     * try to scroll screen
     */
    if( (LastMouseEvent == VI_MOUSE_REPEAT || LastMouseEvent == VI_MOUSE_DCLICK ||
         LastMouseEvent == VI_MOUSE_PRESS) && w->has_border &&
        wid == current_window_id && win_x == w->width - 1 ) {
        if( win_y == w->height - 2 ) {
            return( MoveScreenDown() );
        }
        if( win_y == 1 ) {
            return( MoveScreenUp() );
        }
        /*
         * if we have gadgets, then scroll based on position of scroll
         * thumb. furthermore, if the thumb is selected, then begin
         * thumb dragging mode
         */
        if( w->has_gadgets ) {
            if( win_y == w->vert_scroll_pos ) {
                dragThumb = true;
                return( ERR_NO_ERR );
            } else if( win_y < w->vert_scroll_pos ) {
                return( MovePageUp() );
            } else {
                return( MovePageDown() );
            }
        } else {
            if( win_y < w->height / 2 ) {
                return( MovePageUp() );
            } else {
                return( MovePageDown() );
            }
        }
    }

    /*
     * start dragging
     */
    if( wid == current_window_id && (LastMouseEvent == VI_MOUSE_DRAG ||
                                LastMouseEvent == VI_MOUSE_DRAG_R ) &&
        InsideWindow( wid, win_x, win_y ) ) {
        EditFlags.Dragging = true;
        UpdateDrag( wid, win_x, win_y );
    }

    return( ERR_NO_ERR );

} /* HandleMouseEvent */
