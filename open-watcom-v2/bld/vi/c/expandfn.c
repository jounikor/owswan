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
* Description:  Expand file name to a list.
*
****************************************************************************/


#include "vi.h"
#include "posix.h"
#include "pathgrp2.h"

#include "clibext.h"


/*
 * ExpandFileNames - take a file name, and expand it out to a list of dos
 *                   file names
 */
list_linenum ExpandFileNames( const char *fullmask, char ***argv )
{
    list_linenum    argc;
    list_linenum    i;
    pgroup2         pg;
    char            pathin[FILENAME_MAX];
    const char      *p;
    char            *new;
    bool            wildcard;
    vi_rc           rc;
    char            c;

    argc = 0;
    wildcard = false;
    *argv = NULL;

    /*
     * check if there is anything to expand
     */
    for( p = fullmask; (c = *p) != '\0'; ) {
        if( c == '?' || c == '*' || c == '|' || c == '(' || c == '[' ) {
            wildcard = true;
            break;
        }
        p++;
    }

    if( !wildcard ) {
        // don't change to lowercase any more
        //FileLower( fullmask );
        *argv = _MemReallocList( *argv, argc + 1 );
        new = MemAlloc( strlen( fullmask ) + 1 );
        (*argv)[argc++] = new;
        strcpy( new, fullmask );
        return( argc );
    }

    /*
     * get all matches
     */
    rc = GetSortDir( fullmask, false );
    if( rc != ERR_NO_ERR ) {
        *argv = _MemReallocList( *argv, argc + 1 );
        new = MemAlloc( strlen( fullmask ) + 1 );
        (*argv)[argc++] = new;
        strcpy( new, fullmask );
        return( argc );
    }
    _splitpath2( fullmask, pg.buffer, &pg.drive, &pg.dir, NULL, NULL );

    /*
     * run through matches
     */
    for( i = 0; i < DirFileCount; i++ ) {
        if( IS_SUBDIR( DirFiles[i] ) )
            continue;
        _makepath( pathin, pg.drive, pg.dir, DirFiles[i]->name, NULL );
        *argv = _MemReallocList( *argv, argc + 1 );
        new = MemAlloc( strlen( pathin ) + 1 );
        (*argv)[argc++] = new;
        strcpy( new, pathin );
    }
    return( argc );

} /* ExpandFileNames */
