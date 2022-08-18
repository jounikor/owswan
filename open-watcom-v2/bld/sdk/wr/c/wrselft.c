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


#include "wrglbl.h"
#include "wrmsg.h"
#include "wrmaini.h"
#include "wrdmsgi.h"
#include "selft.rh"
#include "jdlg.h"
#include "winexprt.h"
#include "wclbproc.h"
#include "pathgrp2.h"

#include "clibext.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct {
    const char          *file_name;
    bool                is32bit;
    bool                use_wres;
    WRFileType          file_type;
    HELPFUNC            help_callback;
} WRSFT;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/
WINEXPORT extern INT_PTR CALLBACK WRSelectFileTypeDlgProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/*****************************************************************************/
static void WRSetWinInfo( HWND, WRSFT * );
static bool WRGetWinInfo( HWND, WRSFT * );

/****************************************************************************/
/* external variables                                                       */
/****************************************************************************/

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WRFileType WRFTARRAY[3][2][2] = {
    { { WR_WIN16W_RES, WR_WIN16M_RES }, { WR_WINNTW_RES, WR_WINNTM_RES } },
    { { WR_WIN16_EXE,  WR_WIN16_EXE  }, { WR_WINNT_EXE,  WR_WINNT_EXE  } },
    { { WR_WIN16_DLL,  WR_WIN16_DLL  }, { WR_WINNT_DLL,  WR_WINNT_DLL  } }
};

static WRFileType educatedGuess( const char *name, bool is32bit, bool use_wres )
{
    pgroup2     pg;
    WRFileType  guess;

    if( name == NULL ) {
        return( WR_DONT_KNOW );
    }

    guess = WR_DONT_KNOW;

    _splitpath2( name, pg.buffer, NULL, NULL, NULL, &pg.ext );

    if( CMPFEXT( pg.ext, "exe" ) ) {
        if( is32bit ) {
            guess = WR_WINNT_EXE;
        } else {
            guess = WR_WIN16_EXE;
        }
    } else if( CMPFEXT( pg.ext, "dll" ) ) {
        if( is32bit ) {
            guess = WR_WINNT_DLL;
        } else {
            guess = WR_WIN16_DLL;
        }
    } else if( CMPFEXT( pg.ext, "res" ) ) {
        if( is32bit ) {
            guess = WR_WINNTW_RES;
        } else {
            if( use_wres ) {
                guess = WR_WIN16W_RES;
            } else {
                guess = WR_WIN16M_RES;
            }
        }
    }

    return( guess );
}

WRFileType WRAPI WRSelectFileType( HWND parent, const char *name, bool is32bit,
                                       bool use_wres, HELPFUNC help_callback )
{
    DLGPROC     dlgproc;
    HINSTANCE   inst;
    INT_PTR     modified;
    WRSFT       sft;
    WRFileType  guess;

    guess = WRGuessFileType( name );
    if( guess != WR_DONT_KNOW ) {
        return( guess );
    }

    guess = educatedGuess( name, is32bit, use_wres );
    if( guess != WR_DONT_KNOW ) {
        return( guess );
    }

    sft.help_callback = help_callback;
    sft.file_name = name;
    sft.file_type = WR_DONT_KNOW;
    sft.is32bit = is32bit;
    sft.use_wres  = use_wres;
    inst = WRGetInstance();

    dlgproc = MakeProcInstance_DLG( WRSelectFileTypeDlgProc, inst );

    modified = JDialogBoxParam( inst, "WRSelectFileType", parent, dlgproc, (LPARAM)(LPVOID)&sft );

    FreeProcInstance_DLG( dlgproc );

    if( modified == -1 ) {
        return( WR_DONT_KNOW );
    }

    return( sft.file_type );
}

