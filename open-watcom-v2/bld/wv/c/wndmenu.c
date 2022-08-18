/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Top level debugger menu.
*
****************************************************************************/


#include <ctype.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgerr.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgmad.h"
#include "dbgexec.h"
#include "dbgmain.h"
#include "dbgbrk.h"
#if !defined( GUI_IS_GUI ) && !defined( __WINDOWS__ ) && !defined( __NT__ )
#include "dbgsys.h"
#endif
#include "wndsys.h"
#include "dbgprog.h"
#include "dbgtrace.h"
#include "dbgmisc.h"
#include "remrtrd.h"
#include "dbgreg.h"
#include "dlgexpr.h"
#include "dbgdot.h"
#include "dbgwdisp.h"
#include "dbgwglob.h"
#include "dbgwinsp.h"
#include "dbgwmod.h"
#include "dbgwvar.h"
#include "dlgbreak.h"
#include "dlgdll.h"
#include "dlgfile.h"
#include "dlgsrc.h"
#include "wnddump.h"
#include "wndhelp.h"
#include "wndmenu.h"
#include "dlgabout.h"
#include "dbgscrn.h"
#include "dbgwfunc.h"
#include "dlgoptn.h"
#include "dlgnewp.h"
#include "dlgcmd.h"
#include "dlgwind.h"
#include "menudef.h"


static gui_menu_struct FileMenu[] = {
    #include "mmfile.h"
};

static gui_menu_struct SearchMenu[] = {
    #include "mmsearch.h"
};

static gui_menu_struct RunMenu[] = {
    #include "mmrun.h"
};

static gui_menu_struct BreakMenu[] = {
    #include "mmbreak.h"
};

static gui_menu_struct CodeMenu[] = {
    #include "mmcode.h"
};

static gui_menu_struct DataMenu[] = {
    #include "mmdat.h"
};

static gui_menu_struct UndoMenu[] = {
    #include "mmundo.h"
};

static gui_menu_struct WindowMenu[] = {
    #include "mmwind.h"
};

static gui_menu_struct HelpMenu[] = {
    #include "mmhelp.h"
};

static gui_menu_struct DbgMainMenu[] = {
    MENU_CASCADE( MENU_MAIN_FILE, MainMenuFile, FileMenu )
    MENU_CASCADE( MENU_MAIN_RUN, MainMenuRun, RunMenu )
    MENU_CASCADE( MENU_MAIN_BREAK, MainMenuBreak, BreakMenu )
    MENU_CASCADE( MENU_MAIN_CODE, MainMenuCode, CodeMenu )
    MENU_CASCADE( MENU_MAIN_DATA, MainMenuData, DataMenu )
    MENU_CASCADE( MENU_MAIN_UNDO, MainMenuUndo, UndoMenu )
    MENU_CASCADE( MENU_MAIN_SEARCH, MainMenuSearch, SearchMenu )
    MENU_CASCADE( MENU_MAIN_WINDOW, MainMenuWindow, WindowMenu )
    MENU_CASCADE_DUMMY( MENU_MAIN_ACTION, MainMenuAction )
    MENU_CASCADE( MENU_MAIN_HELP, MainMenuHelp, HelpMenu )
};

gui_menu_items  WndMainMenu = GUI_MENU_ARRAY( DbgMainMenu );
gui_menu_items  WndMainMenuMacro = { ArraySize( DbgMainMenu ) - 2, DbgMainMenu };

wnd_info *WndInfoTab[] = {
    #define pick( a,b,c,d,e,f ) &d,
    #include "wndnames.h"
    #undef pick
};

void PlayDead( bool dead )
{
    WndIgnoreAllEvents = dead;
#if defined( GUI_IS_GUI ) && (defined( __WINDOWS__ ) || defined( __NT__ ))
    if( dead ) {
        UnknownScreen(); // the trap file might steal focus!
    } else {
        DebugScreen();
    }
#endif
}


