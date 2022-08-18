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
* Description:  GUI lib color handling.
*
****************************************************************************/


#include "guiwind.h"
#include <string.h>
#if defined(__WINDOWS__) && !defined(__WINDOWS_386__)
#include <commdlg.h>
#endif
#include "guicolor.h"
#include "guiwnclr.h"
#include "guixwind.h"


WPI_COLOUR GUIColours[] = {
#ifdef __OS2_PM__
//      R G B
    0x00000000, /* GUI_BLACK           */
    0x00000080, /* GUI_BLUE            */
    0x00008000, /* GUI_GREEN           */
    0x00008080, /* GUI_CYAN            */
    0x00C00000, /* GUI_RED             */
    0x00800080, /* GUI_MAGENTA         */
    0x00808000, /* GUI_BROWN           */
    0x00cccccc, /* GUI_WHITE           */
    0x00808080, /* GUI_GREY            */
    0x000000ff, /* GUI_BR_BLUE         */
    0x0000ff00, /* GUI_BR_GREEN        */
    0x0000ffff, /* GUI_BR_CYAN         */
    0x00ff0000, /* GUI_BR_RED          */
    0x00ff00ff, /* GUI_BR_MAGENTA      */
    0x00ffff00, /* GUI_BR_YELLOW       */
    0x00ffffff, /* GUI_BR_WHITE        */
    0x00808080, /* GUIEX_DLG_BKGRND    */
    0x00FFFFFF, /* GUIEX_WND_BKGRND    */
    0x00000080, /* GUIEX_HIGHLIGHT     */
    0x00FFFFFF  /* GUIEX_HIGHLIGHTTEXT */
#else
//      B G R
    0x00000000, /* GUI_BLACK           */
    0x00800000, /* GUI_BLUE            */
    0x00008000, /* GUI_GREEN           */
    0x00808000, /* GUI_CYAN            */
    0x000000C0, /* GUI_RED             */
    0x00800080, /* GUI_MAGENTA         */
    0x00008080, /* GUI_BROWN           */
    0x00c0c0c0, /* GUI_WHITE           */
    0x00808080, /* GUI_GREY            */
    0x00ff0000, /* GUI_BR_BLUE         */
    0x0000ff00, /* GUI_BR_GREEN        */
    0x00ffff00, /* GUI_BR_CYAN         */
    0x000000ff, /* GUI_BR_RED          */
    0x00ff00ff, /* GUI_BR_MAGENTA      */
    0x0000ffff, /* GUI_BR_YELLOW       */
    0x00ffffff, /* GUI_BR_WHITE        */
    0x00808080, /* GUIEX_DLG_BKGRND    */
    0x00FFFFFF, /* GUIEX_WND_BKGRND    */
    0x00800000, /* GUIEX_HIGHLIGHT     */
    0x00FFFFFF  /* GUIEX_HIGHLIGHTTEXT */
#endif
};

#define NUM_COLOURS ( sizeof( GUIColours ) / sizeof( WPI_COLOUR ) )

static void InitSystemRGB( void )
{
#ifndef __OS2_PM__
    // All other colours are hardcoded.
    GUIColours[GUIEX_DLG_BKGRND] = GetSysColor( COLOR_BTNFACE );
    GUIColours[GUIEX_WND_BKGRND] = GetSysColor( COLOR_WINDOW );
    GUIColours[GUIEX_HIGHLIGHT] = GetSysColor( COLOR_HIGHLIGHT );
    GUIColours[GUIEX_HIGHLIGHTTEXT] = GetSysColor( COLOR_HIGHLIGHTTEXT );
#endif
}


bool GUIAPI GUISetRGB( gui_colour colour, gui_rgb rgb )
{
    if( colour < GUI_NUM_COLOURS  ) {
        GUIColours[colour] = GETRGB( rgb );
        return( true );
    }
    return( false );
}

static void FillInRGB( WPI_COLOUR colour, gui_rgb *rgb )
{
    BYTE        r;
    BYTE        g;
    BYTE        b;

    r = GetRValue( colour );
    g = GetGValue( colour );
    b = GetBValue( colour );
    *rgb = GUIRGB( r, g, b );
}

bool GUIAPI GUIGetRGB( gui_colour colour, gui_rgb *rgb )
{
    if( ( colour < GUI_NUM_COLOURS ) && ( rgb != NULL ) ) {
        FillInRGB( GUIColours[colour], rgb );
        return( true );
    }
    return( false );
}

bool GUIAPI GUIGetWndColour( gui_window *wnd, gui_attr attr, gui_colour_set *colour_set )
{
    if( colour_set == NULL ) {
        return( false );
    }
    if( attr < wnd->num_attrs ) {
        colour_set->fore = WNDATTRFG( wnd, attr );
        colour_set->back = WNDATTRBG( wnd, attr );
        return( true );
    }
    return( false );
}

static void SetBKBrush( gui_window *wnd )
{
    static bool sys_rgb_initialized = false;

    if( !sys_rgb_initialized ) {
        InitSystemRGB();
        sys_rgb_initialized = true;
    }

    GUIGetRGB( WNDATTRBG( wnd, GUI_BACKGROUND ), &wnd->bk_rgb );
    wnd->bk_brush = _wpi_createsolidbrush(GUIGetBack( wnd, GUI_BACKGROUND ));
}

static void ChangeBKBrush( gui_window *wnd )
{
    HBRUSH      prev;
    HBRUSH      curr;

    curr = wnd->bk_brush;
    prev = GUIFreeBKBrush( wnd );
    SetBKBrush( wnd );
#ifndef __OS2_PM__
    if( prev != curr ) {
        /* replce brush if it wasn't really ours */
        SET_HBRBACKGROUND( wnd->hwnd, prev );
    } else {
        SET_HBRBACKGROUND( wnd->hwnd, wnd->bk_brush );
    }
#endif
}

