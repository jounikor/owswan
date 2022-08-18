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
* Description:  Common stuff for wcl and owcc.
*
****************************************************************************/


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef __UNIX__
#include <direct.h>
#else
#include <dirent.h>
#endif
#include "wio.h"
#include "diskos.h"
#include "clcommon.h"
#include "pathgrp2.h"
#ifdef TRMEM
#include "trmem.h"
#endif

#include "clibext.h"


#ifndef __UNIX__
#define ATTR_MASK   _A_HIDDEN + _A_SYSTEM + _A_VOLID + _A_SUBDIR
#endif

flags   Flags;
DBG_OPT DebugFlag = DBG_NONE;           /* debug info wanted                  */
DBG_OPT DebugFormat = DBG_FMT_DWARF;    /* debug info format                  */
char    *StackSize = NULL;              /* size of stack                      */
list    *Libs_List;                     /* list of libraires from Cmd         */
char    *Map_Name;                      /* name of map file                   */
list    *Obj_List;                      /* linked list of object filenames    */
char    *Obj_Name;                      /* object file name pattern           */
char    *Exe_Name;                      /* name of executable                 */
list    *Directive_List;                /* linked list of wlink directives    */

static char *DebugOptions[] = {
    "",
    " lines",
    " all",
    "debug dwarf",
    "debug watcom",
    "debug codeview",
};

#ifdef TRMEM
static _trmem_hdl   TRMemHandle;

static void memPrintLine( void *fh, const char *buf, size_t len )
{
    /* unused parameters */ (void)fh; (void)len;

    PrintMsg( "%s\n", buf );
}
#endif

void PrintMsg( const char *fmt, ... )
/***********************************/
{
    char        c;
    int         i;
    char        *p;
    va_list     args;
    char        buf[22];

    va_start( args, fmt );
    while( (c = *fmt++) != '\0' ) {
        if( c == '%' ) {
            c = *fmt++;
            if( c == 's' ) {
                for( p = va_arg( args, char * ); (c = *p++) != '\0'; ) {
                    putchar(c);
                }
            } else if( c == 'd' ) {
                i = va_arg( args, int );
                sprintf( buf, "%d", i );
                for( p = buf; (c = *p++) != '\0'; ) {
                    putchar(c);
                }
            }
        } else {
            putchar(c);
        }
    }
    va_end( args );
}

void  Fputnl( const char *text, FILE *fp )
/****************************************/
{
    fputs( text, fp );
    fputs( "\n", fp );
}

void BuildLinkFile( FILE *fp )
/****************************/
{
    list    *itm;
    char    filename[_MAX_PATH];

    if( Flags.be_quiet ) {
        Fputnl( "option quiet", fp );
    }
    if( DebugFlag != DBG_NONE ) {
        fputs( DebugOptions[DebugFormat], fp );
        Fputnl( DebugOptions[DebugFlag], fp );
    }
    if( StackSize != NULL ) {
        fputs( "option stack=", fp );
        Fputnl( StackSize, fp );
    }

    BuildSystemLink( fp );

    /* pass given directives to linker */
    for( itm = Directive_List; itm != NULL; itm = itm->next ) {
        Fputnl( itm->item, fp );
    }

    fputs( "name ", fp );
    Fputnl( DoQuoted( filename, Exe_Name, '\'' ), fp );
    if( Flags.keep_exename ) {
        Fputnl( "option noextension", fp );
    }
    if( Flags.map_wanted ) {
        if( Map_Name == NULL ) {
            Fputnl( "option map", fp );
        } else {
            fputs( "option map=", fp );
            Fputnl( DoQuoted( filename, Map_Name, '\'' ), fp );
        }
    }
    for( itm = Libs_List; itm != NULL; itm = itm->next ) {
        fputs( "library ", fp );
        Fputnl( DoQuoted( filename, itm->item, '\'' ), fp );
    }
    if( Flags.link_ignorecase ) {
        Fputnl( "option nocaseexact", fp );
    }
}

void  MemInit( void )
/*******************/
{
#ifdef TRMEM
    TRMemHandle = _trmem_open( malloc, free, realloc, NULL, NULL, memPrintLine,
            _TRMEM_ALLOC_SIZE_0 | _TRMEM_REALLOC_SIZE_0 |
            _TRMEM_OUT_OF_MEMORY | _TRMEM_CLOSE_CHECK_FREE );
#endif
}

void  MemFini( void )
/*******************/
{
#ifdef TRMEM
    _trmem_prt_usage( TRMemHandle );
    _trmem_prt_list( TRMemHandle );
    _trmem_close( TRMemHandle );
#endif
}

