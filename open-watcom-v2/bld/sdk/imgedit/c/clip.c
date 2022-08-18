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


typedef struct {
    WPI_RECT    rect;
    HWND        hwnd;
} a_clip_rect;

        /*
         * clipRect stores the values of the rectangle.  these are stored
         * by the mouse coordinates so NO CONVERTHEIGHT is required.  the
         * values are stored in windows format (ie bottom > top ).  if
         * _wpi_getrectvalues is used, top > bottom.
         */
static a_clip_rect      clipRect;
static WPI_HBITMAP      clipand_hbitmap = NULL;
static WPI_HBITMAP      clipxor_hbitmap = NULL;
static short            dragWidth;
static short            dragHeight;
static bool             fEnableCutCopy = false;
static int              prevToolType;
static HCURSOR          prevCursor;
static HCURSOR          pointCursor;
static WPI_POINT        prevPoint;
static bool             firstTime;

/*
 * copyImageToClipboard - copies the image to the clipboard
 */
static void copyImageToClipboard( short width, short height, img_node *node )
{
    HMENU       hmenu;
    WPI_PRES    pres;
    WPI_PRES    mempres;
    HDC         memdc;
    WPI_PRES    clippres;
    HDC         clipdc;
    WPI_HBITMAP hbitmap;
    WPI_HBITMAP old_hbitmap;
    WPI_HBITMAP oldclip_hbitmap;
    WPI_HBITMAP view_hbitmap;
    WPI_RECTDIM left;
    WPI_RECTDIM top;
    WPI_RECTDIM right;
    WPI_RECTDIM bottom;
    int         clip_width, clip_height;

    CleanupClipboard();

    pres = _wpi_getpres( node->viewhwnd );
    mempres = _wpi_createcompatiblepres( pres, Instance, &memdc );
    clippres = _wpi_createcompatiblepres( pres, Instance, &clipdc );
    hbitmap = _wpi_createcompatiblebitmap( pres, width, height );

    view_hbitmap = CreateViewBitmap( node );

    old_hbitmap = _wpi_selectbitmap( mempres, view_hbitmap );
    oldclip_hbitmap = _wpi_selectbitmap( clippres, hbitmap );

    clip_width = _wpi_getwidthrect( clipRect.rect );
    clip_height = _wpi_getheightrect( clipRect.rect );
    /*
     * use getwrectvalues because we want top and left to be our origins
     * (i.e. in PM the origin is the bottom)
     */
    _wpi_getwrectvalues( clipRect.rect, &left, &top, &right, &bottom );

    _wpi_bitblt( clippres, 0, 0, width, height, mempres, left, top, SRCCOPY );
    _wpi_openclipboard( Instance, HMainWindow );
    _wpi_emptyclipboard( Instance );
    _wpi_setclipboarddata( Instance, CF_BITMAP, hbitmap, TRUE );
    _wpi_closeclipboard( Instance );

    _wpi_getoldbitmap( clippres, oldclip_hbitmap );
    _wpi_deletecompatiblepres( clippres, clipdc );

    clippres = _wpi_createcompatiblepres( pres, Instance, &clipdc );
    clipxor_hbitmap = _wpi_createcompatiblebitmap( pres, width, height );

    _wpi_getoldbitmap( mempres, old_hbitmap );
    old_hbitmap = _wpi_selectbitmap( mempres, node->xor_hbitmap );
    oldclip_hbitmap = _wpi_selectbitmap( clippres, clipxor_hbitmap );

    _wpi_bitblt( clippres, 0, 0, width, height, mempres, left, top, SRCCOPY );
    _wpi_getoldbitmap( mempres, old_hbitmap );
    _wpi_getoldbitmap( clippres, oldclip_hbitmap );

    clipand_hbitmap = _wpi_createcompatiblebitmap( pres, width, height );
    old_hbitmap = _wpi_selectbitmap( mempres, node->and_hbitmap );
    oldclip_hbitmap = _wpi_selectbitmap( clippres, clipand_hbitmap );

    _wpi_bitblt( clippres, 0, 0, width, height, mempres, left, top, SRCCOPY );
    _wpi_getoldbitmap( mempres, old_hbitmap );
    _wpi_getoldbitmap( clippres, oldclip_hbitmap );
    _wpi_deletecompatiblepres( mempres, memdc );
    _wpi_deletecompatiblepres( clippres, clipdc );

    _wpi_deletebitmap( view_hbitmap );
    _wpi_releasepres( node->viewhwnd, pres );

    hmenu = GetMenu( _wpi_getframe( HMainWindow ) );
    _wpi_enablemenuitem( hmenu, IMGED_PASTE, TRUE, FALSE );

} /* copyImageToClipboard */

