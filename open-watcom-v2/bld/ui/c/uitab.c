/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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


#include "uidef.h"
#include "uitab.h"

#define _next( t ) ((*vtab->next)( t, vtab->parm ))
#define _tab( t )  ((*vtab->tab)( t, vtab->parm ))
#define _mouse( r, c )  ((*vtab->mousepos)( vtab->mouseparm, r, c )\
                        == vtab->mouseparm)

static void *fwd_tab( VTAB *vtab, VTABAREA *curr, unsigned wrap )
{
    VTABAREA            *chase;

    if( curr == NULL ) {
        chase = vtab->first;
    } else {
        chase = _next( curr );
    }
    for( ;; ) {
        if( chase == NULL ) {
            if( wrap ) {
                chase = vtab->first;
            } else {
                chase = NULL;
                break;
            }
        }
        if( _tab( chase ) )
            break;
        if( chase == curr ) {
            chase = NULL;
            break;
        }
        chase = _next( chase );
    }
    return( chase );
}

static void *bwd_tab( VTAB *vtab, VTABAREA *curr, unsigned wrap )
{
    VTABAREA           *chase, *hold;

    hold = NULL;
    for( chase = vtab->first; chase != curr; chase = _next( chase ) ) {
        if( _tab( chase ) ) {
            hold = chase;
        }
    }
    if( hold == NULL ) {
        if( wrap ) {
            for( ; chase != NULL; chase = _next( chase ) ) {
                if( _tab( chase ) ) {
                    hold = chase;
                }
            }
        }
    }
    return( hold );
}


ui_event uitabfilter( ui_event ui_ev, VTAB *vtab )
/*********************************************/
{
    VTABAREA   *curr, *best, *chase;
    ORD         r, c;
    unsigned    fwd;
    ui_event    retev;

    retev = EV_NO_EVENT;
    if( vtab->first != NULL ) {
        if( vtab->curr == NULL ) {
            curr = fwd_tab( vtab, NULL, true );
            if( curr == NULL )
                return( ui_ev );
            vtab->home = curr->area.col;
        } else {
            curr = vtab->curr;
        }
        switch( ui_ev ) {
        case EV_NO_EVENT :
            break;
        case EV_MOUSE_PRESS :
        case EV_MOUSE_DRAG :
        case EV_MOUSE_RELEASE :
            if( _mouse( &r, &c ) ) {
                best = NULL;
                for( chase = vtab->first; chase != NULL; chase = _next( chase ) ) {
                    if( r < chase->area.row )
                        break;
                    if( r == chase->area.row && c >= chase->area.col &&
                        c < chase->area.col + chase->area.width ) {
                        best = chase;
                        break;
                    }
                }
                if( ui_ev != EV_MOUSE_RELEASE ) {
                    vtab->other = best;
                    if( best == curr || best == NULL ) {
                        /* mouse has no tabbing effect */
                    } else if( _tab( best ) ) {
                        curr = best;
                    }
                }
                retev = ui_ev;
            }
            break;
        case EV_HOME :
            curr = fwd_tab( vtab, NULL, true );
            break;
        case EV_END :
            curr = bwd_tab( vtab, NULL, true );
            break;
        case EV_ENTER :
            if( !vtab->enter )
                return( ui_ev );
            vtab->home = 0;
            ui_ev = EV_CURSOR_DOWN;
            /* fall through */
        case EV_CURSOR_UP:
        case EV_CURSOR_DOWN:
            best = NULL;
            chase = curr;
            fwd = ( ui_ev == EV_CURSOR_DOWN );
            for( ;; ) {
                chase = fwd ? fwd_tab( vtab, chase, vtab->wrap ) :
                              bwd_tab( vtab, chase, vtab->wrap );
                if( chase == NULL ) {
                    if( best == NULL ) {
                        retev = ( fwd ? EV_BOTTOM : EV_TOP );
                    } else {
                        curr = best;
                    }
                    break;
                } else if( chase == curr ) {
                    if( best != NULL ) {
                        curr = best;
                    }
                    break;
                } else if( chase->area.row != curr->area.row ) {
                    if( best == NULL ) {
                        best = chase;
                    } else if( chase->area.row != best->area.row ) {
                        curr = best;
                        break;
                    } else if( abs( chase->area.col - vtab->home ) <
                               abs( best->area.col - vtab->home ) ) {
                        best = chase;
                    } else {
                        curr = best;
                        break;
                    }
                }
            }
            break;
        case EV_CURSOR_RIGHT :
        case EV_TAB_FORWARD :
            curr = fwd_tab( vtab, curr, true );
            break;
        case EV_CURSOR_LEFT :
        case EV_TAB_BACKWARD :
            curr = bwd_tab( vtab, curr, true );
            break;
        default:
            return( ui_ev );
        }
        if( vtab->curr != curr ) {
            if( ui_ev != EV_CURSOR_UP && ui_ev != EV_CURSOR_DOWN ) {
                vtab->home = curr->area.col;
            }
            retev = EV_FIELD_CHANGE;
            vtab->other = vtab->curr;
            vtab->curr = curr;
        }
        return( retev );
    }
    return( ui_ev );
}
