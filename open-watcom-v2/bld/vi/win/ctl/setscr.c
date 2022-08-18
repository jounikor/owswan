/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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


#include "vi.h"
#include "setscr.rh"
#include "stddef.h"
#include "ctltype.h"
#include "util.h"
#include "wclbproc.h"
#include "winctl.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK SetScrDlgProc( HWND hwndDlg, UINT msg, WPARAM wparam, LPARAM lparam );

#define FILEENDSTRINGWIDTH      200

typedef struct {
    int         PageLinesExposed;
    boolbit     JumpyScroll         : 1;
    boolbit     LineBased           : 1;
    boolbit     SavePosition        : 1;
    boolbit     AutoMessageClear    : 1;
    char        FileEndString[FILEENDSTRINGWIDTH];
} dlg_data;

static  dlg_data    dlgData;

static void scrn_get( void *dlg, ctl_elt *ctl, void *data )
{
    switch( ctl->control ) {
    case SETSCR_PAGELINESEXPOSED:
        *(int *)data = ((dlg_data *)dlg)->PageLinesExposed;
        break;
    case SETSCR_JUMPYSCROLL:
        *(bool *)data = ((dlg_data *)dlg)->JumpyScroll;
        break;
    case SETSCR_LINEBASED:
        *(bool *)data = ((dlg_data *)dlg)->LineBased;
        break;
    case SETSCR_SAVEPOSITION:
        *(bool *)data = ((dlg_data *)dlg)->SavePosition;
        break;
    case SETSCR_AUTOMESSAGECLEAR:
        *(bool *)data = ((dlg_data *)dlg)->AutoMessageClear;
        break;
    case SETSCR_FILEENDSTRING:
        *(char **)data = ((dlg_data *)dlg)->FileEndString;
        break;
    }
}

static void scrn_set( void *dlg, ctl_elt *ctl, void *data )
{
    switch( ctl->control ) {
    case SETSCR_PAGELINESEXPOSED:
        ((dlg_data *)dlg)->PageLinesExposed = *(int *)data;
        break;
    case SETSCR_JUMPYSCROLL:
        ((dlg_data *)dlg)->JumpyScroll = *(bool *)data;
        break;
    case SETSCR_LINEBASED:
        ((dlg_data *)dlg)->LineBased = *(bool *)data;
        break;
    case SETSCR_SAVEPOSITION:
        ((dlg_data *)dlg)->SavePosition = *(bool *)data;
        break;
    case SETSCR_AUTOMESSAGECLEAR:
        ((dlg_data *)dlg)->AutoMessageClear = *(bool *)data;
        break;
    case SETSCR_FILEENDSTRING:
        *(char **)data = ((dlg_data *)dlg)->FileEndString;
        break;
    }
}

#include "setscr.gch"

static void globalTodlgData( void )
{
    dlgData.JumpyScroll = EditFlags.JumpyScroll;
    dlgData.LineBased = EditFlags.LineBased;
    dlgData.PageLinesExposed = EditVars.PageLinesExposed;
    strncpy( dlgData.FileEndString, EditVars.FileEndString, FILEENDSTRINGWIDTH - 1 );
    dlgData.SavePosition = EditFlags.SavePosition;
    dlgData.AutoMessageClear = EditFlags.AutoMessageClear;
}

static void dlgDataToGlobal( void )
{
    UtilUpdateBoolean( EditFlags.JumpyScroll, dlgData.JumpyScroll, "jumpyscroll" );
    UtilUpdateBoolean( EditFlags.LineBased, dlgData.LineBased, "linebased" );
    UtilUpdateBoolean( EditFlags.SavePosition, dlgData.SavePosition, "saveposition" );
    UtilUpdateBoolean( EditFlags.AutoMessageClear, dlgData.AutoMessageClear, "automessageclear" );
    UtilUpdateInt( EditVars.PageLinesExposed, dlgData.PageLinesExposed, "pagelinesexposed" );
    UtilUpdateStr( EditVars.FileEndString, dlgData.FileEndString, "fileendstring" );
}

static void setdlgDataDefaults( void )
{
    // this sort of suck ssince the default values aren't isolated in 1 place
    dlgData.JumpyScroll = true;
    dlgData.LineBased = false;
    dlgData.PageLinesExposed = 1;
    dlgData.FileEndString[0] = '\0';
    dlgData.SavePosition = true;
    dlgData.AutoMessageClear = true;
}

/*
 * SetScrDlgProc - processes messages for the Data Control Dialog
 */
WINEXPORT INT_PTR CALLBACK SetScrDlgProc( HWND hwndDlg, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch( msg ) {
    case WM_INITDIALOG:
        CenterWindowInRoot( hwndDlg );
        globalTodlgData();
        ctl_dlg_init( GET_HINSTANCE( hwndDlg ), hwndDlg, &dlgData, &Ctl_setscr );
        return( TRUE );

    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case SETSCR_DEFAULTS:
            setdlgDataDefaults();
            ctl_dlg_reset( GET_HINSTANCE( hwndDlg ),
                           hwndDlg, &dlgData, &Ctl_setscr, TRUE );
            return( TRUE );
        case IDOK:
            if( !ctl_dlg_done( GET_HINSTANCE( hwndDlg ),
                               hwndDlg, &dlgData, &Ctl_setscr ) ) {
                return( TRUE );
            }
            dlgDataToGlobal();

            // fall through
        case IDCANCEL:
            EndDialog( hwndDlg, TRUE );
            return( TRUE );
        }

        ctl_dlg_process( &Ctl_setscr, wparam, lparam );
    }

    return( FALSE );
}

/*
 * GetSetScrDialog - create dialog box & get result
 */
bool GetSetScrDialog( void )
{
    DLGPROC     dlgproc;
    bool        rc;

    dlgproc = MakeProcInstance_DLG( SetScrDlgProc, InstanceHandle );
    rc = ( DialogBox( InstanceHandle, "SETSCR", root_window_id, dlgproc ) != 0 );
    FreeProcInstance_DLG( dlgproc );

    // redisplay all files to ensure screen completely correct
    ReDisplayBuffers( false );
    return( rc );

} /* GetSetScrDialog */
