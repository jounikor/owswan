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
* Description:  Open Watcom file compression utility.
*
****************************************************************************/


/*
 * Based on Japanese version 29-NOV-1988
 * LZSS coded by Haruhiko OKUMURA
 * Adaptive Huffman Coding coded by Haruyasu YOSHIZAKI
 * Edited and translated to English by Kenji RIKITAKE
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef __WATCOMC__
    #include <malloc.h>
#endif
//#include "formglob.h"
#include "wpack.h"
//#include "rsr.h"
#include "txttable.h"
#include "walloca.h"
#ifndef __WATCOMC__
    #include <malloc.h>
#endif
#include "common.h"
#include "wpackio.h"
#include "dtparse.h"
#include "pathgrp2.h"

#include "clibext.h"


typedef struct {
    uint_16     year;           /* full year (e.g., 1990) */
    uint_8      month;          /* 1-12 */
    uint_8      day;            /* 1-31 */
} datestruct;

typedef struct {
    uint_8      hours;          /* 0-23 */
    uint_8      minutes;        /* 0-59 */
    uint_8      seconds;        /* 0-59 */
} timestruct;

// external declarations

extern datestruct DateAdjust;
extern timestruct TimeAdjust;

static bool BannerPrinted = false;

void PackExit( void )
/**************************/
{
    exit( EXIT_FAILED );
}

void Error(int code,char *message)
/******************************/
{
    code=code;
    WriteMsg( "\nError: " );
    WriteMsg( message );
    WriteMsg( "\n" );
    PackExit();
}

void * WPMemAlloc( size_t amount )
/***************************************/
{
    void *  ret;

    ret = malloc( amount );
    if( ret == NULL ) {
        Error( -1, "dynamic memory exhausted!" );
    }
    return( ret );
}

void WPMemFree( void *mem )
/******************************/
{
    free( mem );
}

enum {
    ERROR = 0,
    DO_ENCODE,
    DO_DECODE,
    DO_LIST,
    DO_DELETE
};

static void Usage( bool verbose )
/*******************************/
{
    WriteMsg( "Usage: wpack [-?acdklpqr] [-mNNNN] [-tDATE TIME] arcfile @filename files...\n" );
    if( verbose ) {
        WriteMsg( "-? = print this list\n"
                  "-a = add files to archive\n"
                  "-c = preserve the file name case in the archive\n"
                  "-d = delete files from archive\n"
                  "-k = keep pathnames on files when archiving\n"
                  "-mNNNN = make multiple archives with maximum size NNNN k\n"
                  "-l = generate a listing of the files in the archive\n"
                  "-p = prepend a pathname when unpacking files (e.g. -pc:\\lang)\n"
                  "-q = be quiet\n"
                  "-r = replace pathname when unpacking files (e.g. -rc:\\lang)\n"
                  "-tDATE TIME = use the specified date & time for the files\n"
                  "              ex. -tmm-dd-yy hh:mm:ss\n"
                  "@filename = list of files to process, one per line\n"
                  "files may contain wildcards\n"
                  "the default action is to extract files from the archive\n"
                  "if no files are specified when unpacking, all files will be unpacked\n" );
    }
    PackExit();
}

static void ProcPath( char **argv, arccmd *cmd )
/**********************************************/
{
    size_t len1;

    (*argv)++;
    len1 = strlen( *argv ) + 1;
    cmd->u.path = WPMemAlloc( len1 );
    memcpy( cmd->u.path, *argv, len1 );
}

static void SetCmdTime( arccmd *cmd )
/***********************************/
/* set the time field in the cmd structure */
{
    struct tm   timeval;

    timeval.tm_sec = TimeAdjust.seconds;
    timeval.tm_min = TimeAdjust.minutes;
    timeval.tm_hour = TimeAdjust.hours;
    timeval.tm_mday = DateAdjust.day;
    timeval.tm_mon = DateAdjust.month - 1;
    timeval.tm_year = DateAdjust.year - 1900;
    timeval.tm_isdst = -1;
    cmd->time = mktime( &timeval );
}