static bool StrAmpEqual( const char *str, const char *menu, int len )
{
    const char  *p;
    char        menu_accel;

    while( *str == ' ' ) {
        ++str;
        --len;
    }
    // first look for a matching accelerator character
    menu_accel = 0;
    for( p = str; *p; ++p ) {
        if( *p == '&' ) {
            menu_accel = tolower( p[1] );
            break;
        }
    }
    if( menu_accel ) {
        for( p = menu; *p; ++p ) {
            if( *p == '&' ) {
                if( tolower( p[1] ) == menu_accel )
                    return( true );
                break;
            }
        }
    }
    while( len > 0 && str[len-1] == ' ' ) {
        --len;
    }
    while( --len >= 0 ) {
        if( *menu == '&' )
            ++menu;
        if( tolower( *menu ) != tolower( *str ) )
            return( false );
        if( *menu == NULLCHAR )
            return( false );
        ++menu;
        ++str;
    }
    return( true );
}

static gui_menu_struct *FindMainMenu( void )
{
    const char          *start;
    size_t              len;
    int                 i;

    if( ScanItem( true, &start, &len ) ) {
        for( i = 0; i < WndMainMenu.num_items; i++ ) {
            if( StrAmpEqual( start, WndMainMenu.menu[i].label, len ) ) {
                return( &WndMainMenu.menu[i] );
            }
        }
    }
    return( NULL );
}


char *GetMenuLabel( const gui_menu_items *menus, gui_ctl_id id, char *buff, bool strip_amp )
{
    char        *p;
    const char  *cp;
    int         i;

    for( i = 0; i < menus->num_items; i++ ) {
        if( menus->menu[i].id == id ) {
            for( cp = menus->menu[i].label; *cp != NULLCHAR; ++cp ) {
                if( *cp == '&' && strip_amp )
                    continue;
                if( *cp == '\t' )
                    break;
                *buff++ = *cp;
            }
            *buff = NULLCHAR;
            return( buff );
        }
        p = GetMenuLabel( &menus->menu[i].child, id, buff, strip_amp );
        if( p != NULL ) {
            return( p );
        }
    }
    return( NULL );
}

static gui_menu_struct *FindSubMenu( const char *start, unsigned len, const gui_menu_items *menus )
{
    gui_menu_struct     *sub;
    int                 i;

    for( i = 0; i < menus->num_items; i++ ) {
        if( StrAmpEqual( start, menus->menu[i].label, len ) ) {
            return( &menus->menu[i] );
        }
        sub = FindSubMenu( start, len, &menus->menu[i].child );
        if( sub != NULL ) {
            return( sub );
        }
    }
    return( NULL );
}

#ifdef DEADCODE
int FindMenuLen( gui_menu_struct *child )
{
    char        *p;

    p = child->label + strlen( child->label ) - 1;
    while( *p == ' ' ) --p;
    if( *p == ')' ) {
        while( *p != '(' )
            --p;
        --p;
        while( *p == ' ' ) {
            --p;
        }
    }
    return( p - child->label + 1 );
}
#endif


void AccelMenuItem( gui_menu_struct *menu, bool is_main )
{
    a_window    wnd = WndFindActive();

    if( is_main ) {
        WndMainMenuProc( wnd, menu->id );
    } else {
        WndKeyPopUp( wnd, menu );
        WndNoSelect( wnd );
    }
}

static bool DoProcAccel( bool add_to_menu, gui_menu_struct **menu, gui_menu_items *parent, wnd_class_wv wndclass )
{
    gui_menu_struct     *main_menu;
    gui_menu_struct     *child;
    wnd_info            *info;
    a_window            wnd;
    const char          *start;
    size_t              len;

    *menu = NULL;
    *parent = NoMenu;
    child = NULL;
    if( ScanCmdMain() ) {
        main_menu = FindMainMenu();
        if( main_menu == NULL ) {
            if( add_to_menu )
                return( true );
            Error( ERR_NONE, LIT_DUI( ERR_WANT_MENU_ITEM ) );
        }
        if( ScanItem( true, &start, &len ) ) {
            child = FindSubMenu( start, len, &main_menu->child );
        }
        if( child == NULL ) {
            if( add_to_menu )
                return( true );
            Error( ERR_NONE, LIT_DUI( ERR_WANT_MENU_ITEM ) );
        }
        *menu = child;
        *parent = main_menu->child;
        if( add_to_menu )
            return( true );
        ReqEOC();
        AccelMenuItem( child, true );
    } else {
        info = WndInfoTab[wndclass];
        if( ScanItem( true, &start, &len ) ) {
            child = FindSubMenu( start, len, &info->popup );
        }
        if( child == NULL ) {
            if( add_to_menu )
                return( false );
            Error( ERR_NONE, LIT_DUI( ERR_WANT_MENU_ITEM ) );
        }
        *menu = child;
        *parent = info->popup;
        if( add_to_menu )
            return( false );
        wnd = WndFindActive();
        if( WndClass( wnd ) != wndclass )
            Error( ERR_NONE, LIT_DUI( ERR_MACRO_NOT_VALID ) );
        ReqEOC();
        AccelMenuItem( child, false );
    }
    return( false );
}

