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


#include "vi.h"
#include "win.h"
#include <assert.h>

/*
 * goToLine - go to a specified line number
 */
static vi_rc goToLine( linenum lineno, bool relcurs )
{
    int         text_lines, tl;
    linenum     diff, cwl, nwl;
//    linenum   s, e, hiddcnt;
    bool        dispall, pageshift;
    fcb         *cfcb;
    line        *cline;
    int         pad;
    vi_rc       rc;

    if( lineno < 1 ) {
        return( ERR_NO_SUCH_LINE );
    }

    /*
     * get pointer to requested line
     */
    rc = CGimmeLinePtr( lineno, &cfcb, &cline );
    if( rc != ERR_NO_ERR ) {
        return( rc );
    }
#if 0
    if( cline->u.ld.hidden ) {
        GetHiddenRange( lineno, &s, &e );
        if( lineno > CurrentPos.line ) {
            lineno = e + 1;
        } else {
            lineno = s - 1;
        }
        rc = CGimmeLinePtr( lineno, &cfcb, &cline );
        if( rc != ERR_NO_ERR ) {
            return( rc );
        }
    }
#endif

    /*
     * compute new location
     */
    CurrentFcb = cfcb;
    CurrentLine = cline;
    diff = lineno - CurrentPos.line;
    if( diff == 0 && !EditFlags.GlobalInProgress ) {
        return( ERR_NO_ERR );
    }
    cwl = CurrentPos.line - LeftTopPos.line + 1;
    nwl = cwl + diff;

    /*
     * if we go off the window, relocate
     */
    pageshift = false;
    dispall = false;

    text_lines = WindowAuxInfo( current_window_id, WIND_INFO_TEXT_LINES );
    if( nwl < 1 || nwl > text_lines ) {
        tl = text_lines / 2;
        if( !relcurs ) {
            LeftTopPos.line = lineno - tl;
        } else {
            LeftTopPos.line = lineno + 1 - cwl;
            pad = ( EditFlags.JumpyScroll ) ? 1 : 0;
            if( diff > 0 ) {
                LeftTopPos.line += pad;
                diff += pad;
            } else {
                LeftTopPos.line -= pad;
                diff -= pad;
            }
            if( diff > -tl && diff < tl && !dispall ) {
                pageshift = true;
            }
        }
        if( LeftTopPos.line < 1 ) {
            assert( diff <= 0 ); // < -> <= W.Briscoe 20031003 to avoid debug build failure of
            // C:\watcom\source\docs\nt) wmake -h -f ..\mif\master.mif hbook=wccerrs dotarget=nt
            diff += ( 1 - LeftTopPos.line );
            LeftTopPos.line = 1;
        }
        if( LeftTopPos.line > lineno ) {
            assert( diff > 0 );
            diff = LeftTopPos.line - lineno;
            LeftTopPos.line = lineno;
        }
        dispall = true;
    }
#if 0
    hiddcnt = GetHiddenLineCount( LeftTopPos.line, lineno );
    if( hiddcnt > 0 ) {
        pageshift = false;
        dispall = true;
    }
#endif

    if( CheckCurrentColumn() || EditFlags.Dragging ) {
        // pageshift wont help if we also have to column shift
        // and not really useful if dragging

        dispall = true;
        pageshift = false;
    }


    /* call SetCurrentLineNumber AFTER LeftTopPos.line set & CurrentColumn checked
    */
    SetCurrentLineNumber( lineno );

    if( pageshift ) {
        dispall = false;
        ShiftWindowUpDown( current_window_id, diff );
        if( EditFlags.LineNumbers ) {
            ShiftWindowUpDown( linenum_current_window_id, diff );
        }
        if( diff > 0 ) {
            DCDisplaySomeLines( text_lines - diff, text_lines - 1 );
        } else {
            DCDisplaySomeLines( 0, -diff - 1 );
        }
    }
    UpdateStatusWindow();
    SetWindowCursor();
    if( dispall ) {
        DCInvalidateAllLines(); // lines definitely invalid
        DCDisplayAllLines();
    }
    return( ERR_NO_ERR );

} /* goToLine */

vi_rc GoToLineRelCurs( linenum lineno )
{
    return( goToLine( lineno, true ) );
}

vi_rc GoToLineNoRelCurs( linenum lineno )
{
    return( goToLine( lineno, false ) );
}

