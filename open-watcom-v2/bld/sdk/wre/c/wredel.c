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


#include "wreglbl.h"
#include <ddeml.h>
#include "waccel.h"
#include "wmenu.h"
#include "wstring.h"
#include "wremain.h"
#include "wrenames.h"
#include "wregcres.h"
#include "wreseted.h"
#include "wrectl3d.h"
#include "wredel.h"
#include "wre.rh"
#include "wrdll.h"
#include "wresall.h"
#include "wredde.h"
#include "wreaccel.h"
#include "wremenu.h"
#include "wrestr.h"
#include "wredlg.h"
#include "wremsg.h"
#include "ldstr.h"
#include "wreimage.h"
#include "wreimg.h"
#include "jdlg.h"
#include "wresdefn.h"
#include "wclbproc.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/
WINEXPORT INT_PTR CALLBACK WREResDeleteDlgProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static void WRESetWinInfo( HWND, char * );
static bool WREQueryDeleteName( char * );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

bool WREDeleteCurrResource( bool force )
{
    WRECurrentResInfo   curr;
    bool                ok;

    WREGetCurrentResource( &curr );

    ok = WREDeleteResource( &curr, force );

    return( ok );
}

bool WREDeleteResource( WRECurrentResInfo *curr, bool force )
{
    char                *type_name;
    uint_16             type_id;
    bool                ok;
    LRESULT             index;
    HWND                res_lbox;
    WResLangNode        *lnode;

    type_name = NULL;
    lnode = NULL;
    type_id = 0;

    if( curr->info->current_type == RESOURCE2INT( RT_STRING ) ) {
        return( WREDeleteStringResources( curr, FALSE ) );
    }

    ok = (curr->info != NULL && curr->res != NULL && curr->lang != NULL);

    if( ok )  {
        if( !curr->type->Info.TypeName.IsName ) {
            type_id = curr->type->Info.TypeName.ID.Num;
        }
        type_name = WREGetResName( curr->res, type_id );
        ok = ( type_name != NULL );
    }

    if( ok && !force ) {
        ok = WREQueryDeleteName( type_name );
    }

    // nuke any edit sessions on this resource
    if( ok ) {
        lnode = curr->lang;
        switch( type_id ) {
        case RESOURCE2INT( RT_MENU ):
            WREEndLangMenuSession( lnode );
            break;
        case RESOURCE2INT( RT_STRING ):
            WREEndResStringSessions( curr->info );
            break;
        case RESOURCE2INT( RT_ACCELERATOR ):
            WREEndLangAccelSession( lnode );
            break;
        case RESOURCE2INT( RT_DIALOG ):
            WREEndLangDialogSession( lnode );
            break;
        case RESOURCE2INT( RT_GROUP_CURSOR ):
        case RESOURCE2INT( RT_GROUP_ICON ):
            ok = WREDeleteGroupImages( curr, type_id );
            /* fall through */
        case RESOURCE2INT( RT_BITMAP ):
            if( ok ) {
                WREEndLangImageSession( lnode );
            }
            break;
        }
    }

    if( ok ) {
        ok = WRRemoveLangNodeFromDir( curr->info->info->dir, &curr->type,
                                      &curr->res, &curr->lang );
        curr->info->modified = true;
    }

    if( ok ) {
        if( !curr->type ) {
            curr->info->current_type = 0;
            ok = WREInitResourceWindow( curr->info, 0 );
        } else {
            res_lbox = GetDlgItem( curr->info->info_win, IDM_RNRES );
            index = SendMessage( res_lbox, LB_FINDSTRING, 0, (LPARAM)(LPCSTR)type_name );
            if( index == LB_ERR ) {
                index = 0;
            }
            ok = WRESetResNamesFromType( curr->info, curr->info->current_type,
                                         true, NULL, index );
        }
    }

    if( ok ) {
        WRESetTotalText( curr->info );
    }

    if( type_name != NULL ) {
        WRMemFree( type_name );
    }

    return( ok );
}

