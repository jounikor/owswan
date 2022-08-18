/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  File open dialog.
*
****************************************************************************/


#include "_aui.h"
#include <stdlib.h>
#include <string.h>
#include "wnddlg.h"
#include "pathgrp2.h"

#include "clibext.h"


int DlgGetFileName( open_file_name *ofn )
{
    int         rc;
    a_window    wnd;

    wnd = WndFindActive();
    rc = GUIGetFileName( DlgOpenGetGUIParent(), ofn );
    if( wnd != NULL )
        WndToFront( wnd );
    return( rc );
}

bool DlgFileBrowse( char *title, char *filter, char *path, unsigned len, fn_flags flags )
{
    open_file_name      ofn;
    char                fname[_MAX_PATH];
    char                cd[_MAX_DRIVE+_MAX_PATH];
    pgroup2             pg;
    int                 rc;

    memset( &ofn, 0, sizeof( ofn ) );
    ofn.flags = flags;
    ofn.title = title;
    _splitpath2( path, pg.buffer, &pg.drive, &pg.dir, &pg.fname, &pg.ext );
    _makepath( cd, pg.drive, pg.dir, ".", NULL );
    _makepath( fname, NULL, NULL, pg.fname, pg.ext );
    ofn.initial_dir = cd;
    ofn.file_name = fname;
    ofn.max_file_name = len;
    ofn.filter_list = filter;
    ofn.filter_index = 0;
    rc = DlgGetFileName( &ofn );
    if( rc == FN_RC_FILE_SELECTED ) {
        strcpy( path, fname );
        return( true );
    }
    return( false );
}
