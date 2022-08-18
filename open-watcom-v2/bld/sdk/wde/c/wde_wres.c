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


#include "wdeglbl.h"
#include "wdesdup.h"
#include "wdeobjid.h"
#include "wdecctl.h"
#include "wde_wres.h"

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static char     *WdeGetClassNameFromClass( uint_8 );

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct {
    uint_8  class;
    char   *class_name;
} WdeControlClassItems;

typedef struct {
    OBJ_ID  class;
    char   *class_name;
} WdeCommonControlItems;

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WdeControlClassItems WdeControlClasses[] = {
    { CLASS_BUTTON,     "BUTTON"    },
    { CLASS_EDIT,       "EDIT"      },
    { CLASS_STATIC,     "STATIC"    },
    { CLASS_LISTBOX,    "LISTBOX"   },
    { CLASS_SCROLLBAR,  "SCROLLBAR" },
    { CLASS_COMBOBOX,   "COMBOBOX"  },
    { 0x00,             NULL        }
};

static WdeCommonControlItems WdeCommonControlClasses[] = {
    { SBAR_OBJ,         STATUSCLASSNAME       },
    { LVIEW_OBJ,        WC_LISTVIEW           },
    { TVIEW_OBJ,        WC_TREEVIEW           },
    { TABCNTL_OBJ,      WC_TABCONTROL         },
    { ANIMATE_OBJ,      ANIMATE_CLASS         },
    { UPDOWN_OBJ,       UPDOWN_CLASS          },
    { TRACKBAR_OBJ,     TRACKBAR_CLASS        },
    { PROGRESS_OBJ,     PROGRESS_CLASS        },
    { HOTKEY_OBJ,       HOTKEY_CLASS          },
    { HEADER_OBJ,       WC_HEADER             },
    { 0,                NULL                  }
};


ResNameOrOrdinal *WdeStrToResNameOrOrdinal( char *str )
{
    ResNameOrOrdinal    *rp;
    uint_16             ordID;
    unsigned long       ul;
    char                *ep;

    if( str == NULL ) {
        return( NULL );
    }

    if( str[0] == '\0' ) {
        return( ResStrToNameOrOrd( str ) );
    }

    ul = strtoul( str, &ep, 0 );
    if( !*ep && ul <= 0xffff ) {
        ordID = (uint_16)ul;
        rp = ResNumToNameOrOrd( ordID );
    } else {
        rp = ResStrToNameOrOrd( str );
    }

    return( rp );
}

char *WdeResNameOrOrdinalToStr( ResNameOrOrdinal *name, int base )
{
    char    temp[15];
    char    *cp;

    cp = NULL;

    if( name != NULL ) {
        if( name->ord.fFlag == 0xff ) {
            sprintf( temp, ( base == 10 ) ? "%u" : "%x", name->ord.wOrdinalID );
            cp = WdeStrDup( temp );
        } else {
            cp = (char *)WdeStrDup( name->name );
        }
        if( cp == NULL ) {
            return( NULL );
        }
    }

    return( cp );
}

ControlClass *WdeStrToControlClass( char *str )
{
    ControlClass    *c;
    size_t          slen;

    slen = strlen( str );

    c = (ControlClass *)WRMemAlloc( sizeof( ControlClass ) + slen );

    if( c != NULL ) {
        memcpy( c->ClassName, str, slen + 1 );
    }

    return( c );
}

char *WdeControlClassToStr( ControlClass *name )
{
    char    *class_name;
    char    *cp;
    char    temp[35];

    cp = NULL;

    if( name != NULL ) {
        if( name->Class & 0x80 ) {
            class_name = WdeGetClassNameFromClass( name->Class );
            if( class_name == NULL ) {
                sprintf( temp, "%lu", name->Class );
                cp = WdeStrDup( temp );
                return( cp );
            }
        } else {
            class_name = name->ClassName;
        }
        if( class_name != NULL && class_name[0] != '\0' ) {
            cp = WdeStrDup( class_name );
        }
    }

    return( cp );
}

WdeDialogBoxControl *WdeCopyDialogBoxControl( WdeDialogBoxControl *src )
{
    WdeDialogBoxControl *dest;

    if( src == NULL ) {
        return( NULL );
    }

    dest = WdeAllocDialogBoxControl();
    if( dest == NULL ) {
        return( NULL );
    }

    memcpy( dest, src, sizeof( WdeDialogBoxControl ) );

    SETCTL_CLASSID( dest, WdeCopyControlClass( GETCTL_CLASSID( src ) ) );
    SETCTL_TEXT( dest, WdeCopyResNameOr( GETCTL_TEXT( src ) ) );

    dest->symbol = WdeStrDup( src->symbol );
    dest->helpsymbol = WdeStrDup( src->helpsymbol );

    return( dest );
}

WdeDialogBoxHeader *WdeCopyDialogBoxHeader( WdeDialogBoxHeader *src )
{
    WdeDialogBoxHeader *dest;

    if( src == NULL ) {
        return( NULL );
    }

    dest = WdeAllocDialogBoxHeader();
    if( dest == NULL ) {
        return( NULL );
    }

    memcpy( dest, src, sizeof( WdeDialogBoxHeader ) );

    SETHDR_MENUNAME( dest, WdeCopyResNameOr( GETHDR_MENUNAME( src ) ) );
    SETHDR_CLASSNAME( dest, WdeCopyResNameOr( GETHDR_CLASSNAME( src ) ) );
    SETHDR_CAPTION( dest, WdeStrDup( GETHDR_CAPTION( src ) ) );
    SETHDR_FONTFACENAME( dest,  WdeStrDup( GETHDR_FONTFACENAME( src ) ) );

    dest->symbol = WdeStrDup( src->symbol );
    dest->helpsymbol = WdeStrDup( src->helpsymbol );

    return( dest );
}


