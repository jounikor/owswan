/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2016 The Open Watcom Contributors. All Rights Reserved.
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


#include "drwatcom.h"
#include "memory.rh"
#include "jdlg.h"


#ifndef CHICAGO

/* Local Window callback functions prototypes */
WINEXPORT INT_PTR CALLBACK MemInfoDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

typedef struct {
    DWORD       procid;
} MemDlgInfo;

/*
 * fillImageInfo
 */
static void fillImageInfo( HWND hwnd, DWORD procid ) {

    BOOL        ok;
    MemInfo     procinfo;
    MemByType   imageinfo;
    MemByType   *info;
    char        buf[50];
    char        imagename[_MAX_PATH];
    int         id;

    id = (int)SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_GETCURSEL, 0, 0L );
    SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_GETLBTEXT, id, (LPARAM)(LPSTR)imagename );
    if( strcmp( imagename, TOTAL_MEM_STR ) == 0 ) {
        ok = GetMemInfo( procid, &procinfo );
        info = &procinfo.image;
    } else {
        ok = GetImageMemInfo( procid, imagename, &imageinfo );
        info = &imageinfo;
    }
    if( ok ) {
        sprintf( buf, "%ld KB", info->noaccess /1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_NO_ACCESS, buf );
        sprintf( buf, "%ld KB", info->read / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_READ, buf );
        sprintf( buf, "%ld KB", info->write / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_WRITE, buf );
        sprintf( buf, "%ld KB", info->copy / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_COPY, buf );
        sprintf( buf, "%ld KB", info->exec / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC, buf );
        sprintf( buf, "%ld KB", info->execread / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC_READ, buf );
        sprintf( buf, "%ld KB", info->execwrite / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC_WRITE, buf );
        sprintf( buf, "%ld KB", info->execcopy / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC_COPY, buf );
        sprintf( buf, "%ld KB", info->tot / 1024 );
        SetDlgItemText( hwnd, MEM_IMAGE_TOTAL, buf );
    } else {
        SetDlgItemText( hwnd, MEM_IMAGE_NO_ACCESS, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_READ, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_WRITE, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_COPY, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC_READ, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC_WRITE, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_EXEC_COPY, "??????" );
        SetDlgItemText( hwnd, MEM_IMAGE_TOTAL, "??????" );
    }
}

/*
 * fillMemInfo
 */
