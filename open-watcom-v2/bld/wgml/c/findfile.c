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
* Description:  Implements the global function used by wgml to find and
*               open files:
*                   ff_setup()
*                   ff_teardown()
*                   free_inc_fp()
*                   search_file_in_dirs()
*               plus these local items:
*                   cur_dir_list
*                   gml_lib_dirs
*                   gml_inc_dirs
*                   path_dirs
*                   initialize_cur_directory_list()
*                   initialize_env_directory_list()
*                   try_open()
*
* Note:         The Wiki should be consulted for any term whose meaning is
*               not apparent which is related to the binary device library.
*               This should help in most such cases.
*
****************************************************************************/

#include <errno.h>
#include "wgml.h"
#include "copdir.h"
#include "iopath.h"
#include "pathlist.h"
#include "pathgrp2.h"

#include "clibext.h"


/* Define the global variables. */

#define global
#include "findfile.h"
#undef global

/* Extern function definitions. */

/* Local data. */

static  char  *cur_dir_list = NULL;
static  char  *gml_lib_dirs = NULL;
static  char  *gml_inc_dirs = NULL;
static  char  *path_dirs = NULL;

/* Local function definitions. */

/* Function try_open().
 * Compose full path / filename and try to open for reading.
 *
 * Parameters:
 *      prefix contains the path to use.
 *      filename contains the file name to use.
 *
 * Returns:
 *      true   if the file is found.
 *      false  if the file is not found.
 *
 * Note:
 *      If the file is found, try_file_name and try_fp will be set to the name
 *      as found and FILE * of the file.
 */

static bool try_open( char *prefix, char *filename )
{
    FILE        *fp;
    char        buff[FILENAME_MAX];
    size_t      filename_length;

    /* Prevent buffer overflow. */

    filename_length = strlen( prefix ) + strlen( filename ) + 1;
    if( filename_length > FILENAME_MAX ) {
        xx_simple_err_cc( err_file_max, prefix, filename );
        return( false );
    }

    /* Create the full file name to search for. */

    strcpy( buff, prefix );
    strcat( buff, filename );

    /* Clear the global variables used to contain the results. */

    if( try_file_name != NULL ) {
        mem_free( try_file_name );
        try_file_name = NULL;
    }

    if( try_fp != NULL ) {
        fclose( try_fp );
        try_fp = NULL;
    }

    /* Try to open the file. Return false on failure. */

    for( ;; ) {
        fp = fopen( buff, "rb" );
        if( fp != NULL ) {
            break;
        }
        if( errno != ENOMEM && errno != ENFILE && errno != EMFILE ) {
            break;
        }
        if( !free_resources( errno ) ) {
            break;
        }
    }
    if( fp == NULL ) {
        return( false );
    }

    /* Set the globals on success. */

    try_file_name = mem_alloc( filename_length );
    strcpy( try_file_name, buff );
    try_fp = fp;

    return( true );
}

static char *initialize_cur_directory_list( void )
{
    char    *list;

    list = mem_alloc( 2 );
    list[0] = '.';
    list[1] = '\0';
    return( list );
}

static char *initialize_env_directory_list( const char *name )
{
    char        *env;
    char        *list;
    char        *p;

    env = getenv( name );
    if( env != NULL && *env != '\0' ) {
        p = list = mem_alloc( strlen( env ) + 1 );
        while( *env != '\0' ) {
            if( p != list )
                *p++ = PATH_LIST_SEP;
            env = GetPathElement( env, NULL, &p );
        }
    } else {
        p = list = mem_alloc( 1 );
    }
    *p = '\0';
    return( list );
}

/* Function ff_setup().
 * Initializes the directory lists.
 */

void ff_setup( void )
{
    /* Initialize the globals. */

    try_file_name = NULL;
    try_fp = NULL;

    /* This directory list encodes the current directory. */

    cur_dir_list = initialize_cur_directory_list();

    /* Initialize the directory list for GMLINC. */

    gml_inc_dirs = initialize_env_directory_list( "GMLINC" );

    /* Initialize the directory list for GMLLIB. */

    gml_lib_dirs = initialize_env_directory_list( "GMLLIB" );

    /* Initialize the directory list for PATH. */

    path_dirs = initialize_env_directory_list( "PATH" );

    return;
}

