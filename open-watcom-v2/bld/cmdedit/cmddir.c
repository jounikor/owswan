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
* Description:  DIR command processing.
*
****************************************************************************/


#include <string.h>
#include "cmdedit.h"


#ifdef __DOS__
#define SIZE    80
#else
#define SIZE    260
#endif
#if 0
static char Prev[SIZE+1] = { 'X' };
#endif

int ExpandDirCommand( void )
/**************************/
{
    int         i;
    int         ups;
    char        ch;
    char        *line;
    int         in_quote = FALSE;

    if( MaxCursor == 0 )
        return( 0 );
    Line[MaxCursor] = '\0';
    line = EatWhite( Line );
    if( HideDirCmds ) {
        if( line[0] != '#' )
            return( 0 );
        line++;
    }
    if( line[1] != ' ' && line[1] != '\0' )
        return( 0 );
    switch( line[0] ) {
    case 'o':
        i = 1;
        while( line[i] == ' ' )
            ++i;
        --i;
        i += ReplaceAlias( "d ..\\", line, line + i + 1 );
        break;
    case 'u':
        i = 1;
        while( line[i] == ' ' )
            ++i;
        ups = 1;
        if( line[i] != '\0' ) {
            ups = line[i] - '0';
        }
        MaxCursor = Cursor = ( line - Line ) + 1;
        ReplaceAlias( "d ..", line + 0, line + 1 );
        while( --ups > 0 ) {
            ReplaceAlias( "\\..", line + 4, line + 4 );
        }
        i = MaxCursor - ( line - Line );
        break;
    case 'r':
    case 'd':
        i = 1;
        break;
    default:
        return( 0 );
    }
    Line[MaxCursor] = '\0';
    while( line[i] == ' ' )
        ++i;
    for( ; i != MaxCursor; i++ ) {
        ch = line[i];
        if( ch == '"' )
            in_quote = !in_quote;
        if( !in_quote ) {
            if( ch == ' ' || ch == '/' ) {
                line[i] = '\\';
            } else if( ch == '-' ) {
                i += ReplaceAlias( "..\\", line + i, line + i + 1 );
            }
        }
    }
    return( 1 );
}


static void    DirCmds( char *p )
/*******************************/
{
#if 0
    char        __far *q;
#endif
    char        cmd;
    char        drv;
    USHORT      drive_num;
    int         size;
#if 0
    char        buff[SIZE+1];
#endif
#ifdef __OS2__
    unsigned long    drive_map;
#endif

    cmd = *p++;
    while( *p == ' ' )
        ++p;
    DosQCurDisk( &drive_num, &drive_map );
    drv = drive_num - 1 + 'A';
#if 0
    if( cmd != 'd' || *p != '\0' ) {
        buff[0] = drv;
        buff[1] = ':';
        buff[2] = '\\';
        size = SIZE;
        DosQCurDir( drive_num, buff + 3, &size );
    }
#endif
    if( p[0] != '\0' && p[1] == ':' ) {
        drv = p[0] & ~0x20;
        p += 2;
    }
    if( p[0] == '"' && p[1] != '\0' && p[2] == ':' ) {
        drv = p[1] & ~0x20;
        p += 3;
    }
    DosSelectDisk( drv - 'A' + 1 );
    DosQCurDisk( &drive_num, &drive_map );
    if( ( drive_num - 1 + 'A' ) != drv ) {
        PutString( "Invalid drive specified" );
        return;
    }
    switch( cmd ) {
    case 'd':
        if( p[0] == '\0' )
            break;
        size = strlen( p );
        /* trim trailing slashes (possibly inserted to replace whitespace) */
        while( size > 1 && ( p[size - 1] == '\\' || p[size - 1] == '/' ) ) {
            --size;
        }
        p[size] = 0;
        if( DosChDir( p, 0 ) == 0 )
            break;
        PutString( "\r\nInvalid directory\r\n" );
        break;
#if 0
    case 'r':
        DosSelectDisk( Prev[0] - 'A' + 1 );
        DosQCurDisk( &drive_num, &drive_map );
        drv = drive_num - 1 + 'A';
        if( drv != Prev[0] ) {
            PutString( "\r\nInvalid drive!" );
            break;
        }
        if( DosChDir( Prev + 2, 0 ) == 0 ) break;
        PutString( "\r\nInvalid directory\r\n" );
        break;
#endif
    default:
        PutString( "\r\nInvalid directory command!" );
        break;
    }
#if 0
    if( cmd != 'd' || p[0] != '\0' ) {
        q = buff;
        p = Prev;
        while( *p++ = *q++ ) ;
    }
    size = SIZE;
    Line[0] = drv;
    Line[1] = ':';
    Line[2] = '\n';
    Line[3] = 'c';
    Line[4] = 'd';
    Line[5] = ' ';
    Line[6] = '\\';
    DosQCurDir( drive_num, Line + 7, &size );
#endif
}


void DoDirCommand( void )
/***********************/
{
    Line[MaxCursor] = '\0';
    DirCmds( EatWhite( Line ) + HideDirCmds );
//    Line[0] = 'c';
//    Line[1] = 'd';
//    Line[2] = ' ';
//    Line[3] = '.';
//    Line[4] = '\0';
    MaxCursor = Cursor = 0;
}