WRFileType WRAPI WRGuessFileType( const char *name )
{
    pgroup2     pg;
    WRFileType  guess;

    if( name == NULL ) {
        return( WR_DONT_KNOW );
    }

    guess = WR_DONT_KNOW;

    _splitpath2( name, pg.buffer, NULL, NULL, NULL, &pg.ext );

    if( CMPFEXT( pg.ext, "bmp" ) ) {
        guess = WR_WIN_BITMAP;
    } else if( CMPFEXT( pg.ext, "cur" ) ) {
        guess = WR_WIN_CURSOR;
    } else if( CMPFEXT( pg.ext, "ico" ) ) {
        guess = WR_WIN_ICON;
    } else if( CMPFEXT( pg.ext, "rc" ) ) {
        guess = WR_WIN_RC;
    } else if( CMPFEXT( pg.ext, "dlg" ) ) {
        guess = WR_WIN_RC_DLG;
    } else if( CMPFEXT( pg.ext, "str" ) ) {
        guess = WR_WIN_RC_STR;
    } else if( CMPFEXT( pg.ext, "mnu" ) ) {
        guess = WR_WIN_RC_MENU;
    } else if( CMPFEXT( pg.ext, "acc" ) ) {
        guess = WR_WIN_RC_ACCEL;
    }

    return( guess );
}

void WRSetWinInfo( HWND hDlg, WRSFT *sft )
{
    pgroup2     pg;
    bool        no_exe;

    if( sft == NULL ) {
        return;
    }

    no_exe = false;

    if( sft->file_name != NULL ) {
        SendDlgItemMessage( hDlg, IDM_FILENAME, WM_SETTEXT, 0, (LPARAM)(LPCSTR)sft->file_name );
        _splitpath2( sft->file_name, pg.buffer, NULL, NULL, NULL, &pg.ext );
        if( CMPFEXT( pg.ext, "res" ) ) {
            CheckDlgButton( hDlg, IDM_FTRES, BST_CHECKED );
            no_exe = true;
        } else if( CMPFEXT( pg.ext, "exe" ) ) {
            CheckDlgButton( hDlg, IDM_FTEXE, BST_CHECKED );
        } else if( CMPFEXT( pg.ext, "dll" ) ) {
            CheckDlgButton( hDlg, IDM_FTDLL, BST_CHECKED );
        }
    }

#ifdef __NT__
    if( sft->is32bit ) {
        CheckDlgButton( hDlg, IDM_TSWINNT, BST_CHECKED );
    } else {
        CheckDlgButton( hDlg, IDM_TSWIN, BST_CHECKED );
    }
#else
    EnableWindow( GetDlgItem( hDlg, IDM_TSWINNT ), FALSE );
    CheckDlgButton( hDlg, IDM_TSWIN, BST_CHECKED );
#endif

    if( no_exe ) {
        EnableWindow( GetDlgItem( hDlg, IDM_FTEXE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDM_FTDLL ), FALSE );
        if( sft->is32bit ) {
            EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), FALSE );
        }
        if( sft->use_wres || sft->is32bit ) {
            CheckDlgButton( hDlg, IDM_RFWAT, BST_CHECKED );
        } else {
            CheckDlgButton( hDlg, IDM_RFMS, BST_CHECKED );
        }
    } else {
        EnableWindow( GetDlgItem( hDlg, IDM_RFWAT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), FALSE );
    }
}

bool WRGetWinInfo( HWND hDlg, WRSFT *sft )
{
    int ft, ts, rf;

    if( sft == NULL ) {
        return( true );
    }

    if( IsDlgButtonChecked( hDlg, IDM_FTRES ) ) {
        ft = 0;
    } else if( IsDlgButtonChecked( hDlg, IDM_FTEXE ) ) {
        ft = 1;
    } else if( IsDlgButtonChecked( hDlg, IDM_FTDLL ) ) {
        ft = 2;
    } else {
        ft = -1;
    }

    if( IsDlgButtonChecked( hDlg, IDM_TSWIN ) ) {
        ts = 0;
    } else if( IsDlgButtonChecked( hDlg, IDM_TSWINNT ) ) {
        ts = 1;
    } else {
        ts = -1;
    }

    if( ft != 0 ) {
        rf = 0;
    } else {
        if( IsDlgButtonChecked( hDlg, IDM_RFWAT ) ) {
            rf = 0;
        } else if( IsDlgButtonChecked( hDlg, IDM_RFMS ) ) {
            rf = 1;
        } else {
            rf = -1;
        }
    }

    if( ft >= 0 && ts >= 0 && rf >= 0 ) {
        sft->file_type = WRFTARRAY[ft][ts][rf];
    } else {
        sft->file_type = WR_DONT_KNOW;
        return( false );
    }
    return( true );
}