WResID *WdeCopyWResID( WResID *src )
{
    WResID  *dest;
    size_t  len;

    if( src == NULL ) {
        return ( NULL );
    }

    len = sizeof( WResID );

    if( src->IsName ) {
        len += src->ID.Name.NumChars - 1;
    }

    dest = (WResID *)WRMemAlloc( len );

    if( dest != NULL ) {
        memcpy( dest, src, len );
    }

    return( dest );
}

WResHelpID *WdeCopyWResHelpID( WResHelpID *src )
{
    WResHelpID  *dest;
    size_t      len;

    if( src == NULL ) {
        return( NULL );
    }

    len = sizeof( WResHelpID );

    if( src->IsName ) {
        len += src->ID.Name.NumChars - 1;
    }

    dest = (WResHelpID *)WRMemAlloc( len );

    if( dest != NULL ) {
        memcpy ( dest, src, len );
    }

    return( dest );
}

ResNameOrOrdinal *WdeCopyResNameOr( ResNameOrOrdinal *src )
{
    ResNameOrOrdinal *dest;

    if( src == NULL ) {
        return( NULL );
    }

    if( src->ord.fFlag == 0xff ) {
        dest = ResNumToNameOrOrd( src->ord.wOrdinalID );
    } else {
        dest = ResStrToNameOrOrd( src->name );
    }

    return( dest );
}

ControlClass *WdeCopyControlClass( ControlClass *src )
{
    ControlClass *dest;

    if( src == NULL ) {
        return( NULL );
    }

    if( src->Class & 0x80 ) {
        dest = ResNumToControlClass( src->Class );
    } else {
        dest = WdeStrToControlClass( src->ClassName );
    }

    return( dest );
}

void WdeFreeDialogBoxControl( WdeDialogBoxControl **c )
{
    if( c != NULL && *c != NULL ) {
        if( GETCTL_CLASSID( *c ) ) {
            WRMemFree( GETCTL_CLASSID( *c ) );
        }
        if( GETCTL_TEXT( *c ) ) {
            WRMemFree( GETCTL_TEXT( *c ) );
        }
        if( (*c)->symbol ) {
            WRMemFree( (*c)->symbol );
        }
        if( (*c)->helpsymbol ) {
            WRMemFree( (*c)->helpsymbol );
        }
        WRMemFree( *c );
        *c = NULL;
    }
}

WdeDialogBoxControl *WdeAllocDialogBoxControl( void )
{
    WdeDialogBoxControl *c;

    c = (WdeDialogBoxControl *)WRMemAlloc( sizeof( WdeDialogBoxControl ) );
    if( c == NULL ) {
        return( NULL );
    }
    memset( c, 0, sizeof( WdeDialogBoxControl ) );

    return( c );
}

void WdeFreeDialogBoxHeader( WdeDialogBoxHeader **c )
{
    if( c != NULL && *c != NULL ) {
        if( GETHDR_MENUNAME( *c ) ) {
            WRMemFree( GETHDR_MENUNAME( *c ) );
        }
        if( GETHDR_CLASSNAME( *c ) ) {
            WRMemFree( GETHDR_CLASSNAME( *c ) );
        }
        if( GETHDR_CAPTION( *c ) ) {
            WRMemFree( GETHDR_CAPTION( *c ) );
        }
        if( GETHDR_FONTFACENAME( *c ) ) {
            WRMemFree( GETHDR_FONTFACENAME( *c ) );
        }
        if( (*c)->symbol ) {
            WRMemFree( (*c)->symbol );
        }
        WRMemFree( *c );
        *c = NULL;
    }
}

WdeDialogBoxHeader *WdeAllocDialogBoxHeader( void )
{
    WdeDialogBoxHeader *c;

    c = (WdeDialogBoxHeader *)WRMemAlloc( sizeof( WdeDialogBoxHeader ) );
    if( c == NULL ) {
        return( NULL );
    }

    memset( c, 0, sizeof( WdeDialogBoxHeader ) );

    return( c );
}

char *WdeGetClassNameFromClass( uint_8 class )
{
    int i;

    for( i = 0; WdeControlClasses[i].class != 0x00; i++ ) {
        if( WdeControlClasses[i].class == class ) {
            return ( WdeControlClasses[i].class_name );
        }
    }
    return ( NULL );
}

uint_8 WdeGetClassFromClassName( char *class_name )
{
    int i;

    for( i = 0; WdeControlClasses[i].class != 0x00; i++ ) {
        if( stricmp( WdeControlClasses[i].class_name, class_name ) == 0 ) {
            return ( WdeControlClasses[i].class );
        }
    }

    return( 0x00 );
}

OBJ_ID WdeGetCommonControlClassFromClassName( char *class_name )
{
    int i;

    for( i = 0; WdeCommonControlClasses[i].class != 0x00; i++ ) {
        if( stricmp( WdeCommonControlClasses[i].class_name, class_name ) == 0 ) {
            return( WdeCommonControlClasses[i].class );
        }
    }

    return( 0 );
}