/*
 * RedrawPrevClip - redraw the area if a region has been selected and copied to the
 *                  clipboard OR if a region has been selected and then another is
 *                  selected (without a cut or copy in between)
 *                - the hwnd parameter indicates which window to draw the clip rect on
 *                - if the hwnd does not match the one associated with clipRect, we return
 */
void RedrawPrevClip( HWND hwnd )
{
    WPI_POINT   pointsize;
    int         prevROP2;
    HBRUSH      blackbrush;
    HBRUSH      oldbrush;
    HPEN        whitepen;
    HPEN        oldpen;
    WPI_PRES    pres;
    WPI_RECTDIM left;
    WPI_RECTDIM top;
    WPI_RECTDIM right;
    WPI_RECTDIM bottom;
    img_node    *node;

    if( !fEnableCutCopy || !_wpi_iswindow( Instance, hwnd ) || clipRect.hwnd != hwnd ) {
        return;
    }

    pointsize = GetPointSize( hwnd );

    pres = _wpi_getpres( hwnd );
    _wpi_torgbmode( pres );
    prevROP2 = _wpi_setrop2( pres, R2_XORPEN );
    blackbrush = _wpi_createsolidbrush( BLACK );
    oldbrush = _wpi_selectbrush( pres, blackbrush );

    whitepen = _wpi_createpen( PS_SOLID, 0, WHITE );
    oldpen = _wpi_selectpen( pres, whitepen );

    _wpi_getrectvalues( clipRect.rect, &left, &top, &right, &bottom );

    node = SelectImage( hwnd );
#ifdef __OS2_PM__
    _wpi_rectangle( pres, left * pointsize.x + 1, bottom * pointsize.y + 1,
                    right * pointsize.x, top * pointsize.y );
#else
    _wpi_rectangle( pres, left * pointsize.x, top * pointsize.y,
                    right * pointsize.x, bottom * pointsize.y );
#endif
    _wpi_getoldpen( pres, oldpen );
    _wpi_getoldbrush( pres, oldbrush );
    _wpi_setrop2( pres, prevROP2 );
    _wpi_releasepres( hwnd, pres );
    _wpi_deletepen( whitepen );
    _wpi_deletebrush( blackbrush );

} /* RedrawPrevClip */

/*
 * SetClipRect - set the value of the clipping rectangle
 */
void SetClipRect( HWND hwnd, WPI_POINT *startpt, WPI_POINT *endpt, WPI_POINT pointsize )
{
    WPI_RECTDIM left;
    WPI_RECTDIM top;
    WPI_RECTDIM right;
    WPI_RECTDIM bottom;
    WPI_RECTDIM tmps;
    WPI_RECTDIM tmpe;

    CheckBounds( hwnd, startpt );
    CheckBounds( hwnd, endpt );

    tmps = startpt->x / pointsize.x;
    tmpe = endpt->x / pointsize.x;
    left = tmps;
    if( left > tmpe )
        left = tmpe;
    right = tmps;
    if( right < tmpe )
        right = tmpe;
    ++right;

    tmps = startpt->y / pointsize.y;
    tmpe = endpt->y / pointsize.y;
    top = tmps;
#ifdef __OS2_PM__
    if( top < tmpe )
        top = tmpe;
    ++top;
#else
    if( top > tmpe )
        top = tmpe;
#endif
    bottom = tmps;
#ifdef __OS2_PM__
    if( bottom > tmpe )
        bottom = tmpe;
#else
    if( bottom < tmpe )
        bottom = tmpe;
    ++bottom;
#endif
    _wpi_setrectvalues( &clipRect.rect, left, top, right, bottom );
    clipRect.hwnd = hwnd;

    fEnableCutCopy = true;

} /* SetClipRect */

