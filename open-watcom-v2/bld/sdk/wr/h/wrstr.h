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


#ifndef WRSTR_INCLUDED
#define WRSTR_INCLUDED

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* data types                                                               */
/****************************************************************************/

/****************************************************************************/
/* function prototypes                                                      */
/****************************************************************************/
WRDLLENTRY extern bool WRAPI WRIsBlockEmpty( StringTableBlock *block );
WRDLLENTRY extern bool WRAPI WRMakeDataFromStringBlock( StringTableBlock *block, void **data, size_t *size, bool is32bit );
WRDLLENTRY extern bool WRAPI WRMakeStringBlockFromData( StringTableBlock *block, void *data, size_t size, bool is32bit );
WRDLLENTRY extern bool WRAPI WRMergeStringBlock( StringTableBlock *b1, StringTableBlock *b2, bool replace );
WRDLLENTRY extern bool WRAPI WRMergeStringData( void **s1, uint_32 *sz1, void *s2, uint_32 sz2, bool is32bit, bool replace );

#endif
