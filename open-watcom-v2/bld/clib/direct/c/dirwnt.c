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
* Description:  Win32 implementation of directory functions.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <sys/types.h>
#include <direct.h>
#include <dos.h>
#include <windows.h>
#include "rtdata.h"
#include "strdup.h"
#include "libwin32.h"
#include "ntinfo.h"
#include "seterrno.h"
#include "_direct.h"
#include "_dtaxxx.h"
#include "liballoc.h"
#include "pathmac.h"


static int is_directory( const CHAR_TYPE *name )
/**********************************************/
{
    UINT_WC_TYPE    curr_ch;
    UINT_WC_TYPE    prev_ch;

    curr_ch = NULLCHAR;
    for(;;) {
        prev_ch = curr_ch;
#ifdef __WIDECHAR__
        curr_ch = *name;
#else
        curr_ch = _mbsnextc( (unsigned char *)name );
#endif
        if( curr_ch == NULLCHAR ) {
            if( IS_DIR_SEP( prev_ch ) || prev_ch == DRV_SEP ) {
                /* directory, need add "*.*" */
                return( 2 );
            }
            if( prev_ch == STRING( '.' ) ) {
                /* directory, need add "\\*.*" */
                return( 1 );
            }
            /* without wildcards maybe file or directory, need next check */
            /* need add "\\*.*" if directory */
            return( 0 );
        }
        if( curr_ch == STRING( '*' ) )
            break;
        if( curr_ch == STRING( '?' ) )
            break;
#ifdef __WIDECHAR__
        ++name;
#else
        name = (char *)_mbsinc( (unsigned char *)name );
#endif
    }
    /* with wildcard must be file */
    return( -1 );
}

static DIR_TYPE *__F_NAME(___opendir,___wopendir)( const CHAR_TYPE *dirname, DIR_TYPE *dirp )
/*******************************************************************************************/
{
    WIN32_FIND_DATA     ffd;
    HANDLE              h;

    if( dirp->d_first != _DIR_CLOSED ) {
        FindClose( DTAXXX_HANDLE_OF( dirp->d_dta ) );
        dirp->d_first = _DIR_CLOSED;
    }
    h = __lib_FindFirstFile( dirname, &ffd );
    if( h == INVALID_HANDLE_VALUE ) {
        __set_errno_nt();
        return( NULL );
    }
    DTAXXX_HANDLE_OF( dirp->d_dta ) = h;
    __GetNTDirInfo( dirp, &ffd );
    dirp->d_first = _DIR_ISFIRST;
    return( dirp );
}

static DIR_TYPE *__F_NAME(__opendir,__wopendir)( const CHAR_TYPE *dirname )
/*************************************************************************/
{
    DIR_TYPE        tmp;
    DIR_TYPE        *dirp;
    int             i;
    CHAR_TYPE       pathname[MAX_PATH + 6];

    tmp.d_attr = _A_SUBDIR;               /* assume sub-directory */
    tmp.d_first = _DIR_CLOSED;
    i = is_directory( dirname );
    if( i <= 0 ) {
        /* it is file or may be file or no dirname */
        if( __F_NAME(___opendir,___wopendir)( dirname, &tmp ) == NULL ) {
            return( NULL );
        }
    }
    if( i >= 0 && (tmp.d_attr & _A_SUBDIR) ) {
        size_t          len;

        /* directory, add wildcard */
        len = __F_NAME(strlen,wcslen)( dirname );
        memcpy( pathname, dirname, len * sizeof( CHAR_TYPE ) );
        if( i < 2 ) {
            pathname[len++] = DIR_SEP;
        }
        __F_NAME(strcpy,wcscpy)( &pathname[len], STRING( "*.*" ) );
        if( __F_NAME(___opendir,___wopendir)( pathname, &tmp ) == NULL ) {
            return( NULL );
        }
        dirname = pathname;
    }
    dirp = lib_malloc( sizeof( DIR_TYPE ) );
    if( dirp == NULL ) {
        FindClose( DTAXXX_HANDLE_OF( tmp.d_dta ) );
        __set_errno_dos( ERROR_NOT_ENOUGH_MEMORY );
        return( NULL );
    }
    tmp.d_openpath = __F_NAME(__clib_strdup,__clib_wcsdup)( dirname );
    *dirp = tmp;
    return( dirp );
}

_WCRTLINK DIR_TYPE *__F_NAME(opendir,_wopendir)( const CHAR_TYPE *dirname )
/*************************************************************************/
{
    return( __F_NAME(__opendir,__wopendir)( dirname ) );
}

_WCRTLINK DIRENT_TYPE *__F_NAME(readdir,_wreaddir)( DIR_TYPE *dirp )
/******************************************************************/
{
    WIN32_FIND_DATA     ffd;
    DWORD               err;

    if( dirp == NULL || dirp->d_first == _DIR_CLOSED )
        return( NULL );
    if( dirp->d_first == _DIR_ISFIRST ) {
        dirp->d_first = _DIR_NOTFIRST;
    } else {
        if( !__lib_FindNextFile( DTAXXX_HANDLE_OF( dirp->d_dta ), &ffd ) ) {
            err = GetLastError();
            if( err != ERROR_NO_MORE_FILES ) {
                __set_errno_dos( err );
            }
            return( NULL );
        }
        __GetNTDirInfo( dirp, &ffd );
    }
    return( dirp );

}

_WCRTLINK int __F_NAME(closedir,_wclosedir)( DIR_TYPE *dirp )
/***********************************************************/
{

    if( dirp == NULL || dirp->d_first == _DIR_CLOSED ) {
        return( __set_errno_dos( ERROR_INVALID_HANDLE ) );
    }
    if( !FindClose( DTAXXX_HANDLE_OF( dirp->d_dta ) ) ) {
        return( __set_errno_nt() );
    }
    dirp->d_first = _DIR_CLOSED;
    if( dirp->d_openpath != NULL )
        free( dirp->d_openpath );
    lib_free( dirp );
    return( 0 );
}

_WCRTLINK void __F_NAME(rewinddir,_wrewinddir)( DIR_TYPE *dirp )
/**************************************************************/
{
    if( dirp == NULL || dirp->d_openpath == NULL )
        return;
    __F_NAME(___opendir,___wopendir)( dirp->d_openpath, dirp );
}
