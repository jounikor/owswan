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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <string.h>
#include <windows.h>
#include "libwin32.h"
#include "osver.h"
#include "rtdata.h"

/*
 * Apparently GetFileAttributes 3.51 sometimes gets confused when the
 * file in question is on a FAT drive.  Since FindFirstFile seems to
 * work, use it instead.
 *
 * Implementation with FindFirstFile / FindNextFile
 * has bug for root of drive due to they can not work
 * with it.
 * Now (2009-01-31) this code is not used for Windows NT >= 4
 * it call native GetFileAttributes
 * We hold this problematic code for compatibility with 3.51 even if
 * nowdays it is very archaic system.
 */

DWORD __fixed_GetFileAttributes( LPCTSTR lpFileName )
/***************************************************/
{
    HANDLE              handle;
    WIN32_FIND_DATA     finddata;

    if( WIN32_IS_NT4 ) {    /* >= NT4.0 */
        return( GetFileAttributes( lpFileName ) );
    }
    /* NT3.x or Win95 or Win32s */

    /*** Fail if the filename contains a wildcard ***/
    if( __F_NAME(strchr,wcschr)( lpFileName, STRING( '*' ) ) != NULL ||
        __F_NAME(strchr,wcschr)( lpFileName, STRING( '?' ) ) != NULL ) {
        return( INVALID_FILE_ATTRIBUTES );
    }

    /*** Ok, use FindFirstFile to get the file attribute ***/
    handle = __lib_FindFirstFile( lpFileName, &finddata );
    if( handle == INVALID_HANDLE_VALUE ) {
        return( INVALID_FILE_ATTRIBUTES );
    } else {
        FindClose( handle );
    }
    return( finddata.dwFileAttributes );
}