/*
 * IECopyImage - copy the current clipping rectangle to the clipboard
 *
 * 1) Copy the bitmap to the clipboard (so other apps can use it).
 * 2) Make a copy of the XOR bitmap and the AND bitmaps so that if
 *    screen colors are involved, they will be preserved.
 *    Later, we check the owner of the clipboard to see if we really
 *    want to use the XOR/AND bitmaps or not.
 */
void IECopyImage( void )
{
    short       width;
    short       height;
    img_node    *node;

    node = GetCurrentNode();
    if( node == NULL ) {
        return;
    }

    if( !fEnableCutCopy ) {
        _wpi_setwrectvalues( &clipRect.rect, 0, 0, node->width, node->height );
        width = node->width;
        height = node->height;
    } else {
        width = _wpi_getwidthrect( clipRect.rect );
        height = _wpi_getheightrect( clipRect.rect );
    }

    copyImageToClipboard( width, height, node );

    if( !fEnableCutCopy ) {
        PrintHintTextByID( WIE_ENTIREIMAGECOPIED, NULL );
    } else {
        PrintHintTextByID( WIE_AREACOPIED, NULL );
        RedrawPrevClip( node->hwnd );
        fEnableCutCopy = false;
    }

} /* IECopyImage */

/*
 * PlaceAndPaste - find out where the image is to be placed and then paste it there
 */
void PlaceAndPaste( void )
{
    WPI_HBITMAP     hbitmap;
    WPI_POINT       pointsize;
    img_node        *node;
    WPI_POINT       pt;
    unsigned long   format;
    int             bm_width;
    int             bm_height;

    format = 0;
    node = GetCurrentNode();
    if( node == NULL ) {
        return;
    }

    if( !_wpi_isclipboardformatavailable( Instance, CF_BITMAP, &format ) ) {
        PrintHintTextByID( WIE_NOBITMAPINCLIPBOARD, NULL );
        return;
    }

    pointsize = GetPointSize( node->hwnd );
    if( fEnableCutCopy ) {
        RedrawPrevClip( node->hwnd );
        PasteImage( NULL, pointsize, node->hwnd );
        fEnableCutCopy = false;
        return;
    }

    prevToolType = SetToolType( IMGED_PASTE );
    pointCursor = _wpi_loadcursor( Instance, POINT_CUR );
    prevCursor = _wpi_setcursor( pointCursor );

    _wpi_openclipboard( Instance, HMainWindow );
    hbitmap = _wpi_getclipboarddata( Instance, CF_BITMAP );
    _wpi_getbitmapdim( hbitmap, &bm_width, &bm_height );
    _wpi_closeclipboard( Instance );

    dragWidth = (short)(bm_width * pointsize.x);
    dragHeight = (short)(bm_height * pointsize.y);
    WriteSetSizeText( WIE_CLIPBOARDBITMAPSIZE, bm_width, bm_height );

    _wpi_getcursorpos( &pt );
    _wpi_screentoclient( node->hwnd, &pt );
    pt.x = pt.x / pointsize.x;
    pt.y = pt.y / pointsize.y;
    _wpi_setcapture( node->hwnd );
    firstTime = true;
    DragClipBitmap( node->hwnd, &pt, pointsize );

} /* PlaceAndPaste */

/*
 * PasteImage - paste the image in the clipboard at the current point
 *            - first check to see if the image was cut/copied from our program
 *            - if it was, then we use the clipxor_hbitmap and clipand_hbitmap bitmaps
 *              we created in order to preserve the screen colors if ther were used
 *            - otherwise, just copy from the clipboard
 */
