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
* Description: Standard open/close dialog support.
*
****************************************************************************/

/* TODO: This file is mostly specific to MS Windows.  We need to make a
 * platform neutral version and move the contents to the correct platform
 * specific directories.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#define INCLUDE_COMMDLG_H
#include <wwindows.h>
#include "getfile.h"
#include "wstring.hpp"
#include "wstrlist.hpp"
#include "wfilenam.hpp"
#include "wwindow.hpp"
#include "wclbproc.h"
#include "pathgrp2.h"

#include "clibext.h"


typedef struct {
    WString     *result;
    const char  *tgt_file;
    int         ret_code;
    int         filter_index;
    const char  *filter;
} GetFilesInfo;

static LRESULT getMaxItemLen( HWND lb )
{
    int         cnt;
    LRESULT     len;
    LRESULT     rc;
    int         i;

    cnt = (int)SendMessage( lb, LB_GETCOUNT, 0, 0 );
    len = 0;
    for( i = 0; i < cnt; i++ ) {
        rc = SendMessage( lb, LB_GETTEXTLEN, i, 0 );
        if( rc > len ) {
            len = rc;
        }
    }
    return( len );
}

static void checkRemoveButton( HWND hwnd )
{
    HWND        ctl;
    int         item;

    ctl = GetDlgItem( hwnd, FOD_FILELIST );
    item = (int)SendMessage( ctl, LB_GETCURSEL, 0, 0 );
    if( item == LB_ERR ) {
        ctl = GetDlgItem( hwnd, FOD_REMOVE );
        EnableWindow( ctl, FALSE );
    } else {
        ctl = GetDlgItem( hwnd, FOD_REMOVE );
        EnableWindow( ctl, TRUE );
    }
}

static void getFullFname( HWND hwnd, const char *fname, WString *fullname )
{
    size_t      len;
    pgroup2     pg;

    /* unused parameters */ (void)hwnd;

    _splitpath2( fname, pg.buffer, &pg.drive, NULL, NULL, NULL );
    if( pg.drive[0] == '\0' ) {
        getcwd( pg.buffer, sizeof( pg.buffer ) );
        *fullname = pg.buffer;
        len = strlen( pg.buffer );
        if( pg.buffer[len - 1] != '\\' ) {
            fullname->concat( "\\" );
        }
    }
    fullname->concat( fname );
}

static void formRelDir( const char *filedir, const char *tgtdir,WString *dir )
{
    const char          *tgt_bs;        //location of last found backslash
    const char          *file_bs;

    tgt_bs = tgtdir;
    file_bs = filedir;
    // get rid of identical prefix directories
    for( ;; ) {
        if( tolower( *filedir ) != tolower( *tgtdir ) ) {
            filedir = file_bs;
            tgtdir = tgt_bs;
            break;
        }
        if( *filedir == '\0' ) {
            break;
        } else if( *filedir == '\\' ) {
            tgt_bs = tgtdir;
            file_bs = filedir;
        }
        filedir++;
        tgtdir++;
    }
    *dir = "";
    for( ;; ) {
        while( *tgtdir == '\\' )
            tgtdir++;
        if( *tgtdir != '\\' && *tgtdir != '\0' ) {
            while( *tgtdir != '\\' && *tgtdir != '\0' )
                tgtdir++;
            dir->concat( "..\\" );
        }
        if( *tgtdir == '\0' ) {
            break;
        }
    }
    if( *filedir != '\0' ) {
        dir->concat( filedir + 1 );
    }
}

static void getRelFname( HWND hwnd, const char *fname, WString *relname )
{
    WString         fullpath;
    GetFilesInfo    *info;
    WFileName       tgt;
    WFileName       filename;
    pgroup2         pg;

    info = (GetFilesInfo *)GET_DLGDATA( hwnd );
    getFullFname( hwnd, fname, &fullpath );
    filename = fullpath.gets();
    _splitpath2( info->tgt_file, pg.buffer, &pg.drive, &pg.dir, NULL, NULL );
    if( stricmp( pg.drive, filename.drive() ) == 0 ) {
        formRelDir( filename.dir(), pg.dir, relname );
        relname->concat( filename.fName() );
        relname->concat( filename.ext() );
    } else {
        *relname = fullpath.gets();
    }
}