WINEXPORT INT_PTR CALLBACK WRSelectFileTypeDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    WRSFT       *sft;
    bool        ret;

    ret = false;

    switch( message ) {
    case WM_DESTROY:
        WRUnregisterDialog( hDlg );
        break;

    case WM_INITDIALOG:
        sft = (WRSFT *)lParam;
        SET_DLGDATA( hDlg, sft );
        WRRegisterDialog( hDlg );
        WRSetWinInfo( hDlg, sft );
        ret = true;
        break;

    case WM_SYSCOLORCHANGE:
        WRCtl3dColorChange();
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDM_SFTHELP:
            sft = (WRSFT *)GET_DLGDATA( hDlg );
            if( sft != NULL && sft->help_callback != NULL ) {
                sft->help_callback();
            }
            break;

        case IDOK:
            sft = (WRSFT *)GET_DLGDATA( hDlg );
            if( sft == NULL ) {
                EndDialog( hDlg, FALSE );
                ret = true;
            } else if( WRGetWinInfo( hDlg, sft ) ) {
                EndDialog( hDlg, TRUE );
                ret = true;
            } else {
                WRDisplayErrorMsg( WR_INVALIDSELECTION );
            }
            break;

        case IDCANCEL:
            EndDialog( hDlg, FALSE );
            ret = true;
            break;

        case IDM_TSWINNT:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) != BN_CLICKED ) {
                break;
            }
            if( !IsDlgButtonChecked( hDlg, IDM_FTRES ) ) {
                break;
            }
            if( IsDlgButtonChecked( hDlg, LOWORD( wParam ) ) ) {
                EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), FALSE );
                CheckDlgButton( hDlg, IDM_RFMS, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDM_RFWAT, BST_CHECKED );
            } else {
                EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), TRUE );
            }
            break;

        case IDM_TSWIN:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) != BN_CLICKED ) {
                break;
            }
            if( !IsDlgButtonChecked( hDlg, IDM_FTRES ) ) {
                break;
            }
            if( IsDlgButtonChecked( hDlg, LOWORD( wParam ) ) ) {
                EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), TRUE );
            }
            break;

        case IDM_FTEXE:
        case IDM_FTDLL:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) != BN_CLICKED ) {
                break;
            }
            if( IsDlgButtonChecked( hDlg, LOWORD( wParam ) ) ) {
                EnableWindow( GetDlgItem( hDlg, IDM_RFWAT ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), FALSE );
                CheckDlgButton( hDlg, IDM_RFWAT, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDM_RFMS, BST_UNCHECKED );
            }
            break;

        case IDM_FTRES:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) != BN_CLICKED ) {
                break;
            }
            if( IsDlgButtonChecked( hDlg, LOWORD( wParam ) ) ) {
                EnableWindow( GetDlgItem( hDlg, IDM_RFWAT ), TRUE );
                if( IsDlgButtonChecked( hDlg, IDM_TSWINNT ) ) {
                    EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), FALSE );
                    CheckDlgButton( hDlg, IDM_RFMS, BST_UNCHECKED );
                    CheckDlgButton( hDlg, IDM_RFWAT, BST_CHECKED );
                } else {
                    EnableWindow( GetDlgItem( hDlg, IDM_RFMS ), TRUE );
                    CheckDlgButton( hDlg, IDM_RFMS, BST_UNCHECKED );
                    CheckDlgButton( hDlg, IDM_RFWAT, BST_CHECKED );
                }
            }
            break;
        }
        break;
    }

    return( ret );
}