void PasteImage( WPI_POINT *pt, WPI_POINT pointsize, HWND hwnd )
{
    WPI_HBITMAP hbitmap;
    WPI_HBITMAP dup_hbitmap;
    WPI_HBITMAP holddup_hbitmap;
    WPI_HBITMAP old_hbitmap;
    WPI_HBITMAP old_hbitmap2;
    WPI_PRES    pres;
    WPI_PRES    mempres;
    HDC         memdc;
    WPI_PRES    clippres;
    HDC         clipdc;
    WPI_RECT    client;
    WPI_POINT   truept;
    short       width;
    short       height;
    short       clipwidth;
    short       clipheight;
    img_node    *node;
    int         fstretchbmp;
    int         bm_width, bm_height;
    WPI_RECTDIM left;
    WPI_RECTDIM right;
    WPI_RECTDIM top;
    WPI_RECTDIM bottom;
    WPI_RECTDIM client_l;
    WPI_RECTDIM client_r;
    WPI_RECTDIM client_t;
    WPI_RECTDIM client_b;
#ifdef __OS2_PM__
    int         src_y, dst_y;
#endif

    if( fEnableCutCopy ) {
        _wpi_getwrectvalues( clipRect.rect, &left, &top, &right, &bottom );
        truept.x = left;
        truept.y = top;
        fstretchbmp = StretchPastedImage();
    } else {
        truept.x = pt->x / pointsize.x;
        truept.y = pt->y / pointsize.y;
        fstretchbmp = -1;
    }

    node = SelectImage( hwnd );
    pres = _wpi_getpres( node->viewhwnd );

    if( _wpi_getclipboardowner( Instance ) == HMainWindow && node->imgtype != BITMAP_IMG ) {
        _wpi_getbitmapdim( clipand_hbitmap, &bm_width, &bm_height );
        GetClientRect( node->hwnd, &client );
        if( fEnableCutCopy ) {
            width = (short)_wpi_getwidthrect( clipRect.rect );
            height = (short)_wpi_getheightrect( clipRect.rect );
        } else {
            _wpi_getrectvalues( client, &client_l, &client_t, &client_r, &client_b );
            width = (short)( client_r / pointsize.x - truept.x );
            if( width > (short)bm_width )
                width = (short)bm_width;
            height = (short)( client_b / pointsize.y - truept.y );
            if( height > (short)bm_height ) {
                height = (short)bm_height;
            }
        }

        mempres = _wpi_createcompatiblepres( pres, Instance, &memdc );
        _wpi_setstretchbltmode( mempres, COLORONCOLOR );
        clippres = _wpi_createcompatiblepres( pres, Instance, &clipdc );

        old_hbitmap = _wpi_selectbitmap( mempres, node->and_hbitmap );
        old_hbitmap2 = _wpi_selectbitmap( clippres, clipand_hbitmap );

        if( fstretchbmp == 0 ) {
            clipwidth = (short)bm_width;
            if( clipwidth > (short)width )
                clipwidth = (short)width;
            clipheight = (short)bm_height;
            if( clipheight > (short)height )
                clipheight = (short)height;

            _wpi_patblt( mempres, truept.x, truept.y, width, height, BLACKNESS );
            _wpi_bitblt( mempres, truept.x, truept.y, clipwidth, clipheight,
                         clippres, 0, 0, SRCCOPY );
        } else if( fstretchbmp == 1 ) {
            _wpi_stretchblt( mempres, truept.x, truept.y, width, height,
                             clippres, 0, 0, bm_width, bm_height, SRCCOPY);
        } else {    /* fstretchbmp == -1 */
            _wpi_bitblt( mempres, truept.x, truept.y, width, height, clippres,
                         0, 0, SRCCOPY );
        }
        _wpi_getoldbitmap( mempres, old_hbitmap );
        old_hbitmap = _wpi_selectbitmap( mempres, node->xor_hbitmap );

        dup_hbitmap = DuplicateBitmap( clipxor_hbitmap );
        _wpi_getoldbitmap( clippres, old_hbitmap2 );
        old_hbitmap2 = _wpi_selectbitmap( clippres, dup_hbitmap );

        if( fstretchbmp == 0 ) {
            clipwidth = (short)bm_width;
            if( clipwidth > (short)width )
                clipwidth = (short)width;
            clipheight = (short)bm_height;
            if( clipheight > (short)height )
                clipheight = (short)height;

            _wpi_patblt( mempres, truept.x, truept.y, width, height, WHITENESS );
#ifdef __OS2_PM__
            if( bm_height > height ) {
                src_y = bm_height - height;
                dst_y = truept.y;
            } else {
                src_y = 0;
                dst_y = truept.y + (height - bm_height);
            }
            _wpi_bitblt( mempres, truept.x, dst_y, clipwidth, clipheight,
                         clippres, 0, src_y, SRCCOPY );
#else
            _wpi_bitblt( mempres, truept.x, truept.y, clipwidth, clipheight,
                         clippres, 0, 0, SRCCOPY );
#endif
        } else if( fstretchbmp == 1 ) {
            _wpi_stretchblt( mempres, truept.x, truept.y, width, height,
                             clippres, 0, 0, bm_width, bm_height, SRCCOPY );
        } else {    /* fstretchbmp == -1 */
            _wpi_bitblt( mempres, truept.x, truept.y, width, height, clippres,
                         0, 0, SRCCOPY );
        }
        _wpi_getoldbitmap( mempres, old_hbitmap );
        _wpi_getoldbitmap( clippres, old_hbitmap2 );
        _wpi_deletebitmap( dup_hbitmap );
        _wpi_deletecompatiblepres( mempres, memdc );
        _wpi_deletecompatiblepres( clippres, clipdc );
    } else {
        if( node->imgtype != BITMAP_IMG ) {
            CleanupClipboard();
        }
        _wpi_openclipboard( Instance, HMainWindow );
        hbitmap = _wpi_getclipboarddata( Instance, CF_BITMAP );
        dup_hbitmap = DuplicateBitmap( hbitmap );
        _wpi_closeclipboard( Instance );

        _wpi_getbitmapdim( dup_hbitmap, &bm_width, &bm_height );
        GetClientRect( node->hwnd, &client );

        if( fEnableCutCopy ) {
            width = (short)_wpi_getwidthrect( clipRect.rect );
            height = (short)_wpi_getheightrect( clipRect.rect );
        } else {
            _wpi_getrectvalues( client, &client_l, &client_t, &client_r, &client_b );
            width = (short)( client_r / pointsize.x - truept.x );
            if( width > (short)bm_width )
                width = (short)bm_width;
            height = (short)( client_b / pointsize.y - truept.y );
            if( height > (short)bm_height ) {
                height = (short)bm_height;
            }
        }

        clippres = _wpi_createcompatiblepres( pres, Instance, &clipdc );
        mempres = _wpi_createcompatiblepres( pres, Instance, &memdc );
        _wpi_setstretchbltmode( mempres, COLORONCOLOR );

        holddup_hbitmap = _wpi_selectbitmap( clippres, dup_hbitmap );
        old_hbitmap = _wpi_selectbitmap( mempres, node->xor_hbitmap );

        if( fstretchbmp == 0 ) {
            clipwidth = (short)bm_width;
            if( clipwidth > (short)width )
                clipwidth = (short)width;
            clipheight = (short)bm_height;
            if( clipheight > (short)height )
                clipheight = (short)height;

            _wpi_patblt( mempres, truept.x, truept.y, width, height, WHITENESS );
#ifdef __OS2_PM__
            if( bm_height > height ) {
                src_y = bm_height - height;
                dst_y = truept.y;
            } else {
                src_y = 0;
                dst_y = truept.y + (height - bm_height);
            }
            _wpi_bitblt( mempres, truept.x, dst_y, clipwidth, clipheight,
                         clippres, 0, src_y, SRCCOPY );
#else
            _wpi_bitblt( mempres, truept.x, truept.y, clipwidth, clipheight,
                         clippres, 0, 0, SRCCOPY );
#endif
        } else if( fstretchbmp == 1 ) {
            _wpi_stretchblt( mempres, truept.x, truept.y, width, height,
                             clippres, 0, 0, bm_width, bm_height, SRCCOPY );
        } else {    /* fstretchbmp == -1 */
            _wpi_bitblt( mempres, truept.x, truept.y, width, height, clippres,
                         0, 0, SRCCOPY );
        }
        _wpi_getoldbitmap( clippres, holddup_hbitmap );
        _wpi_deletebitmap( dup_hbitmap );
        _wpi_deletecompatiblepres( clippres, clipdc );

        _wpi_getoldbitmap( mempres, old_hbitmap );
        old_hbitmap = _wpi_selectbitmap( mempres, node->and_hbitmap );
        _wpi_patblt( mempres, truept.x, truept.y, width, height, BLACKNESS );

        _wpi_getoldbitmap( mempres, old_hbitmap );
        _wpi_deletecompatiblepres( mempres, memdc );
    }

    ReleaseCapture();
    _wpi_releasepres( node->viewhwnd, pres );

    InvalidateRect( node->viewhwnd, NULL, TRUE );
    if( !fEnableCutCopy ) {
        _wpi_setcursor( prevCursor );
        _wpi_destroycursor( pointCursor );
        SetToolType( prevToolType );
    }

    fEnableCutCopy = false;
    RecordImage( hwnd );
    BlowupImage( NULL, NULL );

    PrintHintTextByID( WIE_BITMAPPASTED, NULL );

} /* PasteImage */

