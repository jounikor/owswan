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
#ifdef __WIN__
    #include "statwnd.h"
#endif

#include "clibext.h"


/*
 * NewStatusWindow - create a new status window
 */
vi_rc NewStatusWindow( void )
{
    vi_rc   rc = ERR_NO_ERR;

    if( !EditFlags.WindowsStarted ) {
        return( ERR_NO_ERR );
    }
    if( !BAD_ID( status_window_id ) ) {
        CloseAWindow( status_window_id );
        status_window_id = NO_WINDOW;
    }
    if( EditFlags.StatusInfo ) {
        rc = NewWindow2( &status_window_id, &statusw_info );
        UpdateStatusWindow();
    }
    return( rc );

} /* NewStatusWindow */

#ifndef __WIN__
/*
 * StatusLine - display a line in the status window
 */
static void StatusLine( int line, char *str, int format )
{
    int         len, width, blanks, i, j;
    type_style  *style;

    len = strlen( str );
    width = WindowAuxInfo( status_window_id, WIND_INFO_TEXT_COLS );
    style = &statusw_info.text_style;
    switch( format ) {
    case FMT_RIGHT:
        blanks = 0;
        if( width > len ) {
            blanks = width - len;
        }
        break;
    case FMT_CENTRE:
        blanks = 0;
        if( width > len ) {
            blanks = ( width - len ) / 2;
        }
        break;
    default:
        DisplayLineInWindow( status_window_id, line, str );
        return;
    }
    for( i = 1; i <= blanks; i++ ) {
        SetCharInWindowWithColor( status_window_id, line, i, ' ', style );
    }
    for( j = 0; j < len && i <= width; j++, i++ ) {
        SetCharInWindowWithColor( status_window_id, line, i, str[j], style );
    }
    for( ; i <= width; i++ ) {
        SetCharInWindowWithColor( status_window_id, line, i, ' ', style );
    }

} /* StatusLine */
#endif

/*
 * UpdateStatusWindow - update the status window
 */
void UpdateStatusWindow( void )
{
    char        *str, *ptr;
    char        result[5 * MAX_STR];
    char        *res;
    int         digits;
    long        num;
    bool        use_num;
    int         line;
    char        numstr[12];
    int         format;
    char        c;

    if( BAD_ID( status_window_id ) ||
        EditFlags.DisplayHold ||
        EditFlags.Quiet ||
        !EditFlags.StatusInfo ||
        EditVars.StatusString == NULL ) {
        return;
    }

    res = result;
    line = 1;
    format = FMT_LEFT;
    EditFlags.ModeInStatusLine = false;
    for( str = EditVars.StatusString; (c = *str) != '\0'; ++str ) {
        if( c == '$' ) {
            str++;
            ptr = str;
            SKIP_DIGITS( str );
            if( ptr != str ) {
                digits = strtoul( ptr, NULL, 10 );
            } else {
                digits = 0;
            }
            use_num = false;
            num = 0;
            c = *str;
            if( c == '\0' )
                break;
            switch( c ) {
            case '$':
                *res++ = '$';
                break;
            case 'c':
                *res++ = ',';
                break;
            case 'n':
                *res = 0;
                StatusLine( line, result, format );
                res = result;
                line++;
                break;
            case 'L':
                num = CurrentPos.line;
                use_num = true;
                break;
            case 'C':
                num = VirtualColumnOnCurrentLine( CurrentPos.column );
                use_num = true;
                break;
            case 'D':
#ifdef __WIN__
                GetDateString( res );
#else
                GetDateTimeString( res );
#endif
                res += strlen( res );
                break;
            case 'T':
                GetTimeString( res );
                res += strlen( res );
                break;
            case 'M':
                /* print the desired mode */
                EditFlags.ModeInStatusLine = true;
                GetModeString( res );
                res += strlen( res );
                break;
#ifdef __WIN__
            case 'H':
                GetMenuHelpString( res );
                res += strlen( res );
                break;
            case '[':
                *res++ = STATUS_ESC_CHAR;
                *res++ = STATUS_NEXT_BLOCK;
                break;
            case '|':
                *res++ = STATUS_ESC_CHAR;
                *res++ = STATUS_FORMAT_CENTER;
                break;
            case '>':
                *res++ = STATUS_ESC_CHAR;
                *res++ = STATUS_FORMAT_RIGHT;
                break;
            case '<':
                *res++ = STATUS_ESC_CHAR;
                *res++ = STATUS_FORMAT_LEFT;
                break;
#else
            case '|':
                format = FMT_CENTRE;
                break;
            case '>':
                format = FMT_RIGHT;
                break;
            case '<':
                format = FMT_LEFT;
                break;
#endif
            }
            if( use_num ) {
                ltoa( num, numstr, 10 );
                for( digits -= strlen( numstr ); digits > 0; digits-- ) {
                    *res++ = ' ';
                }
                for( ptr = numstr; *ptr != '\0'; ) {
                    *res++ = *ptr++;
                }
            }
        } else {
            *res++ = c;
        }
    }
    *res = 0;
    if( res != result ) {
        StatusLine( line, result, format );
    }

} /* UpdateStatusWindow */
