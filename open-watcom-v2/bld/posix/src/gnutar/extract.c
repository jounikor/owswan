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
* Description:  Extract files from a tar archive.
*
****************************************************************************/


/*
 * Extract files from a tar archive.
 *
 * Written 19 Nov 1985 by John Gilmore, ihnp4!hoptoad!gnu.
 * MS-DOS port 2/87 by Eric Roskos.
 * Minix  port 3/88 by Eric Roskos.
 *
 * @(#) extract.c 1.17 86/10/29 Public Domain - gnu
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <direct.h>
#include <process.h>
#if defined( __UNIX__ ) || defined( __WATCOMC__ )
    #include <utime.h>
#else
    #include <sys/utime.h>
#endif
#include <time.h>
#include "wio.h"

#ifdef BSD42
#include <sys/file.h>
#endif

#include "tar.h"
#include "list.h"
#include "buffer.h"
#include "port.h"
#include "extract.h"


static time_t      now = 0;         /* Current time */

#ifndef MSDOS
static int mychown( char *pathname )
{
    return( chown( pathname, from_oct( 8, head->header.uid ), from_oct( 8, head->header.gid ) ) );
}
#endif

/*
 * After a file/link/symlink/dir creation has failed, see if
 * it's because some required directory was not present, and if
 * so, create all required dirs.
 */
static int make_dirs( char *pathname )
{
    char    *p;                     /* Points into path */
    int     madeone = 0;            /* Did we do anything yet? */
    int     save_errno = errno;     /* Remember caller's errno */
    int     check;

    if( errno != ENOENT )
        return 0;                   /* Not our problem */

    for( p = strchr( pathname, '/' ); p != NULL; p = strchr( p + 1, '/' ) ) {
        /* Avoid mkdir of empty string, if leading or double '/' */
        if( p == pathname || p[-1] == '/' )
            continue;
        /* Avoid mkdir where last part of path is '.' */
        if( p[-1] == '.' && ( p == pathname + 1 || p[-2] == '/' ) )
            continue;
        *p = 0;                     /* Truncate the path there */
#ifdef MSDOS
        check = mkdir( pathname );  /* Try to create it as a dir */
#else
        check = mkdir( pathname, 0777 );  /* Try to create it as a dir */
#endif
        *p = '/';
        if( check == 0 ) {
#ifndef MSDOS
            /* chown, chgrp it same as file being created */
            mychown( pathname );
#endif
            /* FIXME, show mode as modified by current umask */
            pr_mkdir( pathname, (int)( p - pathname ), 0777 );
            madeone++;              /* Remember if we made one */
            continue;
        }
        if( errno == EEXIST )       /* Directory already exists */
            continue;
#ifdef MSDOS
        if( errno == EACCES )       /* DOS version of 'already exists' */
                continue;
#endif
        /*
        * Some other error in the mkdir.  We return to the caller.
        */
        break;
    }

    errno = save_errno;             /* Restore caller's errno */
    return madeone;                 /* Tell them to retry if we made one */
}

/*
 * Extract a file from the archive.
 * Note: xname is the "fixed name" which is supposed to be acceptable
 * to the OS.  We use this instead of the name in the header, but only
 * if we're actually creating a data file.
 */
