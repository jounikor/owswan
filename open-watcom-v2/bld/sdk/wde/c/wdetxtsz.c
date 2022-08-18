/****************************************************************************
*
*                            Open Watcom Project
*
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
#include "wde_wres.h"
#include "wdetxtsz.h"
#include "wdesdup.h"
#include "wdeactn.h"

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

static bool WdeGetTextSize( HWND win, HFONT font, char *text, SIZE *size )
{
    char        *str;
    int         i, len, pos;
    HFONT       old_font;
    HDC         dc;
    bool        ok;

    dc = (HDC)NULL;
    str = NULL;

    ok = (text != NULL && size != NULL);

    if( ok ) {
        len = strlen( text );
        str = (char *)WRMemAlloc( len + 1 );
        ok = (str != NULL);
    }

    if( ok ) {
        pos = 0;
        for( i = 0; i < len + 1; i++ ) {
            if( str[i] != '&' ) {
                text[pos] = str[i];
                pos++;
            }
        }
    }

    if( ok ) {
        ok = ((dc = GetDC( win )) != (HDC)NULL);
    }

    if( ok ) {
        old_font = SelectObject( dc, font );
        ok = ( GetTextExtentPoint( dc, str, pos, size ) != 0 );
    }

    if( str != NULL ) {
        WRMemFree( str );
    }

    if( dc != (HDC)NULL ) {
        SelectObject( dc, old_font );
        ReleaseDC( win, dc );
    }

    return( ok );
}

bool WdeGetNameOrOrdSize( OBJPTR parent, ResNameOrOrdinal *name, SIZE *size )
{
    char        *text;
    HWND        win;
    HFONT       font;
    bool        ok;

    text = NULL;

    ok = (parent != NULL && name != NULL && size != NULL);

    if( ok ) {
         ok = ( Forward( parent, GET_WINDOW_HANDLE, &win, NULL ) != 0 );
    }

    if( ok ) {
         ok = ( Forward( parent, GET_FONT, &font, NULL ) != 0 );
    }

    if( ok ) {
        text = WdeResNameOrOrdinalToStr( name, 10 );
        ok = (text != NULL);
    }

    if( ok ) {
        ok = WdeGetTextSize( win, font, text, size );
    }

    if( text != NULL ) {
        WRMemFree( text );
    }

    return( ok );
}