void ProcAccel( void )
{
    gui_menu_struct     *menu;
    gui_menu_items      parent;

    DoProcAccel( false, &menu, &parent, WndClass( WndFindActive() ));
}

static void FreeLabels( gui_menu_items *menus )
{
    int     i;

    for( i = 0; i < menus->num_items; i++ ) {
        FreeLabels( &menus->menu[i].child );
        if( menus->menu[i].style & WND_MENU_ALLOCATED ) {
            menus->menu[i].style &= ~WND_MENU_ALLOCATED;
            WndFree( (void *)menus->menu[i].label );
            WndFree( (void *)menus->menu[i].hinttext );
        }
    }
}


static void LoadLabels( gui_menu_items *menus )
{
    int     i;

    for( i = 0; i < menus->num_items; i++ ) {
        LoadLabels( &menus->menu[i].child );
        if( (menus->menu[i].style & (GUI_STYLE_MENU_SEPARATOR | WND_MENU_ALLOCATED)) == 0 ) {
            menus->menu[i].label = WndLoadString( (gui_res_id)(pointer_uint)menus->menu[i].label );
            menus->menu[i].hinttext = WndLoadString( (gui_res_id)(pointer_uint)menus->menu[i].hinttext );
            menus->menu[i].style |= WND_MENU_ALLOCATED;
        }
    }
}

void SetBrkMenuItems( void )
{
    bool        on;

    on = ( BrkList != NULL );
    WndEnableMainMenu( MENU_MAIN_BREAK_CLEAR_ALL, on );
    WndEnableMainMenu( MENU_MAIN_BREAK_DISABLE_ALL, on );
    WndEnableMainMenu( MENU_MAIN_BREAK_ENABLE_ALL, on );
    WndEnableMainMenu( MENU_MAIN_BREAK_SAVE_ALL, on );
}

void SetLogMenuItems( bool active )
{
    WndEnableMainMenu( MENU_MAIN_WINDOW_LOG, !active );
}

void SetMADMenuItems( void )
{
    const mad_reg_set_data      *rsd;

    RegFindData( MTK_FLOAT, &rsd );
    WndEnableMainMenu( MENU_MAIN_OPEN_FPU, rsd != NULL );
    RegFindData( MTK_MMX, &rsd );
    WndEnableMainMenu( MENU_MAIN_OPEN_MMX, rsd != NULL );
    RegFindData( MTK_XMM, &rsd );
    WndEnableMainMenu( MENU_MAIN_OPEN_XMM, rsd != NULL );
}

void SetTargMenuItems( void )
{
    WndEnableMainMenu( MENU_MAIN_BREAK_ON_DLL, _IsOn( SW_HAVE_RUNTIME_DLLS ) );
#if defined( GUI_IS_GUI ) && defined( __OS2__ )
    WndEnableMainMenu( MENU_MAIN_FILE_FONT, false );
#endif
    SetMADMenuItems();
}

static void ForAllMenus( void (*rtn)( gui_menu_items *menus ) )
{
    int             i;

    rtn( &WndMainMenu );
    for( i = 0; i < NUM_WNDCLS_ALL; i++ ) {
        rtn( &WndInfoTab[i]->popup );
    }
}

void InitMenus( void )
{
    int         i;

    for( i = 0; i < ArraySize( DbgMainMenu ); ++i ) {
        if( DbgMainMenu[i].id == MENU_MAIN_ACTION ) {
            DbgMainMenu[i].style |= WND_MENU_POPUP;
        }
        if( DbgMainMenu[i].id == MENU_MAIN_WINDOW ) {
            DbgMainMenu[i].style |= GUI_STYLE_MENU_MDIWINDOW;
        }
    }
    ForAllMenus( LoadLabels );
    SetBrkMenuItems();
}

