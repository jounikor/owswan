/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Display profile samples in the GUI.
*
****************************************************************************/


#include <stdio.h>
#include <string.h>
#include "walloca.h"

#include "common.h"
#include "aui.h"
#include "wpaui.h"
#include "dip.h"
#include "sampinfo.h"
#include "wpsrcfld.h"
#include "wpasmfil.h"
#include "msg.h"
#include "memutil.h"
#include "madinter.h"
#include "utils.h"
#include "wpbar.h"
#include "wpgather.h"
#include "wpsort.h"
#include "wpsamp.h"
#include "dipinter.h"
#include "clrsamps.h"
#include "wpgetrow.h"
#include "wpnumrow.h"
#include "wpsrcfil.h"
#include "wpwind.h"
#include "wpdata.h"


STATIC void *           sampleCreateWin( void );
STATIC void             sampleOpenMainImage( void );
STATIC bool             sampleProcTopStatus( a_window, wnd_row, wnd_piece, wnd_line_piece * );
STATIC bool             sampleProcBotStatus( a_window, wnd_row, wnd_piece, wnd_line_piece * );
STATIC bool             sampleProcStatus( a_window, wnd_row, wnd_piece, wnd_line_piece * );
STATIC bool             sampleProcOverview( a_window, wnd_row, wnd_piece, wnd_line_piece * );
STATIC bool             sampleWndEventProc( a_window, gui_event, void * );
STATIC bool             sampleSetLine( a_window, wnd_row, wnd_piece, wnd_line_piece * );
STATIC bool             sampleGetLine( a_window, wnd_row, wnd_piece, wnd_line_piece * );
STATIC int              simageDetailLine( a_window, wnd_row, bool );
STATIC int              smodDetailLine( a_window, wnd_row, bool );
STATIC int              sfileDetailLine( a_window, wnd_row, bool );
STATIC int              srtnDetailLine( a_window, wnd_row, bool );
STATIC int              ssrcDetailLine( a_window, wnd_row, bool );
STATIC int              sasmDetailLine( a_window, wnd_row, bool );
STATIC int              srtnOpenDetail( sio_data *, bool );
STATIC void             sampleRefresh( a_window );
//STATIC void             sampleMenuItem( a_window, gui_ctl_id id, wnd_row, wnd_piece );
STATIC void             sampFixDirtyCurr( a_window );
STATIC bool             simageGetLine( a_window, wnd_row );
STATIC bool             smodGetLine( a_window, wnd_row );
STATIC bool             sfileGetLine( a_window, wnd_row );
STATIC bool             srtnGetLine( a_window, wnd_row );
STATIC bool             ssrcGetLine( a_window, wnd_row );
STATIC bool             sasmGetLine( a_window, wnd_row );
STATIC void             gatherSort( sio_data * );
STATIC void             setDisplay( a_window, sio_data *, bool );

typedef bool (SAMPLEGETRTNS)( a_window wnd, wnd_row row );
typedef int (SAMPLEDETAILRTNS)( a_window wnd, wnd_row row, bool multi_level );

enum {
    PIECE_MOUSE_CATCHER,
    PIECE_BAR,
    PIECE_SEPARATOR,
    PIECE_HOOK,
    PIECE_DETAIL_NAME = PIECE_HOOK,
    PIECE_NAME_TITLE,
    PIECE_NAME_TEXT,
    PIECE_LAST
};

enum {
    PIECE_REL_HEADER=PIECE_MOUSE_CATCHER+1,
    PIECE_REL_PERCENT,
    PIECE_ABS_HEADER,
    PIECE_ABS_PERCENT,
    PIECE_PERCENT_SEPARATOR,
    PIECE_DETAIL_TITLE,
    PIECE_HEADER_LAST
};

enum {
    PIECE_DRAW_LINE=PIECE_MOUSE_CATCHER+1,
};

#define STATUS_ROW          8
#define BAR_TAIL_POINT      2500
#define SEPARATOR_POINT     2500

static char * overviewHeaders[] = {
    LIT( Sample_Header ),
    LIT( Image_Header ),
    LIT( Module_Header ),
    LIT( File_Header ),
    LIT( Routine_Header ),
    LIT( Source_Header ),
    LIT( Empty_Str ),
};

static char * statusHeaders[] = {
    LIT( Image_Names ),
    LIT( Module_Names ),
    LIT( File_Names ),
    LIT( Routine_Names ),
    LIT( Source_Line ),
    LIT( Assembler_Instructions ),
};

static SAMPLEGETRTNS * sampleGetRtns[] = {
    &simageGetLine,
    &smodGetLine,
    &sfileGetLine,
    &srtnGetLine,
    &ssrcGetLine,
    &sasmGetLine,
};

static SAMPLEDETAILRTNS * overviewDetailRtns[] = {
    &simageDetailLine,
    &smodDetailLine,
    &sfileDetailLine,
    &srtnDetailLine,
    &ssrcDetailLine,
    &sasmDetailLine,
};

static gui_menu_struct graphBarMenu[] = {
    { "&Stretch",       MENU_SAMP_BAR_MAX_TIME, GUI_STYLE_MENU_ENABLED, "Stretch the largest value to the edge of the window" },
    { "&Absolute Bars", MENU_SAMP_ABS,          GUI_STYLE_MENU_ENABLED, "Show Absolute Graph Bars" },
    { "&Relative Bars", MENU_SAMP_REL,          GUI_STYLE_MENU_ENABLED, "Show Relative Graph Bars" }
};

static gui_menu_struct sortMenu[] = {
    { "&Sample Count",  MENU_SORT_COUNT,        GUI_STYLE_MENU_ENABLED, "Sort by the number of samples" },
    { "&Name",          MENU_SORT_NAME,         GUI_STYLE_MENU_ENABLED, "Sort by the name" },
};

static gui_menu_struct sampleMenu[] = {
    { "Zoom &In\tF3",           MENU_SAMP_ZOOM_IN,  GUI_STYLE_MENU_ENABLED, "Zoom in form more detail" },
    { "Back &Out\tF4",          MENU_SAMP_BACK_OUT, GUI_STYLE_MENU_ENABLED, "Back out from the current display" },
    { "&Gather Small Values",   MENU_SAMP_GATHER,   GUI_STYLE_MENU_ENABLED, "Gather small samples together" },
    { "&Bar Graph",             MENU_SAMP_BAR,      GUI_STYLE_MENU_ENABLED, "Make adjustments to the bar graph",   ArraySize( graphBarMenu ),  graphBarMenu },
    { "&Sort",                  MENU_SAMP_SORT,     GUI_STYLE_MENU_ENABLED, "Sort the values",                     ArraySize( sortMenu ),      sortMenu },
};

