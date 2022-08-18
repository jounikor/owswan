/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2020 The Open Watcom Contributors. All Rights Reserved.
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
#include "iconinfo.h"
#include "jdlg.h"
#include "wclbproc.h"
#include "pathgrp2.h"

#include "clibext.h"


static image_type       imgType = UNDEF_IMG;
static short            imgHeight = DIM_DEFAULT;
static short            imgWidth = DIM_DEFAULT;
static short            bitCount;
static int              imageCount = 0;

/*
 * SelImgDlgProc - select the image type to edit.
 */
WPI_DLGRESULT CALLBACK SelImgDlgProc( HWND hwnd, WPI_MSG msg, WPI_PARAM1 wparam, WPI_PARAM2 lparam )
{
    bool    ret;

    ret = false;

    if( _wpi_dlg_command( hwnd, &msg, &wparam, &lparam ) ) {
        switch( LOWORD( wparam ) ) {
        case DLGID_OK:
            if( IsDlgButtonChecked( hwnd, SEL_BITMAP ) ) {
                imgType = BITMAP_IMG;
            } else if( IsDlgButtonChecked( hwnd, SEL_ICON ) ) {
                imgType = ICON_IMG;
            } else {
                imgType = CURSOR_IMG;
            }
            _wpi_enddialog( hwnd, DLGID_OK );
            break;

        case DLGID_CANCEL:
            _wpi_enddialog( hwnd, DLGID_CANCEL );
            break;

        case IDB_HELP:
            IEHelpRoutine();
            break;
        }
    } else {
        switch( msg ) {
        case WM_INITDIALOG:
            if ( imgType == UNDEF_IMG ) {
                _wpi_checkradiobutton( hwnd, SEL_BITMAP, SEL_CURSOR, SEL_BITMAP );
            } else {
                _wpi_checkradiobutton( hwnd, SEL_BITMAP, SEL_CURSOR, SEL_BITMAP + ( imgType - BITMAP_IMG ) );
            }
            ret = true;
            break;

#ifndef __OS2_PM__
        case WM_SYSCOLORCHANGE:
            IECtl3dColorChange();
            break;
#endif

        case WM_CLOSE:
            _wpi_enddialog( hwnd, DLGID_CANCEL );
            break;
        default:
            return( _wpi_defdlgproc( hwnd, msg, wparam, lparam ) );
        }
    }
    _wpi_dlgreturn( ret );

} /* SelImgDlgProc */

/*
 * SelBitmapDlgProc - select options for the bitmap (size and color scheme)
 */