/*
 * CutImage - cuts the current clipping rectangle to the clipboard
 */
void CutImage( void )
{
    WPI_PRES    pres;
    WPI_PRES    mempres;
    HDC         memdc;
    WPI_HBITMAP old_hbitmap;
    short       width;
    short       height;
    img_node    *node;
    WPI_RECTDIM left;
    WPI_RECTDIM right;
    WPI_RECTDIM top;
    WPI_RECTDIM bottom;

    node = GetCurrentNode();

    if( node == NULL ) {
        return;
    }

    if( !fEnableCutCopy ) {
        _wpi_setwrectvalues( &clipRect.rect, 0, 0, node->width, node->height );
        width = node->width;
        height = node->height;
    } else {
        width = (short)_wpi_getwidthrect( clipRect.rect );
        height = (short)_wpi_getheightrect( clipRect.rect );
    }
    copyImageToClipboard( width, height, node );

    pres = _wpi_getpres( node->viewhwnd );
    mempres = _wpi_createcompatiblepres( pres, Instance, &memdc );
    _wpi_releasepres( node->viewhwnd, pres );

    old_hbitmap = _wpi_selectbitmap( mempres, node->xor_hbitmap );
    _wpi_getrectvalues( clipRect.rect, &left, &top, &right, &bottom );
    _wpi_patblt( mempres, left, top, width, height, WHITENESS );

    _wpi_getoldbitmap( mempres, old_hbitmap );
    old_hbitmap = _wpi_selectbitmap( mempres, node->and_hbitmap );
    _wpi_patblt( mempres, left, top, width, height, BLACKNESS );
    _wpi_getoldbitmap( mempres, old_hbitmap );
    _wpi_deletecompatiblepres( mempres, memdc );

    InvalidateRect( node->viewhwnd, NULL, FALSE );
    RecordImage( node->hwnd );

    if( !fEnableCutCopy ) {
        PrintHintTextByID( WIE_ENTIREIMAGECUT, NULL );
    } else {
        PrintHintTextByID( WIE_AREACUT, NULL );
        fEnableCutCopy = false;
    }
    BlowupImage( node->hwnd, NULL );

} /* CutImage */