static char *       nameBuff = NULL;
static wnd_bar_info barData;
static char *       dispName;
static clicks_t     dispCount;
static clicks_t     localTicks;
static clicks_t     maxTime;
static gui_ord      barExtent;
static gui_ord      bar2Extent;
static gui_ord      indentPiece = 0;
static gui_ord      relPctStatusIndent;
static gui_ord      absPctStatusIndent;
static bool         sampNewRow;
static bool         barMaxTime;
static bool         dispHighLight;
static bool         absGraphBar;
static bool         relGraphBar;
static char         relData[30];
static char         absData[30];
static char         lineData[96];


static void WPZoomIn( a_window wnd, int row )
/*******************************************/
{
    sio_data        *curr_sio;
    int             detail_rows;
    int             top_line;
    int             old_level;
    int             curr_line;
    bool            multi_level;

    curr_sio = WndExtra( wnd );
    if( row >= curr_sio->level_open && row < STATUS_ROW ) {
        Ring();
        return;
    }
    if( row < curr_sio->level_open ) {
        curr_sio->level_open = row;
        WndSetTop( wnd, 0 );
        gatherSort( curr_sio );
        if( curr_sio->level_open == LEVEL_ROUTINE ) {
            curr_line = srtnOpenDetail( curr_sio, false );
        } else {
            curr_line = WPGetRow( curr_sio );
        }
    } else {
        row = row - STATUS_ROW - 1;
        curr_line = row;
        detail_rows = SampleNumRows( wnd );
        if( detail_rows < row+1 ) {
            Ring();
            return;
        }
        multi_level = false;
        for( ;; ) {
            old_level = curr_sio->level_open;
            curr_line = overviewDetailRtns[curr_sio->level_open](
                                           wnd, row, multi_level );
            if( old_level == curr_sio->level_open ) break;
            detail_rows = SampleNumRows( wnd );
            if( detail_rows != 1 ) break;
            row = 0;
            multi_level = true;
        }
    }
    curr_sio->curr_proc_row = -WND_MAX_ROW;
    curr_sio->curr_display_row = -WND_MAX_ROW;
    detail_rows = SampleNumRows( wnd );
    top_line = WndTop( wnd );
    row = curr_line - top_line;
    if( row >= WndRows( wnd ) ) {
        top_line = curr_line - WndRows( wnd ) / 2;
    }
    if( row < 0 ) {
        top_line = curr_line;
    }
    if( detail_rows-top_line < WndRows( wnd ) ) {
        top_line = detail_rows - WndRows( wnd );
        if( top_line < 0 ) {
            top_line = 0;
        }
    }
    WndSetTop( wnd, top_line );
    WndNewCurrent( wnd, curr_line, PIECE_DETAIL_NAME );
    WndDirty( wnd );
    if( curr_sio->level_open < LEVEL_SOURCE
     && curr_sio->asm_file != NULL ) {
        WPAsmClose( curr_sio->asm_file );
        curr_sio->asm_file = NULL;
    }
    if( curr_sio->level_open < LEVEL_ROUTINE
     && curr_sio->src_file != NULL ) {
        WPSourceClose( curr_sio->src_file );
        curr_sio->src_file = NULL;
    }
}


static void WPBackOut( a_window wnd )
/***********************************/
{
    sio_data *      curr_sio;

    curr_sio = WndExtra( wnd );
    if( curr_sio->level_open == 0 ) {
        Ring();
        return;
    }
    WPZoomIn( wnd, curr_sio->level_open-1 );
}


STATIC void sampleMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
/*************************************************************************************/
{
    sio_data *      curr_sio;
    int             sort_type;

    /* unused parameters */ (void)piece;

    curr_sio = WndExtra( wnd );
    row += STATUS_ROW + 1;
    switch( id ) {
    case MENU_INITIALIZE:
        if( row <= STATUS_ROW ) {
            WndMenuGrayAll( wnd );
            if( row < 0 || row - 1 >= curr_sio->level_open )
                break;
            WndMenuEnable( wnd, MENU_SAMP_ZOOM_IN, true );
            break;
        }
        WndMenuEnableAll( wnd );
        WndMenuCheck( wnd, MENU_SAMP_GATHER, GetCurrentGather( curr_sio ) );
        WndMenuCheck( wnd, MENU_SAMP_BAR_MAX_TIME, GetCurrentMaxBar( curr_sio ) );
        WndMenuCheck( wnd, MENU_SAMP_ABS, GetCurrentAbsBar( curr_sio ) );
        WndMenuCheck( wnd, MENU_SAMP_REL, GetCurrentRelBar( curr_sio ) );
        sort_type = GetCurrentSort( curr_sio );
        WndMenuCheck( wnd, MENU_SORT_COUNT, sort_type==SORT_COUNT );
        WndMenuCheck( wnd, MENU_SORT_NAME, sort_type==SORT_NAME );
        if( row <= STATUS_ROW || curr_sio->level_open >= LEVEL_ROUTINE ) {
            WndMenuEnable( wnd, MENU_SAMP_GATHER, false );
            WndMenuEnable( wnd, MENU_SAMP_SORT, false );
            WndMenuEnable( wnd, MENU_SORT_COUNT, false );
            WndMenuEnable( wnd, MENU_SORT_NAME, false );
            if( row <= STATUS_ROW ) {
                WndMenuEnable( wnd, MENU_SAMP_BAR_MAX_TIME, false );
                WndMenuEnable( wnd, MENU_SAMP_ABS, false );
                WndMenuEnable( wnd, MENU_SAMP_REL, false );
            }
        }
        break;
    case MENU_SAMP_ZOOM_IN:
        WPZoomIn( wnd, row );
        break;
    case MENU_SAMP_BACK_OUT:
        WPBackOut( wnd );
        break;
//    case MENU_SAMP_DATA:
//        WPSImageOpen( curr_sio );
//        break;
    case MENU_SAMP_GATHER:
        FlipCurrentGather( curr_sio );
        WndMenuCheck( wnd, MENU_SAMP_GATHER, GetCurrentGather( curr_sio ) );
        gatherSort( curr_sio );
        setDisplay( wnd, curr_sio, true );
        break;
    case MENU_SAMP_BAR_MAX_TIME:
        FlipCurrentMaxBar( curr_sio );
        WndMenuCheck( wnd, MENU_SAMP_BAR_MAX_TIME, GetCurrentMaxBar( curr_sio ) );
        setDisplay( wnd, curr_sio, false );
        break;
    case MENU_SAMP_ABS:
        FlipCurrentAbsBar( curr_sio );
        WndMenuCheck( wnd, MENU_SAMP_ABS, GetCurrentAbsBar( curr_sio ) );
        setDisplay( wnd, curr_sio, false );
        break;
    case MENU_SAMP_REL:
        FlipCurrentRelBar( curr_sio );
        WndMenuCheck( wnd, MENU_SAMP_ABS, GetCurrentRelBar( curr_sio ) );
        setDisplay( wnd, curr_sio, false );
        break;
    case MENU_SORT_COUNT:
    case MENU_SORT_NAME:
        WndMenuCheck( wnd, MENU_SORT_COUNT, ( id == MENU_SORT_COUNT ) );
        WndMenuCheck( wnd, MENU_SORT_NAME, ( id == MENU_SORT_COUNT ) );
        if( id == MENU_SORT_COUNT ) {
            SetCurrentSort( curr_sio, SORT_COUNT );
        } else {
            SetCurrentSort( curr_sio, SORT_NAME );
        }
        SortCurrent( curr_sio );
        setDisplay( wnd, curr_sio, true );
        break;
    }
}


