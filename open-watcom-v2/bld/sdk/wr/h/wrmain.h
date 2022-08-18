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


#ifndef WRMAIN_INCLUDED
#define WRMAIN_INCLUDED

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct WRSaveIntoData {
    WRInfo                      *info;
    WResID                      *type;
    WResID                      *name;
    void                        *data;
    WResLangType                lang;
    uint_32                     size;
    uint_16                     MemFlags;
    struct WRSaveIntoData       *next;
} WRSaveIntoData;

/****************************************************************************/
/* function prototypes                                                      */
/****************************************************************************/
WRDLLENTRY extern void             WRAPI WRInit( void );
WRDLLENTRY extern void             WRAPI WRFini( void );
WRDLLENTRY extern WRInfo *         WRAPI WRLoadResource( const char *, WRFileType );
WRDLLENTRY extern bool             WRAPI WRUpdateTmp( WRInfo *info );
WRDLLENTRY extern bool             WRAPI WRSaveResource( WRInfo *, bool );
WRDLLENTRY extern bool             WRAPI WRSaveObjectAs( const char *, WRFileType, WRSaveIntoData * );
WRDLLENTRY extern bool             WRAPI WRSaveObjectInto( const char *, WRSaveIntoData *, bool * );
WRDLLENTRY extern bool             WRAPI WRFindAndSetData( WResDir dir, WResID *type, WResID *name, WResLangType *lang, void *data );
WRDLLENTRY extern WResLangNode *   WRAPI WRFindLangNode( WResDir dir, WResID *type, WResID *name, WResLangType *lang );

#endif
