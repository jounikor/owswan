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
* Description:  screen/keyboard/color helper routines for VI
*
****************************************************************************/


#include "vi.h"
#ifdef __WATCOMC__
    #include <conio.h>
#endif
#include "win.h"
#include "dosx.h"
#include "vibios.h"
#include "pragmas.h"

static int  saveRow, saveCol;
int         PageCnt = 0;

#if defined( _M_I86 ) /* || defined( __4G__ ) */
static char     colorPalette[MAX_COLOR_REGISTERS + 1];
#else
static char     colorPalette[MAX_COLOR_REGISTERS + 1] = {
    0, 1, 2, 3, 4, 5, 0x14, 7,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x00
};
#endif
static rgb      oldColors[MAX_COLOR_REGISTERS];
static rgb      newColors[MAX_COLOR_REGISTERS];
static bool     colorChanged[MAX_COLOR_REGISTERS];

/*
 * getCursor - get cursor position
 */
static void getCursor( int *row, int *col )
{
    unsigned short  x;

    x = BIOSGetCursorPos( VideoPage );
    *row = ( x >> 8 );
    *col = x & 0xff;

} /* getCursor */

/*
 * setCursor - set cursor position
 */
static void setCursor( int row, int col )
{
    BIOSSetCursorPos( VideoPage, row, col );

} /* setCursor */

/*
 * KillCursor - do just that, get rid of it
 */
void KillCursor( void )
{
    getCursor( &saveRow, &saveCol );
    setCursor( EditVars.WindMaxHeight, 0 );

} /* KillCursor */

#if 0
/*
 * TurnOffCursor - as it sounds
 */
void TurnOffCursor( void )
{
    setCursor( EditVars.WindMaxHeight, 0 );

} /* TurnOffCursor */
#endif

/*
 * RestoreCursor - bring back a killed cursor
 */
void RestoreCursor( void )
{
    setCursor( saveRow, saveCol );

} /* RestoreCursor */

vi_rc SetFont( const char *data )
{
    /* unused parameters */ (void)data;

    return( ERR_NO_ERR );
}

/*
 * ClearScreen - clear the screen
 */
void ClearScreen( void )
{
    int                 i;
    char_info           what = {0, 0};
    char_info           _FAR *foo;

    if( EditFlags.Quiet ) {
        return;
    }
    foo = Scrn;
    what.cinfo_char = ' ';
    what.cinfo_attr = EditVars.ExitAttr;
    for( i = EditVars.WindMaxWidth * EditVars.WindMaxHeight - 1; i >= 0; i-- ) {
        WRITE_SCREEN( *foo, what );
        foo++;
    }
#ifdef __VIO__
    MyVioShowBuf( 0, EditVars.WindMaxWidth * EditVars.WindMaxHeight );
#endif
    setCursor( 0, 0 );

} /* ClearScreen */

/*
 * GetClockStart - get clock start position
 */
void GetClockStart( void )
{
    ClockStart = &Scrn[EditVars.ClockX + EditVars.ClockY * EditVars.WindMaxWidth];

} /* GetClockStart */

/*
 * GetSpinStart - get spinner start position
 */
void GetSpinStart( void )
{
    SpinLoc = &Scrn[EditVars.SpinX + EditVars.SpinY * EditVars.WindMaxWidth];

} /* GetSpinStart */

/*
 * SetPosToMessageLine - set cursor position
 */
void SetPosToMessageLine( void )
{
    setCursor( EditVars.WindMaxHeight - 1, 0 );

} /* SetPosToMessageLine */

/*
 * SetGenericWindowCursor - put cursor in any window at (l,c)
 */
void SetGenericWindowCursor( window_id wid, int l, int c )
{
    window      *w;
    int         row, col;

    w = WINDOW_FROM_ID( wid );

    row = w->area.y1;
    col = w->area.x1;

    row += l;
    col += c;
    if( !w->has_border ) {
        row--;
        col--;
    }
    setCursor( row, col );

} /* SetGenericWindowCursor */

