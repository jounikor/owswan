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
* Description:  BIOS-style functions for *nix
*
****************************************************************************/


#include "vi.h"
#include "win.h"
#include "stdui.h"
#include "uiextrn.h"
#include "vibios.h"


extern int      PageCnt;

void    BIOSGetColorPalette( void *a )
{
    /* unused parameters */ (void)a;
}

uint_32 BIOSGetColorRegister( unsigned short a )
{
    /* unused parameters */ (void)a;

    return( 0 );
}

void    BIOSSetBlinkAttr( unsigned char on )
{
    /* unused parameters */ (void)on;
}

void    BIOSSetColorRegister( unsigned short reg, unsigned char r, unsigned char g, unsigned char b )
{
    /* unused parameters */ (void)reg; (void)r; (void)g; (void)b;
}

void    BIOSSetCursorPos( unsigned char page, unsigned char row, unsigned char col )
{
    /* unused parameters */ (void)page;

    TermSetCursor( row, col );
}

unsigned short BIOSGetCursorPos( unsigned char page )
{
    CURSORORD   row;
    CURSORORD   col;

    /* unused parameters */ (void)page;

    TermGetCursor( &row, &col );
    return( ( (row & 0xFF) << 8 ) | (col & 0xFF) );
}

static unsigned short vi_keys[EV_FIRST_UNUSED];

struct map {
    ui_event    ui_ev;
    vi_key      key;
};

static struct map events[] = {
    { EV_ESCAPE,                VI_KEY( ESC )           },
    { EV_ENTER,                 VI_KEY( ENTER )         },
    { EV_RUB_OUT,               VI_KEY( BS )            },
    { EV_TAB_FORWARD,           VI_KEY( TAB )           },
    { EV_CTRL_TAB,              VI_KEY( CTRL_TAB )      },
    { EV_CURSOR_LEFT,           VI_KEY( LEFT )          },
    { EV_CURSOR_RIGHT,          VI_KEY( RIGHT )         },
    { EV_CTRL_CURSOR_LEFT,      VI_KEY( CTRL_LEFT )     },
    { EV_CTRL_CURSOR_RIGHT,     VI_KEY( CTRL_RIGHT )    },
    { EV_SHIFT_CURSOR_LEFT,     VI_KEY( SHIFT_LEFT )    },
    { EV_SHIFT_CURSOR_RIGHT,    VI_KEY( SHIFT_RIGHT )   },
    { EV_ALT_CURSOR_LEFT,       VI_KEY( ALT_LEFT )      },
    { EV_ALT_CURSOR_RIGHT,      VI_KEY( ALT_RIGHT )     },
    { EV_CURSOR_UP,             VI_KEY( UP )            },
    { EV_CURSOR_DOWN,           VI_KEY( DOWN )          },
    { EV_SHIFT_CURSOR_UP,       VI_KEY( SHIFT_UP )      },
    { EV_SHIFT_CURSOR_DOWN,     VI_KEY( SHIFT_DOWN )    },
    { EV_CTRL_CURSOR_UP,        VI_KEY( CTRL_UP )       },
    { EV_CTRL_CURSOR_DOWN,      VI_KEY( CTRL_DOWN )     },
    { EV_ALT_CURSOR_UP,         VI_KEY( ALT_UP )        },
    { EV_ALT_CURSOR_DOWN,       VI_KEY( ALT_DOWN )      },
    { EV_HOME,                  VI_KEY( HOME )          },
    { EV_END,                   VI_KEY( END )           },
    { EV_SHIFT_HOME,            VI_KEY( SHIFT_HOME )    },
    { EV_SHIFT_END,             VI_KEY( SHIFT_END )     },
    { EV_CTRL_HOME,             VI_KEY( CTRL_HOME )     },
    { EV_CTRL_END,              VI_KEY( CTRL_END )      },
    { EV_ALT_HOME,              VI_KEY( ALT_HOME )      },
    { EV_ALT_END,               VI_KEY( ALT_END )       },
    { EV_DELETE,                VI_KEY( DEL )           },
    { EV_INSERT,                VI_KEY( INS )           },
    { EV_CTRL_DELETE,           VI_KEY( CTRL_DEL )      },
    { EV_CTRL_INSERT,           VI_KEY( CTRL_INS )      },
    { EV_ALT_DELETE,            VI_KEY( ALT_DEL )       },
    { EV_ALT_INSERT,            VI_KEY( ALT_INS )       },
    { EV_PAGE_UP,               VI_KEY( PAGEUP )        },
    { EV_PAGE_DOWN,             VI_KEY( PAGEDOWN )      },
    { EV_SHIFT_PAGE_UP,         VI_KEY( SHIFT_PAGEUP )  },
    { EV_SHIFT_PAGE_DOWN,       VI_KEY( SHIFT_PAGEDOWN )},
    { EV_CTRL_PAGE_UP,          VI_KEY( CTRL_PAGEUP )   },
    { EV_CTRL_PAGE_DOWN,        VI_KEY( CTRL_PAGEDOWN ) },
    { EV_ALT_PAGE_UP,           VI_KEY( CTRL_PAGEUP )   },
    { EV_ALT_PAGE_DOWN,         VI_KEY( CTRL_PAGEDOWN ) },
    { EV_F11,                   VI_KEY( F11 )           },
    { EV_F12,                   VI_KEY( F12 )           },
    { EV_SHIFT_F11,             VI_KEY( SHIFT_F11 )     },
    { EV_SHIFT_F12,             VI_KEY( SHIFT_F12 )     },
    { EV_CTRL_F11,              VI_KEY( CTRL_F11 )      },
    { EV_CTRL_F12,              VI_KEY( CTRL_F12 )      },
    { EV_ALT_F11,               VI_KEY( ALT_F11 )       },
    { EV_ALT_F12,               VI_KEY( ALT_F12 )       },
    { EV_ALT_A,                 VI_KEY( ALT_A )         },
    { EV_ALT_B,                 VI_KEY( ALT_B )         },
    { EV_ALT_C,                 VI_KEY( ALT_C )         },
    { EV_ALT_D,                 VI_KEY( ALT_D )         },
    { EV_ALT_E,                 VI_KEY( ALT_E )         },
    { EV_ALT_F,                 VI_KEY( ALT_F )         },
    { EV_ALT_G,                 VI_KEY( ALT_G )         },
    { EV_ALT_H,                 VI_KEY( ALT_H )         },
    { EV_ALT_I,                 VI_KEY( ALT_I )         },
    { EV_ALT_J,                 VI_KEY( ALT_J )         },
    { EV_ALT_K,                 VI_KEY( ALT_K )         },
    { EV_ALT_L,                 VI_KEY( ALT_L )         },
    { EV_ALT_M,                 VI_KEY( ALT_M )         },
    { EV_ALT_N,                 VI_KEY( ALT_N )         },
    { EV_ALT_O,                 VI_KEY( ALT_O )         },
    { EV_ALT_P,                 VI_KEY( ALT_P )         },
    { EV_ALT_Q,                 VI_KEY( ALT_Q )         },
    { EV_ALT_R,                 VI_KEY( ALT_R )         },
    { EV_ALT_S,                 VI_KEY( ALT_S )         },
    { EV_ALT_T,                 VI_KEY( ALT_T )         },
    { EV_ALT_U,                 VI_KEY( ALT_U )         },
    { EV_ALT_V,                 VI_KEY( ALT_V )         },
    { EV_ALT_W,                 VI_KEY( ALT_W )         },
    { EV_ALT_X,                 VI_KEY( ALT_X )         },
    { EV_ALT_Y,                 VI_KEY( ALT_Y )         },
    { EV_ALT_Z,                 VI_KEY( ALT_Z )         },
    { EV_MOUSE_PRESS,           VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_PRESS_R,         VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_PRESS_M,         VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_RELEASE,         VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_RELEASE_R,       VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_RELEASE_M,       VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_DRAG,            VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_DRAG_R,          VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_DRAG_M,          VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_HOLD,            VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_HOLD_R,          VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_HOLD_M,          VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_DCLICK,          VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_DCLICK_R,        VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_DCLICK_M,        VI_KEY( MOUSEEVENT )    },
    { EV_MOUSE_MOVE,            VI_KEY( MOUSEEVENT )    }
};

