/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2020 The Open Watcom Contributors. All Rights Reserved.
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
#include "parsecl.h"
#include "win.h"
#ifdef __WIN__
#include "utils.h"
#include "vifont.h"

extern int MouseX;
extern int MouseY;
#endif

static hooktype hookRun = SRC_HOOK_NONE;
static char     *srcHookData;

/*
 * findHook - look for a hook routine
 */
static vars *findHook( hooktype num )
{
    const char  *name;

    switch( num ) {
    #define pick(e,m,t) case e: name = t; break;
    SRCHOOK_DEFS
    #undef pick
    default:
        return( NULL );
    }
    return( GlobVarFind( name ) );

} /* findHook */

/*
 * GetHookVar - get hook variable
 */
vars *GetHookVar( hooktype num )
{
    vars        *v;

    if( num & SRC_HOOK_READ ) {
        v = findHook( num & SRC_HOOK_READ );
    } else if( num & SRC_HOOK_BUFFIN ) {
        v = findHook( num & SRC_HOOK_BUFFIN );
    } else if( num & SRC_HOOK_BUFFOUT ) {
        v = findHook( num & SRC_HOOK_BUFFOUT );
    } else {
        v = findHook( num );
    }
    return( v );

} /* GetHookVar */

/*
 * srcHook - run a specified source hook
 */
static vi_rc srcHook( hooktype num, vi_rc lastrc )
{
    vars        *v;
    srcline     sline;
    vi_rc       rc;

    if( hookRun & num ) {
        return( lastrc );
    }

    /*
     * check script type
     */
    v = GetHookVar( num );
    /*
     * run script, if we have one
     */
    if( v != NULL ) {
        if( num == SRC_HOOK_COMMAND ) {
            GlobVarAddStr( GLOBVAR_COMMAND_BUFFER, CommandBuffer );
        }
//        if( num == SRC_HOOK_MODIFIED ) {
//            lastrc = LastEvent;
//        }

        /*
         * set up for and run script
         */
        hookRun |= num;
        LastRetCode = lastrc;
        rc = Source( v->value, srcHookData, &sline );

        /*
         * if we had a command hook, look for replacement variable
         */
        if( num == SRC_HOOK_COMMAND ) {
            v = GlobVarFind( GLOBVAR_COMMAND_BUFFER );
            if( v != NULL ) {
                strcpy( CommandBuffer, v->value );
            }
        }

        /*
         * we are done now, reset and go back
         */
        LastRetCode = ERR_NO_ERR;
        hookRun &= ~num;
        DCUpdateAll();
        return( rc );

    }
    return( lastrc );

} /* srcHook */

/*
 * SourceHook - activate source hook, no data
 */
vi_rc SourceHook( hooktype num, vi_rc lastrc )
{
    char        data[1];

    data[0] = '\0';
    srcHookData = data;
    return( srcHook( num, lastrc ) );

} /* SourceHook */

/*
 * SourceHookWithData - activate source hook with data
 */
vi_rc SourceHookWithData( hooktype num, char *data )
{
    vi_rc       rc;

    srcHookData = data;
    rc = srcHook( num, ERR_NO_ERR );
    return( rc );

} /* SourceHookWithData */

/*
 * HookScriptCheck - check for hook scripts
 */
void HookScriptCheck( void )
{
    if( findHook( SRC_HOOK_READ ) != NULL ) {
        ReadErrorTokens();
    }

} /* HookScriptCheck */


/*
 * InvokeColSelHook - invoke column hook with specified data
 */
