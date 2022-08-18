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


#include <stdlib.h>
#include <string.h>
#include "guiwind.h"
#include "guidlg.h"
#include "guistr.h"
#include "guiutil.h"

#include "clibext.h"


/* buttons and icons that can be in the dialog */
typedef enum {
    CTLT_NO_CONTROL  = 0x000,
    CTLT_ABORT       = 0x001,
    CTLT_CANCEL      = 0x002,
    CTLT_IGNORE      = 0x004,
    CTLT_NO          = 0x008,
    CTLT_OK          = 0x010,
    CTLT_RETRY       = 0x020,
    CTLT_YES         = 0x040,
    CTLT_EXCLAMATION = CTLT_NO_CONTROL,
    CTLT_QUESTION    = CTLT_NO_CONTROL,
    CTLT_INFORMATION = CTLT_NO_CONTROL,
    CTLT_STOP        = CTLT_NO_CONTROL
} control_types;

/* information about the controls needed for each gui_message_type */
typedef struct message_types {
    gui_message_type    type;
    int                 num_controls;
    control_types       controls;
} message_types;

/* control definition for each control that can be in the dialog */
typedef struct control_pairs {
    gui_control_info    ctl_info;
    control_types       type;
} control_pairs;

/* struct used to describe string broken into segments that can be displayed
 * one per line */
typedef struct string_info {
    char        *text;
    int         length;
} string_info;

#define TEXT_ROW        1
#define TEXT_START_COL  2
#define ICON_ROW        1
#define BUTTON_ROW      3
#define BUTTON_WIDTH    7

/* control definition for each control that can be in the dialog */
static control_pairs MessageControls[] = {
    { GUI_CTL_BUTTON( NULL,     GUI_RET_ABORT,  0, 0, BUTTON_WIDTH ),   CTLT_ABORT       },
    { GUI_CTL_BUTTON( NULL,     GUI_RET_CANCEL, 0, 0, BUTTON_WIDTH ),   CTLT_CANCEL      },
    { GUI_CTL_BUTTON( NULL,     GUI_RET_IGNORE, 0, 0, BUTTON_WIDTH ),   CTLT_IGNORE      },
    { GUI_CTL_BUTTON( NULL,     GUI_RET_NO,     0, 0, BUTTON_WIDTH ),   CTLT_NO          },
    { GUI_CTL_DEFBUTTON( NULL,  GUI_RET_OK,     0, 0, BUTTON_WIDTH ),   CTLT_OK          },
    { GUI_CTL_BUTTON( NULL,     GUI_RET_RETRY,  0, 0, BUTTON_WIDTH ),   CTLT_RETRY       },
    { GUI_CTL_BUTTON( NULL,     GUI_RET_YES,    0, 0, BUTTON_WIDTH ),   CTLT_YES         },
    { GUI_CTL_STRING( "!",                      0, 0, 1 ),              CTLT_EXCLAMATION },
    { GUI_CTL_STRING( "?",                      0, 0, 1 ),              CTLT_QUESTION    },
    { GUI_CTL_STRING( "i",                      0, 0, 1 ),              CTLT_INFORMATION },
    { GUI_CTL_STRING( NULL,                     0, 0, 5 ),              CTLT_STOP        }
};

/* static text controls used for displaying message */
static gui_control_info StaticMessage = GUI_CTL_STRING( NULL, TEXT_START_COL, TEXT_ROW, 0 );

