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


#include "guiwind.h"
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include "guixwind.h"
#ifndef __OS2_PM__
    #include "wwinhelp.h"
#endif


#ifdef __OS2_PM__

static gui_help_instance InitHelp( HWND hwnd, WPI_INST inst, const char *title, const char *help_lib )
{
    HWND        hwndHelpInstance;
    HELPINIT    help;

    help.cb = sizeof( HELPINIT );
    help.pszTutorialName = NULL;
    help.phtHelpTable = NULL;
    help.hmodHelpTableModule = 0;
    help.hmodAccelActionBarModule = 0;
    help.idAccelTable = 0;
    help.idActionBar = 0;
    help.pszHelpWindowTitle = (PSZ)title;
  #ifdef _M_I86
    help.usShowPanelId = CMIC_HIDE_PANEL_ID;
  #else
    help.fShowPanelId = CMIC_HIDE_PANEL_ID;
  #endif
    help.pszHelpLibraryName = (PSZ)help_lib;
    hwndHelpInstance = WinCreateHelpInstance( inst.hab, &help );
    if( hwndHelpInstance != NULLHANDLE ) {
        if( !WinAssociateHelpInstance( hwndHelpInstance, hwnd ) ) {
            WinDestroyHelpInstance( hwndHelpInstance );
            hwndHelpInstance = NULLHANDLE;
        }
    }

    return( (gui_help_instance)hwndHelpInstance );
}

static void FiniHelp( gui_help_instance inst, HWND hwnd, const char *file )
{
    hwnd=hwnd;
    file=file;
    if( (HWND)inst != NULLHANDLE ) {
        WinAssociateHelpInstance( (HWND)inst, NULLHANDLE );
        WinDestroyHelpInstance( (HWND)inst );
    }
}

static bool DisplayContents( gui_help_instance inst, HWND hwnd, const char *file )
{
    hwnd=hwnd;
    file=file;
    return( !WinSendMsg( (HWND)inst, HM_HELP_CONTENTS, NULL, NULL ) );
}

static bool DisplayHelpOnHelp( gui_help_instance inst, HWND hwnd, const char *file )
{
    hwnd=hwnd;
    file=file;
    return( !WinSendMsg( (HWND)inst, HM_DISPLAY_HELP, NULL, NULL ) );
}

static bool DisplayHelpSearch( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    hwnd=hwnd;
    file=file;
    topic=topic;
    return( !WinSendMsg( (HWND)inst, HM_HELP_INDEX, NULL, NULL ) );
}

static bool DisplayHelpKey( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    hwnd=hwnd;
    file=file;
    return( !WinSendMsg( (HWND)inst, HM_DISPLAY_HELP, MPFROMLONG( (LONG)&topic ), MPFROMSHORT( HM_PANELNAME ) ) );
}

static bool DisplayHelpContext( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    hwnd=hwnd;
    file=file;
    return( !WinSendMsg( (HWND)inst, HM_DISPLAY_HELP, MPFROM2SHORT( (SHORT)topic, 0 ), MPFROMSHORT( HM_RESOURCEID ) ) );
}

#else

static gui_help_instance InitHelp( HWND hwnd, WPI_INST inst, const char *title, const char *help_file )
{
    hwnd = hwnd;
    inst = inst;
    title = title;
    help_file = help_file;
    return( (gui_help_instance)true );
}

static void FiniHelp( gui_help_instance inst, HWND hwnd, const char *file )
{
    inst=inst;
    WWinHelp( hwnd, file, (UINT)HELP_QUIT, 0 );
}

static bool DisplayContents( gui_help_instance inst, HWND hwnd, const char *file )
{
    inst=inst;
    return( WWinHelp( hwnd, file, (UINT)HELP_CONTENTS, 0 ) );
}

static bool DisplayHelpOnHelp( gui_help_instance inst, HWND hwnd, const char *file )
{
    inst=inst;
    return( WWinHelp( hwnd, file, (UINT)HELP_HELPONHELP, 0 ) );
}

static bool DisplayHelpSearch( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    inst=inst;
    if( topic == NULL ) {
        topic = "";
    }
    return( WWinHelp( hwnd, file, (UINT)HELP_PARTIALKEY, (HELP_DATA)(LPCSTR)topic ) );
}

static bool DisplayHelpContext( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    inst=inst;
    return( WWinHelp( hwnd, file, (UINT)HELP_CONTEXT, (HELP_DATA)(LPCSTR)topic ) );
}

static bool DisplayHelpKey( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    inst=inst;
    return( WWinHelp( hwnd, file, (UINT)HELP_KEY, (HELP_DATA)(LPCSTR)topic ) );
}

  #if defined( __NT__ )

static bool DisplayContentsHH( gui_help_instance inst, HWND hwnd, const char *file )
{
    inst = inst;
    return( WHtmlHelp( hwnd, file, (UINT)HELP_CONTENTS, 0 ) );
}