static wpackfile *AddFileName( wpackfile *list, char *fname, size_t *listlen )
/****************************************************************************/
{
    char            *packname;
    size_t          len;

    len = *listlen;
    list = realloc( list, ( len + 1 ) * sizeof( *list ) );
    if( fname == NULL ) {
        list[len].filename = NULL;
        list[len].packname = NULL;
    } else {
        packname = NULL;
        packname = strchr( fname, ';' );
        if( packname == NULL ) {
            list[len].packname = NULL;
        } else {
            *packname = '\0';
            ++packname;
            list[len].packname = strdup( packname );
        }
        list[len].filename = strdup( fname );
    }
    *listlen = ++len;
    return( list );
}

static wpackfile *ProcFileName( char **argv )
/*******************************************/
{
    size_t      newlistlen;
    wpackfile   *newlist;
    char        buff[256];
    char        *curr;
    FILE        *io;

    newlist = NULL;
    newlistlen = 0;
    for( ; *argv != NULL ; ++argv ) {
        if( **argv == '@' ) {
            io = fopen( ++(*argv), "r" );
            while( fgets( buff, sizeof( buff ), io ) != NULL ) {
                curr = strrchr( buff, '\n' );
                if( curr != NULL ) {
                    *curr = '\0';
                    if( strlen( buff ) > 0 ) {
                        newlist = AddFileName( newlist, buff, &newlistlen );
                    }
                } else {
                    Error( -1, "invalid line in directive file\n" );
                }
            }
            fclose( io );
        } else {
            newlist = AddFileName( newlist, *argv, &newlistlen );
        }
    }
    newlist = AddFileName( newlist, NULL, &newlistlen );
    return( newlist );
}

static void PrintBanner( void )
/*****************************/
{
    if( !BannerPrinted ) {
        WriteMsg( "WATCOM Install Archiver Version 1.3\n" );
        WriteMsg( "Copyright by WATCOM Systems Inc. 1992.  All rights reserved.\n" );
        BannerPrinted = true;
    }
}

static int ProcessArgs( char **argv, arccmd *cmd )
/************************************************/
{
    int     status;
    char    c;

    cmd->files    = NULL;
    cmd->flags    = 0;
    cmd->u.path   = NULL;
    cmd->internal = 0;
    status = DO_DECODE;
    for( ++argv; *argv != NULL; ++argv ) {
        if( **argv == '-' || **argv == '/' ) {
            (*argv)++;
            c = toupper( **argv );
            switch( c ) {
            case '?':
                Usage( true );
                break;
            case 'A':
                status = DO_ENCODE;
                break;
            case 'C':
                cmd->flags |= PRESERVE_FNAME_CASE;
                break;
            case 'D':
                status = DO_DELETE;
                break;
            case 'I':
                (*argv)++;
                if( *argv != '\0' ) {
                    cmd->internal = strtoul( *argv, NULL, 0 );
                    cmd->flags |= SECURE_PACK;
                }
                break;
            case 'K':
                cmd->flags |= KEEP_PATHNAME;
                break;
            case 'L':
                status = DO_LIST;
                break;
            case 'M':
                cmd->flags |= PACK_LIMIT;
                (*argv)++;
                cmd->u.limit = atoi( *argv );
                break;
            case 'P':
                cmd->flags |= PREPEND_PATH;
                ProcPath( argv, cmd );
                break;
            case 'Q':
                BannerPrinted = true;
                cmd->flags |= BE_QUIET;
                break;
            case 'R':
                cmd->flags |= REPLACE_PATH;
                ProcPath( argv, cmd );
                break;
            case 'T':
                if( !(cmd->flags & USE_DATE_TIME) ){
                    cmd->flags |= USE_DATE_TIME;
                    (*argv)++;
                    WhereAmI();
                    DoDOption( *argv );
                    argv++;
                    if( *argv == NULL ) {
                        PrintBanner();
                        Error( -1, "not enough arguments for time option" );
                    }
                    DoTOption( *argv );
                    SetCmdTime( cmd );
                }
                break;
            default:
                PrintBanner();
                WriteMsg( "unknown option: ");
                WriteMsg( *argv );
                WriteMsg( "\n" );
                Usage( false );
            }
        } else {
            break;
        }
    }
    PrintBanner();
    if( cmd->flags & (REPLACE_PATH | PREPEND_PATH) && status == DO_ENCODE
       || cmd->flags & (KEEP_PATHNAME | PACK_LIMIT) && status == DO_DECODE ) {
        WriteMsg( "inconsistant options specified\n" );
        status = ERROR;
    } else if( *argv == NULL ) {
        WriteMsg( "no archive file specified\n" );
        status = ERROR;
    } else {
        cmd->arcname = *argv;
        argv++;
        cmd->files = ProcFileName( argv );
    }
    return( status );
}