#if 0
void GUICheckBKBrush( gui_window *wnd )
{
    gui_rgb             rgb;
    gui_colour_set      set;

    GUIGetWndColour( wnd, GUI_BACKGROUND, &set );
    GUIGetRGB( set.back, &rgb );
    if( rgb != wnd->bk_rgb ) {
        ChangeBKBrush( wnd );
    }
}
#endif

bool GUIAPI GUISetWndColour( gui_window *wnd, gui_attr attr, gui_colour_set *colour_set )
{
    if( colour_set == NULL ) {
        return( false );
    }
    if( attr < wnd->num_attrs ) {
        WNDATTRFG( wnd, attr ) = colour_set->fore;
        WNDATTRBG( wnd, attr ) = colour_set->back;
        if( attr == GUI_BACKGROUND ) {
            ChangeBKBrush( wnd );
        }
        return( true );
    }
    return( false );
}

bool GUIAPI GUIGetRGBFromUser( gui_rgb init_rgb, gui_rgb *new_rgb )
{
#ifdef __OS2_PM__
    /* unused parameters */ (void)init_rgb; (void)new_rgb;

    return( false );
#else
    CHOOSECOLOR     choose;
    bool            ret;
 #if defined(__WINDOWS__)
    HINSTANCE       h;
    FARPROC         func;
  #ifdef __WINDOWS_386__
    HINDIR          hIndir;
    DWORD           guiColoursAlias;
  #endif
 #endif

    if( new_rgb == NULL ) {
        return( false );
    }
    memset( &choose, 0, sizeof( CHOOSECOLOR ) );
    choose.Flags = CC_PREVENTFULLOPEN;
    choose.hInstance = (HWND)GUIMainHInst;
    choose.Flags |= CC_RGBINIT;
    choose.rgbResult = GETRGB( init_rgb );
 #ifndef __WINDOWS_386__
    choose.lpCustColors = GUIColours;
 #endif
    choose.lStructSize = sizeof( CHOOSECOLOR );

 #if defined(__NT__)
    ret = ( ChooseColor( &choose ) != 0 );
 #else
    h = LoadLibrary( "COMMDLG.DLL" );
    if( (UINT)h < 32 ) {
        return( false );
    }
    func = GetProcAddress( h, "ChooseColor" );
    if( func == NULL ) {
        return( false );
    }
  #ifdef __WINDOWS_386__
    hIndir = GetIndirectFunctionHandle( func, INDIR_PTR, INDIR_ENDLIST );
    if( hIndir == NULL ) {
        FreeLibrary( h );
        return( false );
    }
    guiColoursAlias = AllocAlias16( (void *)GUIColours );
    choose.lpCustColors = (void *)guiColoursAlias;
    ret = (short)InvokeIndirectFunction( hIndir, &choose ) != 0;
    if( guiColoursAlias != 0 ) {
        FreeAlias16( guiColoursAlias );
    }
    FreeIndirectFunctionHandle( hIndir );
  #else
    ret = ((BOOL(WINAPI *)(LPCHOOSECOLOR))func)( &choose ) != 0;
  #endif
    FreeLibrary( h );
 #endif
    if( ret ) {
        FillInRGB( choose.rgbResult, new_rgb );
    }
    return( ret );
#endif
}

/*
 * GUIXSetColours -- record the colours selected by the application
 */

bool GUIXSetColours( gui_window *wnd, int num_attrs, gui_colour_set *colours )
{
    size_t          size;
    gui_colour_set  *attrs;

    if( colours != NULL ) {
        size = sizeof( gui_colour_set ) * num_attrs;
        attrs = (gui_colour_set *)GUIMemAlloc( size );
        if( attrs != NULL ) {
            wnd->attrs = attrs;
            wnd->num_attrs = num_attrs;
            memcpy( attrs, colours, size );
            SetBKBrush( wnd );
            return( true );
        }
    }
    return( false );
}

void GUIXGetWindowColours( gui_window *wnd, gui_colour_set *colours )
{
    memcpy( colours, wnd->attrs, sizeof( gui_colour_set ) * wnd->num_attrs );
}

HBRUSH GUIFreeBKBrush( gui_window * wnd )
{
    HBRUSH brush = WPI_NULL;

    if( wnd->bk_brush != WPI_NULL ) {
        /* make sure bk_brush is not the currently the background brush
         * and, therefore, a system resource */
#ifndef __OS2_PM__
        if( wnd->hwnd != NULLHANDLE ) {  /* NULL if create failed */
            brush = SET_HBRBACKGROUND( wnd->hwnd, NULL );
        }
#endif
        _wpi_deletebrush( wnd->bk_brush );
        wnd->bk_brush = WPI_NULL;
    }
    return( brush );
}

void GUIAPI GUISetWindowColours( gui_window *wnd, int num_colours,
                          gui_colour_set *colours )
{
    GUIFreeColours( wnd );
    GUIFreeBKBrush( wnd );
    GUISetColours( wnd, num_colours, colours );
    GUIWndDirty( wnd );
}

WPI_COLOUR GUIGetFore( gui_window *wnd, gui_attr attr )
{
    return( GUIColours[WNDATTRFG( wnd, attr )] );
}

WPI_COLOUR GUIGetBack( gui_window *wnd, gui_attr attr )
{
    return( GUIColours[WNDATTRBG( wnd, attr )] );
}
