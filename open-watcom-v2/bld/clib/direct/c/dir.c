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
* Description:  DOS implementation of directory functions.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <mbstring.h>
#include <direct.h>
#include <dos.h>
#include "strdup.h"
#include "liballoc.h"
#include "tinyio.h"
#include "seterrno.h"
#include "msdos.h"
#include "_direct.h"
#include "rtdata.h"
#include "pathmac.h"

#define SEEK_ATTRIB (TIO_HIDDEN | TIO_SYSTEM | TIO_SUBDIRECTORY)


static int is_directory( const CHAR_TYPE *name )
/**********************************************/
{
    UINT_WC_TYPE    curr_ch;
    UINT_WC_TYPE    prev_ch;

    curr_ch = NULLCHAR;
    for(;;) {
        prev_ch = curr_ch;
#if defined( __WIDECHAR__ ) || defined( __UNIX__ )
        curr_ch = *name;
#else
        curr_ch = _mbsnextc( (unsigned char *)name );
#endif
        if( curr_ch == NULLCHAR ) {
            if( IS_DIR_SEP( prev_ch ) || prev_ch == DRV_SEP ){
                /* directory, need add "*.*" */
                return( 2 );
            }
            if( prev_ch == STRING( '.' ) ){
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
#if defined( __WIDECHAR__ ) || defined( __UNIX__ )
        ++name;
#else
        name = (char *)_mbsinc( (unsigned char *)name );
#endif
    }
    /* with wildcard must be file */
    return( -1 );
}

static void copy_find_data( DIR_TYPE *dirp, struct find_t *findp )
/****************************************************************/
{
    memcpy( dirp->d_dta, findp, sizeof( dirp->d_dta ) );
    dirp->d_attr = findp->attrib;
    dirp->d_time = findp->wr_time;
    dirp->d_date = findp->wr_date;
    dirp->d_size = findp->size;
#ifdef __WIDECHAR__
    mbstowcs( dirp->d_name, findp->name, sizeof( findp->name ) );
#else
    strcpy( dirp->d_name, findp->name );
#endif
}

static DIR_TYPE *__F_NAME(___opendir,___wopendir)( const CHAR_TYPE *dirname, DIR_TYPE *dirp )
/*******************************************************************************************/
{
    struct find_t   fdta;
#ifdef __WIDECHAR__
    char            mbcsName[MB_CUR_MAX * _MAX_PATH];
#endif

    if( dirp->d_first != _DIR_CLOSED ) {
        _dos_findclose( (struct _find_t *)dirp->d_dta );
        dirp->d_first = _DIR_CLOSED;
    }
    /*** Convert a wide char string to a multibyte string ***/
#ifdef __WIDECHAR__
    if( wcstombs( mbcsName, dirname, sizeof( mbcsName ) ) == (size_t)-1 ) {
        return( NULL );
    }
#endif
    if( _dos_findfirst( __F_NAME(dirname,mbcsName), SEEK_ATTRIB, &fdta ) ) {
        return( NULL );
    }
    copy_find_data( dirp, &fdta );
    dirp->d_first = _DIR_ISFIRST;
    return( dirp );
}

static DIR_TYPE *__F_NAME(__opendir,__wopendir)( const CHAR_TYPE *dirname )
/*************************************************************************/
{
    DIR_TYPE        tmp;
    DIR_TYPE        *dirp;
    int             i;
    CHAR_TYPE       pathname[ _MAX_PATH + 6 ];

    tmp.d_attr = _A_SUBDIR;
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

        /* directory, add wildcards */
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
    dirp = lib_malloc( sizeof( *dirp ) );
    if( dirp == NULL ) {
        _dos_findclose( (struct _find_t *)tmp.d_dta );
        __set_errno_dos( E_nomem );
        return( NULL );
    }
    tmp.d_openpath = __F_NAME(__clib_strdup,__clib_wcsdup)( dirname );
    *dirp = tmp;
    return( dirp );
}


_WCRTLINK DIR_TYPE *__F_NAME(opendir,_wopendir)( const CHAR_TYPE *dirname )
{
    return( __F_NAME(__opendir,__wopendir)( dirname ) );
}


_WCRTLINK DIRENT_TYPE *__F_NAME(readdir,_wreaddir)( DIR_TYPE *dirp )
{
    struct find_t   fdta;

    if( dirp == NULL || dirp->d_first == _DIR_CLOSED )
        return( NULL );
    if( dirp->d_first == _DIR_ISFIRST ) {
        dirp->d_first = _DIR_NOTFIRST;
    } else {
        memcpy( &fdta, dirp->d_dta, sizeof( fdta.reserved ) );
        if( _dos_findnext( &fdta ) ) {
            return( NULL );
        }
        copy_find_data( dirp, &fdta );
    }
    return( dirp );
}


_WCRTLINK int __F_NAME(closedir,_wclosedir)( DIR_TYPE *dirp )
{
    unsigned    rc;

    if( dirp == NULL || dirp->d_first == _DIR_CLOSED ) {
        return( __set_errno_dos( E_ihandle ) );
    }
    rc = _dos_findclose( (struct _find_t *)dirp->d_dta );
    if( rc ) {
        return( rc );
    }
    dirp->d_first = _DIR_CLOSED;
    if( dirp->d_openpath != NULL )
        free( dirp->d_openpath );
    lib_free( dirp );
    return( 0 );
}


_WCRTLINK void __F_NAME(rewinddir,_wrewinddir)( DIR_TYPE *dirp )
{
    if( dirp == NULL || dirp->d_openpath == NULL )
        return;
    __F_NAME(___opendir,___wopendir)( dirp->d_openpath, dirp );
}
