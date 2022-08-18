/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
#include "ieprofil.h"


static HBRUSH hbrush;

/*
 * paintPalette - repaint the color palette
 */
static void paintPalette( HWND hwnd )
{
    WPI_PRES            hdc;
    WPI_POINT           pt;
    HPEN                holdpen;
    HPEN                hgraypen;
    HPEN                hwhitepen;
    PAINTSTRUCT         rect;
    WPI_RECT            client;
    int                 height;

    hdc = _wpi_beginpaint( hwnd, NULL, &rect );
#ifdef __OS2_PM__
    WinFillRect( hdc, &rect, CLR_PALEGRAY );
#endif
    _wpi_torgbmode( hdc );
    GetClientRect( hwnd, &client );
    height = _wpi_getheightrect( client );
#if defined( __NT__ )
    FillRect( (HDC)hdc, (CONST RECT*)&client, hbrush );
#endif

#if defined( __NT__ )
    hgraypen = _wpi_createpen( PS_SOLID, 0, GetSysColor( COLOR_BTNSHADOW ) );
#else
    hgraypen = _wpi_createpen( PS_SOLID, 0, DKGRAY );
#endif
    holdpen = _wpi_selectpen( hdc, hgraypen );
    pt.x = 2;
    pt.y = 50;
    _wpi_cvth_pt( &pt, height );
    _wpi_movetoex( hdc, &pt, NULL );

    pt.y = 6;
    _wpi_cvth_pt( &pt, height );
    _wpi_lineto( hdc, &pt );
    pt.x = 90;
    _wpi_lineto( hdc, &pt );

    _wpi_getoldpen( hdc, holdpen );
    _wpi_deletepen( hgraypen );

#if defined( __NT__ )
    hwhitepen = _wpi_createpen( PS_SOLID, 0, GetSysColor( COLOR_BTNHIGHLIGHT ) );
#else
    hwhitepen = _wpi_createpen( PS_SOLID, 0, WHITE );
#endif
    holdpen = _wpi_selectpen( hdc, hwhitepen );
    pt.y = 50;
    _wpi_cvth_pt( &pt, height );
    _wpi_lineto( hdc, &pt );
    pt.x = 2;
    _wpi_lineto( hdc, &pt );

    _wpi_getoldpen( hdc, holdpen );
    _wpi_deletepen( hwhitepen );
    _wpi_endpaint( hwnd, hdc, &rect );

} /* paintPalette */

/*
 * ColorPalWinProc - handle messages for the color palette
 */
WPI_MRESULT CALLBACK ColorPalWinProc( HWND hwnd, WPI_MSG msg, WPI_PARAM1 mp1, WPI_PARAM2 mp2 )
{
    HMENU               hsysmenu;
    WPI_RECT            rcpal;
    WPI_RECTDIM         left;
    WPI_RECTDIM         right;
    WPI_RECTDIM         top;
    WPI_RECTDIM         bottom;
    static HMENU        hmenu;
    static HWND         hframe;

    switch( msg ) {
    case WM_CREATE:
        hframe = _wpi_getframe( hwnd );
        hsysmenu = _wpi_getcurrentsysmenu( hframe );
        _wpi_deletemenu( hsysmenu, SC_RESTORE, FALSE );
        _wpi_deletemenu( hsysmenu, SC_SIZE, FALSE );
        _wpi_deletemenu( hsysmenu, SC_MINIMIZE, FALSE );
        _wpi_deletemenu( hsysmenu, SC_MAXIMIZE, FALSE );
        _wpi_deletemenu( hsysmenu, SC_TASKLIST, FALSE );
#ifdef __OS2_PM__
        _wpi_deletemenu( hsysmenu, SC_HIDE, FALSE );
#endif
        _wpi_deletesysmenupos( hsysmenu, 1 );
        _wpi_deletesysmenupos( hsysmenu, 2 );
        hbrush = _wpi_createsolidbrush( LTGRAY );
        hmenu = GetMenu( _wpi_getframe( HMainWindow ) );
        break;

    case WM_PAINT:
        _wpi_deletebrush( hbrush );
        SetBkColor( (HDC)mp1, GetSysColor( COLOR_BTNFACE ) );
        SetTextColor( (HDC)mp1, GetSysColor( COLOR_BTNTEXT ) );
        hbrush = _wpi_createsolidbrush( GetSysColor( COLOR_BTNFACE ) );
        paintPalette( hwnd );
        break;

#ifndef __OS2_PM__
#ifdef __NT__
    case WM_SYSCOLORCHANGE:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        _wpi_deletebrush( hbrush );
        hbrush = _wpi_createsolidbrush( GetSysColor( COLOR_BTNFACE ) );
        SetBkColor( (HDC)mp1, GetSysColor( COLOR_BTNFACE ) );
        SetTextColor( (HDC)mp1, GetSysColor( COLOR_BTNTEXT ) );
        return( (WPI_MRESULT)hbrush );
#else
    case WM_CTLCOLOR:
        if( HIWORD( mp2 ) == CTLCOLOR_STATIC || HIWORD( mp2 ) == CTLCOLOR_BTN ) {
            SetBkColor( (HDC)LOWORD( mp1 ), LTGRAY );
            SetTextColor( (HDC)LOWORD( mp1 ), BLACK );
            return( (WPI_MRESULT)hbrush );
        }
        break;
#endif
#endif

    case WM_MOVE:
        _wpi_getwindowrect( _wpi_getframe( hwnd ), &rcpal );
        _wpi_getrectvalues( rcpal, &left, &top, &right, &bottom );
        ImgedConfigInfo.pal_xpos = (short)left;
        ImgedConfigInfo.pal_ypos = (short)top;
        break;

    case WM_CLOSE:
        CheckPaletteItem( hmenu );
        break;

    case WM_DESTROY:
        _wpi_deletebrush( hbrush );
        break;

    default:
        return( DefWindowProc( hwnd, msg, mp1, mp2 ) );
    }
    return( 0 );

} /* ColorPalWinProc */

