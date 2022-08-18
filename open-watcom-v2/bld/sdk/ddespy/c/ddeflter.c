/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  Filter DDE traffic.
*
****************************************************************************/


#include "wddespy.h"
#include "cbfilt.rh"
#include "msgfilt.rh"

static struct {
    bool        *filter;
    WORD        first;
    WORD        last;
} FilterDlgInfo;


static bool     MsgFilter[MFILTER_LAST_MSG - MFILTER_FIRST_MSG + 1];
static bool     CBFilter[CFILTER_LAST_MSG - CFILTER_FIRST_MSG + 1];

/*
 * The Matching array matches the Filter array elements to the actual
 * message values.  For example to find if we are processing the WM_DDE_ACK
 * message find its index in the MsgMatching array then use that index to
 * access the MsgFilter array. If that element of the MsgFilter array is TRUE
 * we should process the message.
 */

static WORD MsgMatching[] = {
    WM_DDE_ACK,
    WM_DDE_ADVISE,
    WM_DDE_DATA,
    WM_DDE_EXECUTE,
    WM_DDE_POKE,
    WM_DDE_REQUEST,
    WM_DDE_TERMINATE,
    WM_DDE_UNADVISE,
    WM_DDE_INITIATE
};

static WORD CBMatching[] = {
    XTYP_ADVSTART,
    XTYP_ADVREQ,
    XTYP_ADVDATA,
    XTYP_CONNECT,
    XTYP_ERROR,
    XTYP_EXECUTE,
    XTYP_POKE,
    XTYP_REQUEST,
    XTYP_ADVSTOP,
    XTYP_UNREGISTER,
    XTYP_CONNECT_CONFIRM,
    XTYP_WILDCONNECT,
    XTYP_DISCONNECT,
    XTYP_XACT_COMPLETE,
    XTYP_REGISTER
};


/*
 * GetFilter - return a string representing the filter state to be
 *             stored in the .ini file
 *           - msgfilter must be at least
 *             MFILTER_LAST_MSG - MFILTER_FIRST_MSG + 2 bytes
 *           - cbfilter must be at least
 *             CFILTER_LAST_MSG - CFILTER_FIRST_MSG + 2 bytes
 */
void GetFilter( char *msgfilter, char *cbfilter )
{
    WORD        i;

    for( i = 0; i <= MFILTER_LAST_MSG - MFILTER_FIRST_MSG; i++ ) {
        if( MsgFilter[i] ) {
            msgfilter[i] = '1';
        } else {
            msgfilter[i] = '0';
        }
    }
    msgfilter[i] = '\0';
    for( i = 0; i <= CFILTER_LAST_MSG - CFILTER_FIRST_MSG; i++ ) {
        if( CBFilter[i] ) {
            cbfilter[i] = '1';
        } else {
            cbfilter[i] = '0';
        }
    }
    cbfilter[i] = '\0';

} /* GetFilter */

/*
 * SetFilter - set the filters based on a string read from the .ini file
 */
void SetFilter( char *msgfilter, char *cbfilter )
{
    WORD        i;

    i = 0;
    while( msgfilter[i] != '\0' ) {
        if( msgfilter[i] == '1' ) {
            MsgFilter[i] = true;
        } else {
            MsgFilter[i] = false;
        }
        i++;
        if( i > MFILTER_LAST_MSG - MFILTER_FIRST_MSG ) {
            break;
        }
    }
    i = 0;
    while( cbfilter[i] != '\0' ) {
        if( cbfilter[i] == '1' ) {
            CBFilter[i] = true;
        } else {
            CBFilter[i] = false;
        }
        i++;
        if( i > CFILTER_LAST_MSG - CFILTER_FIRST_MSG ) {
            break;
        }
    }

} /* SetFilter */

/*
 * DoFilter - return true if we want to process this message or false
 *            otherwise
 */
bool DoFilter( UINT msg, WORD filter_type )
{
    WORD        *match;
    bool        *filter;
    WORD        i;
    WORD        limit;
    bool        ret;

    if( filter_type == FILTER_MESSAGE ) {
        match = MsgMatching;
        filter = MsgFilter;
        limit = MFILTER_LAST_MSG - MFILTER_FIRST_MSG + 1;
    } else if( filter_type == FILTER_CB ) {
        match = CBMatching;
        filter = CBFilter;
        limit = CFILTER_LAST_MSG - CFILTER_FIRST_MSG + 1;
    } else {
        return( false );
    }
    ret = false;
    for( i = 0; i < limit; i++ ) {
        if( match[i] == msg ) {
            if( filter[i] ) {
                ret = true;
            }
            break;
        }
    }
    if( i >= limit ) {
        return( true );
    }
    return( ret );

} /* DoFilter */

/*
 * FilterDlgProc - handle the dialogs to set message and callback
 *                 filters
 */
INT_PTR CALLBACK FilterDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    unsigned            i;
    WORD                cmd;
    bool                ret;

    ret = false;

    switch( msg ) {
    case WM_INITDIALOG:

        /*
         * lparam is TRUE if this is a message filter dialog or
         * FALSE if this is a callback filter dialog.
         */

        if( lparam ) {
            FilterDlgInfo.filter = MsgFilter;
            FilterDlgInfo.first = MFILTER_FIRST_MSG;
            FilterDlgInfo.last = MFILTER_LAST_MSG;
        } else {
            FilterDlgInfo.filter = CBFilter;
            FilterDlgInfo.first = CFILTER_FIRST_MSG;
            FilterDlgInfo.last = CFILTER_LAST_MSG;
        }
        for( i = FilterDlgInfo.first; i <= FilterDlgInfo.last; i++ ) {
            if( FilterDlgInfo.filter[i - FilterDlgInfo.first] ) {
                CheckDlgButton( hwnd, i, BST_CHECKED );
            }
        }
        ret = true;
        break;
#ifndef NOUSE3D
    case WM_SYSCOLORCHANGE:
        CvrCtl3dColorChange();
        ret = true;
        break;
#endif
    case WM_COMMAND:
        cmd = LOWORD( wparam );
        switch( cmd ) {
        case IDOK:
            for( i = FilterDlgInfo.first; i <= FilterDlgInfo.last; i++ ) {
                if( IsDlgButtonChecked( hwnd, i ) ) {
                    FilterDlgInfo.filter[i - FilterDlgInfo.first] = true;
                } else {
                    FilterDlgInfo.filter[i - FilterDlgInfo.first] = false;
                }
            }
            ret = true;
            EndDialog( hwnd, -1 );
            break;
        case IDCANCEL:
            EndDialog( hwnd, -1 );
            ret = true;
            break;
        case CFILTER_ALL:
        case MFILTER_ALL:
            for( i = FilterDlgInfo.first; i <= FilterDlgInfo.last; i++ ) {
                CheckDlgButton( hwnd, i, BST_CHECKED );
            }
            ret = true;
            break;
        case CFILTER_NONE:
        case MFILTER_NONE:
            for( i = FilterDlgInfo.first; i <= FilterDlgInfo.last; i++ ) {
                CheckDlgButton( hwnd, i, BST_UNCHECKED );
            }
            ret = true;
            break;
        }
        break;
    }
    return( ret );

} /* FilterDlgProc */