WPI_DLGRESULT CALLBACK SelBitmapDlgProc( HWND hwnd, WPI_MSG msg, WPI_PARAM1 wparam, WPI_PARAM2 lparam )
{
    char        *title;
    char        *text;
    char        *msg_text;
    BOOL        err;
    bool        ret;

    ret = false;

    if( _wpi_dlg_command( hwnd, &msg, &wparam, &lparam ) ) {
        switch( LOWORD( wparam ) ) {
        case SEL_SELECT:
            _wpi_enddialog( hwnd, SEL_SELECT );
            break;

        case DLGID_OK:
            imgHeight = _wpi_getdlgitemshort( hwnd, BMP_HEIGHT, &err, FALSE );
            imgWidth = _wpi_getdlgitemshort( hwnd, BMP_WIDTH, &err, FALSE );
            if( imgHeight > MAX_DIM || imgHeight < MIN_DIM ||
                imgWidth > MAX_DIM || imgWidth < MIN_DIM ) {
                title = IEAllocRCString( WIE_NOTE );
                text = IEAllocRCString( WIE_DIMENSIONSBETWEEN );
                if( text != NULL ) {
                    msg_text = (char *)MemAlloc( strlen( text ) + 20 + 1 );
                    if( msg_text != NULL ) {
                        sprintf( msg_text, text, MIN_DIM, MAX_DIM );
                        MessageBox( hwnd, msg_text, title, MB_OK | MB_ICONINFORMATION );
                        MemFree( msg_text );
                    }
                    IEFreeRCString( text );
                }
                if( title != NULL ) {
                    IEFreeRCString( title );
                }
                break;
            }

            if( _wpi_isbuttonchecked( hwnd, BMP_TRUECOLOR ) ) {
                bitCount = 24;
            } else if( _wpi_isbuttonchecked( hwnd, BMP_256COLOR ) ) {
                bitCount = 8;
            } else if( _wpi_isbuttonchecked( hwnd, BMP_16COLOR ) ) {
                bitCount = 4;
            } else if( _wpi_isbuttonchecked( hwnd, BMP_2COLOR ) ) {
                bitCount = 1;
            }
            _wpi_enddialog( hwnd, DLGID_OK );
            break;

        case DLGID_CANCEL:
            _wpi_enddialog( hwnd, DLGID_CANCEL );
            break;

        case IDB_HELP:
            IEHelpRoutine();
            break;
        }

    } else {

        switch( msg ) {
        case WM_INITDIALOG: // WS_GROUP
            _wpi_setdlgitemshort( hwnd, BMP_HEIGHT, imgHeight, FALSE );
            _wpi_setdlgitemshort( hwnd, BMP_WIDTH, imgWidth, FALSE );
            if( bitCount == 1 ) {
                _wpi_checkradiobutton( hwnd, BMP_TRUECOLOR, BMP_2COLOR, BMP_2COLOR );
            } else if( bitCount == 4 ) {
                _wpi_checkradiobutton( hwnd, BMP_TRUECOLOR, BMP_2COLOR, BMP_16COLOR );
            } else if( bitCount == 8 ) {
                _wpi_checkradiobutton( hwnd, BMP_TRUECOLOR, BMP_2COLOR, BMP_256COLOR );
            } else {
                _wpi_checkradiobutton( hwnd, BMP_TRUECOLOR, BMP_2COLOR, BMP_TRUECOLOR );
            }
            ret = true;
            break;

#ifndef __OS2_PM__
        case WM_SYSCOLORCHANGE:
            IECtl3dColorChange();
            break;
#endif

        case WM_CLOSE:
            _wpi_enddialog( hwnd, IDCANCEL );
            break;
        default:
            return( _wpi_defdlgproc( hwnd, msg, wparam, lparam ) );
        }
    }
    _wpi_dlgreturn( ret );

} /* SelBitmapDlgProc */

#ifndef __OS2_PM__

/*
 * SelCursorDlgProc - select the target device to use the cursor on
 */
WPI_DLGRESULT CALLBACK SelCursorDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static HWND hlistbox;
    char        *mono32x32;
    int         index;
    bool        ret;

    lparam = lparam;

    ret = false;

    switch( msg ) {
    case WM_INITDIALOG:
        hlistbox = GetDlgItem( hwnd, TARGETLISTBOX );
        mono32x32 = IEAllocRCString( WIE_MONO32X32 );
        if( mono32x32 != NULL ) {
            SendMessage( hlistbox, LB_INSERTSTRING, 0, (LPARAM)(LPCSTR)mono32x32 );
            IEFreeRCString( mono32x32 );
        } else {
            SendMessage( hlistbox, LB_INSERTSTRING, 0, (LPARAM)(LPCSTR)"Monochrome, 32x32" );
        }
        SendMessage( hlistbox, LB_SETCURSEL, 0, 0L );
        ret = true;
        break;

    case WM_SYSCOLORCHANGE:
        IECtl3dColorChange();
        break;

    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case IDOK:
            index = (int)SendMessage( hlistbox, LB_GETCURSEL, 0, 0L );
            if( index == 0 ) {
                imgWidth = 32;
                imgHeight = 32;
                bitCount = 1;
                EndDialog( hwnd, TRUE );
            }
            break;

        case IDCANCEL:
            EndDialog( hwnd, IDCANCEL );
            break;

        case IDB_HELP:
            IEHelpRoutine();
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog( hwnd, IDCANCEL );
        ret = true;
        break;
    }
    _wpi_dlgreturn( ret );

} /* SelCursorDlgProc */

#endif

/*
 * initializeImage - initialize the bitmaps according to the image type
 */