static void WriteNumber( unsigned long number, unsigned indent )
/**************************************************************/
{
    char            numstr[ 11 ];

    ultoa( number, numstr, 10 );
    IndentLine( indent - strlen( numstr ) );
    WriteMsg( numstr );
}

static int DisplayArchive( arccmd *cmd )
/**************************************/
{
    file_info **    filedata;
    file_info **    currfile;
    file_info *     nextfile;
    unsigned long   currspot;
    unsigned long   compressed;
    unsigned long   totalcomp;
    unsigned long   totaluncomp;
    arc_header      header;

    filedata = ReadHeader( cmd, &header );
    if( filedata == NULL ) {
        WriteMsg( "archive file does not exist\n" );
    } else {
        WriteMsg( "  Unpacked     Packed Name\n" );
        WriteMsg( "---------- ---------- ----\n" );
        totalcomp = 0;
        totaluncomp = 0;
        currspot = sizeof( arc_header );
        for( currfile = filedata; *currfile != NULL; currfile++ ) {
            totaluncomp += (*currfile)->length;
            WriteNumber( (*currfile)->length, 10 );
            nextfile = *(currfile + 1);
            if( nextfile == NULL ) {
                compressed = header.info_offset - currspot;
            } else {
                compressed = nextfile->disk_addr - currspot;
                currspot = nextfile->disk_addr;
            }
            totalcomp += compressed;
            WriteNumber( compressed, 11 );
            WriteLen( " ", 1 );
            WriteLen( (*currfile)->name, (*currfile)->namelen & NAMELEN_MASK );
            WriteMsg( "\n" );
        }
        WriteMsg( "---------- ----------\n" );
        WriteNumber( totaluncomp, 10 );
        WriteNumber( totalcomp, 11 );
    }
    return( true );
}

static void HandleError( arccmd *cmd )
/************************************/
{
    /* unused parameters */ (void)cmd;
}

