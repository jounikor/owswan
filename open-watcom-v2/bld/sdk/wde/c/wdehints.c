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


#include "wdeglbl.h"
#include "wdestat.h"
#include "wdemain.h"
#include "wde.rh"
#include "wdehints.h"
#include "wdemsgbx.h"
#include "wrdll.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define MAX_NESTED_POPUPS 2

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct {
    int         id;
    msg_id      hint;
} WdeHintItem;

typedef struct {
    int         loc[MAX_NESTED_POPUPS];
    HMENU       hpopup;
    msg_id      hint;
} WdePopupHintItem;

typedef struct {
    int                 num;
    HMENU               hmenu;
    WdePopupHintItem    *hint_items;
} WdePopupListItem;

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static WdeHintItem      *WdeGetHintItem( ctl_id id );
static void             WdeHandlePopupHint( HMENU, HMENU );
static msg_id           WdeGetPopupHint( WdePopupListItem *, HMENU );
static WdePopupListItem *WdeFindPopupListItem( HMENU hmenu );
static bool             WdeCreateWdePopupListItem( int, HMENU, WdePopupHintItem * );
static bool             WdeInitHintItems( int, HMENU, WdePopupHintItem * );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static LIST  *WdePopupList = NULL;

static WdeHintItem WdeHints[] = {
    { IDM_NEW_RES,              WDE_HINT_NEW_RES            },
    { IDM_OPEN_RES,             WDE_HINT_OPEN_RES           },
    { IDM_SAVE_RES,             WDE_HINT_SAVE_RES           },
    { IDM_SAVEAS_RES,           WDE_HINT_SAVEAS_RES         },
    { IDM_MS_CUSTOM,            WDE_HINT_MS_CUSTOM          },
    { IDM_BOR_CUSTOM,           WDE_HINT_BOR_CUSTOM         },
    { IDM_LOADLIB,              WDE_HINT_LOADLIB            },
    { IDM_SELCUST1,             WDE_HINT_SELCUST1           },
    { IDM_SELCUST2,             WDE_HINT_SELCUST2           },
    { IDM_LOAD_SYMBOLS,         WDE_HINT_LOAD_SYMBOLS       },
    { IDM_VIEW_SYMBOLS,         WDE_HINT_VIEW_SYMBOLS       },
    { IDM_WRITE_SYMBOLS,        WDE_HINT_WRITE_SYMBOLS      },
    { IDM_EXIT,                 WDE_HINT_EXIT               },
    { IDM_SAME_WIDTH,           WDE_HINT_SAME_WIDTH         },
    { IDM_SAME_HEIGHT,          WDE_HINT_SAME_HEIGHT        },
    { IDM_SAME_SIZE,            WDE_HINT_SAME_SIZE          },
    { IDM_SPACE_HORZ,           WDE_HINT_SPACE_HORZ         },
    { IDM_SPACE_VERT,           WDE_HINT_SPACE_VERT         },
    { IDM_SIZETOTEXT,           WDE_HINT_SIZETOTEXT         },
    { IDM_GOTO_OBJECT,          WDE_HINT_GOTO_OBJECT        },
    { IDM_DEFINEOBJECT,         WDE_HINT_DEFINEOBJECT       },
    { IDM_GEN_DEFINEOBJECT,     WDE_HINT_GEN_DEFINEOBJECT   },
    { IDM_OPTIONS,              WDE_HINT_OPTIONS            },
    { IDM_SELECT_DIALOG,        WDE_HINT_SELECT_DIALOG      },
    { IDM_REMOVE_DIALOG,        WDE_HINT_REMOVE_DIALOG      },
    { IDM_DIALOG_RESTORE,       WDE_HINT_DIALOG_RESTORE     },
    { IDM_DIALOG_NEW,           WDE_HINT_DIALOG_NEW         },
    { IDM_DIALOG_SAVE,          WDE_HINT_DIALOG_SAVE        },
    { IDM_DIALOG_SAVEAS,        WDE_HINT_DIALOG_SAVEAS      },
    { IDM_DIALOG_SAVEINTO,      WDE_HINT_DIALOG_SAVEINTO    },
    { IDM_TEST_MODE,            WDE_HINT_TEST_MODE          },
    { IDM_SET_ORDER,            WDE_HINT_SET_ORDER          },
    { IDM_SET_TABS,             WDE_HINT_SET_TABS           },
    { IDM_SET_GROUPS,           WDE_HINT_SET_GROUPS         },
    { IDM_SELECT_MODE,          WDE_HINT_SELECT_MODE        },
    { IDM_DIALOG_TOOL,          WDE_HINT_DIALOG_TOOL        },
    { IDM_PBUTTON_TOOL,         WDE_HINT_PBUTTON_TOOL       },
    { IDM_CBUTTON_TOOL,         WDE_HINT_CBUTTON_TOOL       },
    { IDM_RBUTTON_TOOL,         WDE_HINT_RBUTTON_TOOL       },
    { IDM_GBUTTON_TOOL,         WDE_HINT_GBUTTON_TOOL       },
    { IDM_FRAME_TOOL,           WDE_HINT_FRAME_TOOL         },
    { IDM_TEXT_TOOL,            WDE_HINT_TEXT_TOOL          },
    { IDM_ICON_TOOL,            WDE_HINT_ICON_TOOL          },
    { IDM_EDIT_TOOL,            WDE_HINT_EDIT_TOOL          },
    { IDM_LISTBOX_TOOL,         WDE_HINT_LISTBOX_TOOL       },
    { IDM_COMBOBOX_TOOL,        WDE_HINT_COMBOBOX_TOOL      },
    { IDM_HSCROLL_TOOL,         WDE_HINT_HSCROLL_TOOL       },
    { IDM_VSCROLL_TOOL,         WDE_HINT_VSCROLL_TOOL       },
    { IDM_SIZEBOX_TOOL,         WDE_HINT_SIZEBOX_TOOL       },
    { IDM_STATUSBAR_TOOL,       WDE_HINT_STATUSBAR_TOOL     },
    { IDM_LISTVIEW_TOOL,        WDE_HINT_LISTVIEW_TOOL      },
    { IDM_TREEVIEW_TOOL,        WDE_HINT_TREEVIEW_TOOL      },
    { IDM_TABCNTL_TOOL,         WDE_HINT_TABCNTL_TOOL       },
    { IDM_ANIMATE_TOOL,         WDE_HINT_ANIMATE_TOOL       },
    { IDM_UPDOWN_TOOL,          WDE_HINT_UPDOWN_TOOL        },
    { IDM_TRACKBAR_TOOL,        WDE_HINT_TRACKBAR_TOOL      },
    { IDM_PROGRESS_TOOL,        WDE_HINT_PROGRESS_TOOL      },
    { IDM_HOTKEY_TOOL,          WDE_HINT_HOTKEY_TOOL        },
    { IDM_HEADER_TOOL,          WDE_HINT_HEADER_TOOL        },
    { IDM_CUSTOM1_TOOL,         WDE_HINT_CUSTOM1_TOOL       },
    { IDM_CUSTOM2_TOOL,         WDE_HINT_CUSTOM2_TOOL       },
    { IDM_STICKY_TOOLS,         WDE_HINT_STICKY_TOOLS       },
    { IDM_SHOW_TOOLS,           WDE_HINT_SHOW_TOOLS         },
    { IDM_SHOW_RIBBON,          WDE_HINT_SHOW_RIBBON        },
    { IDM_MDI_CASCADE,          WDE_HINT_MDI_CASCADE        },
    { IDM_MDI_TILEH,            WDE_HINT_MDI_TILEH          },
    { IDM_MDI_TILEV,            WDE_HINT_MDI_TILEV          },
    { IDM_MDI_ARRANGE,          WDE_HINT_MDI_ARRANGE        },
    { IDM_ABOUT,                WDE_HINT_ABOUT              },
    { IDM_HELP,                 WDE_HINT_HELP               },
    { IDM_HELP_SEARCH,          WDE_HINT_HELP_SEARCH        },
    { IDM_HELP_ON_HELP,         WDE_HINT_HELP_ON_HELP       },
    { IDM_DELETEOBJECT,         WDE_HINT_DELETEOBJECT       },
    { IDM_CUTOBJECT,            WDE_HINT_CUTOBJECT          },
    { IDM_PASTEOBJECT,          WDE_HINT_PASTEOBJECT        },
    { IDM_COPYOBJECT,           WDE_HINT_COPYOBJECT         },
    { IDM_FMLEFT,               WDE_HINT_FMLEFT             },
    { IDM_FMHCENTRE,            WDE_HINT_FMHCENTRE          },
    { IDM_FMRIGHT,              WDE_HINT_FMRIGHT            },
    { IDM_FMTOP,                WDE_HINT_FMTOP              },
    { IDM_FMVCENTRE,            WDE_HINT_FMVCENTRE          },
    { IDM_FMBOTTOM,             WDE_HINT_FMBOTTOM           },
    { IDM_DDE_CLEAR,            WDE_HINT_DDE_CLEAR          },
    { IDM_DDE_UPDATE_PRJ,       WDE_HINT_DDE_UPDATE_PRJ     },
    { -1,                       0                           }
};