/*
 * CheckPaletteItem - handle when the color palette menu item has been selected
 */
void CheckPaletteItem( HMENU hmenu )
{
    HWND        frame_wnd;

    if( HColorPalette == NULL ) {
        _wpi_checkmenuitem( hmenu, IMGED_COLOR, MF_CHECKED, FALSE );
        return;
    }
    frame_wnd = _wpi_getframe( HColorPalette );

    if( _wpi_isitemchecked( hmenu, IMGED_COLOR ) ) {
        _wpi_checkmenuitem( hmenu, IMGED_COLOR, MF_UNCHECKED, FALSE );
        ShowWindow( frame_wnd, SW_HIDE );
        ImgedConfigInfo.show_state &= ~SET_SHOW_CLR;
    } else {
        _wpi_checkmenuitem( hmenu, IMGED_COLOR, MF_CHECKED, FALSE );
        ShowWindow( frame_wnd, SW_SHOWNA );
        _wpi_setfocus( HMainWindow );
        ImgedConfigInfo.show_state |= SET_SHOW_CLR;
    }

} /* CheckPaletteItem */

/*
 * CreateColorPal - create the color palette window depending on the OS
 *                  we're compiling for
 */
void CreateColorPal( void )
{
    HMENU       hmenu;
#ifdef __OS2_PM__
    PM_CreateColorPal();
#else
    Win_CreateColorPal();
#endif

    hmenu = GetMenu( _wpi_getframe( HMainWindow ) );
    if( ImgedConfigInfo.show_state & SET_SHOW_CLR ) {
        CheckPaletteItem( hmenu );
    }

    CreateCurrentWnd( HColorPalette );
    CreateColorControls( HColorPalette );

} /* CreateColorPal */

#ifndef __OS2_PM__

/*
 * SetRGBValues - set the RGB values for the initialized images
 */
void SetRGBValues( RGBQUAD *argbvals, int upperlimit )
{
    int                 i;
    RGBQUAD             *argb;
    PALETTEENTRY        *pe;
    int                 num;
    HDC                 hdc;

    hdc = GetDC( HColorPalette );
    pe = MemAlloc( upperlimit * sizeof( PALETTEENTRY ) );
    num = GetSystemPaletteEntries( hdc, 0, upperlimit, pe );
    ReleaseDC( HColorPalette, hdc );

    argb = argbvals;
    if( num > upperlimit )
        num = upperlimit;
    for( i = 0; i < num; i++ ) {
        argb[i].rgbBlue = pe[i].peBlue;
        argb[i].rgbGreen = pe[i].peGreen;
        argb[i].rgbRed = pe[i].peRed;
        argb[i].rgbReserved = 0;
    }
    MemFree( pe );

} /* SetRGBValues */

#endif