static int findMatchingFile( HWND hwnd, const char *fname )
{
    int         rc;
    WString     fullname;
    HWND        lb;

    getFullFname( hwnd, fname, &fullname );
    lb = GetDlgItem( hwnd, FOD_FILELIST );
//    fullname.toLower();
    rc = (int)SendMessage( lb, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(LPSTR)fullname.gets() );
    if( rc == LB_ERR ) {
        getRelFname( hwnd, fname, &fullname );
//        fullname.toLower();
        rc = (int)SendMessage( lb, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(LPSTR)fullname.gets() );
    }
    return( rc );
}

static void addFileToList( HWND hwnd, char *fname )
{
    HWND        ctl;
    int         item;
    int         match;
    WFileName   fullname;
    bool        isLong = false;

    size_t len = strlen( fname ) - 1;
    if( fname[0] == '"' && fname[len] == '"' ) {
        fname++;
        fname[len - 1] = '\0';
        isLong = true;
    } else {
        WFileName filename( fname );
        isLong = filename.needQuotes();
    }

    match = findMatchingFile( hwnd, fname );
    ctl = GetDlgItem( hwnd, FOD_FILELIST );
    if( match == LB_ERR ) {
        if( IsDlgButtonChecked( hwnd, FOD_STORE_ABSOLUTE ) ) {
            getFullFname( hwnd, fname, &fullname );
        } else {
            getRelFname( hwnd, fname, &fullname );
        }
//        fullname.toLower();
        if( isLong ) {
            fullname.addQuotes();
        }
        item = (int)SendMessage( ctl, LB_ADDSTRING, 0, (LPARAM)(LPSTR)fullname.gets() );
    } else {
        item = match;
    }
    SendMessage( ctl, LB_SETCURSEL, item, 0 );
    checkRemoveButton( hwnd );
}

static void addCurrentFile( HWND hwnd  )
{
    char        *fname;
    int          len;
    HWND         ctl;
    struct stat  buf;

    ctl = GetDlgItem( hwnd, FOD_FILENAME );
    len = GetWindowTextLength( ctl );
    if( len > 0 ) {
        fname = new char [len + 1];
        GetWindowText( ctl, fname, len + 1 );
        if( fname[strlen( fname ) - 1] != '\\' ) {
            stat( fname, &buf );
            if( !S_ISDIR( buf.st_mode ) ) {
                if( strpbrk( fname, "?*" ) == NULL ) {
                    addFileToList( hwnd, fname );
                    SetWindowText( ctl, "" );
                }
            }
        }
        delete[] fname;
    }
}

#ifdef __NT__
static void addCurrentFile95( HWND hwnd )
{
    char        fname[MAX_PATH];
    struct stat buf;
    SendMessage( GetParent( hwnd ), CDM_GETSPEC, MAX_PATH, (LPARAM)fname );
    if( fname[strlen( fname ) - 1] == '\\' )
        return;
    stat( fname, &buf );
    if( S_ISDIR( buf.st_mode ) )
        return;
    if( strpbrk( fname, "?*" ) != NULL )
        return;
    addFileToList( hwnd, fname );
}
#endif