static WdePopupHintItem WdeInitialPopupHints[] = {
    { { 0, -1 },  NULL, WDE_HINT_FILEMENU               },
    { { 0, 3  },  NULL, WDE_HINT_LOADCUSTMENU           },
    { { 1, -1 },  NULL, WDE_HINT_WINDOWMENU             },
    { { 2, -1 },  NULL, WDE_HINT_HELPMENU               }
};

static WdePopupHintItem WdeResPopupHints[] = {
    { { 0, -1 },  NULL, WDE_HINT_FILEMENU               },
    { { 0, 5  },  NULL, WDE_HINT_LOADCUSTMENU           },
    { { 1, -1 },  NULL, WDE_HINT_EDITMENU               },
    { { 1, 5  },  NULL, WDE_HINT_ALIGNMENU              },
    { { 1, 6  },  NULL, WDE_HINT_SIZEMENU               },
    { { 1, 7  },  NULL, WDE_HINT_SPACEMENU              },
    { { 1, 15 },  NULL, WDE_HINT_EDITSYMMENU            },
    { { 2, -1 },  NULL, WDE_HINT_DIALOGMENU             },
    { { 3, -1 },  NULL, WDE_HINT_CONTROLMENU            },
    { { 3, 19 },  NULL, WDE_HINT_COMMCONTROLMENU        },
    { { 4, -1 },  NULL, WDE_HINT_WINDOWMENU             },
    { { 5, -1 },  NULL, WDE_HINT_HELPMENU               },
};

