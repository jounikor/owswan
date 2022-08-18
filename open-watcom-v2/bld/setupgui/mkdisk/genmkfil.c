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
* Description:  Generate makefile used for setup.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( __WATCOMC__ ) || !defined( __UNIX__ )
#include <process.h>
#endif
#include "bool.h"

#include "clibext.h"


typedef struct file_info {
    char                *file;
    char                *pack;
    char                *path;
    char                *rel_file;
    struct file_info    *next;
} FILE_INFO;

typedef struct version_info {
    char                *version;
    struct version_info *next;
} VERSION_INFO;


FILE_INFO               *FileList = NULL;
VERSION_INFO            *VersionList = NULL;


bool AddFile( const char *path, const char *file, const char *rel_file, const char *patch )
//=========================================================================================
{
    FILE_INFO   *new;
    FILE_INFO   **owner;

    new = malloc( sizeof( FILE_INFO ) );
    if( new != NULL ) {
        new->path = strdup( path );
        if( new->path != NULL ) {
            new->file = strdup( file );
            if( new->file != NULL ) {
                new->rel_file = strdup( rel_file );
                if( new->rel_file != NULL ) {
                    new->pack = strdup( patch );
                    if( new->pack != NULL ) {
                        new->next = NULL;
                        owner = &FileList;
                        while( *owner != NULL ) {
                            owner = &(*owner)->next;
                        }
                        *owner = new;
                        return( true );
                    }
                    free( new->rel_file );
                }
                free( new->file );
            }
            free( new->path );
        }
        free( new );
    }
    printf( "Out of memory\n" );
    return( false );
}


bool InVersionList( const char *str )
//===================================
{
    VERSION_INFO        *ver;

    if( VersionList == NULL ) {
        return( true );
    } else {
        for( ver = VersionList; ver != NULL; ver = ver->next ) {
            if( stricmp( str, ver->version ) == 0 ) {
                return( true );
            }
        }
        return( false );
    }
}


bool ReadList( FILE *fp )
//=======================
{
//    char                *type;
    char                *path;
    char                *file;
    char                *rel_file;
//    char                *extra;
    char                *patch;
    char                *where;
    char                buf[128];

    while( fgets( buf, sizeof( buf ), fp ) != NULL ) {
        buf[strlen( buf ) - 1] = '\0';
        if( buf[0] == '\0' )
            continue;
        if( buf[0] == '#' ) {
            if( buf[2] == '@' ) {     // database
                buf[1] = '-';         // (so strtok works)
            } else {
                continue;
            }
        }
//        type = strtok( buf, " \t" );
        strtok( buf, " \t" );
        path = strtok( NULL, " \t" );
        file = strtok( NULL, " \t" );
        rel_file = strtok( NULL, " \t" );
//        extra = strtok( NULL, " \t" );
        strtok( NULL, " \t" );
        patch = strtok( NULL, " \t" );
        for( ;; ) {
            where = strtok( NULL, " \t" );
            if( where == NULL )
                break;
            if( InVersionList( where ) ) {
                if( !AddFile( path, file, rel_file, patch ) ) {
                    return( false );
                }
                break;
            }
        }
    }
    return( true );
}


void CreateMakeFile( void )
//=========================
{
    FILE                *fp;
    FILE_INFO           *curr;

    fp = fopen( "mkfile", "w" );
    if( fp == NULL ) {
        printf( "Cannot create file 'mkfile'\n" );
        fp = stdout;
    }

    fprintf( fp, "all : .symbolic\nall : &\n" );
    for( curr = FileList; ; ) {
        fprintf( fp, "    $(%%packdir)\\%s", curr->pack );
        curr = curr->next;
        if( curr == NULL )
            break;
        fprintf( fp, " &\n" );
    }
    fprintf( fp, "\n\n" );

    for( curr = FileList; curr != NULL; curr = curr->next ) {
        fprintf( fp, "$(%%packdir)\\%s : ", curr->pack );
        if( strchr( curr->path, ':' ) == NULL ) {
            fprintf( fp, "$(%%relroot)\\" );
        }
        if( stricmp( curr->rel_file, "." ) != 0 ) {
            fprintf( fp, "%s\n", curr->rel_file );
        } else {
            if( stricmp( curr->path, "." ) != 0 ) {
                fprintf( fp, "%s\\", curr->path );
            }
            fprintf( fp, "%s\n", curr->file );
        }
//      fprintf( fp, "    @del $^@ >nul\n    wpack -a -q $^@ $[@\n" );
        fprintf( fp, "    wpack -a -q $^@ $[@\n" );
    }
    fclose( fp );
}


void AddVersion( const char *str )
//================================
{
    VERSION_INFO        *ver;

    ver = malloc( sizeof( VERSION_INFO ) );
    if( ver == NULL ) {
        printf( "Out of memory\n" );
    } else {
        ver->version = strdup( str );
        ver->next = VersionList;
        VersionList = ver;
    }
}

void FreeVersion( void )
//======================
{
    VERSION_INFO        *ver;

    while( (ver = VersionList) != NULL ) {
        VersionList = ver->next;
        free( ver->version );
        free( ver );
    }
}


int main( int argc, char *argv[] )
//================================
{
    int                 i;
    bool                ok;
    FILE                *fp;

    if( argc < 2 ) {
        printf( "Usage: GEMMKFIL <file_list> [versions]\n" );
        return( 1 );
    }
    fp = fopen( argv[1], "r" );
    if( fp == NULL ) {
        printf( "Cannot open '%s'\n", argv[1] );
        return( 1 );
    }
    for( i = 2; argv[i] != NULL; ++i ) {
        AddVersion( argv[i] );
    }
    ok = ReadList( fp );
    fclose( fp );
    if( ok ) {
        CreateMakeFile();
    }
    FreeVersion();
    return( 0 );
}