void SetCurrentLineNumber( linenum l )
{
    long        last;
    long        height;

    CurrentPos.line = l;
    UpdateCursorDrag();
    GlobVarAddRowAndCol();

    if( CurrentFile != NULL ) {
        height = WindowAuxInfo( current_window_id, WIND_INFO_TEXT_LINES );
        last = CurrentFile->fcbs.tail->end_line - height + 1;
        if( LeftTopPos.line > last ) {
             last = LeftTopPos.line;
        }
    } else {
        last = 1;
    }
    PositionVerticalScrollThumb( current_window_id, LeftTopPos.line, last );
}

/*
 * GoToColumnOK - go to a specified column that does not need a max check
 */
vi_rc GoToColumnOK( int colno )
{
    return( GoToColumn( colno, colno ) );

} /* GoToColumnOK */

/*
 * GoToColumnOnCurrentLine - go to a specified column on the current line
 */
vi_rc GoToColumnOnCurrentLine( int colno )
{
    if( CurrentLine == NULL ) {
        return( ERR_NO_FILE );
    }

    if( EditFlags.Modeless ) {
        GoToColumn( colno, CurrentLine->len + 1 );
    } else {
        GoToColumn( colno, CurrentLine->len );
    }

    return( ERR_NO_ERR );

} /* GoToColumnOnCurrentLine */

/*
 * GoToColumn - go to a specified column
 */
vi_rc GoToColumn( int colno, int maxcol )
{
    int vc;

    if( CurrentFile == NULL ) {
        return( ERR_NO_FILE );
    }

    if( maxcol == 0 ) {
        maxcol = 1;
    }
    if( colno == 0 ) {
        colno = 1;
    }
    if( colno < 1 || colno > maxcol ) {
        return( ERR_NO_SUCH_COLUMN );
    }

    /*
     * compute new location, and re-display text if needed
     */
    VirtualColumnDesired = VirtualColumnOnCurrentLine( colno );
    CurrentPos.column = colno;
    if( !CheckLeftColumn() ) {
        DCDisplayAllLines();
        PositionHorizontalScrollThumb( current_window_id, LeftTopPos.column );
    }

    SetWindowCursor();
    vc = VirtualColumnOnCurrentLine( CurrentPos.column );
    UpdateStatusWindow();
    GlobVarAddLong( GLOBVAR_COLUMN, vc );
    UpdateCursorDrag();
    return( ERR_NO_ERR );

} /* GoToColumn */

/*
 * SetCurrentLine - reset current line after changes in current file structure
 */
vi_rc SetCurrentLine( linenum lineno )
{
    int         text_lines;
    fcb         *cfcb;
    line        *cline;
    vi_rc       rc;

    if( lineno <= 0 ) {
        lineno = 1;
    }
    rc = CGimmeLinePtr( lineno, &cfcb, &cline );
    if( rc != ERR_NO_ERR ) {
        return( rc );
    }

    CurrentLine = cline;
    CurrentFcb = cfcb;

    text_lines = WindowAuxInfo( current_window_id, WIND_INFO_TEXT_LINES );
    if( lineno < LeftTopPos.line || lineno > (LeftTopPos.line + text_lines - 1) ) {
        LeftTopPos.line = lineno - text_lines / 2;
    }
    if( LeftTopPos.line < 1 ) {
        LeftTopPos.line = 1;
    }

    CheckCurrentColumn();
    SetCurrentLineNumber( lineno );
    UpdateStatusWindow();
    SetWindowCursor();
    DCDisplayAllLines();
    return( ERR_NO_ERR );

} /* SetCurrentLine */

/*
 * CheckLeftColumn - check if CurrentPos.column and LeftTopPos.column give a position
 *                   in the window; if not, LeftTopPos.column is changed appropriatly
 */
bool CheckLeftColumn( void )
{
    int     diff, wc, pad;
    bool    rc;

    wc = VirtualColumnOnCurrentLine( CurrentPos.column ) - LeftTopPos.column;

    rc = ColumnInWindow( wc, &diff );
    if( !rc ) {
        // |diff| is already at least 1
        pad = ( EditFlags.JumpyScroll ) ? SCROLL_HLINE - 1 : 0;
        if( diff < 0 ) {
            LeftTopPos.column += diff - pad;
        } else {
            LeftTopPos.column += diff + pad;
        }
        if( LeftTopPos.column < 0 ) {
            LeftTopPos.column = 0;
        }
    }
    return( rc );

} /* CheckLeftColumn */

/*
 * ValidateCurrentColumn - give CurrentPos.column an acceptable value
 */