static void fillMemInfo( HWND hwnd, DWORD procid, BOOL first_time ) {

    MemInfo     data;
    ProcStats   procinfo;
    char        buf[128];
    char        imagename[_MAX_PATH];
    DWORD       i;
    LRESULT     id;

    if( GetProcessInfo( procid, &procinfo ) ) {
        RCsprintf( buf, STR_MEM_INFO_FOR_PROC, procid, procinfo.name );
    } else {
        RCsprintf( buf, STR_MEM_INFO_FOR_PROC, procid, "" );
    }
    SetWindowText( hwnd, buf );
    if( GetMemInfo( procid, &data ) ) {
        sprintf( buf, "%ld KB", data.mapped.noaccess /1024 );
        SetDlgItemText( hwnd, MEM_MAP_NO_ACCESS, buf );
        sprintf( buf, "%ld KB", data.mapped.read / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_READ, buf );
        sprintf( buf, "%ld KB", data.mapped.write / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_WRITE, buf );
        sprintf( buf, "%ld KB", data.mapped.copy / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_COPY, buf );
        sprintf( buf, "%ld KB", data.mapped.exec / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_EXEC, buf );
        sprintf( buf, "%ld KB", data.mapped.execread / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_EXEC_READ, buf );
        sprintf( buf, "%ld KB", data.mapped.execwrite / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_EXEC_WRITE, buf );
        sprintf( buf, "%ld KB", data.mapped.execcopy / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_EXEC_COPY, buf );
        sprintf( buf, "%ld KB", data.mapped.tot / 1024 );
        SetDlgItemText( hwnd, MEM_MAP_TOTAL, buf );

        sprintf( buf, "%ld KB", data.res.noaccess /1024 );
        SetDlgItemText( hwnd, MEM_RES_NO_ACCESS, buf );
        sprintf( buf, "%ld KB", data.res.read / 1024 );
        SetDlgItemText( hwnd, MEM_RES_READ, buf );
        sprintf( buf, "%ld KB", data.res.write / 1024 );
        SetDlgItemText( hwnd, MEM_RES_WRITE, buf );
        sprintf( buf, "%ld KB", data.res.copy / 1024 );
        SetDlgItemText( hwnd, MEM_RES_COPY, buf );
        sprintf( buf, "%ld KB", data.res.exec / 1024 );
        SetDlgItemText( hwnd, MEM_RES_EXEC, buf );
        sprintf( buf, "%ld KB", data.res.execread / 1024 );
        SetDlgItemText( hwnd, MEM_RES_EXEC_READ, buf );
        sprintf( buf, "%ld KB", data.res.execwrite / 1024 );
        SetDlgItemText( hwnd, MEM_RES_EXEC_WRITE, buf );
        sprintf( buf, "%ld KB", data.res.execcopy / 1024 );
        SetDlgItemText( hwnd, MEM_RES_EXEC_COPY, buf );
        sprintf( buf, "%ld KB", data.res.tot / 1024 );
        SetDlgItemText( hwnd, MEM_RES_TOTAL, buf );
        if( !first_time ) {
            id = SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_GETCURSEL, 0, 0L );
            if( id != CB_ERR ) {
                SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_GETLBTEXT, (WPARAM)id, (LPARAM)(LPSTR)imagename );
            }
        } else {
            id = CB_ERR;
        }
        SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_RESETCONTENT, 0, 0L );
        for( i = 0; i < data.modcnt; i++ ) {
            SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)data.modlist[i] );
        }
        SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)TOTAL_MEM_STR );
        if( id != CB_ERR ) {
            id = SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_FINDSTRINGEXACT, -1, (LPARAM)(LPCSTR)imagename );
        }
        if( id == CB_ERR ) {
            SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_SELECTSTRING, -1, (LPARAM)(LPCSTR)TOTAL_MEM_STR );
        } else {
            SendDlgItemMessage( hwnd, MEM_IMAGE_NAME, CB_SETCURSEL, (WPARAM)id, 0L );
        }
        FreeModuleList( data.modlist, data.modcnt );
    } else {
        SetDlgItemText( hwnd, MEM_MAP_NO_ACCESS, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_READ, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_WRITE, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_COPY, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_EXEC, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_EXEC_READ, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_EXEC_WRITE, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_EXEC_COPY, "??????" );
        SetDlgItemText( hwnd, MEM_MAP_TOTAL, "??????" );
        SetDlgItemText( hwnd, MEM_RES_NO_ACCESS, "??????" );
        SetDlgItemText( hwnd, MEM_RES_READ, "??????" );
        SetDlgItemText( hwnd, MEM_RES_WRITE, "??????" );
        SetDlgItemText( hwnd, MEM_RES_COPY, "??????" );
        SetDlgItemText( hwnd, MEM_RES_EXEC, "??????" );
        SetDlgItemText( hwnd, MEM_RES_EXEC_READ, "??????" );
        SetDlgItemText( hwnd, MEM_RES_EXEC_WRITE, "??????" );
        SetDlgItemText( hwnd, MEM_RES_EXEC_COPY, "??????" );
        SetDlgItemText( hwnd, MEM_RES_TOTAL, "??????" );
    }
    fillImageInfo( hwnd, procid );
}

/*
 * MemInfoDlgProc
 */
INT_PTR CALLBACK MemInfoDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    WORD        cmd;
    MemDlgInfo  *info;
    bool        ret;

    ret = false;

    info = (MemDlgInfo *)GET_DLGDATA( hwnd );
    switch( msg ) {
    case WM_INITDIALOG:
        info = MemAlloc( sizeof( MemDlgInfo ) );
        info->procid = lparam;
        SET_DLGDATA( hwnd, info );
        fillMemInfo( hwnd, info->procid, TRUE );
        ret = true;
        break;
    case WM_COMMAND:
        cmd = LOWORD( wparam );
        switch( cmd ) {
        case IDOK:
            SendMessage( hwnd, WM_CLOSE, 0, 0 );
            break;
        case MEM_REFRESH:
            RefreshCostlyInfo();
            fillMemInfo( hwnd, info->procid, FALSE );
            break;
        case MEM_IMAGE_NAME:
            if( HIWORD( wparam ) == CBN_SELCHANGE ) {
                fillImageInfo( hwnd, info->procid );
            }
            break;
        }
        ret = true;
        break;
    case WM_CLOSE:
        MemFree( info );
        EndDialog( hwnd, 0 );
        ret = true;
        break;
    }
    return( ret );
}

void DoMemDlg( HWND hwnd, DWORD procid )
{
    RefreshCostlyInfo();
    JDialogBoxParam( Instance, "MEMORY_DLG", hwnd, MemInfoDlgProc, procid );
}
#endif