static vi_key get_vi_key( ui_event ui_ev )
{
    if( iseditchar( ui_ev ) ) {
        return( (vi_key)ui_ev );
    }
    if( ui_ev >= EV_F1 && ui_ev <= EV_F10 ) {
        return( VI_KEY( F1 ) + ( ui_ev - EV_F1 ) );
    }
    if( ui_ev >= EV_SHIFT_F1 && ui_ev <= EV_SHIFT_F10 ) {
        return( VI_KEY( SHIFT_F1 ) + ( ui_ev - EV_SHIFT_F1 ) );
    }
    if( ui_ev >= EV_CTRL_F1 && ui_ev <= EV_CTRL_F10 ) {
        return( VI_KEY( CTRL_F1 ) + ( ui_ev - EV_CTRL_F1 ) );
    }
    if( ui_ev >= EV_ALT_F1 && ui_ev <= EV_ALT_F10 ) {
        return( VI_KEY( ALT_F1 ) + ( ui_ev - EV_ALT_F1 ) );
    }
    return( VI_KEY( DUMMY ) );
}

int BIOSKeyboardInit( void )
{
    int i;

    for( i = 0; i < EV_FIRST_UNUSED; i++ ) {
        vi_keys[i] = get_vi_key( i );
    }
    for( i = 0; i < sizeof( events ) / sizeof( struct map ); i++ ) {
        vi_keys[events[i].ui_ev] = events[i].key;
    }
    return( 0 );
}

/*
 * BIOSGetKeyboard - get a keyboard char
 */
unsigned BIOSGetKeyboard( unsigned *scan )
{
    vi_key      key;
    ui_event    ui_ev;

    key = VI_KEY( DUMMY );
    do {
        ui_ev = uieventsource( 0 );
        if( ui_ev < EV_FIRST_UNUSED ) {
            key = vi_keys[ui_ev];
        }
    } while( key == VI_KEY( DUMMY ) );
    if( scan != NULL ) {
        *scan = 0;
    }
    return( key );

} /* BIOSGetKeyboard */

/*
 * BIOSKeyboardHit - test for keyboard hit
 */
bool BIOSKeyboardHit( void )
{
    return( TermKeyboardHit() );

} /* BIOSKeyboardHit */

/*
 * BIOSUpdateScreen - update the screen
 */
void  BIOSUpdateScreen( size_t offset, unsigned nchars )
{
    SAREA       area;

    if( PageCnt > 0 || EditFlags.Quiet ) {
        return;
    }

    if( nchars == EditVars.WindMaxWidth * EditVars.WindMaxHeight ) {
        TermRefresh( NULL );
    } else {
        area.row = offset / EditVars.WindMaxWidth;
        area.col = offset % EditVars.WindMaxWidth;
        area.width = nchars;
        area.height = 1;
        TermRefresh( &area );
    }

} /* BIOSUpdateScreen */
