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


#include "dbgdefn.h"
#include <stddef.h>
#include "dbgdata.h"
#include "dbglit.h"
#include "sortlist.h"
#include "dbgprog.h"
#include "dbgimg.h"


extern void             SetLastSym( char *to );
extern bool             SymBrowse( char **name );

char *ImgSymFileName( image_entry *image, bool always )
{
    if( image->dip_handle != NO_MOD || always ) {
        if( image->symfile_name != NULL ) {
            return( image->symfile_name );
        } else {
            return( image->image_name );
        }
    } else {
        return( LIT_ENG( Empty ) );
    }
}

static int ImageCompare( void *_pa, void *_pb )
{
    image_entry **pa = _pa;
    image_entry **pb = _pb;

    if( *pa == ImagePrimary() )
        return( -1 );
    if( *pb == ImagePrimary() )
        return( 1 );
    if( DIPImagePriority( (*pa)->dip_handle ) < DIPImagePriority( (*pb)->dip_handle ) )
        return( -1 );
    if( DIPImagePriority( (*pa)->dip_handle ) > DIPImagePriority( (*pb)->dip_handle ) )
        return( 1 );
    return( 0 );
}

void    ImgSort( void )
{
    DbgImageList = SortLinkedList( DbgImageList, offsetof( image_entry, link ),
                                ImageCompare, DbgAlloc, DbgFree );
}