static wnd_info     WPSampleInfo = {
    sampleWndEventProc,
    sampleRefresh,
    sampleGetLine,
    sampleMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    WndFirstMenuItem,
    SampleNumRows,
    NoNextRow,
    NoNotify,
    NoChkUpdate,
    PopUp( sampleMenu )
};



void WPSampleOpen( void )
/***********************/
{
    if( CurrSIOData->sample_window == NULL ) {
        CurrSIOData->sample_window = sampleCreateWin();
        if( CurrSIOData->sample_window != NULL ) {
            sampleOpenMainImage();
            WndSetThumb( CurrSIOData->sample_window );
        }
    }
    if( CurrSIOData->sample_window != NULL ) {
        WndToFront( CurrSIOData->sample_window );
        WndShowWindow( CurrSIOData->sample_window );
        WPSetRowHeight( CurrSIOData->sample_window );
    }
}



STATIC void *sampleCreateWin( void )
/**********************************/
{
    a_window            wnd;
    wnd_create_struct   info;
    char                title[512];

    WndInitCreateStruct( &info );
    sprintf( title, LIT( Sample_Data ), CurrSIOData->samp_file_name );
    info.title = title;
    info.info = &WPSampleInfo;
    info.extra = CurrSIOData;
//    info.colour = GetWndColours( class );
    info.title_rows = STATUS_ROW + 1;
    info.style |= GUI_INIT_INVISIBLE;
    wnd = WndCreateWithStruct( &info );
    if( wnd == NULL )
        return( wnd );
//    WndSetFontInfo( wnd, GetWndFont( wnd ) );
//-//    WndSetSysFont( wnd, true );
    WndClrSwitches( wnd, WSW_MUST_CLICK_ON_PIECE | WSW_ONLY_MODIFY_TABSTOP );
    WndSetSwitches( wnd, WSW_RBUTTON_CHANGE_CURR );
    return( wnd );
}



STATIC void sampleOpenMainImage( void )
/*************************************/
{
    image_info      *curr_image;
    int             count;

    gatherSort( CurrSIOData );
    for( count = CurrSIOData->image_count; count-- > 0; ) {
        curr_image = CurrSIOData->images[count];
        if( curr_image->main_load ) {
            if( curr_image->dip_handle != NO_MOD && curr_image->mod_count > 2 ) {
                CurrSIOData->curr_image = curr_image;
                CurrSIOData->level_open = LEVEL_IMAGE;
                gatherSort( CurrSIOData );
            }
            break;
        }
    }
    WndNewCurrent( CurrSIOData->sample_window, 0, PIECE_DETAIL_NAME );
    WndDirty( CurrSIOData->sample_window );
}



STATIC bool sampleWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
/**************************************************************************/
{
    sio_data        *curr_sio;

    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        return( true );
    case GUI_FONT_CHANGED:
        WPSetRowHeight( wnd );
        return( true );
    case GUI_RESIZE:
        WPAdjustRowHeight( wnd, false );
        return( true );
    case GUI_NOW_ACTIVE:
        curr_sio = WndExtra( wnd );
        curr_sio->curr_proc_row = -WND_MAX_ROW;
        curr_sio->curr_display_row = -WND_MAX_ROW;
        WPDipSetProc( curr_sio->dip_process );
        SetCurrentMAD( curr_sio->config.arch );
        CurrSIOData = curr_sio;
        return( true );
    case GUI_NO_EVENT:
        sampFixDirtyCurr( wnd );
        return( true );
    case GUI_DESTROY:
        curr_sio = WndExtra( wnd );
        if( curr_sio != NULL ) {
            ClearSample( curr_sio );
        }
        return( true );
    }
    return( false );
}



STATIC bool sampleGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
/*******************************************************************************************/
{
    sio_data        *curr_sio;

    if( row <= -4  ) {
        return( sampleProcOverview( wnd, row, piece, line ) );
    }
    if( row == -3 ) {
        return( sampleProcTopStatus( wnd, row, piece, line ) );
    }
    if( row == -2  ) {
        return( sampleProcStatus( wnd, row, piece, line ) );
    }
    if( row == -1 ) {
        return( sampleProcBotStatus( wnd, row, piece, line ) );
    }
    curr_sio = WndExtra( wnd );
    if( !sampleGetRtns[curr_sio->level_open]( wnd, row ) )
        return( false );
    return( sampleSetLine( wnd, row, piece, line ) );
}