/*
 * DragClipBitmap - routine to show the clipped bitmap
 */
void DragClipBitmap( HWND hwnd, WPI_POINT *newpt, WPI_POINT pointsize )
{
    int         prevROP2;
    HBRUSH      hbrush;
    HBRUSH      holdbrush;
    HPEN        hwhitepen;
    HPEN        holdpen;
    WPI_PRES    pres;

    pres = _wpi_getpres( hwnd );
    if( pres == (HDC)NULL ) {
        return;
    }
    prevROP2 = _wpi_setrop2( pres, R2_XORPEN );
    hbrush = _wpi_createsolidbrush( CLR_BLACK );
    holdbrush = _wpi_selectbrush( pres, hbrush );
    hwhitepen = _wpi_createpen( PS_SOLID, 0, CLR_WHITE );
    holdpen = _wpi_selectpen( pres, hwhitepen );

    if( !firstTime ) {
        _wpi_rectangle( pres, prevPoint.x * pointsize.x, prevPoint.y * pointsize.y,
                        prevPoint.x * pointsize.x + dragWidth,
                        prevPoint.y * pointsize.y + dragHeight );
    }

    _wpi_rectangle( pres, newpt->x * pointsize.x, newpt->y * pointsize.y,
                          newpt->x * pointsize.x + dragWidth,
                          newpt->y * pointsize.y + dragHeight );

    _wpi_getoldpen( pres, holdpen );
    _wpi_getoldbrush( pres, holdbrush );
    _wpi_deletepen( hwhitepen );
    _wpi_deletebrush( hbrush );

    _wpi_setrop2( pres, prevROP2 );
    _wpi_releasepres( hwnd, pres );
    memcpy( &prevPoint, newpt, sizeof( WPI_POINT ) );
    firstTime = false;

} /* DragClipBitmap */