static void initializeImage( img_node *node, const char *filename )
{
    node->imgtype = imgType;
    node->width = imgWidth;
    node->height = imgHeight;
    node->bitcount = bitCount;
    node->hotspot.x = 0;
    node->hotspot.y = 0;
    node->num_of_images = 1;
    node->nexticon = NULL;
    node->issaved = true;
    node->next = NULL;
    if( filename != NULL ) {
        strcpy( node->fname, filename );
    } else {
        sprintf( node->fname, "%s.%d", IEImageUntitled, imageCount );
    }

    if( imgType == BITMAP_IMG ) {
        MakeBitmap( node, true );
    } else {
        MakeIcon( node, true );           // also makes cursors
    }

} /* initializeImage */

/*
 * NewImage - create a new image and return the image type (bitmap, icon, or cursor)
 */
bool NewImage( image_type img_type, const char *filename )
{
    WPI_DLGPROC         dlgproc;
    INT_PTR             button_type;
    short               width;
    short               height;
    short               bcount;
    img_node            node;
    pgroup2             pg;
    bool                ok;

    // If filename is not NULL and we don't know the image type,
    // then guess based on the file extesion.
    if( filename != NULL && img_type == UNDEF_IMG ) {
        _splitpath2( filename, pg.buffer, NULL, NULL, NULL, &pg.ext );
        img_type = GetImageFileType( pg.ext, false );
    }

    if( img_type == UNDEF_IMG ) {
        dlgproc = _wpi_makedlgprocinstance( SelImgDlgProc, Instance );
        button_type = _wpi_dialogbox( HMainWindow, dlgproc, Instance, SELECTIMAGE, 0L );
        _wpi_freedlgprocinstance( dlgproc );

        if( button_type == DLGID_CANCEL ) {
            return( false );
        }
    } else {
        imgType = img_type;
    }

    imageCount++;

    ok = true;
    switch( imgType ) {
    case BITMAP_IMG:
        dlgproc = _wpi_makedlgprocinstance( SelBitmapDlgProc, Instance );
        button_type = _wpi_dialogbox( HMainWindow, dlgproc, Instance, BITMAPTYPE, 0L );
        _wpi_freedlgprocinstance( dlgproc );
        if( button_type == DLGID_CANCEL ) {
            imgType = UNDEF_IMG;
            imageCount--;
            ok = false;
        } else if( button_type == SEL_SELECT ) {
#ifdef __OS2_PM__
            IEDisplayErrorMsg( WIE_NOTE, WIE_NOTIMPLEMENTED, MB_OK | MB_ICONINFORMATION );
            ok = false;
#else
            if( !SelectDynamicBitmap( &node, imageCount, filename ) ) {
                ok = false;
            }
#endif
        } else {
            initializeImage( &node, filename );
        }
        break;

    case ICON_IMG:
        if( !CreateNewIcon( &width, &height, &bcount, true ) ) {
            imgType = UNDEF_IMG;
            ok = false;
            break;
        }
        imgWidth = width;
        imgHeight = height;
        bitCount = bcount;
        initializeImage( &node, filename );
        break;

    case CURSOR_IMG:
#ifdef __OS2_PM__
        if( !CreateNewIcon( &width, &height, &bcount, false ) ) {
            imgType = UNDEF_IMG;
            ok = false;
            break;
        }
        imgWidth = width;
        imgHeight = height;
        bitCount = bcount;
#else
        dlgproc = MakeProcInstance_DLG( SelCursorDlgProc, Instance );
        button_type = JDialogBox( Instance, "CURSORTYPE", HMainWindow, dlgproc );
        FreeProcInstance_DLG( dlgproc );
        if( button_type == IDCANCEL ) {
            imgType = UNDEF_IMG;
            ok = false;
            break;
        }
#endif
        initializeImage( &node, filename );
        break;

    default:
        ok = false;
        break;
    }

    if( ok ) {
        node.wrinfo = NULL;
        node.lnode = NULL;

        CreateNewDrawPad( &node );

        SetupMenuAfterOpen();
        ok = ( imgType != UNDEF_IMG );
    }
    return( ok );

} /* NewImage */
