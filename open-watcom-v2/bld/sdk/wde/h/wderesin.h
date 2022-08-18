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


#ifndef WDERESIN_INCLUDED
#define WDERESIN_INCLUDED

#include "wrdll.h"
#include "wresall.h"
#include "wdehash.h"

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define GETHDR_STYLE( ph )              ((ph)->Style)
#define GETHDR_NUMITEMS( ph )           ((ph)->NumOfItems)
#define GETHDR_SIZE( ph )               ((ph)->SizeInfo)
#define GETHDR_PSIZE( ph )              (&(ph)->SizeInfo)
#define GETHDR_SIZEX( ph )              ((ph)->SizeInfo.x)
#define GETHDR_SIZEY( ph )              ((ph)->SizeInfo.y)
#define GETHDR_SIZEW( ph )              ((ph)->SizeInfo.width)
#define GETHDR_SIZEH( ph )              ((ph)->SizeInfo.height)
#define GETHDR_MENUNAME( ph )           ((ph)->MenuName)
#define GETHDR_CLASSNAME( ph )          ((ph)->ClassName)
#define GETHDR_CAPTION( ph )            ((ph)->Caption)
#define GETHDR_FONTPOINTSIZE( ph )      ((ph)->PointSize)
#define GETHDR_FONTFACENAME( ph )       ((ph)->FontName)
#define GETHDR_FONTWEIGHT( ph )         ((ph)->FontWeight)
#define GETHDR_FONTITALIC( ph )         ((ph)->FontItalic)
#define GETHDR_FONTCHARSET( ph )        ((ph)->FontCharset)
#define GETHDR_HELPID( ph )             ((ph)->HelpId)
#define GETHDR_EXSTYLE( ph )            ((ph)->ExtendedStyle)

#define SETHDR_STYLE( ph, s )           ((ph)->Style = (s))
#define SETHDR_NUMITEMS( ph, s )        ((ph)->NumOfItems = (s))
#define SETHDR_SIZE( ph, s )            ((ph)->SizeInfo = (s))
#define SETHDR_SIZEX( ph, s )           ((ph)->SizeInfo.x = (s))
#define SETHDR_SIZEY( ph, s )           ((ph)->SizeInfo.y = (s))
#define SETHDR_SIZEW( ph, s )           ((ph)->SizeInfo.width = (s))
#define SETHDR_SIZEH( ph, s )           ((ph)->SizeInfo.height = (s))
#define SETHDR_MENUNAME( ph, s )        ((ph)->MenuName = (s))
#define SETHDR_CLASSNAME( ph, s )       ((ph)->ClassName = (s))
#define SETHDR_CAPTION( ph, s )         ((ph)->Caption = (s))
#define SETHDR_FONTPOINTSIZE( ph, s )   ((ph)->PointSize = (s))
#define SETHDR_FONTFACENAME( ph, s )    ((ph)->FontName = (s))
#define SETHDR_FONTWEIGHT( ph, s )      ((ph)->FontWeight = (s))
#define SETHDR_FONTITALIC( ph, s )      ((ph)->FontItalic = (s))
#define SETHDR_FONTCHARSET( ph, s )     ((ph)->FontCharset = (s))
#define SETHDR_HELPID( ph, s )          ((ph)->HelpId = (s))
#define SETHDR_EXSTYLE( ph, s )         ((ph)->ExtendedStyle = (s))

#define GETCTL_SIZE( pc )           ((pc)->SizeInfo)
#define GETCTL_PSIZE( pc )          (&(pc)->SizeInfo)
#define GETCTL_SIZEX( pc )          ((pc)->SizeInfo.x)
#define GETCTL_SIZEY( pc )          ((pc)->SizeInfo.y)
#define GETCTL_SIZEW( pc )          ((pc)->SizeInfo.width)
#define GETCTL_SIZEH( pc )          ((pc)->SizeInfo.height)
#define GETCTL_ID( pc )             ((pc)->ID)
#define GETCTL_STYLE( pc )          ((pc)->Style)
#define GETCTL_CLASSID( pc )        ((pc)->ClassID)
#define GETCTL_TEXT( pc )           ((pc)->Text)
#define GETCTL_EXTRABYTES( pc )     ((pc)->ExtraBytes)
#define GETCTL_HELPID( pc )         ((pc)->HelpId)
#define GETCTL_EXSTYLE( pc )        ((pc)->ExtendedStyle)