/* Function ff_teardown().
 * Releases the memory allocated by functions in this module.
 */

void ff_teardown( void )
{
    if( try_file_name != NULL ) {
        mem_free( try_file_name );
        try_file_name = NULL;
    }

    if( try_fp != NULL) {
        fclose( try_fp );
        try_fp = NULL;
    }

    if( cur_dir_list != NULL ) {
        mem_free( cur_dir_list );
    }

    if( gml_inc_dirs != NULL ) {
        mem_free( gml_inc_dirs );
    }

    if( gml_lib_dirs != NULL ) {
        mem_free( gml_lib_dirs );
    }

    if( path_dirs != NULL ) {
        mem_free( path_dirs );
    }

    return;
}

/* Function search_file_in_dirs().
 * Searches for filename in curdir and the directories given in the
 * environment variables, as appropriate to the value of sequence.
 *
 * Parameters:
 *      in all cases, dirseq indicates the type of file sought.
 *      if dirseq is ds_bin_lib, then:
 *          filename contains the defined name of a device, driver or font;
 *          defext and altext are ignored.
 *      if dirseq is ds_opt_file, then:
 *          filename contains the name of the file as provided by the user;
 *          defext and altext are ignored.
 *      if dirseq is ds_doc_spec or ds_lib_src, then:
 *          filename contains the name of the file as provided by the user;
 *          defext points to the first extension to use;
 *          altext points to the second extension to use, if any.
 *
 * Returns:
 *      true   if the file is found.
 *      false  if the file is not found.
 *
 * Note:
 *      if the file is found, then try_file() will have set try_file_name
 *      and try_fp to the name as found and FILE * of the file.
 */