STATIC bool sampleProcTopStatus( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
/*************************************************************************************************/
{
    gui_point           start;
    gui_point           end;
    gui_ord             vertical_x;
    gui_ord             max_x;
    gui_ord             max_y;
    gui_ord             client_width;
    gui_ord             cross_y;

    /* unused parameters */ (void)row; (void)piece; (void)line;

    if( piece > PIECE_DRAW_LINE )
        return( false );
    if( piece == PIECE_MOUSE_CATCHER ) {
        line->indent = 0;
        line->tabstop = false;
        line->attr = WPA_PLAIN;
        line->text = LIT( Empty_Str );
        line->extent = WndWidth( wnd );
        return( true );
    }
    max_y = WndMaxCharY( wnd );
    max_x = WndAvgCharX( wnd );
    vertical_x = SEPARATOR_POINT + max_x / 2;
    client_width = WPGetClientWidth( wnd );
    cross_y = max_y * (STATUS_ROW-1) - max_y/4;
    start.x = 0;
    end.x = client_width;
    start.y = cross_y;
    end.y = start.y;
    GUIDrawLine( WndGui( wnd ), &start, &end, GUI_PEN_SOLID, 0, WPA_PLAIN );
    start.x = vertical_x;
    end.x = start.x;
    start.y = 0;
    end.y = cross_y;
    GUIDrawLine( WndGui( wnd ), &start, &end, GUI_PEN_SOLID, 0, WPA_PLAIN );
    return( true );
}



STATIC bool sampleProcBotStatus( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
/*************************************************************************************************/
{
    gui_point           start;
    gui_point           end;
    gui_ord             vertical_x;
    gui_ord             max_x;
    gui_ord             max_y;
    gui_ord             client_height;
    gui_ord             client_width;
    gui_ord             cross_y;

    /* unused parameters */ (void)row; (void)piece; (void)line;

    if( piece > PIECE_DRAW_LINE )
        return( false );
    if( piece == PIECE_MOUSE_CATCHER ) {
        line->indent = 0;
        line->tabstop = false;
        line->attr = WPA_PLAIN;
        line->text = LIT( Empty_Str );
        line->extent = WndWidth( wnd );
        return( true );
    }
    max_y = WndMaxCharY( wnd );
    max_x = WndAvgCharX( wnd );
    vertical_x = SEPARATOR_POINT + max_x / 2;
    client_height = WPGetClientHeight( wnd );
    client_width = WPGetClientWidth( wnd );
    cross_y = max_y * STATUS_ROW + max_y/4;
    start.x = 0;
    end.x = client_width;
    start.y = cross_y;
    end.y = start.y;
    GUIDrawLine( WndGui( wnd ), &start, &end, GUI_PEN_SOLID, 0, WPA_PLAIN );
    start.x = vertical_x;
    end.x = vertical_x;
    start.y = cross_y;
    end.y = client_height;
    GUIDrawLine( WndGui( wnd ), &start, &end, GUI_PEN_SOLID, 0, WPA_PLAIN );
    return( true );
}



STATIC bool sampleProcStatus( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
/**********************************************************************************************/
{
    sio_data        *curr_sio;
    clicks_t        abs_count;
    clicks_t        rel_count;
    gui_ord         point_adjust;

    /* unused parameters */ (void)row;

    if( piece >= PIECE_HEADER_LAST ) {
        return( false );
    }
    curr_sio = WndExtra( wnd );
    if( piece == PIECE_MOUSE_CATCHER ) {
        line->indent = 0;
        line->tabstop = false;
        line->attr = WPA_PLAIN;
        line->text = LIT( Empty_Str );
        abs_count = curr_sio->abs_count;
        rel_count = curr_sio->rel_count;
        sprintf( relData, "%ld.%ld%%", rel_count/10, rel_count-((rel_count/10)*10) );
        sprintf( absData, "%ld.%ld%%", abs_count/10, abs_count-((abs_count/10)*10) );
        if( WPPixelTruncWidth( WndMaxCharX( wnd ) / 2 ) == 0 ) {
            point_adjust = WndMaxCharX( wnd ) / 2;
        } else {
            point_adjust = 0;
        }
        absPctStatusIndent = BAR_TAIL_POINT - WndExtentX( wnd, absData )
                           - point_adjust;
        indentPiece = BAR_TAIL_POINT - WndExtentX( wnd, "199.9%" )
                    - (2 * WndExtentX( wnd, " " )) - point_adjust;
        relPctStatusIndent = indentPiece - WndExtentX( wnd, relData );
        indentPiece -= WndExtentX( wnd, "199.9%" );
        if( GUIIsGUI() ) {
            indentPiece -= WndExtentX( wnd, LIT( Rel_Header ) )
                         + WndExtentX( wnd, LIT( Abs_Header ) );
            relPctStatusIndent -= WndExtentX( wnd, LIT( Abs_Header ) );
            line->extent = indentPiece;
        }
    } else if( piece == PIECE_REL_HEADER ) {
        line->indent = indentPiece;
        if( GUIIsGUI() ) {
            line->text = LIT( Rel_Header );
        } else {
            line->text = LIT( Empty_Str );
        }
        line->tabstop = false;
        if( relGraphBar ) {
            line->attr = WPA_REL_BAR;
        } else {
            line->attr = WPA_PLAIN_INACTIVE;
        }
    } else if( piece == PIECE_REL_PERCENT ) {
        line->text = relData;
        line->indent = relPctStatusIndent;
        line->tabstop = false;
        if( curr_sio->rel_on_screen ) {
            line->attr = WPA_REL_BAR;
        } else {
            line->attr = WPA_PLAIN_INACTIVE;
        }
    } else if( piece == PIECE_ABS_HEADER ) {
        indentPiece = BAR_TAIL_POINT - WndExtentX( wnd, "199.9%" );
        if( GUIIsGUI() ) {
            line->text = LIT( Abs_Header );
            indentPiece -= WndExtentX( wnd, LIT( Abs_Header ) );
        } else {
            line->text = LIT( Empty_Str );
        }
        line->indent = indentPiece;
        line->tabstop = false;
        if( curr_sio->abs_on_screen ) {
            line->attr = WPA_ABS_BAR;
        } else {
            line->attr = WPA_PLAIN_INACTIVE;
        }
    } else if( piece == PIECE_ABS_PERCENT ) {
        line->text = absData;
        line->indent = absPctStatusIndent;
        line->tabstop = false;
        if( curr_sio->abs_on_screen ) {
            line->attr = WPA_ABS_BAR;
        } else {
            line->attr = WPA_PLAIN_INACTIVE;
        }
    } else if( piece == PIECE_PERCENT_SEPARATOR ) {
        if( !GUIIsGUI() ) {
            line->vertical_line = true;
        }
        line->indent = SEPARATOR_POINT;
        line->text = LIT( Empty_Str );
        line->attr = WPA_PLAIN;
        line->tabstop = false;
    } else if( piece == PIECE_DETAIL_TITLE ) {
        line->indent = SEPARATOR_POINT + WndMaxCharX( wnd );
        curr_sio = WndExtra( wnd );
        if( curr_sio->level_open == LEVEL_ROUTINE ) {
            sprintf( lineData, "%s: %.5d", statusHeaders[curr_sio->level_open], curr_sio->curr_display_row+1 );
            line->text = lineData;
        } else {
            line->text = statusHeaders[curr_sio->level_open];
        }
        line->tabstop = false;
        line->attr = WPA_PLAIN;
    }
    return( true );
}



STATIC bool sampleProcOverview( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
/************************************************************************************************/
{
    sio_data *      curr_sio;
    clicks_t        tick_count;
    clicks_t        total_ticks;
    char *          text;

    if( piece >= PIECE_LAST ) {
        return( false );
    }
    curr_sio = WndExtra( wnd );
    row += STATUS_ROW + 1;
    text = LIT( Empty_Str );
    tick_count = 0;
    if( row <= curr_sio->level_open ) {
        if( row == 0 ) {
            text = curr_sio->samp_file_name;
            tick_count = curr_sio->total_samples;
        } else if( row == 1 ) {
            text = curr_sio->curr_image->name;
            tick_count = curr_sio->curr_image->agg_count;
        } else if( row == 2 ) {
            text = curr_sio->curr_mod->name;
            tick_count = curr_sio->curr_mod->agg_count;
        } else if( row == 3 ) {
            text = curr_sio->curr_file->name;
            tick_count = curr_sio->curr_file->agg_count;
        } else if( row == 4 ) {
            text = curr_sio->curr_rtn->name;
            tick_count = curr_sio->curr_rtn->tick_count;
        }
    }
    if( piece == PIECE_MOUSE_CATCHER ) {
        line->indent = 0;
        line->tabstop = false;
        line->master_tabstop = true;
        line->attr = WPA_PLAIN;
        line->text = LIT( Empty_Str );
        if( row <= curr_sio->level_open ) {
            line->extent = BAR_TAIL_POINT;
        }
    } else if( piece == PIECE_BAR ) {
        line->indent = BAR_TAIL_POINT;
        if( curr_sio->total_samples == 0 ) {
            curr_sio->total_samples = 1;
        }
        total_ticks = curr_sio->total_samples;
        line->attr = WPA_PLAIN;
        line->tabstop = false;
        line->master_tabstop = true;
        if( row > curr_sio->level_open ) {
            line->text = LIT( Empty_Str );
        } else {
            line->draw_bar = true;
            barData.bar_style = GUI_BAR_SHADOW;
            barData.bar_colour = WPA_ABS_BAR;
            barData.bar_group = false;
            barData.bar_selected = false;
            tick_count *= BAR_TAIL_POINT - (WndMaxCharX( wnd ) / 2);
            line->extent = tick_count / total_ticks;
            if( line->extent == 0 && tick_count != 0 ) {
                line->extent = 1;
            }
            line->indent -= line->extent;
            line->text = (char *)&barData;
        }
    } else if( piece == PIECE_SEPARATOR ) {
        indentPiece = SEPARATOR_POINT;
        line->indent = indentPiece;
        line->vertical_line = true;
        line->text = LIT( Empty_Str );
        line->attr = WPA_PLAIN;
        line->tabstop = false;
        line->master_tabstop = true;
    } else if( piece == PIECE_HOOK ) {
        if( row > curr_sio->level_open ) {
            return( false );
        }
        line->tabstop = false;
        line->master_tabstop = true;
        line->text = LIT( Empty_Str );
        if( row == 0 ) {
            indentPiece += WndMaxCharX( wnd );
        } else {
            indentPiece += (row*2 - 1) * WndMaxCharX( wnd );
            line->indent = indentPiece;
            indentPiece += 2 * WndMaxCharX( wnd );
            line->draw_hook = true;
            line->attr = WPA_PLAIN;
        }
    } else if( piece == PIECE_NAME_TITLE ) {
        line->indent = indentPiece;
        nameBuff = ProfRealloc( nameBuff, strlen( overviewHeaders[row] ) + 1 );
        strcpy( nameBuff, overviewHeaders[row] );
        line->text = nameBuff;
        line->tabstop = false;
        line->master_tabstop = true;
        if( curr_sio->level_open == row ) {
            line->attr = WPA_OVERVIEW_NAME;
        } else {
            line->attr = WPA_PLAIN;
        }
        indentPiece += WndExtentX( wnd, nameBuff );
    } else {
        line->indent = indentPiece;
        nameBuff = ProfRealloc( nameBuff, strlen( text ) + 1 );
        strcpy( nameBuff, text );
        line->text = nameBuff;
        line->tabstop = false;
        line->master_tabstop = true;
        if( curr_sio->level_open == row ) {
            line->attr = WPA_OVERVIEW_NAME;
        } else {
            line->attr = WPA_PLAIN;
        }
    }
    return( true );
}



STATIC bool simageGetLine( a_window wnd, wnd_row row )
/****************************************************/
{
    sio_data *      curr_sio;
    image_info *    image;

    curr_sio = WndExtra( wnd );
    sampNewRow = row != curr_sio->curr_proc_row;
    if( sampNewRow ) {
        curr_sio->curr_proc_row = row;
        image = SImageGetImage( wnd, row );
        if( image == NULL ) {
            return( false );
        }
        curr_sio->curr_image = image;
        dispHighLight = image->main_load;
        dispName = image->name;
        dispCount = image->agg_count;
        localTicks = curr_sio->total_samples;
        maxTime = curr_sio->max_time;
        barMaxTime = curr_sio->bar_max;
        absGraphBar = curr_sio->abs_bar;
        relGraphBar = curr_sio->rel_bar;
    }
    return( true );
}



STATIC bool smodGetLine( a_window wnd, wnd_row row )
/**************************************************/
{
    sio_data        *curr_sio;
    mod_info        *mod;

    curr_sio = WndExtra( wnd );
    sampNewRow = row != curr_sio->curr_proc_row;
    if( sampNewRow ) {
        curr_sio->curr_proc_row = row;
        mod = SModGetModule( wnd, row );
        if( mod == NULL ) {
            return( false );
        }
        curr_sio->curr_mod = mod;
        dispHighLight = false;
        dispName = mod->name;
        dispCount = mod->agg_count;
        localTicks = curr_sio->curr_image->agg_count;
        maxTime = curr_sio->curr_image->max_time;
        barMaxTime = curr_sio->curr_image->bar_max;
        absGraphBar = curr_sio->curr_image->abs_bar;
        relGraphBar = curr_sio->curr_image->rel_bar;
    }
    return( true );
}



STATIC bool sfileGetLine( a_window wnd, wnd_row row )
/***************************************************/
{
    sio_data        *curr_sio;
    file_info       *curr_file;

    curr_sio = WndExtra( wnd );
    sampNewRow = row != curr_sio->curr_proc_row;
    if( sampNewRow ) {
        curr_sio->curr_proc_row = row;
        curr_file = SFileGetFile( wnd, row );
        if( curr_file == NULL ) {
            return( false );
        }
        dispHighLight = false;
        dispName = curr_file->name;
        dispCount = curr_file->agg_count;
        localTicks = curr_sio->curr_mod->agg_count;
        maxTime = curr_sio->curr_mod->max_time;
        barMaxTime = curr_sio->curr_mod->bar_max;
        absGraphBar = curr_sio->curr_mod->abs_bar;
        relGraphBar = curr_sio->curr_mod->rel_bar;
    }
    return( true );
}



STATIC bool srtnGetLine( a_window wnd, wnd_row row )
/**************************************************/
{
    sio_data        *curr_sio;
    rtn_info        *curr_rtn;

    curr_sio = WndExtra( wnd );
    sampNewRow = row != curr_sio->curr_proc_row;
    if( sampNewRow ) {
        curr_sio->curr_proc_row = row;
        curr_rtn = SRtnGetRoutine( wnd, row );
        if( curr_rtn == NULL ) {
            return( false );
        }
        dispHighLight = false;
        dispName = curr_rtn->name;
        dispCount = curr_rtn->tick_count;
        localTicks = curr_sio->curr_file->agg_count;
        maxTime = curr_sio->curr_file->max_time;
        barMaxTime = curr_sio->curr_file->bar_max;
        absGraphBar = curr_sio->curr_file->abs_bar;
        relGraphBar = curr_sio->curr_file->rel_bar;
    }
    return( true );
}



STATIC bool ssrcGetLine( a_window wnd, wnd_row row )
/**************************************************/
{
    sio_data        *curr_sio;
    wp_srcfile      *wp_src;
    wp_srcline      *lines;
    int             index;
    unsigned        adjusted_row;

    curr_sio = WndExtra( wnd );
    if( curr_sio->src_file == NULL ) {
        return( false );
    }
    adjusted_row = row + 1;
    sampNewRow = ( row != curr_sio->curr_proc_row );
    if( sampNewRow ) {
        curr_sio->curr_proc_row = row;
        dispName = WPSourceGetLine( wnd, adjusted_row );
        if( dispName == NULL ) {
            return( false );
        }
        dispHighLight = false;
        wp_src = curr_sio->src_file;
        lines = wp_src->src_lines;
        dispCount = 0;
        for( index = 0; index < wp_src->wp_line_count; ++index ) {
            if( adjusted_row == lines[index].line ) {
                dispCount = lines[index].tick_count;
                break;
            }
        }
        localTicks = curr_sio->curr_file->agg_count;
        maxTime = wp_src->max_time;
        barMaxTime = curr_sio->asm_src_info.bar_max;
        absGraphBar = curr_sio->asm_src_info.abs_bar;
        relGraphBar = curr_sio->asm_src_info.rel_bar;
    }
    return( true );
}



STATIC bool sasmGetLine( a_window wnd, wnd_row row )
/**************************************************/
{
    sio_data        *curr_sio;
    wp_asmfile      *wpasm_file;
    wp_asmline      *asm_line;
    int             asm_group;
    int             asm_row;

    curr_sio = WndExtra( wnd );
    if( curr_sio->asm_file == NULL ) {
        return( false );
    }
    sampNewRow = row != curr_sio->curr_proc_row;
    if( sampNewRow ) {
        curr_sio->curr_proc_row = row;
        dispName = WPAsmGetLine( wnd, row );
        if( dispName == NULL ) {
            return( false );
        }
        wpasm_file = curr_sio->asm_file;
        asm_line = WPGetAsmLoc( wpasm_file, row, &asm_group, &asm_row );
        dispHighLight = asm_line->source_line;
        if( dispHighLight ) {
            dispCount = 0;
        } else {
            dispCount = asm_line->u.asm_line.tick_count;
        }
        localTicks = curr_sio->curr_mod->agg_count;
        maxTime = wpasm_file->max_time;
        barMaxTime = curr_sio->asm_src_info.bar_max;
        absGraphBar = curr_sio->asm_src_info.abs_bar;
        relGraphBar = curr_sio->asm_src_info.rel_bar;
    } else if( dispName == NULL ) {
        return( false );
    }
    return( true );
}



STATIC bool sampleSetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
/*******************************************************************************************/
{
    sio_data        *curr_sio;
    clicks_t        bar_range;
    gui_ord         catcher_extent;
    gui_ord         slacker;
    int             wnd_rows;

    if( piece >= PIECE_LAST ) {
        return( false );
    }
    curr_sio = WndExtra( wnd );
    wnd_rows = SampleNumRows( wnd );
    if( row > wnd_rows-1 ) {
        Ring();
        return( false );
    }
    catcher_extent = 0;
    if( sampNewRow ) {
        catcher_extent = BAR_TAIL_POINT - (WndMaxCharX( wnd ) / 2);
        catcher_extent = WPPixelTruncWidth( catcher_extent );
        bar_range = catcher_extent * dispCount;
        if( curr_sio->total_samples == 0 ) {
            curr_sio->total_samples = 1;
        }
        if( localTicks == 0 ) {
            localTicks = 1;
        }
        if( maxTime == 0 ) {
            maxTime = 1;
        }
        slacker = WPPixelWidth( wnd );
        if( !absGraphBar ) {
            bar2Extent = 0;
        } else {
            if( !barMaxTime || relGraphBar ) {
                bar2Extent = bar_range / curr_sio->total_samples;
            } else {
                bar2Extent = bar_range / maxTime;
            }
            if( bar2Extent < slacker && bar_range != 0 ) {
                bar2Extent = slacker;
            }
        }
        if( !relGraphBar ) {
            barExtent = 0;
        } else {
            if( barMaxTime ) {
                barExtent = bar_range / maxTime;
            } else {
                barExtent = bar_range / localTicks;
            }
            if( barExtent < slacker && bar_range != 0 ) {
                barExtent = slacker;
            }
        }
        barExtent = WPPixelTruncWidth( barExtent );
        bar2Extent = WPPixelTruncWidth( bar2Extent );
        if( barExtent < bar2Extent ) {
            barExtent = bar2Extent;
        }
    }
    if( piece == PIECE_MOUSE_CATCHER ) {
        line->indent = 0;
        line->text = LIT( Empty_Str );
        line->extent = catcher_extent - barExtent;
        line->tabstop = false;
        line->master_tabstop = true;
    } else if( piece == PIECE_BAR ) {
        line->extent = barExtent;
        line->indent = BAR_TAIL_POINT - barExtent;
        if( WPPixelTruncWidth( WndMaxCharX( wnd ) / 2 ) == 0 ) {
            line->indent -= WndMaxCharX( wnd ) / 2;
        }
        if( barExtent || bar2Extent ) {
            barData.bar_style = GUI_BAR_SHADOW;
            barData.bar_colour = WPA_REL_BAR;
            barData.bar_colour2 = WPA_ABS_BAR;
            barData.bar_size2 = bar2Extent;
            barData.bar_group = true;
            barData.bar_selected = false;
            line->draw_bar = true;
            line->text = (char *)&barData;
        } else {
            line->text = LIT( Empty_Str );
        }
        line->tabstop = false;
        line->master_tabstop = true;
    } else if( piece == PIECE_SEPARATOR ) {
        line->indent = SEPARATOR_POINT;
        line->vertical_line = true;
        line->text = LIT( Empty_Str );
        line->attr = WPA_PLAIN;
        line->tabstop = false;
        line->master_tabstop = true;
    } else if( piece == PIECE_DETAIL_NAME ) {
        line->indent = SEPARATOR_POINT + WndMaxCharX( wnd );
        line->text = dispName;
        if( dispHighLight ) {
            line->attr = WPA_STANDOUT;
        } else {
            line->attr = WPA_PLAIN;
        }
        line->tabstop = true;
        line->master_tabstop = true;
    } else {
        return( false );
    }
    return( true );
}



STATIC void findRtnFromRow( sio_data *curr_sio, int row )
/*******************************************************/
{
    file_info           *curr_file;
    rtn_info            *curr_rtn;
    cue_handle          *cueh;
    sym_handle          *sh;
    int                 index;
    mod_handle          mh;
    address             addr;

    cueh = alloca( DIPHandleSize( HK_CUE ) );
    curr_file = curr_sio->curr_file;
    mh = curr_sio->curr_mod->mh;
    if( DIPLineCue( mh, curr_sio->curr_file->fid, row, 0, cueh ) == SR_NONE ) {
        if( DIPLineCue( mh, curr_sio->curr_file->fid, 0, 0, cueh ) == SR_NONE ) {
            return;
        }
    }
    sh = alloca( DIPHandleSize( HK_SYM ) );
    addr = DIPCueAddr( cueh );
    if( DIPAddrSym( mh, addr, sh ) == SR_NONE )
        return;
    for( index = 0; index < curr_file->rtn_count; ++index ) {
        curr_rtn = curr_file->routine[index];
        if( curr_rtn->sh != NULL && DIPSymCmp( curr_rtn->sh, sh ) == 0 ) {
            curr_sio->curr_rtn = curr_rtn;
            break;
        }
    }
}



STATIC void sampFixDirtyCurr( a_window wnd )
/******************************************/
{
    sio_data        *curr_sio;
    wp_srcfile      *src_file;
    rtn_info        *curr_rtn;
    int             src_line;
    wnd_row         row;
    wnd_piece       piece;

    curr_sio = WndExtra( wnd );
    WndGetCurrent( wnd, &row, &piece );
    if( row == WND_NO_ROW )
        return;
    if( curr_sio->level_open == LEVEL_SAMPLE ) {
        if( !simageGetLine( wnd, row ) ) {
            return;
        }
    } else if( curr_sio->level_open == LEVEL_IMAGE ) {
        if( !smodGetLine( wnd, row ) ) {
            return;
        }
    } else if( curr_sio->level_open == LEVEL_MODULE ) {
        if( !sfileGetLine( wnd, row ) ) {
            return;
        }
    } else if( curr_sio->level_open == LEVEL_FILE ) {
        if( !srtnGetLine( wnd, row ) ) {
            return;
        }
    } else {
        if( curr_sio->level_open == LEVEL_ROUTINE ) {
            if( !ssrcGetLine( wnd, row ) ) {
                return;
            }
        } else {
            if( !sasmGetLine( wnd, row ) ) {
                return;
            }
        }
        src_line = row + 1;
        if( curr_sio->level_open == LEVEL_SOURCE ) {
            src_line = WPAsmFindSrcLine( curr_sio, src_line );
        }
        curr_rtn = curr_sio->curr_rtn;
        findRtnFromRow( curr_sio, src_line );
        if( curr_rtn != curr_sio->curr_rtn ) {
            WndRowDirty( wnd, LEVEL_ROUTINE-STATUS_ROW-1+WndTop(wnd) );
        }
        src_file = curr_sio->src_file;
        if( src_file != NULL ) {
            src_file->samp_line = src_line;
        }
    }
    curr_sio->curr_proc_row = -WND_MAX_ROW;
    if( row != curr_sio->curr_display_row ) {
        curr_sio->curr_display_row = row;
        if( curr_sio->total_samples == 0 || localTicks == 0 ) {
            curr_sio->abs_count = 0;
            curr_sio->rel_count = 0;
        } else {
            curr_sio->abs_count = (dispCount*1000) / curr_sio->total_samples;
            curr_sio->rel_count = (dispCount*1000) / localTicks;
        }
        curr_sio->abs_on_screen = absGraphBar;
        curr_sio->rel_on_screen = relGraphBar;
        if( dispCount > 0 ) {
            if( curr_sio->rel_count == 0 ) {
                curr_sio->rel_count = 1;
            }
            if( curr_sio->abs_count == 0 ) {
                curr_sio->abs_count = 1;
            }
        }
        WndRowDirty( wnd, -2+WndTop(wnd) );
    }
}



STATIC int simageDetailLine( a_window wnd, wnd_row row, bool multi_level )
/************************************************************************/
{
    sio_data        *curr_sio;
    image_info      *image;

    curr_sio = WndExtra( wnd );
    image = SImageGetImage( wnd, row );
    if( image == NULL ) {
        if( !multi_level ) {
            Ring();
        }
        return( row );
    }
    if( image->exe_not_found ) {
        if( !multi_level ) {
            ErrorMsg( LIT( Exe_Not_Found ), image->name );
        }
        return( row );
    }
    if( image->dip_handle == NO_MOD ) {
        if( !multi_level ) {
            ErrorMsg( LIT( No_Symbol_Info ), image->name );
        }
        return( row );
    }
    if( image->exe_changed ) {
        if( !multi_level ) {
            ErrorMsg( LIT( Exe_Has_Changed ), image->name );
            image->exe_changed = false;
        }
    }
    curr_sio->level_open++;
    curr_sio->curr_image = image;
    gatherSort( curr_sio );
    return( 0 );
}



STATIC int smodDetailLine( a_window wnd, wnd_row row, bool multi_level )
/**********************************************************************/
{
    sio_data        *curr_sio;
    mod_info        *mod;

    curr_sio = WndExtra( wnd );
    mod = SModGetModule( wnd, row );
    if( mod->agg_count == 0 && mod->file_count == 2 ) {
        if( !multi_level ) {
            ErrorMsg( LIT( No_Symbol_Info ), mod->name );
        }
        return( row );
    }
    curr_sio->level_open++;
    curr_sio->curr_mod = mod;
    gatherSort( curr_sio );
    return( 0 );
}



STATIC int sfileDetailLine( a_window wnd, wnd_row row, bool multi_level )
/***********************************************************************/
{
    sio_data        *curr_sio;
    file_info       *curr_file;

    curr_sio = WndExtra( wnd );
    curr_file = SFileGetFile( wnd, row );
    if( curr_file->rtn_count == 0 ) {
        if( !multi_level ) {
            ErrorMsg( LIT( No_Routine_Names ), curr_file->name );
        }
        return( row );
    }
    curr_sio->level_open++;
    curr_sio->curr_file = curr_file;
    gatherSort( curr_sio );
    return( 0 );
}



STATIC int srtnDetailLine( a_window wnd, wnd_row row, bool multi_level )
/**********************************************************************/
{
    sio_data        *curr_sio;
    rtn_info        *curr_rtn;
    int             line;

    /* unused parameters */ (void)multi_level;

    curr_sio = WndExtra( wnd );
    curr_rtn = SRtnGetRoutine( wnd, row );
    curr_sio->curr_rtn = curr_rtn;
    line = srtnOpenDetail( curr_sio, true );
    return( line );
}



STATIC int srtnOpenDetail( sio_data *curr_sio, bool go_down )
/***********************************************************/
{
    a_window        wnd;
    wp_srcfile      *src_file;
    int             line;
    int             top_line;

    wnd = curr_sio->sample_window;
    src_file = curr_sio->src_file;
    if( src_file == NULL ) {
        src_file = WPSourceOpen( curr_sio, true );
        if( src_file == NULL ) {
            if( go_down ) {
                curr_sio->level_open = LEVEL_ROUTINE;
                line = ssrcDetailLine( wnd, 0, true );
                if( curr_sio->level_open == LEVEL_ROUTINE ) {
                    curr_sio->level_open = LEVEL_FILE;
                }
                return( line );
            }
            curr_sio->level_open = LEVEL_FILE;
            return( 0 );
        }
    }
    curr_sio->level_open = LEVEL_ROUTINE;
    line = src_file->samp_line;
    if( line < 1 ) {
        line = src_file->rtn_line;
    }
    top_line = line - 1 - WndRows( wnd ) / 2;
    if( top_line >= 0 ) {
        WndSetTop( wnd, top_line );
    }
    return( line-1 );
}



STATIC int ssrcDetailLine( a_window wnd, wnd_row row, bool multi_level )
/**********************************************************************/
{
    sio_data        *curr_sio;
    wp_asmfile      *asm_file;
    int             top_line;

    curr_sio = WndExtra( wnd );
    asm_file = WPAsmOpen( curr_sio, row+1, multi_level );
    if( asm_file == NULL ) {
        return( row );
    }
    curr_sio->level_open++;
    curr_sio->asm_file = asm_file;
    top_line = asm_file->entry_line - WndRows( wnd ) / 2;
    if( top_line >= 0 ) {
        WndSetTop( wnd, top_line );
    }
    return( asm_file->entry_line );
}



STATIC int sasmDetailLine( a_window wnd, wnd_row row, bool multi_level )
/**********************************************************************/
{
    /* unused parameters */ (void)wnd; (void)multi_level;

    Ring();
    return( row );
}



STATIC void sampleRefresh( a_window wnd )
/***************************************/
{
    WndZapped( wnd );
}



void WPDoPopUp( a_window wnd, gui_menu_struct * gui_menu )
/********************************************************/
{
    sio_data *      curr_sio;

    WndPopUp( wnd, gui_menu );
    WndNoSelect( wnd );
    curr_sio = WndExtra( wnd );
    curr_sio->curr_proc_row = -WND_MAX_ROW;
    curr_sio->curr_display_row = -WND_MAX_ROW;
}



void WPFindDoPopUp( a_window wnd, gui_ctl_id id )
/***********************************************/
{
    gui_menu_struct *   gui_menu;
    int                 index;

    index = 0;
    for( ;; ) {
        gui_menu = &sampleMenu[index++];
        if( gui_menu->id == id ) {
            break;
        }
    }
    WPDoPopUp( wnd, gui_menu );
}



STATIC void gatherSort( sio_data * curr_sio )
/*******************************************/
{
    GatherCurrent( curr_sio );
    SortCurrent( curr_sio );
}



STATIC void setDisplay( a_window wnd, sio_data * curr_sio, bool do_top )
/**********************************************************************/
{
    curr_sio->curr_proc_row = -WND_MAX_ROW;
    curr_sio->curr_display_row = -WND_MAX_ROW;
    if( do_top ) {
        WndSetTop( wnd, 0 );
        WndNewCurrent( wnd, 0, PIECE_DETAIL_NAME );
    }
    WndDirty( wnd );
}
