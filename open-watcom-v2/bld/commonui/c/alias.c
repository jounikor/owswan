/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Helper routines for memory aliases.
*
****************************************************************************/


#include "commonui.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "bool.h"
#include "cguimem.h"
#include "alias.h"
#ifndef NOUSE3D
    #include "ctl3dcvr.h"
#endif
#include "ldstr.h"
#include "uistr.grh"
#include "wclbproc.h"


/* Window callback functions prototypes */
WINEXPORT INT_PTR CALLBACK AliasDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

#define CONST_LEN       15

static AliasHdl         CurHdl;         /* used for dialog box processing */

/*
 * findAlias - search for an alias with the given handle and identifier
 */
static AnAlias *findAlias( AliasHdl hdl, ULONG_PTR id )
{
    AnAlias     *cur;

    for( cur = hdl->data; cur != NULL; cur = cur->next ) {
        if( cur->id == id ) {
            return( cur );
        }
        if( cur->id > id ) {
            break;
        }
    }
    return( NULL );

} /* findAlias */

/*
 * InitAliasHdl - initialize an alias handle before any aliases are
 *                assigned to it
 */
void InitAliasHdl( AliasHdl *hdl,
                   void (*updatefn)( ULONG_PTR, char *, char *, void * ),
                   void *userdata )
{
    *hdl = MemAlloc( sizeof( AliasList ) );
    (*hdl)->data = NULL;
    (*hdl)->userdata = userdata;
    (*hdl)->updatefn = updatefn;

} /* InitAliasHdl */

/*
 * insertAlias - insert an alias in the alias list using linear
 *               insertion sort
 */
static void insertAlias( AliasHdl hdl, AnAlias *alias )
{
    AnAlias     **cur;

    cur = &hdl->data;
    for( ;; ) {
        if( *cur == NULL || (*cur)->id > alias->id ) {
            alias->next = *cur;
            *cur = alias;
            break;
        }
        cur = &(*cur)->next;
    }

} /* insertAlias */

/*
 * AddAlias - add an alias to an alias list
 *          - if an alias already exists for this identifier replace it
 */
void AddAlias( AliasHdl hdl, char *text, ULONG_PTR id )
{
    AnAlias     *cur;
    size_t      len;

    cur = findAlias( hdl, id );
    if( cur == NULL ) {
        cur = MemAlloc( sizeof( AnAlias ) );
        cur->id = id;
        insertAlias( hdl, cur );
        if( hdl->updatefn != NULL ) {
            hdl->updatefn( id, text, NULL, hdl->userdata );
        }
    } else {
        if( hdl->updatefn != NULL ) {
            hdl->updatefn( id, text, cur->name, hdl->userdata );
        }
        MemFree( cur->name );
    }
    len = strlen( text ) + 1;
    cur->name = MemAlloc( len );
    strcpy( cur->name, text );

} /* AddAlias */

/*
 * FreeAlias - free all memory associated with an alias list
 *           - InitAliasHdl must be called before this handle can be
 *             used again
 */

void FreeAlias( AliasHdl hdl )
{
    AnAlias     *cur;
    AnAlias     *next;

    for( cur = hdl->data; cur != NULL; cur = next ) {
        next = cur->next;
        MemFree( cur->name );
        MemFree( cur );
    }
    MemFree( hdl );

} /* FreeAlias */

/*
 * LookupAlias - return the string associated with an identifier or NULL if
 *               no alias exists
 */
char *LookupAlias( AliasHdl hdl, ULONG_PTR id )
{
    AnAlias     *cur;

    cur = findAlias( hdl, id );
    if( cur == NULL ) {
        return( NULL );
    }
    return( cur->name );

} /* LookupAlias */

/*
 * getIthAlias - get the alias in the given alias list with the given index
 */
static AnAlias *getIthAlias( AliasHdl hdl, int i )
{
    AnAlias     *ret;

    ret = hdl->data;
    while( i-- > 0 ) {
        ret = ret->next;
    }
    return( ret );
}

/*
 * findAliasFromText - find an alias in the given alias list with the given text
 */
static AnAlias *findAliasFromText( AliasHdl hdl, char *alias )
{
    AnAlias     *cur;

    for( cur = hdl->data; cur != NULL; cur = cur->next ) {
        if( !strcmp( alias, cur->name ) ) {
            break;
        }
    }
    return( cur );

} /* findAliasFromText */

/*
 * AliasDlgProc - alias list dialog procedure
 */