/* information about the controls needed for each gui_message_type */
static message_types ControlsNeeded[] = {
    { GUI_ABORT_RETRY_IGNORE,   3,  CTLT_ABORT | CTLT_IGNORE | CTLT_RETRY   },
    { GUI_EXCLAMATION,          1,  CTLT_EXCLAMATION | CTLT_OK              },
    { GUI_INFORMATION,          1,  CTLT_INFORMATION | CTLT_OK              },
    { GUI_QUESTION,             1,  CTLT_QUESTION | CTLT_OK                 },
    { GUI_STOP,                 1,  CTLT_STOP | CTLT_OK                     },
    { GUI_OK,                   1,  CTLT_OK                                 },
    { GUI_OK_CANCEL,            2,  CTLT_OK | CTLT_CANCEL                   },
    { GUI_RETRY_CANCEL,         2,  CTLT_RETRY | CTLT_CANCEL                },
    { GUI_YES_NO,               2,  CTLT_YES | CTLT_NO                      },
    { GUI_YES_NO_CANCEL,        3,  CTLT_YES | CTLT_NO | CTLT_CANCEL        },
    { GUI_SYSTEMMODAL,          0,  CTLT_NO_CONTROL                         }
};

static bool MessagesInitialized = false;

static void InitMessageControls( void )
{
    int j;

    for( j = 0; j < GUI_ARRAY_SIZE( MessageControls ); j++ ) {
        switch( MessageControls[j].type ) {
        case CTLT_ABORT:
            MessageControls[j].ctl_info.text = LIT( XAbort );
            break;
        case CTLT_CANCEL:
            MessageControls[j].ctl_info.text = LIT( Cancel );
            break;
        case CTLT_IGNORE:
            MessageControls[j].ctl_info.text = LIT( XIgnore );
            break;
        case CTLT_NO:
            MessageControls[j].ctl_info.text = LIT( XNo );
            break;
        case CTLT_OK:
            MessageControls[j].ctl_info.text = LIT( OK );
            break;
        case CTLT_RETRY:
            MessageControls[j].ctl_info.text = LIT( XRetry );
            break;
        case CTLT_YES:
            MessageControls[j].ctl_info.text = LIT( XYes );
            break;
        case CTLT_STOP:
            MessageControls[j].ctl_info.text = LIT( Stop_Bang );
            break;
        default:
            break;
        }
    }
}

/*
 * DisplayMessageGUIEventProc - callback function for dialog box
 */

static bool DisplayMessageGUIEventProc( gui_window *wnd, gui_event gui_ev, void *param )
{
    gui_message_return *ret;
    gui_ctl_id          id;

    ret = GUIGetExtra( wnd );
    switch( gui_ev ) {
    case GUI_CONTROL_CLICKED:
        GUI_GETID( param, id );
        switch( id ) {
        case GUI_RET_ABORT:
        case GUI_RET_CANCEL:
        case GUI_RET_IGNORE:
        case GUI_RET_NO:
        case GUI_RET_OK:
        case GUI_RET_RETRY:
        case GUI_RET_YES:
            *ret = (gui_message_return)id;
            GUICloseDialog( wnd );
            return( true );
        default:
            break;
        }
        break;
    default:
        break;
    }
    return( false );
}

/*
 * tabFilter -- Expand tabs to blanks
 */

#define TAB_SIZE 4

static char *tabFilter( const char *message )
{
    char    *new_message;
    char    *start;
    size_t  tab_pos;
    size_t  len;

    /* allocate another chunk of memory since */
    /* reallocating space for string literals is a no no */
    new_message = (char *)GUIStrDup( message, NULL );
    for( ;; ) {
        tab_pos = strcspn( new_message, "\t" );
        len = strlen( new_message );
        if( tab_pos == len )
            break;      /* no more tabs */
        new_message = (char *)GUIMemRealloc( new_message, len + TAB_SIZE + 1 );
        /* don't forget the NULL */
        start = new_message + tab_pos;
        memmove( start + TAB_SIZE, start + 1, strlen( start + 1 ) + 1 );
        memset( start, ' ', TAB_SIZE );
    }
    return( new_message );
}


/*
 * getNumStringControls -- divide string up into segments that can be
 *                         displayed one per line.  Attempt to break
 *                         lines at spaces.
 */