void  *MemAlloc( size_t size )
/****************************/
{
    void        *ptr;

#ifdef TRMEM
    ptr = _trmem_alloc( size, _trmem_guess_who(), TRMemHandle );
#else
    ptr = malloc( size );
#endif
    if( ptr == NULL ) {
        PrintMsg( WclMsgs[OUT_OF_MEMORY] );
        exit( 1 );
    }
    return( ptr );
}

char *MemStrDup( const char *str )
/********************************/
{
    char        *ptr;

#ifdef TRMEM
    ptr = _trmem_strdup( str, _trmem_guess_who(), TRMemHandle );
#else
    ptr = strdup( str );
#endif
    if( ptr == NULL ) {
        PrintMsg( WclMsgs[OUT_OF_MEMORY] );
        exit( 1 );
    }
    return( ptr );
}

#if 0
char *MemStrLenDup( const char *str, size_t len )
/***********************************************/
{
    char        *ptr;

#ifdef TRMEM
    ptr = _trmem_alloc( len + 1, _trmem_guess_who(), TRMemHandle );
#else
    ptr = malloc( len + 1 );
#endif
    if( ptr == NULL ) {
        PrintMsg( WclMsgs[OUT_OF_MEMORY] );
        exit( 1 );
    }
    memcpy( ptr, str, len );
    ptr[len] = '\0';
    return( ptr );
}
#endif

void  *MemRealloc( void *p, size_t size )
/***************************************/
{
    void        *ptr;

#ifdef TRMEM
    ptr = _trmem_realloc( p, size, _trmem_guess_who(), TRMemHandle );
#else
    ptr = realloc( p, size );
#endif
    if( ptr == NULL ) {
        PrintMsg( WclMsgs[OUT_OF_MEMORY] );
        exit( 1 );
    }
    return( ptr );
}

void  MemFree( void *ptr )
/************************/
{
#ifdef TRMEM
        _trmem_free( ptr, _trmem_guess_who(), TRMemHandle );;
#else
        free( ptr );
#endif
}

void ListAppend( list **itm_list, list *new )
/*********************************************/
{
    list    *itm;

    if( *itm_list == NULL ) {
        *itm_list = new;
    } else {
        itm = *itm_list;
        while( itm->next != NULL ) {
            itm = itm->next;
        }
        itm->next = new;
    }
}

void ListFree( list *itm_list )
/*****************************/
{
    list    *next;

    while( itm_list != NULL ) {
        next = itm_list->next;
        MemFree( itm_list->item );
        MemFree( itm_list );
        itm_list = next;
    }
}

void  AddNameObj( const char *name )
/**********************************/
{
    list        *curr_name;
    list        *last_name;
    list        *new_name;
    char        path[_MAX_PATH];
    pgroup2     pg1;
    pgroup2     pg2;

    last_name = NULL;
    for( curr_name = Obj_List; curr_name != NULL; curr_name = curr_name->next ) {
        if( fname_cmp( name, curr_name->item ) == 0 )
            return;
        last_name = curr_name;
    }
    new_name = MemAlloc( sizeof( list ) );
    if( Obj_List == NULL ) {
        Obj_List = new_name;
    } else {
        last_name->next = new_name;
    }
    new_name->item = MemStrDup( name );
    new_name->next = NULL;
    if( Obj_Name != NULL ) {
        /* construct full name of object file from Obj_Name information */
        _splitpath2( Obj_Name, pg1.buffer, &pg1.drive, &pg1.dir, &pg1.fname, &pg1.ext );
        if( pg1.ext[0] == '\0' )
            pg1.ext = OBJ_EXT;
        if( pg1.fname[0] == '\0' || pg1.fname[0] == '*' ) {
            /* there's no usable basename in the -fo= pattern, but there drive and directory
             * and extension should still be applied.
             * OTOH the input name may have its own, explicitly given
             * drive, directory and extension, so let those take precedence */
            _splitpath2( name, pg2.buffer, &pg2.drive, &pg2.dir, &pg2.fname, &pg2.ext );
            pg1.fname = pg2.fname;
            if( pg2.ext[0] != '\0' )
                pg1.ext = pg2.ext;
            if( pg2.drive[0] != '\0' )
                pg1.drive = pg2.drive;
            if( pg2.dir[0] != '\0' ) {
                pg1.dir = pg2.dir;
            }
        }
        _makepath( path, pg1.drive, pg1.dir, pg1.fname, pg1.ext );
        name = path;
    }
    AddDirectivePath( "file ", name );
}