void extract_archive( char *xname )
{
    char    *data;
    int     fd, check, written;
    size_t  namelen;
    long    size;
    int     standard;               /* Is header standard? */
    struct  utimbuf  acc_upd_times;
    struct  stat     st;

    saverec( &head );               /* Make sure it sticks around */
    userec( head );                 /* And go past it in the archive */
    decode_header( head, hstat, &standard, 1 );   /* Snarf fields */

    /* Print the record from 'head' and 'hstat' */
    if( f_verbose )
        print_header( xname );

    switch( head->header.linkflag ) {
    default:
        annofile( stderr, tar );
        fprintf( stderr, "Unknown file type %d for %s\n", head->header.linkflag, head->header.name );
        /* fall through */
    case LF_OLDNORMAL:
    case LF_NORMAL:
        /*
         * Appears to be a file. See if it's really a directory.
         */
        namelen = strlen( head->header.name ) - 1;
        if( head->header.name[namelen] == '/' )
            goto really_dir;

        /* FIXME, deal with protection issues */
        /* FIXME, f_keep doesn't work on V7, st_mode loses too */
again_file:
#ifdef V7
        fd = creat( xname, phstat->st_mode );
#else
        fd = open( /* head->header.name */ xname,
                f_keep ?
                O_NDELAY | O_WRONLY | O_APPEND | O_CREAT | O_EXCL | convmode( xname ) :
                O_NDELAY | O_WRONLY | O_APPEND | O_CREAT | O_TRUNC | convmode( xname ),
                phstat->st_mode );
#endif
        if( fd < 0 ) {
            /* use original name here */
            if( make_dirs( head->header.name ) )
                goto again_file;
            annofile( stderr, tar );
            fprintf( stderr, "Could not make file " );
            perror( /* head->header.name */ xname );
            skip_file( (long)phstat->st_size );
            goto quit;
        }

        for( size = phstat->st_size; size > 0; size -= written ) {

            /*
             * Locate data, determine max length writeable, write it, record
             * that we have used the data, then check if the write worked.
             */
            data = findrec()->charptr;
            written = (int)( endofrecs()->charptr - data );
            if( written > size )
                written = size;
            errno = 0;
            check = write( fd, data, written );

            /*
             * The following is in violation of strict typing, since the arg
             * to userec should be a struct rec *. FIXME.
             */
            /* BartoszP: to avoid error type casting was made */
            userec( (union record *)( data + written - 1 ) );
            if( check == written )
                continue;

            /*
             * Error in writing to file. Print it, skip to next file in
             * archive.
             */
            annofile( stderr, tar );
            fprintf( stderr, "Tried to write %d bytes to file, could only write %d:\n", written, check );
            perror( /* head->header.name */ xname );
            (void)close( fd );
            skip_file( (long)( size - written ) );
            goto quit;
        }

        check = close( fd );
        if( check < 0 ) {
            annofile( stderr, tar );
            fprintf( stderr, "Error while closing " );
            perror( /* head->header.name */ xname );
        }

#ifndef MSDOS
        /* deal with uid/gid FIXME: mtimes/suid */
        /* FIXME - should use new name-string fields if present */
        mychown( xname );
#endif
        /*
         * Set the modified time of the file.
         *
         * Note that we set the accessed time to "now", which is really "the
         * time we started extracting files".
         */
        if( !f_modified ) {
            if( !now )
                now = time( NULL );                     /* Just do it once */
            acc_upd_times.actime = now;                 /* Accessed now */
            acc_upd_times.modtime = phstat->st_mtime;   /* Mod'd */
            chmod( xname, S_IREAD|S_IWRITE );
            if( utime( /* head->header.name */ xname, &acc_upd_times ) < 0 ) {
                annofile( stderr, tar );
                perror( /* head->header.name */ xname );
            }
            chmod( xname, phstat->st_mode );
        }

        /*
         * If '-p' is not set, OR if the file has pretty normal mode bits, we
         * can skip the chmod and save a sys call. This works because we did
         * umask(0) if -p is set, so the open() that created the file will
         * have set the modes properly. FIXME: I don't know what open() does
         * w/UID/GID/SVTX bits. However, if we've done a chown(), they got
         * reset. Also skip CHMOD for MS/DOS (at least for now) since none of
         * these bits are defined.
         */
#ifndef MSDOS
        if( f_use_protection && (phstat->st_mode & (S_ISUID | S_ISGID | S_ISVTX)) ) {
            if( chmod( /* head->header.name */ xname, phstat->st_mode ) < 0 ) {
                annofile( stderr, tar );
                perror( /* head->header.name */ xname );
            }
        }
#endif
quit:
        break;
    case LF_LINK:
#ifndef MSDOS
again_link:
        check = link( head->header.linkname, head->header.name );
        if( check == 0 )
            break;
        if( make_dirs( head->header.linkname ) )
            goto again_link;
        annofile( stderr, tar );
        fprintf( stderr, "Could not link %s to ", head->header.name );
        perror( head->header.linkname );
#else
        fprintf( stderr, "Cannot link %s to %s: linking not supported by DOS\n", head->header.name, head->header.linkname );
#endif
        break;
#if defined S_IFLNK && defined __UNIX__
    case LF_SYMLINK:
again_symlink:
        check = symlink( head->header.linkname, head->header.name );
        /* FIXME, don't worry uid, gid, etc... */
        if( check == 0 )
            break;
        if( make_dirs( head->header.linkname ) )
            goto again_symlink;
        annofile( stderr, tar );
        fprintf( stderr, "Could not create symlink " );
        perror( head->header.linkname );
        break;
#endif
    case LF_CHR:
        phstat->st_mode |= S_IFCHR;
        goto make_node;
#ifdef S_IFBLK
    case LF_BLK:
        phstat->st_mode |= S_IFBLK;
#endif
make_node:
#ifndef MSDOS
        /* FIXME: constant 1024 should be #define'd fs blocksize.  But,
         * this only works on my Minix system anyway; on a standard
         * Minix or Unix, the 3rd parameter will be ignored. */
        check = mknod( head->header.name, (int)phstat->st_mode, (int)phstat->st_dev, (int)( phstat->st_size / 1024 ) );
        if( check != 0 ) {
            if( make_dirs( head->header.name ) )
                goto make_node;
            annofile( stderr, tar );
            fprintf( stderr, "Could not make special file " );
            perror( head->header.name );
            break;
        }
#else
        /* with DOS, either it's there or you've got to change driver */
        annofile( stderr, tar );
        fprintf( stderr, "Cannot create special file %s\n", head->header.name );
#endif
        break;
    case LF_DIR:
        /* Check for trailing / */
        namelen = strlen( head->header.name ) - 1;
really_dir:
        while( namelen && head->header.name[namelen] == '/' )
            head->header.name[namelen--] = '\0';            /* Zap / */

        /* FIXME, deal with umask */
again_dir:
#ifdef MSDOS

        /*
         * don't do the mkdir under MSDOS if the dir already exists. (Won't
         * this give an error under Unix too?  Should it? I don't think
         * Unix's tar does.)
         */
        if( stat( head->header.name, &st ) == 0 && (st.st_mode & S_IFDIR) ) {
            check = 0;
        } else {
            check = mkdir( head->header.name );
        }
#else
        check = mkdir( head->header.name, (int)phstat->st_mode );
#endif

#ifndef MSDOS
        if( check == 0 )
            mychown( head->header.name );
#endif
        if( check != 0 ) {
            if( make_dirs( head->header.name ) )
                goto again_dir;
            annofile( stderr, tar );
            fprintf( stderr, "Could not make directory " );
            perror( head->header.name );
            break;
        }

        /* FIXME, deal with uid/gid */
        /* FIXME, Remember timestamps for after files created? */
        break;
    case LF_FIFO:
        abort();                                /* FIXME */
        break;
    }

    /* We don't need to save it any longer. */
    saverec( NULL );                            /* Unsave it */
}