void FiniMenus( void )
{
    ForAllMenus( FreeLabels );
}

void WndMenuSetHotKey( gui_menu_struct *menu, bool is_main, const char *key )
{
    char                *p;
    char                *new;
    const char          *cp;
    size_t              len;

    if( menu == NULL )
        return;
    for( cp = menu->label; *cp != NULLCHAR; ++cp ) {
        if( *cp == '\t' ) {
            break;
        }
    }
    len = cp - menu->label;
    new = WndAlloc( len + 1 + strlen( key ) + 1 );
    memcpy( new, menu->label, len );
    p = new + len;
    *p++ = '\t';
    StrCopy( key, p );
    if( menu->style & WND_MENU_ALLOCATED )
        WndFree( (void *)menu->label );
    menu->style |= WND_MENU_ALLOCATED;
    menu->label = new;
    if( is_main ) {
        WndSetMainMenuText( menu );
    }
}


gui_menu_struct *AddMenuAccel( const char *key, const char *cmd, wnd_class_wv wndclass, bool *is_main )
{
    const char          *old;
    gui_menu_struct     *menu;
    gui_menu_items      parent;

    old = ReScan( cmd );
    menu = NULL;
    if( ScanCmd( GetCmdName( CMD_ACCEL ) ) == 0 ) {
        *is_main = DoProcAccel( true, &menu, &parent, wndclass );
    }
    ReScan( old );
    if( menu == NULL || !ScanEOC() )
        return( NULL );
    WndMenuSetHotKey( menu, *is_main, key );
    return( menu );
}


static void     DoMatch( void )
{
    a_window    wnd;

    wnd = WndFindActive();
    if( wnd == NULL )
        return;
    if( WndKeyPiece( wnd ) == WND_NO_PIECE ) {
        Error( ERR_NONE, LIT_DUI( ERR_MATCH_NOT_SUPPORTED ) );
    } else {
        WndSetSwitches( wnd, WSW_CHOOSING );
    }
}


static  void    ExamMemAt( void )
{
    address     addr;

    addr = NilAddr;
    if( DlgDataAddr( LIT_DUI( Mem_Addr ), &addr ) ) {
        WndAddrInspect( addr );
    }
}


static  void    GoToPromptedAddr( void )
{
    address     addr;

    addr = NilAddr;
    if( DlgCodeAddr( LIT_DUI( GoWhere ), &addr ) ) {
        GoToAddr( addr );
    }
}