static void addAllFiles( HWND hwnd )
{
    HWND        ctl;
    int         cnt;
    int         i;
    char        *buf;
    LRESULT     alloced;
    LRESULT     len;

    alloced = 512;
    buf = new char [alloced];

    ctl = GetDlgItem( hwnd, FOD_FILES );
    cnt = (int)SendMessage( ctl, LB_GETCOUNT, 0, 0 );
    for( i = 0; i < cnt; i++ ) {
        len = SendMessage( ctl, LB_GETTEXTLEN, i, 0 );
        len++;
        if( len > alloced ) {
            delete[] buf;
            buf = new char [len];
            alloced = len;
        }
        SendMessage( ctl, LB_GETTEXT, i, (LPARAM)(LPSTR)buf );
        addFileToList( hwnd, buf );
    }
    delete[] buf;
}

#ifdef __NT__
static void addAllFiles95( HWND hwnd )
{
    int             i;
    int             n;
    GetFilesInfo    *info;
    const char      *ext;
    char            folder[_MAX_PATH];
    char            path[_MAX_PATH];
    WIN32_FIND_DATA wfd;
    HANDLE          find_handle;
    BOOL            found;
    char            *fname;

    info = (GetFilesInfo *)GET_DLGDATA( hwnd );
    ext = info->filter;
    n = info->filter_index * 2 - 1;
    for( i = 0; i < n; i++ ) {
        ext = strchr( ext, '\0' );
        ext++;
    }
    SendMessage( GetParent( hwnd ), CDM_GETFOLDERPATH, _MAX_PATH, (LPARAM)folder );
    _makepath( path, NULL, folder, ext, NULL );
    find_handle = FindFirstFile( path, &wfd );
    if( find_handle != INVALID_HANDLE_VALUE ) {
        found = TRUE;
        while( found ) {
            if( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                fname = strrchr( wfd.cFileName, '\\' );
                if( fname != NULL ) {
                    fname++;
                } else {
                    fname = wfd.cFileName;
                }
                addFileToList( hwnd, fname );
            }
            found = FindNextFile( find_handle, &wfd );
        }
        FindClose( find_handle );
    }
}
#endif

void GetResults( HWND hwnd )
{
    char                *buf;
    int                 cnt;
    int                 i;
    LRESULT             len;
    HWND                lb;
    GetFilesInfo        *info;

    info = (GetFilesInfo *)GET_DLGDATA( hwnd );
    lb = GetDlgItem( hwnd, FOD_FILELIST );
    len = getMaxItemLen( lb );
    buf = new char [len + 1];
    *info->result = "";
    cnt = (int)SendMessage( lb, LB_GETCOUNT, 0, 0 );
    for( i = 0; i < cnt; i++ ) {
        SendMessage( lb, LB_GETTEXT, i, (LPARAM)(LPSTR)buf );
        info->result->concat( buf );
        info->result->concat( " " );
    }
    delete[] buf;
}

void initFileList( HWND hwnd )
{
    HWND                lb;
    GetFilesInfo        *info;

    info = (GetFilesInfo *)GET_DLGDATA( hwnd );
    lb = GetDlgItem( hwnd, FOD_FILELIST );
    if( (*info->result).size() > 0 ) {
        WStringList names( *info->result );
        for( ; names.count() > 0; ) {
            SendMessage( lb, LB_ADDSTRING, 0, (LPARAM)(LPSTR)names.cStringAt( 0 ) );
            names.removeAt( 0 );
        }
    }
}

void doClose( HWND hwnd )
{
    HWND         ctl;

    ctl = GetDlgItem( hwnd, FOD_FILENAME );
    SetWindowText( ctl, "x_it_dlg.now" );
}