INT_PTR CALLBACK AliasDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    AnAlias     *cur;
    size_t      len;
    char        buf[CONST_LEN];
    char        msgbuf[256];
    int         sel;
    char        *endptr;
    char        *realend;
    char        *alias;
    long        id;
    WORD        cmd;

    switch( msg ) {
    case WM_INITDIALOG:
        if( (LPCSTR)lparam != NULL ) {
            SetWindowText( hwnd, (LPCSTR)lparam );
        }
        SendDlgItemMessage( hwnd, ALIAS_TEXT, EM_LIMITTEXT, 20, 0 );
        for( cur = CurHdl->data; cur != NULL; cur = cur->next ) {
#ifdef _WIN64
            sprintf( buf, "0x%16llX", cur->id );
#else
            sprintf( buf, "0x%08lX", cur->id );
#endif
            SendDlgItemMessage( hwnd, ALIAS_ID_LIST, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)buf );
        }
        break;
#ifndef NOUSE3D
    case WM_SYSCOLORCHANGE:
        CvrCtl3dColorChange();
        break;
#endif
    case WM_COMMAND:
        cmd = LOWORD( wparam );
        switch( cmd ) {
            case IDOK:
            case ALIAS_DO_MORE:
                SendDlgItemMessage( hwnd, ALIAS_CUR_ID, WM_GETTEXT, CONST_LEN, (LPARAM)(LPSTR)buf );
                realend = buf;
                while( *realend != '\0' ) {
                    realend++;
                }
                realend--;
                while( isspace( *realend ) ) {
                    realend--;
                }
                realend++;
                id = strtol( buf, &endptr, 0 );
                if( endptr != realend || *buf == '\0' ) {
                    RCMessageBox( hwnd, ALIAS_VALUE_MUST_BE_INT, "", MB_OK );
                    break;
                }
                len = SendDlgItemMessage( hwnd, ALIAS_TEXT, WM_GETTEXTLENGTH, 0, 0 );
                alias = MemAlloc( len + 1 );
                len = SendDlgItemMessage( hwnd, ALIAS_TEXT, WM_GETTEXT, len + 1, (LPARAM)(LPSTR)alias );
                /* check for spaces */
                endptr = alias;
                while( !isspace( *endptr ) && *endptr != '\0' ) {
                    endptr++;
                }
                realend = endptr;
                while( isspace( *endptr ) ) {
                    endptr++;
                }
                if( *endptr != '\0' ) {
                    RCMessageBox( hwnd, ALIAS_NO_SPACES_ALLOWED, "", MB_OK );
                    MemFree( alias );
                    break;
                }
                realend = '\0'; /* truncate trailing spaces */
                cur = findAliasFromText( CurHdl, alias );
                if( cur == NULL ) {
                    AddAlias( CurHdl, alias, id );
                } else {
                    RCsprintf( msgbuf, ALIAS_NO_DUPLICATES_ALLOWED, alias, cur->id );
                    MessageBox( hwnd, msgbuf, "", MB_OK );
                    MemFree( alias );
                    break;
                }
                MemFree( alias );
                EndDialog( hwnd, cmd );
                break;
            case IDCANCEL:
                EndDialog( hwnd, cmd );
                break;
            case ALIAS_ID_LIST:
                if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCHANGE ) {
                    sel = (int)SendDlgItemMessage( hwnd, ALIAS_ID_LIST, LB_GETCURSEL, 0, 0L );
                    SendDlgItemMessage( hwnd, ALIAS_ID_LIST, LB_GETTEXT, sel, (LPARAM)(LPSTR)buf );
                    SendDlgItemMessage( hwnd, ALIAS_CUR_ID, WM_SETTEXT, 0, (LPARAM)(LPCSTR)buf );
                    cur = getIthAlias( CurHdl, sel );
                    SendDlgItemMessage( hwnd, ALIAS_TEXT, WM_SETTEXT, 0, (LPARAM)(LPCSTR)cur->name );
                }
                break;
            default:
                return( FALSE );
        }
    default:
        return( FALSE );
    }
    return( TRUE );
}

/*
 * Query4Aliases - display the alias list dialog box
 */
void Query4Aliases( AliasHdl hdl, HANDLE instance, HWND hwnd, char *title )
{
    DLGPROC     dlgproc;
    INT_PTR     ret;

    CurHdl = hdl;
    dlgproc = MakeProcInstance_DLG( AliasDlgProc, instance );
    for( ;; ) {
        ret = DialogBoxParam( instance, "ALIAS_DLG", hwnd, dlgproc, (LPARAM)(LPCSTR)title );
        if( ret != ALIAS_DO_MORE ) {
            break;
        }
    }
    FreeProcInstance_DLG( dlgproc );
    CurHdl = NULL;

} /* Query4Aliases */

/*
 * EnumAliases - enumerate all aliases in a given alias list
 */
void EnumAliases( AliasHdl hdl, void (*enumfn)( ULONG_PTR, char *, void * ), void *userdata )
{
    AnAlias     *cur;

    for( cur = hdl->data; cur != NULL; cur = cur->next ) {
        enumfn( cur->id, cur->name, userdata );
    }
    enumfn( (ULONG_PTR)-1L, NULL, userdata );

} /* EnumAliases */
