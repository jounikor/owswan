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


#include "imgedit.h"
#include "ieclrpal.h"


/* Local Window callback functions prototypes */
WPI_EXPORT WPI_DLGRESULT CALLBACK SelColorDlgProc( HWND hwnd, WPI_MSG msg, WPI_PARAM1 wparam, WPI_PARAM2 lparam );

static palette_box      screenColor;
static palette_box      inverseColor;
static palette_box      availColor[16];

/*
 * displayColors - display the colors
 */
static void displayColors( HWND hwnd )
{
    short       i;
    WPI_PRES    pres;
    HWND        currentwnd;

    inverseColor.color = GetInverseColor( screenColor.color );

    currentwnd = _wpi_getdlgitem( hwnd, BK_CURRENT );

    pres = _wpi_getpres( currentwnd );
    _wpi_torgbmode( pres );
    DisplayColorBox( pres, &screenColor );
    DisplayColorBox( pres, &inverseColor );

    for( i = 0; i < 16; i++ ) {
        DisplayColorBox( pres, &availColor[i] );
    }
    _wpi_releasepres( currentwnd, pres );

} /* displayColors */

/*
 * showColors - displays the colors to choose from
 */
static void showColors( HWND hwnd )
{
    InitFromColorPalette( &screenColor, &inverseColor, availColor );
    displayColors( hwnd );

} /* showColors */

/*
 * selectColor - select the color
 */
static void selectColor( WPI_POINT *pt, HWND hwnd )
{
    int         i;
    WPI_PRES    pres;
    HWND        currentwnd;
    int         top;
    int         bottom;
    WPI_RECT    wrect;

    currentwnd = _wpi_getdlgitem( hwnd, BK_CURRENT );
    pres = _wpi_getpres( currentwnd );
    _wpi_mapwindowpoints( hwnd, currentwnd, pt, 1 );

    _wpi_torgbmode( pres );
    for( i = 0; i < 16; i++ ) {
        top = availColor[i].box.top;
        bottom = availColor[i].box.bottom;

        top = _wpi_cvth_y( top, 2 * SQR_SIZE );
        bottom = _wpi_cvth_y( bottom, 2 * SQR_SIZE );
        _wpi_setintwrectvalues( &wrect, availColor[i].box.left, top,
                                        availColor[i].box.right, bottom );
        if( _wpi_ptinrect( &wrect, *pt ) ) {
            screenColor.color = availColor[i].color;
            DisplayColorBox( pres, &screenColor );

            inverseColor.color = GetInverseColor( screenColor.color );
            DisplayColorBox( pres, &inverseColor );
            break;
        }
    }
    _wpi_releasepres( currentwnd, pres );

} /* selectColor */

/*
 * SelColorDlgProc - select the color to represent the background
 */
WPI_DLGRESULT CALLBACK SelColorDlgProc( HWND hwnd, WPI_MSG msg, WPI_PARAM1 wparam, WPI_PARAM2 lparam )
{
    PAINTSTRUCT         ps;
    WPI_POINT           pt;
    WPI_PRES            pres;
    bool                ret;

    ret = false;

    if( _wpi_dlg_command( hwnd, &msg, &wparam, &lparam ) ) {
        switch( LOWORD( wparam ) ) {
        case IDOK:
            _wpi_enddialog( hwnd, IDOK );
            break;

        case IDCANCEL:
            _wpi_enddialog( hwnd, IDCANCEL );
            break;

        case IDB_HELP:
            IEHelpRoutine();
            break;
        }
    } else {
        switch( msg ) {
        case WM_INITDIALOG:
            showColors( hwnd );
            ret = true;
            break;

#ifndef __OS2_PM__
        case WM_SYSCOLORCHANGE:
            IECtl3dColorChange();
            break;
#endif

        case WM_LBUTTONDOWN:
            WPI_MAKEPOINT( wparam, lparam, pt );
            selectColor( &pt, hwnd );
            break;

        case WM_PAINT:
            pres = _wpi_beginpaint( hwnd, NULL, &ps );
#ifdef __OS2_PM__
            WinFillRect( pres, &ps, CLR_PALEGRAY );
#endif
            displayColors( hwnd );
            _wpi_endpaint( hwnd, pres, &ps );
            _wpi_setfocus( hwnd );
            break;

        case WM_CLOSE:
            _wpi_enddialog( hwnd, IDCANCEL );
            break;
        default:
            return( _wpi_defdlgproc( hwnd, msg, wparam, lparam ) );
        }
    }
    _wpi_dlgreturn( ret );

} /* SelColorDlgProc */

/*
 * ChooseBkColor - choose the color to represent the background
 */
void ChooseBkColor( void )
{
    WPI_DLGPROC         dlgproc;
    WPI_DLGRESULT       button_type;

    screenColor.color = GetViewBkColor();
    dlgproc = _wpi_makedlgprocinstance( SelColorDlgProc, Instance );
    button_type = _wpi_dialogbox( HMainWindow, dlgproc, Instance, SELBKCOLOR, 0L );
    _wpi_freedlgprocinstance( dlgproc );

    if( button_type == IDCANCEL ) {
        return;
    }

    SetViewBkColor( screenColor.color );
    SetScreenClr( screenColor.color );
    PrintHintTextByID( WIE_NEWBKCOLORSELECTED, NULL );

} /* ChooseBkColor */