static int DeleteEntry( arccmd *cmd )
/***********************************/
{
    char            tempname[ L_tmpnam ];
    pgroup2         pg1;
    pgroup2         pg2;
    char            *tmpfname;
    arc_header      header;         // archive main header.
    file_info       **filedata;     // block of file infos from old archive.
    file_info       **currdata;
    file_info       *nextdata;
    wpackfile       *currfile;
    wpackfile       *endfile;
    bool            deletethis;
    bool            onedeleted;
    unsigned long   offset;
    unsigned long   compressed;
    unsigned_16     num_left;
    unsigned        entrylen;
    size_t          namelen;

    if( cmd->files == NULL  ||  cmd->files->filename ) {
        Error( -1, "No files to delete\n" );
    }
    filedata = ReadHeader( cmd, &header );
    if( filedata == NULL ) {
        Error( -1, "archive file does not exist\n" );
    }

    tmpnam( tempname );     // get a temporary file name & put in same dir.
    namelen = strlen( tempname ) + strlen( cmd->arcname ) + 1;  // as arcname
    tmpfname = alloca( namelen );
    _splitpath2( cmd->arcname, pg1.buffer, &pg1.drive, &pg1.dir, NULL, NULL );
    _splitpath2( tempname, pg2.buffer, NULL, NULL, &pg2.fname, &pg2.ext );
    _makepath( tmpfname, pg1.drive, pg1.dir, pg2.fname, pg2.ext );
    outfile = QOpenW( tmpfname );
    if( outfile < 0 )
        PackExit();
    for( endfile = cmd->files; (endfile + 1)->filename != NULL; endfile++ ) {}
    QWrite( outfile, &header, sizeof( arc_header ) );   // reserve space
    offset = sizeof( arc_header );
    QSeek( infile, sizeof( arc_header ), SEEK_SET );
    onedeleted = false;
    for( currdata = filedata; *currdata != NULL; currdata++ ) {
        deletethis = false;
        for( currfile = cmd->files; currfile->filename != NULL; currfile++ ) {
            namelen = (*currdata)->namelen & NAMELEN_MASK;
            if( strlen( currfile->filename ) == namelen &&
                strnicmp( currfile->filename, (*currdata)->name, namelen ) == 0 ) {
                deletethis = true;
                break;
            }
        }
        nextdata = *(currdata + 1);
        if( nextdata == NULL ) {
            compressed = header.info_offset - (*currdata)->disk_addr;
        } else {
            compressed = nextdata->disk_addr - (*currdata)->disk_addr;
        }
        if( deletethis ) {
            onedeleted = true;
            header.num_files--;
            *currdata = NULL;
            QSeek( infile, nextdata->disk_addr, SEEK_SET );
            *currfile = *endfile;       // delete file name from list.
            endfile->filename = NULL;
            endfile->packname = NULL;
            endfile--;
        } else {
            CopyInfo( outfile, infile, compressed );
            (*currdata)->disk_addr = offset;
            offset += compressed;
        }
    }
    for( currfile = cmd->files; currfile->filename != NULL; currfile++ ) {
        WriteMsg( "Could not find file '" );
        WriteMsg( currfile->filename );
        WriteMsg( "' for deletion\n" );
    }
    QClose( infile );
    num_left = header.num_files;
    if( num_left == 0 ) {
        WriteMsg( "no files left in archive\n" );
        remove( cmd->arcname );
        QClose( outfile );
        remove( tmpfname );
    } else if( !onedeleted ) {
        WriteMsg( "no files deleted from archive\n" );
        QClose( outfile );
        remove( tmpfname );
    } else {
        currdata = filedata;
        header.info_offset = offset;
        header.info_len = 0;
        while( num_left > 0 ) {
            if( *currdata != NULL ) {
                num_left--;
                entrylen = sizeof(file_info) +
                                 ((*currdata)->namelen & NAMELEN_MASK) - 1;
                header.info_len += entrylen;
                QWrite( outfile, *currdata, entrylen );
            }
            currdata++;
        }
        QSeek( outfile, 0, SEEK_SET );
        QWrite( outfile, &header, sizeof( arc_header ) );
        QClose( outfile );
        remove( cmd->arcname );
        rename( tmpfname, cmd->arcname );
    }
    return( true );
}

static int (*CmdJumpTable[])(arccmd *) = {
    NULL,       /* Error */
    Encode,
    Decode,
    DisplayArchive,
    DeleteEntry
};


#if 0
static char *StartUp()
{
    RSRNAME( txt_table, "TEXT_TABLE" );
    char *msg;

#ifdef __WATCOMC__
    _nheapgrow();
#endif
//  form_init();
//  form_start_up( false );
    msg = rsr_load_file( "wpack.rsr" );
    if( msg == NULL ) {
        msg = rsr_text_connect( &txt_table.rsr_string );
    }
    return( msg );
}

static void CloseDown()
//=====================
{
    rsr_text_disconnect();
//  form_close_down();
//  form_fini();
}
#endif

int main( int argc, char **argv )
/*******************************/
{
    int     action;
    arccmd  cmd;

    if (argc < 2) {
        Usage( false );
    } else {
        action = ProcessArgs( argv, &cmd );
    }
#if 0
    if( StartUp() != NULL ) {
        Log( "Cannot load resource", NULL );
        exit ( -1 );
    }
#endif
#if defined( __WATCOMC__ ) && !defined( __OS2__ )
    _nheapgrow();
#endif
    SetupTextTable();
    InitIO();
    if( CmdJumpTable[action] == NULL ) {
        HandleError( &cmd );
        return( EXIT_FAILED );
    }
    (*CmdJumpTable[action])( &cmd );
//  CloseDown();
    return( EXIT_OK );
}