vi_rc InvokeColSelHook( int sc, int ec )
{
    int         j, i;
    char        wordbuff[MAX_STR];
    char        data[MAX_STR + 32];
    int         lne;
#ifndef __WIN__
    int         x1;
    bool        has_border;
#endif

#ifdef __WIN__
    if( LastEvent != VI_KEY( FAKEMOUSE ) ) {
        lne = (CurrentPos.line - LeftTopPos.line) * FontHeight( WIN_TEXT_FONT( &EditWindow ) );
    } else {
        lne = MouseY;
    }
#else
    has_border = ( WindowAuxInfo( current_window_id, WIND_INFO_HAS_BORDER ) != 0 );
    x1 = WindowAuxInfo( current_window_id, WIND_INFO_X1 );
    if( LastEvent != VI_KEY( MOUSEEVENT ) ) {
        lne = WindowAuxInfo( current_window_id, WIND_INFO_Y1 ) + CurrentPos.line - LeftTopPos.line;
        if( has_border ) {
            ++lne;
        }
    } else {
        lne = MouseRow;
    }
#endif

    j = 0;
    if( ec - sc >= MAX_STR ) {
        ec = sc + MAX_STR - 2;
    }
    for( i = sc - 1; i <= ec - 1; i++ ) {
        wordbuff[j++] = CurrentLine->data[i];
    }
    wordbuff[j] = '\0';
#ifdef __WIN__
    sc = MyTextExtent( current_window_id, WIN_TEXT_STYLE( &EditWindow ),
        &CurrentLine->data[0], sc );
    ec = MyTextExtent( current_window_id, WIN_TEXT_STYLE( &EditWindow ),
        &CurrentLine->data[0], ec );
#else
    sc = x1 + VirtualColumnOnCurrentLine( sc ) - LeftTopPos.column;
    ec = x1 + VirtualColumnOnCurrentLine( ec ) - LeftTopPos.column;
    if( !has_border ) {
        sc--;
        ec--;
    }
#endif
    MySprintf( data, "\"%s\" %d %d %d %d", wordbuff, lne, sc, ec, ec - sc + 1 );
    return( SourceHookWithData( SRC_HOOK_MOUSE_CHARSEL, data ) );

} /* InvokeColSelHook */


/*
 * InvokeLineSelHook - invoke the mouse selection
 */
vi_rc InvokeLineSelHook( linenum s, linenum e )
{
    char        tmp[32];
    int         lne, col;
#ifndef __WIN__
    bool        has_border;
#endif

#ifdef __WIN__
    if( LastEvent != VI_KEY( FAKEMOUSE ) ) {
        /* assume we're not in insert mode *ouch* */
        col = PixelFromColumnOnCurrentLine( CurrentPos.column );
        lne = (CurrentPos.line - LeftTopPos.line) * FontHeight( WIN_TEXT_FONT( &EditWindow ) );
    } else {
        col = MouseX;
        lne = MouseY;
    }
#else
    if( LastEvent != VI_KEY( MOUSEEVENT ) ) {
        has_border = ( WindowAuxInfo( current_window_id, WIND_INFO_HAS_BORDER ) != 0 );
        lne = WindowAuxInfo( current_window_id, WIND_INFO_Y1 ) + CurrentPos.line - LeftTopPos.line;
        col = WindowAuxInfo( current_window_id, WIND_INFO_X1 ) + VirtualColumnOnCurrentLine( CurrentPos.column ) - LeftTopPos.column - 1;
        if( has_border ) {
            ++lne;
            ++col;
        }
        if( col < 0 ) {
            col = 0;
        }
    } else {
        col = MouseCol;
        lne = MouseRow;
    }
#endif
    MySprintf( tmp, "%d %d %l %l", lne, col, s, e );
    return( SourceHookWithData( SRC_HOOK_MOUSE_LINESEL, tmp ) );

} /* InvokeLineSelHook */

/*
 * InvokeMenuHook - invoke the menu hook
 */
vi_rc InvokeMenuHook( int menunum, int line )
{
    char        tmp[16];
    vi_rc       rc;

    MySprintf( tmp, "%d %d", menunum, line );
    rc = SourceHookWithData( SRC_HOOK_MENU, tmp );
    return( rc );

} /* InvokeMenuHook */