static bool getNumStringControls( int *num_controls, const char *old_message, string_info **info )
{
    gui_rect            scale;
    gui_text_metrics    metrics;
    int                 max_width;
    char                *tmp_n;              /* temp newline character */
    int                 len;
    char                *start;
    char                *end_char;
    char                *message;
    char                *new_message;
    bool                ok;
    string_info         *new_info;
    int                 new_num;

    *info = NULL;
    *num_controls = 0;

    new_message = tabFilter( old_message );     /* expand out tab characters */
    if( new_message == NULL ) {
        return( false );
    }

    GUIGetScale( &scale );
    GUIGetDlgTextMetrics( &metrics );
    if( strlen( old_message ) > 256 ) {
        // If message is long, go to wider box
        max_width = ( 7 * ( scale.width / metrics.avg.x ) ) / 8;
    } else {
        max_width = ( 2 * ( scale.width / metrics.avg.x ) ) / 3;
    }

    tmp_n = NULL;
    message = new_message;
    while( message != tmp_n ) {
        tmp_n = message + strcspn( message, "\n\r" );
        if( tmp_n - message < max_width ) {
            start   = message;
            len     = tmp_n - start;
            if( *tmp_n == '\0' ) {
                message = tmp_n;            /* at the end of original string */
            } else {
                message = tmp_n + 1;        /* skip over newline */
            }
        } else {                            /* search back for space */
            for( end_char = message + max_width; end_char > message ; --end_char ) {
                if( *end_char == ' ' ) {
                    break;
                }
            }
            if( end_char == message ) {
                start   = message;          /* no spaces found */
                len     = max_width;
                message += max_width;
            } else {
                start   = message;
                len     = end_char - start + 1;
                message = end_char + 1;     /* skip over blank */
            }
        }                                   /* add new line to error box */
        new_num = *num_controls + 1;
        new_info = (string_info *)GUIMemRealloc( *info, sizeof( string_info ) * new_num );
        ok = ( new_info != NULL );
        if( !ok ) {
            break;
        }
        *info = new_info;
        *num_controls = new_num;
        new_info[new_num - 1].length = len;
        new_info[new_num - 1].text = GUIStrDupLen( start, len, &ok );
        if( !ok ) {
            break;
        }
    } /* for */
    // de-allocate memory allocated in tabFilter routine */
    GUIMemFree( new_message );
    return( ok );
}


static void freeStringControls( int num_controls, string_info *info )
{
    int i;

    for( i = 0; i < num_controls; ++i ) {
        GUIMemFree( info[i].text );
    }
    GUIMemFree( info );
}


/*
 * AdjustVert -- adjust the vertical position of buttons and icons according
 *               to the number of lines of text ( also adjusts cols )
 */

static int AdjustVert( gui_text_ord *pcols, control_types controls_to_use,
                gui_control_info *controls_info, int num_controls,
                int num_string_controls )
{
    int num_buttons;
    int i;
    int j;
    gui_text_ord cols;
    gui_text_ord end;

    if( !MessagesInitialized ) {
        InitMessageControls();
        MessagesInitialized = true;
    }

    i = num_string_controls;
    num_buttons = 0;
    cols = *pcols;
    for( j = 0; j < GUI_ARRAY_SIZE( MessageControls ); j++ ) {
        if( ( i < num_controls ) && ( controls_to_use & MessageControls[j].type ) ) {
            memcpy( &controls_info[i], &MessageControls[j].ctl_info, sizeof( gui_control_info ) );
            switch( controls_info[i].control_class ) {
            case GUI_PUSH_BUTTON:
            case GUI_DEFPUSH_BUTTON:
                num_buttons ++;
                controls_info[i].rect.y = BUTTON_ROW + num_string_controls - 1;
                break;
            case GUI_STATIC:
                controls_info[i].rect.y = ICON_ROW + ( num_string_controls - 1 ) / 2;
                break;
            default:
                break;
            }
            end = TEXT_START_COL + controls_info[i].rect.width;
            if( cols < end )
                cols = end;
            i++;
        }
    }
    *pcols = cols;
    return( num_buttons );
}