static WdePopupHintItem WdeDDEPopupHints[] = {
    { { 0, -1 },  NULL, WDE_HINT_DDEFILEMENU            },
    { { 0, 5  },  NULL, WDE_HINT_LOADCUSTMENU           },
    { { 1, -1 },  NULL, WDE_HINT_EDITMENU               },
    { { 1, 5  },  NULL, WDE_HINT_ALIGNMENU              },
    { { 1, 6  },  NULL, WDE_HINT_SIZEMENU               },
    { { 1, 15 },  NULL, WDE_HINT_EDITSYMMENU            },
    { { 2, -1 },  NULL, WDE_HINT_DIALOGMENU             },
    { { 3, -1 },  NULL, WDE_HINT_CONTROLMENU            },
    { { 4, -1 },  NULL, WDE_HINT_WINDOWMENU             },
    { { 5, -1 },  NULL, WDE_HINT_HELPMENU               }
};

void WdeHandleMenuSelect( WPARAM wParam, LPARAM lParam )
{
    HMENU   hmenu;
    HMENU   hpopup;
    WORD    flags;

    if( MENU_CLOSED( wParam, lParam ) ) {
        WdeSetStatusText( NULL, "", true );
    } else {
        hmenu = WdeGetMenuHandle();
        flags = GET_WM_MENUSELECT_FLAGS( wParam, lParam );
        if( flags & (MF_SYSMENU | MF_SEPARATOR) ) {
            WdeSetStatusText( NULL, "", true );
        } else if( flags & MF_POPUP ) {
#ifdef __NT__
            hpopup = GetSubMenu( (HMENU)lParam, GET_WM_MENUSELECT_ITEM( wParam, lParam ) );
#else
            hpopup = (HMENU)GET_WM_MENUSELECT_ITEM( wParam, lParam );
#endif
            WdeHandlePopupHint( hmenu, hpopup );
        } else {
            WdeDisplayHint( GET_WM_MENUSELECT_ITEM( wParam, lParam ) );
        }
    }
}

