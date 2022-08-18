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


#ifndef WDEFONT_INCLUDED
#define WDEFONT_INCLUDED

#include "wresall.h"
#include "wderesiz.h"

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct {
     char           name[LF_FULLFACESIZE];
     int            fonttype;
     int            num_children;
     LIST           *family_list;
} WdeFontNames;

typedef struct {
     ENUMLOGFONT    elf;
     NEWTEXTMETRIC  ntm;
     int            fonttype;
     uint_32        pointsize;
} WdeFontData;

/****************************************************************************/
/* function prototypes                                                      */
/****************************************************************************/
extern void     WdeDumpFontList( void );
extern void     WdeFreeFontList( void );
extern LIST     *WdeGetFontList( void );
extern void     WdeSetFontList( HWND );
extern bool     WdeDialogToScreen( void *, WdeResizeRatio *, WdeDialogSizeInfo *, RECT * );
extern bool     WdeScreenToDialog( void *, WdeResizeRatio *, RECT *, WdeDialogSizeInfo * );
extern HFONT    WdeGetFont( char *, int, int );
extern bool     WdeGetResizerFromFont( WdeResizeRatio *, char *, int );

#endif
