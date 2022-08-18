/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Main profiler window procedure.
*
****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#ifdef __WATCOMC__
    #include <process.h>
#endif

#include "common.h"
#include "aui.h"
#include "wpaui.h"
#include "dip.h"
#include "sampinfo.h"
#include "msg.h"
#include "clibext.h"
#include "dlgsamp.h"
#include "getsamps.h"
#include "wpcnvt.h"
#include "wpdriver.h"
#include "wphelp.h"
#include "wpsamp.h"
#include "aboutmsg.h"
#include "clrsamps.h"
#include "dlgoptn.h"
#include "wpstart.h"
#include "wpdata.h"
#include "memutil.h"


#define OFN_SAMPLE_FILE (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST)

static gui_menu_struct fileMenu[] = {
    { "&Open...",               MENU_OPEN_SAMPLE,   GUI_STYLE_MENU_ENABLED, "Open a sample file." },
    { "&Close",                 MENU_CLOSE_SAMPLE,  GUI_STYLE_MENU_ENABLED, "Close the current sample information." },
    { "",                       0,                  GUI_STYLE_MENU_SEPARATOR },
    { "Op&tions...",            MENU_OPTIONS,       GUI_STYLE_MENU_ENABLED, "Set profiler options." },
#if !defined( __WINDOWS__ ) && !defined( __NT__ ) && !defined( __OS2__ ) && !defined( __UNIX__ )
    { "S&ystem",                MENU_SYSTEM,        GUI_STYLE_MENU_ENABLED, "Start an operating system shell." },
#endif
#ifdef TRMEM
    { "&Memory Usage(Debug)",   MENU_MEMPRT,        GUI_STYLE_MENU_ENABLED, "Print out current memory usage." },
#endif
    { "",                       0,                  GUI_STYLE_MENU_SEPARATOR },
    { "E&xit",                  MENU_EXIT,          GUI_STYLE_MENU_ENABLED, "Exit the Open Watcom Profiler." },
};


static gui_menu_struct convertMenu[] = {
    { "Current &Module...", MENU_CONVERT_MODULE,    GUI_STYLE_MENU_ENABLED, "Convert the current module." },
    { "Current &Image...",  MENU_CONVERT_IMAGE,     GUI_STYLE_MENU_ENABLED, "Convert the current image." },
    { "&All Images...",     MENU_CONVERT_ALL,       GUI_STYLE_MENU_ENABLED, "Convert all of the images." },
};


static gui_menu_struct helpMenu[] = {
    { "&Contents",              MENU_HELP_CONTENTS, GUI_STYLE_MENU_ENABLED, "List Open Watcom Profile help topics." },
#if defined( __WINDOWS__ ) || defined( __NT__ ) || defined( __OS2_PM__ )
    { "&Search for Help On...", MENU_HELP_SEARCH,   GUI_STYLE_MENU_ENABLED, "Search for help on a specific topic." },
    { "",                       0,                  GUI_STYLE_MENU_SEPARATOR },
    { "&How to Use Help",       MENU_HELP_ONHELP,   GUI_STYLE_MENU_ENABLED, "Display information on how to use help." },
#endif
    { "",                       0,                  GUI_STYLE_MENU_SEPARATOR },
    { "&About...",              MENU_ABOUT,         GUI_STYLE_MENU_ENABLED, "Display program information." },
};


static gui_menu_struct WPMainMenu[] = {
    { "&File",      MENU_FILE,      GUI_STYLE_MENU_ENABLED,                             "",                                         WndMenuFields( fileMenu ) },
    { "&Convert",   MENU_CONVERT,   GUI_STYLE_MENU_ENABLED,                             "Convert the data to an output format.",    WndMenuFields( convertMenu ) },
    { "&Windows",   MENU_WINDOWS,   GUI_STYLE_MENU_ENABLED + GUI_STYLE_MENU_MDIWINDOW,  "Select an active window." },
    { "&Actions",   MENU_ACTIONS,   GUI_STYLE_MENU_ENABLED + WND_MENU_POPUP,            "Select an action for the active window." },
    { "&Help",      MENU_HELP,      GUI_STYLE_MENU_ENABLED,                             "Display Open Watcom Profiler help.",       WndMenuFields( helpMenu ) },
};

gui_menu_items  WndMainMenu = { ArraySize( WPMainMenu ), WPMainMenu };

bool WndMainMenuProc( a_window wnd, gui_ctl_id id )
/*************************************************/
{
    a_window    active;
    sio_data    *curr_sio;
#if !defined( __WINDOWS__ ) && !defined( __NT__ ) && !defined( __OS2__ ) && !defined( __UNIX__ )
    char        *sys_spec;
#endif

    /* unused parameters */ (void)wnd;

    active = WndFindActive();
    switch( id ) {
    case MENU_OPEN_SAMPLE:
        DlgOpenSample();
        break;
    case MENU_CLOSE_SAMPLE:
        curr_sio = WndExtra( active );
        if( curr_sio != NULL ) {
            ClearSample( curr_sio );
            WndClose( active );
        }
        break;
#if !defined( __WINDOWS__ ) && !defined( __NT__ ) && !defined( __OS2__ ) && !defined( __UNIX__ )
    case MENU_SYSTEM:
        GUISpawnStart();
        sys_spec = getenv( "COMSPEC" );
        if( sys_spec == NULL || spawnl( P_WAIT, sys_spec, sys_spec, NULL ) == -1 ) {
            ErrorMsg( LIT( Bad_System_Load ) );
        }
        GUISpawnEnd();
        break;
#endif
#ifdef TRMEM
    case MENU_MEMPRT:
        GUISpawnStart();
        WPMemPrtUsage();
        getchar();
        GUISpawnEnd();
        break;
#endif
    case MENU_OPTIONS:
        DlgGetOptions( active );
        break;
    case MENU_HELP_CONTENTS:
        WPProcHelp( GUI_HELP_CONTENTS );
        break;
#if defined( __WINDOWS__ ) || defined( __NT__ ) || defined( __OS2_PM__ )
    case MENU_HELP_ONHELP:
        WPProcHelp( GUI_HELP_ON_HELP );
        break;
    case MENU_HELP_SEARCH:
        WPProcHelp( GUI_HELP_SEARCH );
        break;
#endif
    case MENU_ABOUT:
        DlgAbout();
        break;
    case MENU_EXIT:
        WPFini();
        break;
    case MENU_CONVERT_MODULE:
    case MENU_CONVERT_IMAGE:
    case MENU_CONVERT_ALL:
        WPConvert( active, id );
        break;
    default:
        return( false );
    }
    return( true );
}



void OpenSample( void )
/*********************/
{
    gui_mcursor_handle  old_cursor;

#if defined( __WINDOWS__ ) || defined( __NT__ ) || defined( __OS2_PM__ )
    AboutClose();
#else
    AboutSetOff();
#endif
    old_cursor = WndHourGlass( NULL );
    if( GetSampleInfo() ) {
        WPSampleOpen();
    }
    WndHourGlass( old_cursor );
}