void ValidateCurrentColumn( void )
{
    line        *cline;

    if( CurrentLine == NULL ) {
        return;
    }
    if( CurrentLine->u.ld.nolinedata ) {
        cline = WorkLine;
    } else {
        cline = CurrentLine;
    }

    if( CurrentPos.column >= cline->len ) {
        if( (EditFlags.InsertModeActive || EditFlags.Modeless) &&
            CurrentPos.column > cline->len ) {
            CurrentPos.column = cline->len + 1;
        } else {
            CurrentPos.column = cline->len;
        }
    }
    if( CurrentPos.column <= 0  ) {
        CurrentPos.column = 1;
    }

} /* ValidateCurrentColumn */

/*
 * CheckCurrentColumn - check state of current column, return true if need to
 *                      redisplay page
 */
bool CheckCurrentColumn( void )
{
    int     clen, vcp;
    bool    dispall = false;

    clen = VirtualLineLen( CurrentLine->data );
    if( clen == 0 ) {
        clen = 1;
    }
    ValidateCurrentColumn();

    vcp = VirtualColumnOnCurrentLine( CurrentPos.column );

    if( vcp != VirtualColumnDesired ) {
        if( clen >= VirtualColumnDesired ) {
            CurrentPos.column = RealColumnOnCurrentLine( VirtualColumnDesired );
        } else {
            if( EditFlags.InsertModeActive || EditFlags.Modeless ) {
                CurrentPos.column = CurrentLine->len + 1;
            } else {
                CurrentPos.column = CurrentLine->len;
            }
        }
        ValidateCurrentColumn();
        dispall = !CheckLeftColumn();
        /* changed CurrentPos.column - update horiz scrollbar
        */
        PositionHorizontalScrollThumb( current_window_id, LeftTopPos.column );
    }
    GlobVarAddLong( GLOBVAR_COLUMN, CurrentPos.column );
    return( dispall );

} /* CheckCurrentColumn */

/*
 * ShiftTab - figure out value of reverse tab
 */
int ShiftTab( int col, int ta )
{
    int i, j;

    col--;
    j = col / ta;
    i = col - j * ta;
    if( i == 0 && col != 0 ) {
        i = ta;
    }
    return( i );

} /* ShiftTab */

/*
 * SetCurrentColumn - set CurrentPos.column, positioning LeftTopPos.column nicely
 */
vi_rc SetCurrentColumn( int newcol )
{
    long        oldpos;
    int         text_cols;

    oldpos = CurrentPos.column - LeftTopPos.column;
    if( newcol <= 0 ) {
        newcol = 1;
    }

    text_cols = WindowAuxInfo( current_window_id, WIND_INFO_TEXT_COLS );
    if( oldpos < 0 || oldpos >= text_cols ) {
        LeftTopPos.column = newcol - SCROLL_HLINE - 1;
    } else {
        LeftTopPos.column = newcol - oldpos - 1;
    }
    if( LeftTopPos.column < 0 ) {
        LeftTopPos.column = 0;
    }

    CurrentPos.column = newcol;

    CheckCurrentColumn();
    UpdateCursorDrag();
    GlobVarAddRowAndCol();

    PositionHorizontalScrollThumb( current_window_id, LeftTopPos.column );
    UpdateStatusWindow();
    SetWindowCursor();
    DCDisplayAllLines();
    return( ERR_NO_ERR );

} /* SetCurrentColumn */

/*
 * LocateCmd - parse a locate command (format: locate r,c[,len])
 */
vi_rc LocateCmd( const char *data )
{
    char        tmp[MAX_STR];
    linenum     r;
    int         c;
    int         len;

#ifdef __WIN__
    if( BAD_ID( current_window_id ) ) {
        return( ERR_INVALID_LOCATE );
    }
#endif
    data = GetNextWord1( data, tmp );
    if( *tmp == '\0' ) {
        return( ERR_INVALID_LOCATE );
    }
    r = atol( tmp );
    data = GetNextWord1( data, tmp );
    if( *tmp == '\0' ) {
        return( ERR_INVALID_LOCATE );
    }
    c = atoi( tmp );

    // real selection length
    len = 0;
    if( *data != 0 ) {
        data = GetNextWord1( data, tmp );
        if( *tmp == '\0' ) {
            return( ERR_INVALID_LOCATE );
        }
        len = atoi( tmp );
    }

    GoToLineNoRelCurs( r );

    c = RealColumnOnCurrentLine( c );
    GoToColumnOnCurrentLine( c + len );

#ifdef __WIN__
    // likely only called by dde, which doesn't use event loop,
    // so must ensure cache ok and set cursor here

    DCInvalidateAllLines();
    DCDisplayAllLines();
    DCUpdate();
    SetWindowCursor();
    SetWindowCursorForReal();
#endif

    if( len > 0 ) {
        SetSelRegionCols( CurrentPos.line, c, c + len - 1 );
    }
    return( ERR_NO_ERR );

} /* LocateCmd */
