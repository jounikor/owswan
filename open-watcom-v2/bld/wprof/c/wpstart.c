/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Execution profiler command line processing.
*
****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef __WATCOMC__
    #include <process.h>
#endif
#include "wio.h"
#include "watcom.h"
#include "common.h"
#include "msg.h"
#include "dip.h"
#include "aui.h"
#include "sampinfo.h"
#include "memutil.h"
#include "clibext.h"
#include "getsamps.h"
#include "madinter.h"
#include "utils.h"
#include "wphelp.h"
#include "dipinter.h"
#include "clrsamps.h"
#include "rptsamps.h"
#include "wpstart.h"
#include "wpdata.h"


STATIC bool     procCmd( char * );
STATIC int      minLook( char * * );
STATIC char *   eatBlanks( char * );
STATIC char *   eatAlphaNum( char * );
STATIC char *   eatAllChars( char * );

enum {
    FAIL_OPT,
    DIP_OPT,
#if defined( __DOS__ )
    NOCHARREMAP_OPT,
    NOGRAPHICSMOUSE_OPT,
#endif
    HELP_OPT,
    R_OPT
};

STATIC char * cmdNames[] = {
    "dip",
#if defined( __DOS__ )
    "nocharremap",
    "nographicsmouse",
#endif
    "?",
    "help",
#ifndef NDEBUG
    "r",
#endif
    NULL
};

STATIC unsigned_8 cmdLen[] = {
    3,
#if defined( __DOS__ )
    4,
    3,
#endif
    1,
    1,
#ifndef NDEBUG
    1
#endif
};

STATIC int cmdType[] = {
    DIP_OPT,
#if defined( __DOS__ )
    NOGRAPHICSMOUSE_OPT,
    NOCHARREMAP_OPT,
#endif
    HELP_OPT,
    HELP_OPT,
#ifndef NDEBUG
    R_OPT
#endif
};

STATIC char * cmdUsage[] = {
    LIT( Usage1 ),
    LIT( Usage2 ),
    LIT( Usage3 ),
    LIT( Usage4 ),
    LIT( Usage5 ),
#if defined( __DOS__ )
    LIT( Usage6 ),
    LIT( Usage7 ),
    LIT( Usage8 ),
#endif
    NULL
};


bool            WPWndInitDone = false;
char            SamplePath[ _MAX_PATH ];
char            *WProfDips = NULL;

static size_t   WProfDipSize = 0;


void WPInit( void )
/*****************/
{
    char        *rover;
    bool        do_report;
    char        buff[256];

    WPMemOpen();
    SamplePath[0] = 0;
    InitPaths();
    rover = getenv( "WPROF" );
    if( rover != NULL ) {
        procCmd( rover );
    }
    getcmd( buff );
    do_report = procCmd( buff );
    WndInit( "Open Watcom Profiler" );
    WPWndInitDone = true;
    InitMADInfo();
    WPDipInit();
    if( do_report ) {
        if( GetSampleInfo() ) {
            ReportSampleInfo();
        }
        exit( 0 );
    }
}



void WPFini( void )
/*****************/
{
    ClearAllSamples();
    WPFiniHelp();
    WndFini();
    WPDipFini();
    FiniMADInfo();
    WPMemClose();
}