void WdeDisplayHint( ctl_id id )
{
    char        *buf;
    char        *mditext;
    WdeHintItem *hint;

    if( id < WDE_MDI_FIRST ) {
        hint = WdeGetHintItem( id );
        if( hint != NULL ) {
            WdeSetStatusByID( 0, hint->hint );
        }
    } else {
        mditext = WdeAllocRCString( WDE_HINT_MDIMSG );
        if( mditext != NULL ) {
            buf = WRMemAlloc( strlen( mditext ) + 20 + 1 );
            if( buf != NULL ) {
                sprintf( buf, mditext, WDE_MDI_FIRST + 1 - id );
                WdeSetStatusText( NULL, buf, true );
                WRMemFree( buf );
            }
            WdeFreeRCString( mditext );
        }
    }
}

WdeHintItem *WdeGetHintItem( ctl_id id )
{
    int i;

    for( i = 0; WdeHints[i].id != -1; i++ ) {
        if( WdeHints[i].id == id ) {
            return( &WdeHints[i] );
        }
    }

    return( NULL );
}

WdePopupListItem *WdeFindPopupListItem( HMENU hmenu )
{
    LIST             *plist;
    WdePopupListItem *p;

    for( plist = WdePopupList; plist != NULL; plist = ListNext( plist ) ) {
        p = (WdePopupListItem *)ListElement( plist );
        if( p->hmenu == hmenu ) {
            return( p );
        }
    }

    return( NULL );
}

msg_id WdeGetPopupHint( WdePopupListItem *p, HMENU hpopup )
{
    int i;

    for( i = 0; i < p->num; i++ ) {
        if( p->hint_items[i].hpopup == hpopup ) {
            return( p->hint_items[i].hint );
        }
    }

    return( 0 );
}

void WdeHandlePopupHint( HMENU hmenu, HMENU hpopup )
{
    WdePopupListItem    *p;
    msg_id              hint;

    hint = 0;

    p = WdeFindPopupListItem( hmenu );

    if( p != NULL ) {
        hint = WdeGetPopupHint( p, hpopup );
    }

    if( hint > 0 ) {
        WdeSetStatusByID( 0, hint );
    } else {
        WdeSetStatusText( NULL, "", true );
    }
}

bool WdeInitHints( void )
{
    bool ret;

    ret = TRUE;

    if( !WdeIsDDE() ) {
        ret = WdeCreateWdePopupListItem( 4, WdeGetInitialMenuHandle(),
                                         WdeInitialPopupHints );
    }

    if( ret ) {
        if( WdeIsDDE() ) {
            ret = WdeCreateWdePopupListItem( 10, WdeGetResMenuHandle(),
                                             WdeDDEPopupHints );
        } else {
            ret = WdeCreateWdePopupListItem( 10, WdeGetResMenuHandle(),
                                             WdeResPopupHints );
        }
    }

    return( ret );
}

void WdeFiniHints( void )
{
    LIST             *plist;
    WdePopupListItem *p;

    for ( plist = WdePopupList; plist != NULL; plist = ListConsume ( plist ) ) {
        p = (WdePopupListItem *)ListElement( plist );
        WRMemFree( p );
    }
}

bool WdeCreateWdePopupListItem( int num, HMENU hmenu, WdePopupHintItem *hint_items )
{
    WdePopupListItem *p;

    p = (WdePopupListItem *)WRMemAlloc( sizeof( WdePopupListItem ) );

    if( p != NULL ) {
        p->num = num;
        p->hmenu = hmenu;
        p->hint_items = hint_items;
        if( WdeInitHintItems( num, hmenu, hint_items ) ) {
            ListAddElt( &WdePopupList, (OBJPTR)p );
        } else {
            WRMemFree( p );
            return( FALSE );
        }
    } else {
        return( FALSE );
    }

    return( TRUE );
}

bool WdeInitHintItems( int num, HMENU hmenu, WdePopupHintItem *hint_items )
{
    int     i;
    int     j;
    HMENU   hpopup;

    for( i = 0; i < num; i++ ) {
        hpopup = hmenu;
        for( j = 0; j < MAX_NESTED_POPUPS && hint_items[i].loc[j] != -1; j++ ) {
            hpopup = GetSubMenu( hpopup, hint_items[i].loc[j] );
        }
        hint_items[i].hpopup = hpopup;
    }

    return( TRUE );
}
