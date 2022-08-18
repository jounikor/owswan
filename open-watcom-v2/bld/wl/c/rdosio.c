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
* Description:  POSIX conforming versions of the linker i/o functions.
*
****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <conio.h>
#include <errno.h>
#include "wio.h"
#include "linkstd.h"
#include "msg.h"
#include "alloc.h"
#include "wlnkmsg.h"
#include "objio.h"
#include "fileio.h"

#include "clibext.h"

#define TOOMANY EMFILE

static int      OpenFiles;      // the number of open files
static unsigned LastResult;
static bool     CaughtBreak;    // set to true if break hit.

void TrapBreak( int sig_num )
/***************************/
{
    sig_num = sig_num;          // to avoid a warning, will be optimized out.
    CaughtBreak = true;
}

void CheckBreak( void )
/*********************/
{
    if( CaughtBreak ) {
        CaughtBreak = false;        /* prevent recursion */
        LnkMsg( FTL+MSG_BREAK_HIT, NULL );    /* suicides */
    }
}

void SetBreak( void )
/*******************/
{
}

void RestoreBreak( void )
/***********************/
{
}

void LnkFilesInit( void )
/***********************/
{
    OpenFiles = 0;
    CaughtBreak = false;
}

void PrintIOError( unsigned msg, const char *types, const char *name )
/********************************************************************/
{
    LnkMsg( msg, types, name, strerror( errno ) );
}

static int DoOpen( const char *name, unsigned mode, bool isexe )
/**************************************************************/
{
    int     h;
    int     perm;

    CheckBreak();
    mode |= O_BINARY;
    perm = PMODE_RW;
    if( isexe )
        perm = PMODE_RWX;
    for( ;; ) {
        if( OpenFiles >= MAX_OPEN_FILES )
            CleanCachedHandles();
        h = open( name, mode, perm );
        if( h != -1 ) {
            OpenFiles++;
            break;
        }
        if( errno != TOOMANY )
            break;
        if( !CleanCachedHandles() ) {
            break;
        }
    }
    return( h );
}

f_handle QOpenR( const char *name )
/*********************************/
{
    int     h;

    h = DoOpen( name, O_RDONLY, false );
    if( h != -1 )
        return( h );
    LnkMsg( FTL+MSG_CANT_OPEN, "12", name, strerror( errno ) );
    return( NIL_FHANDLE );
}

f_handle QOpenRW( const char *name )
/**********************************/
{
    int     h;

    h = DoOpen( name, O_RDWR | O_CREAT | O_TRUNC, false );
    if( h != -1 )
        return( h );
    LnkMsg( FTL+MSG_CANT_OPEN, "12", name, strerror( errno ) );
    return( NIL_FHANDLE );
}

f_handle ExeCreate( const char *name )
/************************************/
{
    int     h;

    h = DoOpen( name, O_RDWR | O_CREAT | O_TRUNC, true );
    if( h != -1 )
        return( h );
    LnkMsg( FTL+MSG_CANT_OPEN, "12", name, strerror( errno ) );
    return( NIL_FHANDLE );
}

f_handle ExeOpen( const char *name )
/**********************************/
{
    int     h;

    h = DoOpen( name, O_RDWR, true );
    if( h != -1 )
        return( h );
    LnkMsg( FTL+MSG_CANT_OPEN, "12", name, strerror( errno ) );
    return( NIL_FHANDLE );
}

size_t QRead( f_handle file, void *buffer, size_t len, const char *name )
/***********************************************************************/
/* read into far memory */
{
    size_t      h;

    CheckBreak();
    h = read( file, buffer, len );
    if( h == IOERROR ) {
        LnkMsg( ERR+MSG_IO_PROBLEM, "12", name, strerror( errno ) );
    }
    return( h );
}