STATIC bool procCmd( char * cmd )
/*******************************/
{
    char    *rover;
    size_t  name_len;
    size_t  old_len;
    int     cmd_type;
    int     index;
    bool    do_report;
    bool    do_option;

    do_report = false;
    for( ;; ) {
        cmd = eatBlanks( cmd );
        if( *cmd == NULLCHAR )
            break;
#ifdef __UNIX__
        if( *cmd == '-' ) {
#else
        if( *cmd == '-' || *cmd == '/' ) {
#endif
            do_option = true;
            ++cmd;
            cmd_type = minLook( &cmd );
        } else if( *cmd == '?' ) {
            do_option = true;
            cmd_type = HELP_OPT;
        } else {
            cmd_type = 0;
            do_option = false;
            rover = cmd;
            cmd = eatAllChars( cmd );
            name_len = cmd - rover;
            if( name_len > _MAX_PATH - 1 ) {
                name_len = _MAX_PATH - 1;
            }
            memcpy( SamplePath, rover, name_len );
            SamplePath[name_len] = NULLCHAR;
        }
        if( do_option ) {
            switch( cmd_type ) {
            case FAIL_OPT:
                ErrorMsg( LIT( Cmd_Option_Not_Valid ), cmd-1 );
#if defined( __WINDOWS__ ) || defined( __NT__ ) || defined( __OS2_PM__ )
                fatal( LIT( Usage ) );
#else
                /* fall through */
#endif
            case HELP_OPT:
                for( index = 0; cmdUsage[index] != NULL; ++index ) {
                    ErrorMsg( cmdUsage[index] );
                }
                exit( 0 );
            case DIP_OPT:
                cmd = eatBlanks( cmd );
                if( *cmd == '=' ) {
                    cmd = eatBlanks( cmd+1 );
                }
                rover = cmd;
                cmd = eatAlphaNum( cmd );
                if( *cmd == NULLCHAR || cmd-rover == 0 ) {
                    if( WProfDips != NULL ) {
                        ProfFree( WProfDips );
                        WProfDips = NULL;
                        WProfDipSize = 0;
                    }
                } else {
                    name_len = cmd - rover;
                    old_len = WProfDipSize;
                    WProfDipSize = old_len + name_len + 1;
                    if( old_len == 0 ) {
                        WProfDipSize++;
                    } else {
                        old_len--;
                    }
                    WProfDips = ProfRealloc( WProfDips, WProfDipSize );
                    memcpy( WProfDips + old_len, rover, name_len );
                    old_len += name_len;
                    WProfDips[old_len++] = NULLCHAR;
                    WProfDips[old_len] = NULLCHAR;
                }
                break;
#if defined( __DOS__ )
            case NOGRAPHICSMOUSE_OPT:
            case NOCHARREMAP_OPT:
                WndStyle &= ~(GUI_CHARMAP_DLG|GUI_CHARMAP_MOUSE);
                break;
#endif
#ifndef NDEBUG
            case R_OPT:
                do_report = true;
                break;
#endif
            }
        }
    }
    return( do_report );
}



STATIC char * eatBlanks( char * cmd )
/***********************************/
{
    while( isspace( *cmd ) && *cmd != NULLCHAR ) {
        ++cmd;
    }
    return( cmd );
}



STATIC char * eatAlphaNum( char * cmd )
/*************************************/
{
    while( isalnum( *cmd ) && *cmd != NULLCHAR ) {
        ++cmd;
    }
    return( cmd );
}



STATIC char * eatAllChars( char * cmd )
/*************************************/
{
    while( !isspace( *cmd ) && *cmd != NULLCHAR ) {
        ++cmd;
    }
    return( cmd );
}



STATIC int minLook( char * * value )
/**********************************/
{
    int         index;
    int         curr_len;
    char * *    strtab;
    byte *      lentab;
    char *      strlook;
    char *      strchck;
    char *      base_val;
    char        check_char;

    base_val = *value;
    lentab = cmdLen;
    strtab = cmdNames;
    index = 0;
    for(;;) {
        strlook = *strtab++;
        if( strlook == NULL ) {
            return( FAIL_OPT );
        }
        strchck = base_val;
        curr_len = 0;
        for(;;) {
            check_char = tolower( *strchck );
            if( check_char == NULLCHAR
             || !(isalpha( check_char ) || check_char == '?') ) {
                if( curr_len >= *lentab ) {
                    *value += curr_len;
                    return( cmdType[index] );
                }
                break;
            }
            if( *strlook != check_char )
                break;
            strlook++;
            strchck++;
            curr_len++;
        }
        lentab++;
        index++;
    }
}