#define SETCTL_SIZE( pc, s )        ((pc)->SizeInfo = (s))
#define SETCTL_SIZEX( pc, s )       ((pc)->SizeInfo.x = (s))
#define SETCTL_SIZEY( pc, s )       ((pc)->SizeInfo.y = (s))
#define SETCTL_SIZEW( pc, s )       ((pc)->SizeInfo.width = (s))
#define SETCTL_SIZEH( pc, s )       ((pc)->SizeInfo.height = (s))
#define SETCTL_ID( pc, s )          ((pc)->ID = (s))
#define SETCTL_STYLE( pc, s )       ((pc)->Style = (s))
#define SETCTL_CLASSID( pc, s )     ((pc)->ClassID = (s))
#define SETCTL_TEXT( pc, s )        ((pc)->Text = (s))
#define SETCTL_EXTRABYTES( pc, s )  ((pc)->ExtraBytes = (s))
#define SETCTL_HELPID( pc, s )      ((pc)->HelpId = (s))
#define SETCTL_EXSTYLE( pc, s )     ((pc)->ExtendedStyle = (s))

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

typedef struct {
    int             x;
    int             y;
    int             width;
    int             height;
} WdeDialogSizeInfo;

typedef struct WdeResInfoStruct {
    WRInfo              *info;
    char                *sym_name;
    WResTypeNode        *dlg_entry;         /* type node for dialogs           */
    LIST                *dlg_item_list;     /* list of WdeResDlgItem's         */
    OBJPTR              next_current;
    WdeHashTable        *hash_table;
    HWND                res_win;
    HWND                edit_win;
    HWND                forms_win;
    bool                modified;
    bool                symbols_dirty;
    bool                active;
    bool                editting;
    bool                is32bit;
} WdeResInfo;

typedef struct WdeDialogBoxHeader {
    uint_32             HelpId;
    uint_32             ExtendedStyle;
    DialogStyle         Style;
    uint_16             NumOfItems;
    WdeDialogSizeInfo   SizeInfo;
    ResNameOrOrdinal    *MenuName;          // NameOrOrdinal
    ResNameOrOrdinal    *ClassName;         // NameOrOrdinal
    char                *Caption;           // String
    uint_16             PointSize;          // only here if (Style & DS_SETFONT)
    uint_16             FontWeight;         // only here if (Style & DS_SETFONT)
    uint_8              FontItalic;         // only here if (Style & DS_SETFONT)
    uint_8              FontCharset;        // only here if (Style & DS_SETFONT)
    char                *FontName;          // only here if (Style & DS_SETFONT)

    char                *symbol;
    char                *helpsymbol;

    bool                FontWeightDefined;
    bool                FontItalicDefined;
    bool                FontCharsetDefined;
    bool                is32bit;
    bool                is32bitEx;
} WdeDialogBoxHeader;


typedef struct WdeDialogBoxControl {
    uint_32             HelpId;
    uint_32             ExtendedStyle;
    uint_32             Style;
    WdeDialogSizeInfo   SizeInfo;
    uint_16             ID;
    ControlClass        *ClassID;
    ResNameOrOrdinal    *Text;
    uint_16             ExtraBytes;         /* should be 0 */

    char                *symbol;
    char                *helpsymbol;
} WdeDialogBoxControl;

typedef struct WdeDialogBoxInfoStruct {
    WdeDialogBoxHeader  *dialog_header;
    LIST                *control_list;      /* list of DialogBoxControl's      */
    uint_16             MemoryFlags;
} WdeDialogBoxInfo;

typedef struct WdeResDlgItem {
    WdeDialogBoxInfo    *dialog_info;
    OBJPTR              object;
    WResID              *dialog_name;
    bool                modified;
    WResResNode         *rnode;
    WResLangNode        *lnode;
    bool                is32bit;
} WdeResDlgItem;

/****************************************************************************/
/* function prototypes                                                      */
/****************************************************************************/
extern bool             WdeFreeResInfo( WdeResInfo * );
extern bool             WdeFreeDialogBoxInfo( WdeDialogBoxInfo * );
extern void             WdeFreeResDlgItem( WdeResDlgItem **, bool );
extern WdeResDlgItem    *WdeAllocResDlgItem( void );
extern WdeResInfo       *WdeAllocResInfo( void );
extern bool             WdeIsResModified( WdeResInfo * );
extern void             WdeSetResModified( WdeResInfo *, bool );

#endif
