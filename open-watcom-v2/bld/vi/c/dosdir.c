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
* Description:  DOSish directory access functions.
*
****************************************************************************/


#include "vi.h"
#include <direct.h>
#include "wio.h"

#include "clibext.h"


/*
 * MyGetFileSize - do just that
 */
vi_rc MyGetFileSize( const char *name, long *size )
{
    DIR         *d;

    d = opendir( name );
    if( d == NULL ) {
        return( ERR_FILE_NOT_FOUND );
    }
    *size = d->d_size;
    closedir( d );
    return( ERR_NO_ERR );

} /* MyGetFileSize */

/*
 * IsDirectory - check if a specified path is a directory
 */
bool IsDirectory( const char *name )
{
    struct _finddata_t  fdt;
    unsigned            rc;

    if( strpbrk( name, "?*" ) != NULL ) {
        return( false );
    }
    rc = access( name, F_OK );
    if( rc != 0 )
        return( false ); /* not valid */

    if( name[1] == DRV_SEP && name[2] == FILE_SEP && name[3] == '\0' ) {
        /* this is a root dir -- this is OK */
        return( true );
    }

    /* check if it is actually a sub-directory */
    return( _findfirst( name, &fdt ) != -1 && (fdt.attrib & _A_SUBDIR) );

} /* IsDirectory */

/*
 * GetFileInfo - get info from a directory entry
 */
void GetFileInfo( direct_ent *tmp, struct dirent *dire, const char *path )
{
    /* unused parameters */ (void)path;

    tmp->attr = dire->d_attr;
    tmp->date = *((date_struct *)&dire->d_date);
    tmp->time = *((time_struct *)&dire->d_time);
    tmp->fsize = dire->d_size;

} /* GetFileInfo */

/*
 * FormatFileEntry - print a file entry
 */
void FormatFileEntry( direct_ent *file, char *res )
{
    char        buff[11];
    char        tmp[FILENAME_MAX];
    long        size;

    size = file->fsize;
    MySprintf( tmp, "  %S", file->name );
    if( IS_SUBDIR( file ) ) {
        tmp[1] = FILE_SEP;
        size = 0;
    } else if( !IsTextFile( file->name ) ) {
        tmp[1] = '*';
    }

    /*
     * build attributes
     */
    strcpy( buff, "-------" );
    if( IS_SUBDIR( file ) ) {
        buff[0] = 'd';
    }
    if( file->attr & _A_ARCH ) {
        buff[1] = 'a';
    }
    if( file->attr & _A_HIDDEN ) {
        buff[2] = 'h';
    }
    if( file->attr & _A_SYSTEM ) {
        buff[3] = 's';
    }
    buff[4] = 'r';
    if( (file->attr & _A_RDONLY) == 0 ) {
        buff[5] = 'w';
    }
    if( !IsTextFile( file->name ) ) {
        buff[6] = 'x';
    }

    MySprintf( res, "%s  %s %L  %D/%D/%d  %D:%D",
               tmp,
               buff,
               size,
               (int)file->date.month,
               (int)file->date.day,
               (int)file->date.year + 1980,
               (int)file->time.hour,
               (int)file->time.min );

} /* FormatFileEntry */