/*
 * CentreButtons -- centre the buttons horizontally
 */

static void CentreButtons( gui_text_ord cols, int num_buttons, gui_control_info *controls_info, int num_controls )
{
    int button_number;
    int space_per_button;
    int i;

    button_number = 0;
    space_per_button = 0;
    if( num_buttons > 0 ) {
        space_per_button = cols / num_buttons;
    }
    for( i = 0; i < num_controls; i++ ) {
        switch( controls_info[i].control_class ) {
        case GUI_PUSH_BUTTON:
        case GUI_DEFPUSH_BUTTON:
            button_number++;
            controls_info[i].rect.x = space_per_button * button_number
                                    - ( ( space_per_button + BUTTON_WIDTH ) / 2 );
            break;
        default:
            break;
        }
    }
}

/*
 * GUIDisplayMessage -- display the message, return the user's response
 */

gui_message_return GUIAPI GUIDisplayMessage( gui_window *wnd, const char *message,
                                      const char *title, gui_message_type type )
{
    gui_text_ord        rows;
    gui_text_ord        cols;
    gui_text_ord        end;
    gui_control_info    *controls_info;
    int                 num_controls;
    int                 num_string_controls;
    gui_message_return  ret;
    int                 i;
    control_types       controls_to_use;
    string_info         *strings;
    int                 num_buttons;

    /* unused parameters */ (void)wnd;

    /* figure out the number of icon and button controls and which ones */
    num_controls = 0;
    controls_to_use = 0;
    for( i = 0; i < GUI_ARRAY_SIZE( ControlsNeeded ); i++ ) {
        if( type & ControlsNeeded[i].type ) {
            num_controls += ControlsNeeded[i].num_controls;
            controls_to_use |= ControlsNeeded[i].controls;
        }
    }

    /* figure out how manu GUI_STATIC controls are required for the text */
    num_string_controls = 0;
    strings = NULL;
    if( !getNumStringControls( &num_string_controls, message, &strings ) ) {
        freeStringControls( num_string_controls, strings );
        return( GUI_RET_CANCEL );
    }

    num_controls += num_string_controls;
    controls_info = (gui_control_info *)GUIMemAlloc( sizeof( gui_control_info ) * num_controls );
    if( controls_info == NULL ) {
        freeStringControls( num_string_controls, strings );
        return( GUI_RET_CANCEL );
    }

    if( title != NULL ) {
        cols = strlen( title ) + 2;
    } else {
        cols = 2;
    }

    /* create GUI_STATIC controls, as many as required */
    if( num_string_controls > 0 ) {
        for( i = 0; i < num_string_controls; i++ ) {
            uiyield();
            StaticMessage.text = strings[i].text;
            StaticMessage.rect.width = strings[i].length;
            StaticMessage.rect.y = TEXT_ROW + i;
            memcpy( &controls_info[i], &StaticMessage, sizeof( gui_control_info ) );
            end = TEXT_START_COL + controls_info[i].rect.width;
            if( cols < end ) {
                cols = end;
            }
        }
    }
    rows = num_string_controls + BUTTON_ROW + 1;

    /* adjust the vertical position of buttons and icons according to the
     * number of lines of text ( also adjusts cols )
     */
    num_buttons = AdjustVert( &cols, controls_to_use, controls_info, num_controls, num_string_controls );

    if( cols < ( num_buttons * ( BUTTON_WIDTH + 4 ) ) ) {
        cols = num_buttons * ( BUTTON_WIDTH + 4 ) ;
    }
    /* centre the buttons horizontally */
    CentreButtons( cols, num_buttons, controls_info, num_controls );
    ret = GUI_RET_CANCEL; /* default -- if escape hit */
    GUIDlgOpen( title, rows, cols, controls_info, num_controls, &DisplayMessageGUIEventProc, &ret );
    // deallocate used memory
    freeStringControls( num_string_controls, strings );
    GUIMemFree( controls_info );
    return( ret );
}