size_t QWrite( f_handle file, const void *buffer, size_t len, const char *name )
/******************************************************************************/
/* write from far memory */
{
    size_t      h;
    char        rc_buff[RESOURCE_MAX_SIZE];

    if( len == 0 )
        return( 0 );

    CheckBreak();
    h = write( file, buffer, len );
    if( name != NULL ) {
        if( h == IOERROR ) {
            LnkMsg( ERR+MSG_IO_PROBLEM, "12", name, strerror( errno ) );
        } else if( (unsigned)h != len ) {
            Msg_Get( MSG_IOERRLIST_7, rc_buff );
            LnkMsg( (FTL+MSG_IO_PROBLEM) & ~OUT_MAP, "12", name, rc_buff );
        }
    }
    return( h );
}

char NLSeq[] = { "\r\n" };

void QWriteNL( f_handle file, const char *name )
/**********************************************/
{
    QWrite( file, NLSeq, sizeof( NLSeq ) - 1, name );
}

void QClose( f_handle file, const char *name )
/********************************************/
/* file close */
{
    int         h;

    CheckBreak();
    h = close( file );
    OpenFiles--;
    if( h != -1 )
        return;
    LnkMsg( ERR+MSG_IO_PROBLEM, "12", name, strerror( errno ) );
}

long QLSeek( f_handle file, long position, int start, const char *name )
/**********************************************************************/
{
    long    h;

    CheckBreak();
    h = lseek( file, position, start );
    if( h == -1L && name != NULL ) {
        LnkMsg( ERR+MSG_IO_PROBLEM, "12", name, strerror( errno ) );
    }
    return( h );
}

void QSeek( f_handle file, unsigned long position, const char *name )
/*******************************************************************/
{
    QLSeek( file, position, SEEK_SET, name );
}

unsigned long QPos( f_handle file )
/*********************************/
{
    CheckBreak();
    return( lseek( file, 0L, SEEK_CUR ) );
}

unsigned long QFileSize( f_handle file )
/**************************************/
{
    long        result;

    result = filelength( file );
    if( result == -1L ) {
        result = 0;
    }
    return( result );
}

void QDelete( const char *name )
/******************************/
{
    int   h;

    if( name == NULL )
        return;
    h = remove( name );
    if( h == -1 && errno != ENOENT ) { /* file not found is OK */
        LnkMsg( ERR+MSG_IO_PROBLEM, "12", name, strerror( errno ) );
    }
}

bool QReadStr( f_handle file, char *dest, size_t size, const char *name )
/***********************************************************************/
/* quick read string (for reading directive file) */
{
    bool            eof;
    char            ch;
    size_t          len;

    eof = false;
    while( --size > 0 ) {
        len = QRead( file, &ch, 1, name );
        if( len == 0 || len == IOERROR ) {
            eof = true;
            break;
        } else if( ch != '\r' ) {
            *dest++ = ch;
        }
        if( ch == '\n' ) {
            break;
        }
    }
    *dest = '\0';
    return( eof );
}

bool QIsDevice( f_handle file )
/*****************************/
{
    return( isatty( file ) != 0 );
}

static f_handle NSOpen( const char *name, unsigned mode )
/*******************************************************/
{
    int         h;

    h = DoOpen( name, mode, false );
    LastResult = h;
    if( h != -1 )
        return( h );
    return( NIL_FHANDLE );
}

f_handle QObjOpen( const char *name )
/***********************************/
{
    return( NSOpen( name, O_RDONLY ) );
}

f_handle TempFileOpen( const char *name )
/***************************************/
{
    return( NSOpen( name, O_RDWR ) );
}

bool QModTime( const char *name, time_t *time )
/*********************************************/
{
    int         result;
    struct stat buf;

    result = stat( name, &buf );
    *time = buf.st_mtime;
    return( result != 0 );
}

time_t QFModTime( int handle )
/****************************/
{
    struct stat buf;

    fstat( handle, &buf );
    return( buf.st_mtime );
}

int WaitForKey( void )
/********************/
{
    return( getch() );
}