bool search_file_in_dirs( const char *filename, const char *defext, const char *altext, dirseq sequence )
{
    pgroup2         pg;
    char            alternate_file[FILENAME_MAX];
    char            default_file[FILENAME_MAX];
    char            primary_file[FILENAME_MAX];
    char            *member_name = NULL;
    char            *searchdirs[5];
    char            *path_list;
    char            *p;
    char            c;
    char            dir_name[FILENAME_MAX];
    size_t          member_length;
    char            **pd;

    /* Ensure filename will fit into buff. */

    if( strlen( filename ) >= FILENAME_MAX ) {
        xx_simple_err_cc( err_file_max, filename, "" );
        return( false );
    }

    /* Initialize the filename buffers. */

    primary_file[0] = '\0';
    alternate_file[0] = '\0';
    default_file[0] = '\0';

    _splitpath2( filename, pg.buffer, &pg.drive, &pg.dir, &pg.fname, &pg.ext );

    /* For ds_bin_lib, filename contains a defined name. */

    if( sequence != ds_bin_lib ) {

        /* Determine if filename contains path information. */

        if( pg.drive[0] != '\0' || pg.dir[0] != '\0' ) {
            xx_simple_err_c( err_file_name, filename );
            return( false );
        }

        /* Ensure the file name will fit in the buffers if the literal extensions
         * are used. Note that all literal extensions contain 4 characters.
         */

        if( pg.ext[0] == '\0' ) {
            if( strlen( filename ) >= FILENAME_MAX - 4 ) {
                switch( sequence ) {
                case ds_opt_file:
                    xx_simple_err_cc( err_file_max, filename, "." OPT_EXT );
                    break;
                case ds_doc_spec:
                    xx_simple_err_cc( err_file_max, filename, "." GML_EXT );
                    break;
                case ds_bin_lib:
                    xx_simple_err_cc( err_file_max, filename, "." COP_EXT );
                    break;
                case ds_lib_src:
                    xx_simple_err_cc( err_file_max, filename, "." PCD_EXT );
                    break;
                default:
                    xx_simple_err_cc( err_file_max, filename, ".xxx" );
                }
                return( false );
            }
        }
    }

    /* Set up the file names and the dirs for the specified sequence. */

    pd = searchdirs;
    switch( sequence ) {
    case ds_opt_file:
        if( pg.ext[0] == '\0' )
            pg.ext = OPT_EXT;
        _makepath( primary_file, NULL, NULL, pg.fname, pg.ext );
        *pd++ = cur_dir_list;
        *pd++ = gml_lib_dirs;
        *pd++ = gml_inc_dirs;
        *pd++ = path_dirs;
        break;
    case ds_doc_spec:
        if( pg.ext[0] == '\0' ) {
            if( altext != NULL && altext[0] != '\0' ) {
                _makepath( alternate_file, NULL, NULL, pg.fname, altext );
            }
            if( defext == NULL || FNAMECMPSTR( defext, GML_EXT )) {
                _makepath( default_file, NULL, NULL, pg.fname, GML_EXT );
            }
            pg.ext = GML_EXT;
        }
        _makepath( primary_file, NULL, NULL, pg.fname, pg.ext );
        *pd++ = cur_dir_list;
        *pd++ = gml_inc_dirs;
        *pd++ = gml_lib_dirs;
        *pd++ = path_dirs;
        break;
    case ds_bin_lib:
        *pd++ = gml_lib_dirs;
        *pd++ = gml_inc_dirs;
        *pd++ = path_dirs;
        break;
    case ds_lib_src:
        if( altext == NULL || altext[0] == '\0' )
            altext = FON_EXT;
        _makepath( alternate_file, NULL, NULL, pg.fname, altext );
        if( pg.ext[0] == '\0' )
            pg.ext = PCD_EXT;
        _makepath( primary_file, NULL, NULL, pg.fname, pg.ext );
        *pd++ = cur_dir_list;
        *pd++ = gml_inc_dirs;
        break;
    default:
        internal_err( __FILE__, __LINE__ );
        return( false );
    }
    *pd = NULL;

    /* Search each directory for each filename. */

    for( pd = searchdirs; (path_list = *pd) != NULL; pd++ ) {
        while( (c = *path_list) != '\0' ) {
            p = dir_name;
            do {
                ++path_list;
                if( IS_PATH_LIST_SEP( c ) )
                    break;
                *p++ = c;
            } while( (c = *path_list) != '\0' );
            c = p[-1];
            if( !IS_PATH_SEP( c ) ) {
                *p++ = DIR_SEP;
            }
            *p = '\0';

            /* For ds_bin_lib, set primary file from the defined name. */

            if( sequence == ds_bin_lib ) {

            /* See if dir_ptr contains a wgmlst.cop file. */

                if( !try_open( dir_name, "wgmlst.cop" ) ) {
                    continue;
                }

                /* try_fp now contains a FILE * to the directory file. */

                member_name = get_member_name( filename );
                if( member_name == NULL ) {
                    continue;
                }

                /* Construct primary_file and open it normally. */

                member_length = strlen( member_name );
                if( memchr( member_name, '.', member_length ) == NULL ) {

                    /* Avoid buffer overflow from member_name. */

                    if( member_length >= FILENAME_MAX - 4 ) {
                        xx_simple_err_cc( err_file_max, member_name, "." COP_EXT );
                        mem_free( member_name );
                        return( false );
                    }
#ifdef __UNIX__
                    strlwr( member_name );
#endif
                    _makepath( primary_file, NULL, NULL, member_name, COP_EXT );
                }
                mem_free( member_name );
            }

            if( try_open( dir_name, primary_file ) ) {
                return( true );
            }

            /* Not finding the file is only a problem for ds_bin_lib. */

            if( sequence == ds_bin_lib ) {
                xx_simple_err_cc( err_mem_dir, dir_name, primary_file );
                return( false );
            }

            if( alternate_file != NULL ) {
                if( try_open( dir_name, alternate_file ) ) {
                    return( true );
                }
            }

            if( default_file != NULL ) {
                if( try_open( dir_name, default_file ) ) {
                    return( true );
                }
            }
        }
    }

    return( false );
}