void HideCursor( void )
{
    setCursor( -1, -1 );
}

/*
 * getColorRegister - as it sounds
 */
static void getColorRegister( vi_color reg, rgb *c )
{
    uint_32     res;

    res = BIOSGetColorRegister( colorPalette[reg] );
    c->red = (res >> (8 - 2)) & 0xFC;
    c->blue = (res >> (16 - 2)) & 0xFC;
    c->green = (res >> (24 - 2)) & 0xFC;

} /* getColorRegister */

/*
 * setColorRegister - as it sounds
 */
static void setColorRegister( vi_color reg, rgb *c )
{
    BIOSSetColorRegister( colorPalette[reg], c->red >> 2, c->green >> 2, c->blue >> 2 );

} /* setColorRegister */

/*
 * getColorPalette - as it sounds
 */
static void getColorPalette( char *p )
{
//#if defined( _M_I86 ) || defined( __OS2__ ) /* || defined( __4G__ ) */
    BIOSGetColorPalette( p );
//#else
    /* unused parameters */ (void)p;
//#endif

} /* getColorPalette */

/*
 * InitColors - set up default colors
 */
void InitColors( void )
{
    /*
     * set color palette (if in color mode)
     */
    if( EditFlags.Color && !EditFlags.Quiet ) {

        BIOSSetBlinkAttr( false );
        getColorPalette( colorPalette );

    }

} /* InitColors */

void ResetColors( void )
{
    int i;

    if( EditFlags.Color && !EditFlags.Quiet ) {
        BIOSSetBlinkAttr( false );
        for( i = 0; i < MAX_COLOR_REGISTERS; i++ ) {
            if( colorChanged[i] ) {
                setColorRegister( i, &newColors[i] );
            }
        }
    }
}

/*
 * FiniColors - reset colors on exit
 */
void FiniColors( void )
{
    int i;

    if( EditFlags.Color && !EditFlags.Quiet ) {
        for( i = 0; i < MAX_COLOR_REGISTERS; i++ ) {
            if( colorChanged[i] ) {
                setColorRegister( i, &oldColors[i] );
            }
        }
        BIOSSetBlinkAttr( true );
    }

} /* FiniColors */

/*
 * SetAColor - perform a setcolor command
 */
vi_rc SetAColor( const char *data )
{
    rgb         c;
    int         clr;
    char        token[MAX_STR];

    data = GetNextWord1( data, token );
    if( *token == '\0' ) {
        return( ERR_INVALID_SETCOLOR );
    }
    clr = atoi( token );
    data = GetNextWord1( data, token );
    if( *token == '\0' ) {
        return( ERR_INVALID_SETCOLOR );
    }
    c.red = atoi( token );
    data = GetNextWord1( data, token );
    if( *token == '\0' ) {
        return( ERR_INVALID_SETCOLOR );
    }
    c.green = atoi( token );
    data = GetNextWord1( data, token );
    if( *token == '\0' ) {
        return( ERR_INVALID_SETCOLOR );
    }
    c.blue = atoi( token );
    if( !EditFlags.Quiet ) {
        if( !colorChanged[clr] ) {
            getColorRegister( clr, &oldColors[clr] );
            colorChanged[clr] = true;
        }
        setColorRegister( clr, &c );
        newColors[clr] = c;
    }
    return( ERR_NO_ERR );

} /* SetAColor */

/*
 * GetNumColors - get # colors stored (in array newColors)
 */
int GetNumColors( void )
{
    return( MAX_COLOR_REGISTERS );
}

/*
 * GetColorSetting - get a specific color setting
 */
bool GetColorSetting( vi_color clr, rgb *c )
{
    if( !colorChanged[clr] ) {
        return( false );
    }
    *c = newColors[clr];
    return( true );

} /* GetColorSetting */
