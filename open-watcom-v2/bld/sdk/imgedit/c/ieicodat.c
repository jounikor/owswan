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


#include "imgedit.h"
#include <commdlg.h>
#include <dos.h>
#include <cderr.h>
#include "colours.h"
#include "ieicodat.h"


/*
 * readIconFromData - read the icon file and set up structures
 */
static bool readIconFromData( BYTE *data, const char *fname  )
{
    an_img_file         *iconfile;
    img_node            *node;
    int                 num_of_images;
    HDC                 hdc;
    int                 i;
    an_img              *icon;
    char                filename[_MAX_FNAME + _MAX_EXT];
    int                 pos;

    GetFnameFromPath( fname, filename );
    iconfile = ImageOpen( fp );
    if( iconfile == NULL ) {
        fclose( fp );
        WImgEditError( WIE_ERR_BAD_ICON_FILE, filename );
        return( false );
    }
    num_of_images = iconfile->count;

#if 0
    /* See biBitCount test below... */
    for( i = 0; i < num_of_images; i++ ) {
        if( iconfile->resources[i].color_count != 2 &&
            iconfile->resources[i].color_count != 8 &&
            iconfile->resources[i].color_count != 16 &&
            iconfile->resources[i].color_count != 0 ) {
            WImgEditError( WIE_ERR_BAD_ICON_CLR, filename );
            ImageClose( iconfile );
            fclose( fp );
            return( false );
        }
    }
#endif

    node = MemAlloc( sizeof( img_node ) * num_of_images );

    hdc = GetDC( NULL );
    for( i = 0; i < num_of_images; i++ ) {
        icon = ImgResourceToImg( fp, iconfile, i );

        if( icon->bm->bmiHeader.biBitCount != 4 &&
            icon->bm->bmiHeader.biBitCount != 1 &&
            icon->bm->bmiHeader.biBitCount != 8 ) {
            WImgEditError( WIE_ERR_BAD_ICON_CLR, filename );
            ReleaseDC( NULL, hdc );
            ImageFini( icon );
            ImageClose( iconfile );
            fclose( fp );
            MemFree( node );
            return( false );
        }

        node[i].imgtype = ICON_IMG;
        node[i].bitcount = icon->bm->bmiHeader.biBitCount;
        node[i].width = icon->bm->bmiHeader.biWidth;
        node[i].height = icon->bm->bmiHeader.biHeight;
        if( node[i].height == 0 ) {
            node[i].height = node.width;
        }
        node[i].hotspot.x = 0;
        node[i].hotspot.y = 0;
        node[i].and_hbitmap = ImgToAndBitmap( hdc, icon );
        node[i].xor_hbitmap = ImgToXorBitmap( hdc, icon );
        node[i].num_of_images = num_of_images;
        node[i].viewhwnd = NULL;
        if( i > 0 ) {
            node[i - 1].nexticon = &node[i];
        }
        node[i].issaved = true;
        node[i].next = NULL;
        strupr( strcpy( node[i].fname, fname ) );
        ImageFini( icon );
    }
    node[i - 1].nexticon = NULL;

    ReleaseDC( NULL, hdc );
    ImageClose( iconfile );
    fclose( fp );

    WriteIconLoadedText( filename, node->num_of_images );
    CreateNewDrawPad( node );

    MemFree( node );
    return( true );

} /* readInIconFile */