char  *MakePath( const char *path )
/*********************************/
{
    char        *p;
    size_t      len;

    p = strrchr( path, SYS_DIR_SEP_CHAR );
#ifndef __UNIX__
    if( p == NULL ) {
        p = strchr( path, ':' );
    }
#endif
    if( p == NULL ) {
        return( MemStrDup( "" ) );
    } else {
        len = p + 1 - path;
        p = MemAlloc( len + 1 );
        memcpy( p, path, len );
        p[len] = '\0';
        return( p );
    }
}

char  *GetName( const char *path, char *buffer )
/**********************************************/
{
    const char      *p;
#ifndef __UNIX__
    static DIR      *dirp;
    struct dirent   *dire;

    if( path != NULL ) {                /* if given a filespec to open,  */
        if( *path == '\0' ) {           /*   but filespec is empty, then */
            closedir( dirp );           /*   close directory and return  */
            return( NULL );             /*   (for clean up after error)  */
        }
        dirp = opendir( path );         /* try to find matching filenames */
        if( dirp == NULL ) {
            PrintMsg( WclMsgs[UNABLE_TO_OPEN], path );
            if( strpbrk( path, "?*" ) != NULL )
                return( NULL );
            if( (p = strrchr( path, '\\' )) == NULL )
                p = strrchr( path, ':' );
            if( p == NULL )
                p = path - 1;
            if( *(++p) == '\0' )
                return( NULL );
            return( strcpy( buffer, p ) );
        }
    }

    while( (dire = readdir( dirp )) != NULL ) {
        if( ( dire->d_attr & ATTR_MASK ) == 0 ) {    /* valid file? */
            return( dire->d_name );
        }
    }
    closedir( dirp );
    return( NULL );
#else
    if( path == NULL || *path == '\0' )
        return( NULL );
    if( strpbrk( path, "?*" ) != NULL ) {
//        PrintMsg( WclMsgs[UNABLE_TO_OPEN], path );
        return( NULL );
    }
    p = strrchr( path, '/' );
    if( p == NULL )
        p = path - 1;
    if( *(++p) == '\0' )
        return( NULL );
    return( strcpy( buffer, p ) );
#endif
}

void FindPath( const char *name, char *buf )
/******************************************/
{
    _searchenv( name, "PATH", buf );
    if( buf[0] == '\0' ) {
        PrintMsg( WclMsgs[UNABLE_TO_FIND], name );
        exit( 1 );
    }
}

char *DoQuoted( char *buffer, const char *name, char quote_char )
/***************************************************************/
{
    char *p;
    int  quotes;

    p = buffer;
    if( name != NULL ) {
        quotes = ( strchr( name, ' ' ) != NULL );
        if( quotes )
            *p++ = quote_char;
        while( (*p = *name) != '\0' ) {
            ++p;
            ++name;
        }
        if( quotes ) {
            *p++ = quote_char;
        }
    }
    *p = '\0';
    return( buffer );
}

void AddDirective( const char *directive )
/****************************************/
{
    list    *itm;

    itm = MemAlloc( sizeof( list ) );
    itm->next = NULL;
    itm->item = MemAlloc( strlen( directive ) + 1 );
    strcpy( itm->item, directive );
    ListAppend( &Directive_List, itm );
}

void AddDirectivePath( const char *directive, const char *path )
/**************************************************************/
{
    list        *new_item;
    size_t      len;
    char        *p;

    len = strlen( directive );
    new_item = MemAlloc( sizeof( list ) );
    new_item->next = NULL;
    p = new_item->item = MemAlloc( len + strlen( path ) + 2 + 1 );
    memcpy( p, directive, len );
    p += len;
    DoQuoted( p, path, '\'' );
#ifndef __UNIX__
    while( (p = strchr( p, '/' )) != NULL ) {
        *p++ = '\\';
    }
#endif
    ListAppend( &Directive_List, new_item );
}

char *RemoveExt( char *fname )
/****************************/
{
    char    *dot;

    if( (dot = strrchr( fname, '.' )) != NULL )
        *dot = '\0';
    return( fname );
}

int HasFileExtension( const char *p, const char *ext )
/****************************************************/
{
    const char  *dot;

    if( (dot = strrchr( p, '.' )) != NULL ) {
        if( fname_cmp( dot + 1, ext ) == 0 ) {
            return( 1 );                /* indicate file extension matches */
        }
    }
    return( 0 );                        /* indicate no match */
}


void  MakeName( char *name, const char *ext )
/*******************************************/
{
    /* If the last '.' is before the last path seperator character */
    if( strrchr( name, '.' ) <= strpbrk( name, PATH_SEPS_STR ) ) {
        strcat( name, ext );
    }
}