bool WREDeleteStringResources( WRECurrentResInfo *curr, bool removing )
{
    WResTypeNode        *tnode;
    char                *text;
    bool                ok;

    ok = true;

    if( !removing ) {
        text = AllocRCString( WRE_ALLSTRINGS );
        ok = WREQueryDeleteName( text );
        if( text != NULL ) {
            FreeRCString( text );
        }
    }

    if( ok ) {
        tnode = curr->type;
        if( tnode == NULL ) {
            tnode = WRFindTypeNode( curr->info->info->dir, RESOURCE2INT( RT_STRING ), NULL );
        }
        if( tnode != NULL ) {
            curr->info->modified = true;
            ok = WRRemoveTypeNodeFromDir( curr->info->info->dir, tnode );
        }
    }

    // nuke any edit sessions on these string resources
    if( ok ) {
        curr->type = NULL;
        if( !removing ) {
            WREEndResStringSessions( curr->info );
        }
        curr->info->current_type = 0;
        ok = WREInitResourceWindow( curr->info, 0 );
    }

    return( ok );
}

bool WRERemoveEmptyResource( WRECurrentResInfo *curr )
{
    char                *type_name;
    uint_16             type_id;
    bool                ok;
    LRESULT             index;
    HWND                res_lbox;

    type_name = NULL;
    ok = true;
    type_id = 0;

    if( ok )  {
        if( !curr->type->Info.TypeName.IsName ) {
            type_id = curr->type->Info.TypeName.ID.Num;
        }
        type_name = WREGetResName( curr->res, type_id );
        ok = ( type_name != NULL );
    }

    if( ok ) {
        ok = WRRemoveLangNodeFromDir( curr->info->info->dir, &curr->type,
                                      &curr->res, &curr->lang );
        curr->info->modified = true;
    }

    if( ok ) {
        if( !curr->type ) {
            curr->info->current_type = 0;
            ok = WREInitResourceWindow( curr->info, 0 );
        } else {
            res_lbox = GetDlgItem( curr->info->info_win, IDM_RNRES );
            index = SendMessage( res_lbox, LB_FINDSTRING, 0, (LPARAM)(LPCSTR)type_name );
            if( index == LB_ERR ) {
                index = 0;
            }
            ok = WRESetResNamesFromType( curr->info, curr->info->current_type,
                                         true, NULL, index );
        }
    }

    if( type_name != NULL ) {
        WRMemFree( type_name );
    }

    return( ok );
}

bool WREQueryDeleteName( char *name )
{
    HWND        dialog_owner;
    DLGPROC     dlgproc;
    HINSTANCE   app_inst;
    INT_PTR     modified;

    dialog_owner = WREGetMainWindowHandle();
    app_inst = WREGetAppInstance();

    dlgproc = MakeProcInstance_DLG( WREResDeleteDlgProc, app_inst );

    modified = JDialogBoxParam( app_inst, "WREDeleteResource", dialog_owner, dlgproc, (LPARAM)name );

    FreeProcInstance_DLG( dlgproc );

    return( modified != -1 && modified == IDOK );
}

void WRESetWinInfo( HWND hDlg, char *name )
{
    WRESetEditWithStr( GetDlgItem( hDlg, IDM_DELNAME ), name );
}

INT_PTR CALLBACK WREResDeleteDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    char    *name;
    BOOL    ret;

    ret = FALSE;

    switch( message ) {
    case WM_INITDIALOG:
        name = (char *)lParam;
        WRESetWinInfo( hDlg, name );
        ret = TRUE;
        break;

    case WM_SYSCOLORCHANGE:
        WRECtl3dColorChange();
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case IDM_HELP:
            WREHelpRoutine();
            break;

        case IDOK:
            EndDialog( hDlg, TRUE );
            ret = TRUE;
            break;

        case IDCANCEL:
            EndDialog( hDlg, FALSE );
            ret = TRUE;
            break;
        }
    }

    return( ret );
}