bool WndMainMenuProc( a_window wnd, gui_ctl_id id )
{
    bool        save;

    /* unused parameters */ (void)wnd;

    switch( id ) {
#if defined( GUI_IS_GUI )
    case MENU_MAIN_FILE_FONT:
        FontChange();
        break;
#endif
    case MENU_MAIN_FILE_VIEW:
        FileBrowse();
        break;
#if !defined( GUI_IS_GUI ) && !defined( __WINDOWS__ ) && !defined( __NT__ )
    case MENU_MAIN_FILE_SYSTEM:
        DoSystem( NULL, 0, LOC_DEFAULT );
        break;
#endif
    case MENU_MAIN_FILE_OPTIONS:
        DlgOptSet();
        break;
    case MENU_MAIN_FILE_COMMAND:
        DlgCmd();
        break;
    case MENU_MAIN_FILE_EXIT:
        DebugExit();
        break;
    case MENU_MAIN_FILE_ABOUT:
        DlgAbout();
        VarInfoRelease();
        break;
    case MENU_MAIN_SEARCH_ALL:
        ProcSearchAll();
        break;
    case MENU_MAIN_SEARCH_FIND:
        ProcWndSearch(WndFindActive());
        break;
    case MENU_MAIN_SEARCH_NEXT:
        ProcWndFindNext(WndFindActive());
        break;
    case MENU_MAIN_SEARCH_PREV:
        ProcWndFindPrev(WndFindActive());
        break;
    case MENU_MAIN_SEARCH_MATCH:
        DoMatch();
        break;
    case MENU_TOOL_GO:
    case MENU_MAIN_RUN_GO:
        Go( true );
        break;
    case MENU_MAIN_RUN_SKIP_TO_CURSOR:
        SkipToAddr( GetCodeDot() );
        break;
    case MENU_MAIN_RUN_TO_CURSOR:
        GoToAddr( GetCodeDot() );
        break;
    case MENU_MAIN_BREAK_TOGGLE:
        ToggleBreak( GetCodeDot() );
        break;
    case MENU_MAIN_BREAK_AT_CURSOR:
        DlgBreak( GetCodeDot() );
        break;
    case MENU_MAIN_RUN_EXECUTE_TO:
        GoToPromptedAddr();
        break;
    case MENU_TOOL_HOME:
    case MENU_MAIN_HOME:
        GoHome();
        break;
    case MENU_TOOL_REDO:
    case MENU_MAIN_REDOIT:
        PosMachState( 1 );
        break;
    case MENU_TOOL_UNDO:
    case MENU_MAIN_UNDOIT:
        PosMachState( -1 );
        break;
    case MENU_TOOL_UP_STACK:
    case MENU_MAIN_UP_STACK:
        MoveStackPos( -1 );
        break;
    case MENU_TOOL_DOWN_STACK:
    case MENU_MAIN_DOWN_STACK:
        MoveStackPos( 1 );
        break;
    case MENU_TOOL_TRACE_OVER:
    case MENU_MAIN_RUN_TRACE_OVER:
        ExecTrace( TRACE_OVER, DbgLevel );
        break;
    case MENU_TOOL_STEP_INTO:
    case MENU_MAIN_RUN_STEP_INTO:
        ExecTrace( TRACE_INTO, DbgLevel );
        break;
    case MENU_MAIN_RUN_TRACE_NEXT:
        ExecTrace( TRACE_NEXT, DbgLevel );
        break;
    case MENU_TOOL_RETURN_TO_CALLER:
    case MENU_MAIN_RUN_RETURN_TO_CALLER:
        GoToReturn();
        break;
    case MENU_MAIN_FILE_OPEN:
        DlgNewProg();
        break;
    case MENU_MAIN_RUN_RESTART:
        ReStart();
        break;
    case MENU_MAIN_DEBUG_STARTUP:
        {
            bool old;

            old = SetProgStartHook( false );
            ReStart();
            SetProgStartHook( old );
        }
        break;
    case MENU_MAIN_BREAK_CREATE_NEW:
        DlgBreak( NilAddr );
        break;
    case MENU_MAIN_BREAK_VIEW_ALL:
        WndClassInspect( WND_BREAK );
        break;
    case MENU_MAIN_BREAK_ON_DLL:
        DlgBreakDLL();
        break;
    case MENU_MAIN_BREAK_ON_DEBUG_MESSAGE:
        _SwitchToggle( SW_BREAK_ON_DEBUG_MESSAGE );
        WndCheckMainMenu( MENU_MAIN_BREAK_ON_DEBUG_MESSAGE, _IsOn( SW_BREAK_ON_DEBUG_MESSAGE ) );
        break;
    case MENU_MAIN_BREAK_CLEAR_ALL:
        BrkClearAll();
        break;
    case MENU_MAIN_BREAK_DISABLE_ALL:
        BrkDisableAll();
        break;
    case MENU_MAIN_BREAK_ENABLE_ALL:
        BrkEnableAll();
        break;
    case MENU_MAIN_BREAK_SAVE_ALL:
        BreakSave( true );
        break;
    case MENU_MAIN_BREAK_RESTORE_ALL:
        BreakSave( false );
        break;
    case MENU_MAIN_SAVE_REPLAY:
        ReplaySave( true );
        break;
    case MENU_MAIN_RESTORE_REPLAY:
        ReplaySave( false );
        break;
    case MENU_MAIN_FILE_SAVE_CONFIGURATION:
        ConfigSave( true );
        break;
    case MENU_MAIN_FILE_LOAD_CONFIGURATION:
        save = WndStopRefresh( true );
        ConfigSave( false );
        WndStopRefresh( save );
        break;
    case MENU_MAIN_FILE_SOURCE_PATH:
        DlgSource();
        break;
    case MENU_MAIN_WINDOW_ZOOM:
        wnd = WndFindActive();
        if( wnd == NULL )
            break;
        if( WndIsMaximized( wnd ) ) {
            WndRestoreWindow( wnd );
        } else {
            WndMaximizeWindow( wnd );
        }
        break;
    case MENU_MAIN_WINDOW_NEXT:
        WndChooseNew();
        break;
    case MENU_MAIN_WINDOW_PROGRAM:
        Flip( 0 );
        break;
    case MENU_MAIN_WINDOW_DUMP:
        WndDumpPrompt( WndFindActive() );
        break;
    case MENU_MAIN_WINDOW_LOG:
        WndDumpLog( WndFindActive() );
        break;
    case MENU_MAIN_WINDOW_SETTINGS:
        DlgWndSet();
        break;
#if 0 // defined( GUI_IS_GUI ) && (defined( __WINDOWS__ ) || defined( __OS2__ )
    case MENU_MAIN_WINDOW_HARD_MODE:
        ToggleHardMode();
        break;
#endif
    case MENU_MAIN_OPEN_ASSEMBLY:
        WndAsmInspect( NilAddr );
        break;
    case MENU_MAIN_OPEN_CALL:
        WndClassInspect( WND_CALL );
        break;
    case MENU_MAIN_OPEN_LOG:
        WndClassInspect( WND_DIALOGUE );
        break;
    case MENU_MAIN_REPLAY:
        WndClassInspect( WND_REPLAY );
        break;
    case MENU_MAIN_OPEN_IMAGE:
        WndClassInspect( WND_IMAGE );
        break;
    case MENU_MAIN_OPEN_FPU:
        WndClassInspect( WND_FPU );
        break;
    case MENU_MAIN_OPEN_THREADS:
        if( HaveRemoteRunThread() ) {
            WndClassInspect( WND_RUN_THREAD );
        } else {
            WndClassInspect( WND_THREAD );
        }
        break;
    case MENU_MAIN_OPEN_FUNCTIONS:
        wnd = WndClassInspect( WND_GBLFUNCTIONS );
        if( wnd == NULL )
            break;
        FuncNewMod( wnd, NO_MOD );
        break;
    case MENU_MAIN_OPEN_FILESCOPE:
        WndClassInspect( WND_FILESCOPE );
        break;
    case MENU_MAIN_OPEN_GLOBALS:
        wnd = WndClassInspect( WND_GLOBALS );
        if( wnd == NULL )
            break;
        GlobNewMod( wnd, NO_MOD );
        break;
    case MENU_MAIN_OPEN_LOCALS:
        WndClassInspect( WND_LOCALS );
        break;
    case MENU_MAIN_OPEN_MODULES:
        wnd = WndClassInspect( WND_MODULES );
        if( wnd == NULL )
            break;
        ModNewHandle( wnd, NO_MOD );
        break;
    case MENU_MAIN_OPEN_REGISTER:
        WndClassInspect( WND_REGISTER );
        break;
    case MENU_MAIN_OPEN_MMX:
        WndClassInspect( WND_MMX );
        break;
    case MENU_MAIN_OPEN_XMM:
        WndClassInspect( WND_XMM );
        break;
    case MENU_MAIN_OPEN_SOURCE:
        WndClassInspect( WND_SOURCE );
//      WndSrcInspect( NilAddr );
        break;
    case MENU_MAIN_OPEN_WATCH:
        WndClassInspect( WND_WATCH );
        break;
    case MENU_MAIN_OPEN_MACROS:
        WndClassInspect( WND_MACRO );
        break;
    case MENU_MAIN_OPEN_IO:
        WndClassInspect( WND_IO );
        break;
    case MENU_MAIN_OPEN_STACK:
        WndClassInspect( WND_STACK );
        break;
    case MENU_MAIN_OPEN_MEMORY_AT:
        ExamMemAt();
        break;
    case MENU_MAIN_HELP_CONTENTS:
        DoProcHelp( GUI_HELP_CONTENTS );
        break;
#if defined( GUI_IS_GUI )
    case MENU_MAIN_HELP_ON_HELP:
        DoProcHelp( GUI_HELP_ON_HELP );
        break;
    case MENU_MAIN_HELP_SEARCH:
        DoProcHelp( GUI_HELP_SEARCH );
        break;
#endif
    default:
        return( false );
    }
    return( true );
}