/*
 * CheckForClipboard - check to see if the paste menu item can be enabled
 */
void CheckForClipboard( HMENU hmenu )
{
    unsigned long       format;

    format = 0;
    if( _wpi_isclipboardformatavailable( Instance, CF_BITMAP, &format ) &&
        DoImagesExist() ) {
        _wpi_enablemenuitem( hmenu, IMGED_PASTE, TRUE, FALSE );
    } else {
        _wpi_enablemenuitem( hmenu, IMGED_PASTE, FALSE, FALSE );
    }

} /* CheckForClipboard */

/*
 * SetRectExists - set whether or not there is a clipping rectangle on the
 *                 screen (indicated by fEnableCutCopy flag)
 */
void SetRectExists( bool does_rect_exist )
{
    fEnableCutCopy = does_rect_exist;

} /* SetRectExists */

/*
 * CleanupClipboard - clean up the clipand_hbitmap and clipxor_hbitmap bitmaps if
 *                    they were created by a cut or copy
 */
void CleanupClipboard( void )
{
    if( clipxor_hbitmap != NULL ) {
        _wpi_deletebitmap( clipxor_hbitmap );
        _wpi_deletebitmap( clipand_hbitmap );
        clipxor_hbitmap = NULL;
        clipand_hbitmap = NULL;
    }

} /* CleanupClipboard */

/*
 * DontPaste - when escape is hit while dragging the image to paste
 */
void DontPaste( HWND hwnd, WPI_POINT *topleft, WPI_POINT pointsize )
{
    firstTime = true;
    DragClipBitmap( hwnd, topleft, pointsize );
    _wpi_setcursor( prevCursor );
    _wpi_destroycursor( pointCursor );
    ReleaseCapture();
    SetToolType( prevToolType );
    PrintHintTextByID( WIE_PASTECANCELLED, NULL );

} /* DontPaste */

/*
 * DoesRectExist - return whether or not a rectangle exists
 */
bool DoesRectExist( WPI_RECT *rc )
{
    if( fEnableCutCopy ) {
        *rc = clipRect.rect;
    } else {
        _wpi_setrectvalues( rc, 0, 0, 0, 0 );
    }
    return( fEnableCutCopy );

} /* DoesRectExist */

/*
 * SetDeviceClipRect - set the new clipping rectangle with device units of image
 */
void SetDeviceClipRect( WPI_RECT *rect )
{
    clipRect.rect = *rect;
    fEnableCutCopy = true;

} /* SetDeviceClipRect */