UINT_PTR CALLBACK AddSrcDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    WORD        cmd;
    int         item;
    LRESULT     rc;
    HWND        ctl;

    lparam = lparam;
    switch( msg ) {
    case WM_INITDIALOG:
        {
            OPENFILENAME        *of;

            ctl = GetDlgItem( hwnd, FOD_REMOVE );
            EnableWindow( ctl, FALSE );
            ctl = GetDlgItem( hwnd, IDOK );
            EnableWindow( ctl, FALSE );
            of = (OPENFILENAME *)lparam;
            SET_DLGDATA( hwnd, of->lCustData );
            initFileList( hwnd );
        }
        break;
    case WM_COMMAND:
        cmd = LOWORD( wparam );
        switch( cmd ) {
        case FOD_FILES:
            if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_DBLCLK ) {
                addCurrentFile( hwnd );
                return( TRUE );
            } else {
                return( FALSE );
            }
        case IDOK:
            return( FALSE );
        case FOD_ADD:
            addCurrentFile( hwnd );
            PostMessage( hwnd, WM_COMMAND, IDOK, 0 );
            return( TRUE );
        case FOD_ADDALL:
            addAllFiles( hwnd );
            break;
        case FOD_REMOVE:
            ctl = GetDlgItem( hwnd, FOD_FILELIST );
            item = (int)SendMessage( ctl, LB_GETCURSEL, 0, 0 );
            if( item != LB_ERR ) {
                rc = SendMessage( ctl, LB_DELETESTRING, item, 0 );
                if( item != 0 ) {
                    SendMessage( ctl, LB_SETCURSEL, item - 1, 0 );
                } else {
                    SendMessage( ctl, LB_SETCURSEL, item, 0 );
                }
            }
            checkRemoveButton( hwnd );
            break;
        case FOD_FILELIST:
            if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCHANGE ||
                GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCANCEL ) {
                checkRemoveButton( hwnd );
            }
            break;
        case FOD_CLOSE:
            GetResults( hwnd );
            doClose( hwnd );
            PostMessage( hwnd, WM_COMMAND, IDOK, 0 );
            return( TRUE );
        default:
            return( FALSE );
        }
        break;
    default:
        return( FALSE );
    }
    return( TRUE );
}

#ifdef __NT__
UINT_PTR CALLBACK AddSrcDlgProc95( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    WORD            cmd;
    int             item;
    LRESULT         rc;
    HWND            ctl;
    HWND            dlg;
    GetFilesInfo    *info;

    lparam = lparam;
    switch( msg ) {
    case WM_INITDIALOG:
        {
            OPENFILENAME        *of;

            of = (OPENFILENAME *)lparam;
            ctl = GetDlgItem( hwnd, FOD_REMOVE );
            EnableWindow( ctl, FALSE );
            dlg = GetParent( hwnd );
            SendMessage( dlg, CDM_SETCONTROLTEXT, IDOK, (LPARAM)"&Add" );
            SET_DLGDATA( hwnd, of->lCustData );
            info = (GetFilesInfo *)of->lCustData;
            info->filter_index = of->nFilterIndex;
            info->filter = of->lpstrFilter;
            initFileList( hwnd );
        }
        break;
    case WM_COMMAND:
        cmd = LOWORD( wparam );
        switch( cmd ) {
        case FOD_ADD:
            addCurrentFile95( hwnd );
            return( TRUE );
        case FOD_ADDALL:
            addAllFiles95( hwnd );
            break;
        case FOD_REMOVE:
            ctl = GetDlgItem( hwnd, FOD_FILELIST );
            item = (int)SendMessage( ctl, LB_GETCURSEL, 0, 0 );
            if( item != LB_ERR ) {
                rc = SendMessage( ctl, LB_DELETESTRING, item, 0 );
                if( item != 0 ) {
                    SendMessage( ctl, LB_SETCURSEL, item - 1, 0 );
                } else {
                    SendMessage( ctl, LB_SETCURSEL, item, 0 );
                }
            }
            checkRemoveButton( hwnd );
            break;
        case FOD_FILELIST:
            if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCHANGE ||
                GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCANCEL ) {
                checkRemoveButton( hwnd );
            }
            break;
        case FOD_CLOSE:
            GetResults( hwnd );
            info = (GetFilesInfo *)GET_DLGDATA( hwnd );
            info->ret_code = IDOK;
            PostMessage( GetParent( hwnd ), WM_COMMAND, IDCANCEL, 0L );
            return( TRUE );
        case IDCANCEL:
            info = (GetFilesInfo *)GET_DLGDATA( hwnd );
            info->ret_code = IDCANCEL;
            PostMessage( GetParent( hwnd ), WM_COMMAND, IDCANCEL, 0L );
            return( TRUE );
        default:
            return( FALSE );
        }
        break;
    case WM_NOTIFY:
        switch( ((NMHDR *)lparam)->code ) {
        case CDN_FILEOK:
            PostMessage( hwnd, WM_COMMAND, FOD_ADD, 0L );
            SET_DLGRESULT( hwnd, 1L );
            return( TRUE );
        case CDN_TYPECHANGE:
            info = (GetFilesInfo *)GET_DLGDATA( hwnd );
            info->filter_index = ((OFNOTIFY *)lparam)->lpOFN->nFilterIndex;
            break;
        }
        break;
    default:
        return( FALSE );
    }
    return( TRUE );
}
#endif

