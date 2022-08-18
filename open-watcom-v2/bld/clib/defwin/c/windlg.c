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


#include "variety.h"
#define INCLUDE_COMMDLG_H
#include <wwindows.h>
#include "win.h"
#include "windlg.rh"


#define MAXDLGITEMDATA  (1)

typedef struct {
   int      x, y, cx, cy;
   WORD     id;
   DWORD    style;
   char     *classname;
   char     *captiontext;
   BYTE     infodata[MAXDLGITEMDATA];
   BYTE     infodatalen;
} itemdata;

WINEXPORT BOOL     CALLBACK IntervalDialogProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam );

static itemdata _getint[] = {
{  4,  10, 50, 24, -1, SS_LEFT, "STATIC", "&Enter Number of Lines:", { 0 }, 0 },

{ 55,  14, 68, 12, DLG1_EDIT, ES_LEFT | WS_BORDER | WS_TABSTOP | WS_GROUP, "EDIT", "", { 0 }, 0 },

{ 30,  38, 36, 12, IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, "BUTTON", "&OK", { 0 }, 0 },

{ 84,  38, 36, 12, IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP | WS_GROUP, "BUTTON", "&Cancel", { 0 }, 0 }
};

#define MAX_INT_ITEMS sizeof( _getint ) / sizeof( itemdata )


/*
 * IntervalDialogProc - control dialog for getting interval
 */
BOOL CALLBACK IntervalDialogProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    char        tmp[128];

    lparam = lparam;
    switch( message ) {
    case WM_INITDIALOG:
        ultoa( _AutoClearLines, tmp, 10 );
        SendDlgItemMessage( hwnd, DLG1_EDIT, EM_REPLACESEL, 0, (LPARAM)(LPSTR)tmp );
        return( TRUE );

    case WM_COMMAND:
        if( LOWORD( wparam ) == IDOK ) {
            GetDlgItemText( hwnd, DLG1_EDIT, (LPSTR)tmp, 128 );
            _AutoClearLines = (DWORD)atol( tmp );
            EndDialog( hwnd, TRUE );
            return( TRUE );
        } else if( wparam == IDCANCEL ) {
            EndDialog( hwnd, TRUE );
            return( TRUE );
        }
        break;
    case WM_CLOSE:
        EndDialog( hwnd, TRUE );
        return( TRUE );
    }

    return( FALSE );

} /* IntervalDialogProc */

/*
 * _GetAutoClearInterval - return number of lines between clearing
 */
void _GetAutoClearInterval( void )
{
    TEMPLATE_HANDLE     old_dlgtemplate;
    TEMPLATE_HANDLE     new_dlgtemplate;
    int                 i;
    size_t              templatelen;

    old_dlgtemplate = _DialogTemplate( DS_LOCALEDIT | DS_MODALFRAME | WS_CAPTION | WS_SYSMENU,
                22, 25, 150, 60, "", "",
                "Set Number of Lines Between Auto-Clears",
                0, "", &templatelen );

    if( old_dlgtemplate == NULL )
        return;
    for( i = 0; i < MAX_INT_ITEMS; i++ ) {
        new_dlgtemplate = _AddControl( old_dlgtemplate,
                _getint[i].x, _getint[i].y, _getint[i].cx, _getint[i].cy,
                _getint[i].id, _getint[i].style | WS_VISIBLE,
                _getint[i].classname, _getint[i].captiontext,
                _getint[i].infodata, _getint[i].infodatalen, &templatelen );
        if( new_dlgtemplate == NULL  ) {
            GlobalFree( old_dlgtemplate );
            return;
        }
        old_dlgtemplate = new_dlgtemplate;
    }

    _DoneAddingControls( old_dlgtemplate );
    _DynamicDialogBox( IntervalDialogProc, _MainWindowData->inst, _MainWindow, old_dlgtemplate );

} /* _GetAutoClearInterval */