static bool DisplayHelpSearchHH( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    inst = inst;
    if( topic == NULL ) {
        topic = "";
    }
    return( WHtmlHelp( hwnd, file, (UINT)HELP_PARTIALKEY, (HELP_DATA)(LPCSTR)topic ) );
}

static bool DisplayHelpContextHH( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    inst=inst;
    return( WHtmlHelp( hwnd, file, (UINT)HELP_CONTEXT, (HELP_DATA)(LPCSTR)topic ) );
}

static bool DisplayHelpKeyHH( gui_help_instance inst, HWND hwnd, const char *file, const char *topic )
{
    inst = inst;
    return( WHtmlHelp( hwnd, file, (UINT)HELP_KEY, (HELP_DATA)(LPCSTR)topic ) );
}

  #endif

#endif


gui_help_instance GUIAPI GUIHelpInit( gui_window *wnd, const char *file, const char *title )
{
    return( InitHelp( wnd->hwnd, GUIMainHInst, title, file ) );
}

void GUIAPI GUIHelpFini( gui_help_instance inst, gui_window *wnd, const char *file )
{
    FiniHelp( inst, wnd->hwnd, file );
}

bool GUIAPI GUIShowHelp( gui_help_instance inst, gui_window *wnd, gui_help_actions act,
                  const char *file, const char *topic )
{
    bool        ret;

    ret = false;

    switch( act ) {
    case GUI_HELP_CONTENTS:
        ret = DisplayContents( inst, wnd->hwnd, file );
        break;
    case GUI_HELP_ON_HELP:
        ret = DisplayHelpOnHelp( inst, wnd->hwnd, file );
        break;
    case GUI_HELP_SEARCH:
        ret = DisplayHelpSearch( inst, wnd->hwnd, file, topic );
        break;
    case GUI_HELP_CONTEXT:
        ret = DisplayHelpContext( inst, wnd->hwnd, file, topic );
        break;
    case GUI_HELP_KEY:
        ret = DisplayHelpKey( inst, wnd->hwnd, file, topic );
        break;
    }

    return( ret );
}

bool GUIAPI GUIShowHtmlHelp( gui_help_instance inst, gui_window *wnd, gui_help_actions act, const char *file, const char *topic )
{
    bool        ret;

#ifndef __NT__
    /* unused parameters */ (void)inst; (void)wnd; (void)act; (void)file; (void)topic;
#endif

    ret = false;

#ifdef __OS2_PM__
#elif defined( __NT__ )
    switch( act ) {
    case GUI_HELP_CONTENTS:
        ret = DisplayContentsHH( inst, wnd->hwnd, file );
        break;
    case GUI_HELP_SEARCH:
        ret = DisplayHelpSearchHH( inst, wnd->hwnd, file, topic );
        break;
    case GUI_HELP_CONTEXT:
        ret = DisplayHelpContextHH( inst, wnd->hwnd, file, topic );
        break;
    case GUI_HELP_KEY:
        ret = DisplayHelpKeyHH( inst, wnd->hwnd, file, topic );
        break;
    }
#endif
    return( ret );
}

bool GUIAPI GUIDisplayHelp( gui_window *wnd, const char *file, const char *topic )
{
#ifdef __OS2_PM__
    wnd = wnd;
    file = file;
    topic = topic;
    return( false );
#else
    if( topic == NULL ) {
        return( WWinHelp( wnd->hwnd, file, (UINT)HELP_INDEX, 0 ) );
    } else {
        return( WWinHelp( wnd->hwnd, file, (UINT)HELP_KEY, (HELP_DATA)(LPCSTR)topic ) );
    }
#endif
}

bool GUIAPI GUIDisplayHelpWin4( gui_window *wnd, const char *file, const char *topic )
{
#ifdef __OS2_PM__
    wnd = wnd;
    file = file;
    topic = topic;
    return( false );
#else
    if( topic == NULL ) {
  #if defined( __NT__ )
    #if !defined( _WIN64 )
        DWORD   version;

        version = GetVersion();
        version = 100 * LOBYTE(LOWORD(version)) + HIBYTE(LOWORD(version));
        if( version < 351 ) {
            return( WWinHelp( wnd->hwnd, file, (UINT)HELP_INDEX, 0 ) );
        }
    #endif
        // NT 3.51 or higher
        return( WWinHelp( wnd->hwnd, file, (UINT)HELP_FINDER, 0 ) );
  #else
        return( WWinHelp( wnd->hwnd, file, (UINT)HELP_INDEX, 0 ) );
  #endif
    } else {
        return( WWinHelp( wnd->hwnd, file, (UINT)HELP_KEY, (HELP_DATA)(LPCSTR)topic ) );
    }
#endif
}

bool GUIAPI GUIDisplayHelpId( gui_window *wnd, const char *file, gui_hlp_id id )
{
#ifdef __OS2_PM__
    wnd = wnd;
    file = file;
    id = id;
    return( false );
#else
    return( WWinHelp( wnd->hwnd, file, (UINT)HELP_CONTEXT, (HELP_DATA)id ) );
#endif
}