static BOOL fileSelectDlg( HINSTANCE hinst, HWND parent, GetFilesInfo *info,
                    const char *caption, const char *filters  )
{
    static DWORD    last_filter_index = 1L;
    OPENFILENAME    of;
    int             rc;
    char            fname[256];
    pgroup2         pg;
    char            newpath[_MAX_PATH];

    _splitpath2( info->tgt_file, pg.buffer, &pg.drive, &pg.dir, NULL, NULL );
    _makepath( newpath, pg.drive, pg.dir, NULL, NULL );
    fname[0] = 0;
    memset( &of, 0, sizeof( OPENFILENAME ) );
    of.lStructSize = sizeof( OPENFILENAME );
    of.hwndOwner = parent;
    of.hInstance = hinst;
    of.lpstrFilter = (LPSTR) filters;
    of.lpstrDefExt = "";
    of.nFilterIndex = last_filter_index;
    of.lpstrFile = fname;
    of.nMaxFile = sizeof( fname );
    of.lpstrTitle = caption;
    of.lCustData = (LPARAM)info;
    of.lpstrInitialDir = newpath;
#if defined( __NT__ )
  #if !defined( _WIN64 )
    if( LOBYTE( LOWORD( GetVersion() ) ) >= 4 ) {
  #endif
        of.lpTemplateName = "ADD_SRC_DLG_95";
        of.Flags = OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER;
        of.lpfnHook = AddSrcDlgProc95;
  #if !defined( _WIN64 )
    } else {
  #endif
#endif
#if !defined( _WIN64 )
        of.lpTemplateName = "ADD_SRC_DLG";
        of.Flags = OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
        of.lpfnHook = MakeProcInstance_OFNHOOK( AddSrcDlgProc, hinst );
#endif
#if defined( __NT__ )
  #if !defined( _WIN64 )
    }
  #endif
#endif
    rc = GetOpenFileName( &of );
    FreeProcInstance_OFNHOOK( of.lpfnHook );
    last_filter_index = of.nFilterIndex;
#if defined( __NT__ )
  #if !defined( _WIN64 )
    if( LOBYTE( LOWORD( GetVersion() ) ) >= 4 ) {
  #endif
        rc = info->ret_code;
  #if !defined( _WIN64 )
    }
  #endif
#endif
    return( rc );
}

int GetNewFiles( WWindow *parent, WString *results, const char *caption,
                 const char *filters, const char *tgt_file )
{
    HWND                owner;
    HINSTANCE           hinst;
    GetFilesInfo        info;

    // this is a grody kludge to get an HWND from a gui_window
    // it assumes that the HWND is the first field in the gui_window structure
    owner = *( (HWND *)(parent->handle() ) );
    hinst = (HINSTANCE)GET_HINSTANCE( owner );
    info.result = results;
    info.tgt_file = tgt_file;
    return( fileSelectDlg( hinst, owner, &info, caption, filters ) );
}
