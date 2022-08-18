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
* Description:  Automatic pointer conversion for select message parameters.
*
****************************************************************************/


#include "variety.h"
#include <string.h>
#include "cover.h"

#define MAX_CNAME       10

BOOL TryAlias( HWND hwnd, WORD msg, LONG *lparam )
{
    DWORD               alias;
    MDICREATESTRUCT     *mcs;
    char                class[MAX_CNAME];
    int                 len;

    if( hwnd == (HWND)~0 )
        return( FALSE );

    /*
     * misc. messages
     */
    switch( msg ) {
    case WM_NCCALCSIZE:
        alias = AllocAlias16( (LPSTR)*lparam );
        *lparam = alias;
        return( TRUE );
    case WM_MDICREATE:
        mcs = (MDICREATESTRUCT *)*lparam;
        mcs->szClass = (LPSTR)AllocAlias16( (LPSTR)mcs->szClass );
        mcs->szTitle = (LPSTR)AllocAlias16( (LPSTR)mcs->szTitle );
        alias = AllocAlias16( (LPSTR)*lparam );
        *lparam = alias;
        return( TRUE );
    }

    /*
     * try class specific messages
     */
    if( hwnd == 0 )
        return( FALSE );
    len = GetClassName( hwnd, class, sizeof( class ) );
    class[len] = '\0';

    /*
     * combo box messages
     */
    if( _stricmp( class, "COMBOBOX" ) == 0 ) {
        switch( msg ) {
        case CB_ADDSTRING:
        case CB_DIR:
        case CB_FINDSTRING:
        case CB_FINDSTRINGEXACT:
        case CB_GETLBTEXT:
        case CB_INSERTSTRING:
        case CB_SELECTSTRING:
            alias = AllocAlias16( (LPSTR)*lparam );
            *lparam = alias;
            return( TRUE );
        }
    }

    /*
     * edit control messages
     */
    if( _stricmp( class, "EDIT" ) == 0 ) {
        switch( msg ) {
        case EM_GETLINE:
        case EM_GETRECT:
        case EM_REPLACESEL:
        case EM_SETRECT:
        case EM_SETRECTNP:
        case EM_SETTABSTOPS:
            alias = AllocAlias16( (LPSTR)*lparam );
            *lparam = alias;
            return( TRUE );
        }
    }

    /*
     * list box messages
     */
    if( _stricmp( class, "LISTBOX" ) == 0 ) {
        switch( msg ) {
        case LB_ADDSTRING:
        case LB_DIR:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_GETITEMRECT:
        case LB_GETSELITEMS:
        case LB_GETTEXT:
        case LB_INSERTSTRING:
        case LB_SELECTSTRING:
        case LB_SETTABSTOPS:
            alias = AllocAlias16( (LPSTR)*lparam );
            *lparam = alias;
            return( TRUE );
        }
    }

    return( FALSE );
}
