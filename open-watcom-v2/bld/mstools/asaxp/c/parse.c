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
* Description:  Command line parsing for asaxp clone tool.
*
****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "bool.h"
#include "asaxp.h"
#include "cmdline.h"
#include "cmdscan.h"
#include "context.h"
#include "error.h"
#include "memory.h"
#include "message.h"
#include "parse.h"
#include "cmdlnprs.h"


#include "parseext.c"

/*
 * Initialize the OPT_STORAGE structure.
 */
void InitParse( OPT_STORAGE *cmdOpts )
/************************************/
{
    OPT_INIT( cmdOpts );
}


/*
 * Destroy the OPT_STORAGE structure.
 */
void FiniParse( OPT_STORAGE *cmdOpts )
/************************************/
{
    OPT_FINI( cmdOpts );
}


/*
 * Appends '.' to a filename without extension ie. 'my_file' becomes 'my_file.'
 */
static char *VerifyDot( char *filename )
/**************************************/
{
    char *              newfilename;
    char *              tempfilename;
    bool                quotes_found = false;

    if (strchr(filename,'.')==NULL) {
        /*** Strip quotes from filename ***/
        newfilename = ReallocMem( filename, strlen(filename)+2 );
        if( *newfilename == '"' ) {
            tempfilename = newfilename + 1;                     /* skip leading " */
            tempfilename[ strlen(tempfilename)-1 ] = '\0';      /* smite trailing " */
            quotes_found = true;
        } else {
            tempfilename = newfilename;
        }
        /*** Append '.' at the end of filename and add quotes if needed ***/
        if (quotes_found) {
            filename = DupQuoteStrMem(strcat(tempfilename,"."),'"');
        } else {
            filename = DupStrMem(strcat(tempfilename,"."));
        }
        FreeMem( newfilename );
    }
    return filename;
}


/*
 * Gripe about a command line error.
 */
static void cmd_line_error( void )
/********************************/
{
    char *              str;

    GoToMarkContext();
    str = CmdScanString();
    Warning( "Ignoring invalid option '%s'", str );
}


/*
 * Parse the command string contained in the current context.
 */
void CmdStringParse( OPT_STORAGE *cmdOpts, int *itemsParsed )
/***********************************************************/
{
    char                ch;
    char *              filename;


    for( ;; ) {
        /*** Find the start of the next item ***/
        CmdScanWhitespace();
        ch = GetCharContext();
        if( ch == '\0' )  break;
        MarkPosContext();               /* mark start of switch */

        /*** Handle switches, command files, and input files ***/
        if( ch == '-'  ||  ch == '/' ) {        /* switch */
            if( OPT_PROCESS( cmdOpts ) ) {
                cmd_line_error();
            }
        } else {                                /* filename */
            UngetCharContext();
            filename = CmdScanFileName();
            filename = VerifyDot(filename);
            add_string( &(cmdOpts->t010101010101_value), filename, '\0' );
            cmdOpts->t010101010101 = true;
        }
        (*itemsParsed)++;
    }
    CloseContext();
}


/*
 * Suppress warning messages.
 */
static void handle_nowwarn( OPT_STORAGE *cmdOpts, int x )
/*******************************************************/
{
    /* unused parammeters */ (void)x; (void)cmdOpts;

    DisableWarnings( true );
}


/*
 * Takes care of the t010101010101 option.
 */
static bool parse_t010101010101( OPT_STRING **p )
/***********************************************/
{
    /* unused parammeters */ (void)p;

    return( true );
}


/*
 * For the /optName option, read in :string and store the string into the
 * given OPT_STRING.  If onlyOne is non-zero, any previous string in p will
 * be deleted.  If quote is non-zero, make sure the string is quoted.
 * Use quote if there aren't any quotes already.
 */
static bool do_string_parse( OPT_STRING **p, char *optName, bool onlyOne,
                            char quote )
/**********************************************************************/
{
    char *              str;

    str = CmdScanString();
    if( str == NULL ) {
        FatalError( "/%s option requires an argument", optName );
        return( false );
    }
    if( onlyOne )
        OPT_CLEAN_STRING( p );
    add_string( p, str, quote );
    return( true );
}


/*
 * Parse the /D option.
 */
static bool parse_D( OPT_STRING **p )
/***********************************/
{
    return( do_string_parse( p, "D", false, '\0' ) );
}


static void handle_Fo( OPT_STORAGE *cmdOpts, int x )
/**************************************************/
{
    char *              filename;

    /* unused parammeters */ (void)x;

    OPT_CLEAN_STRING( &cmdOpts->fo_value );
    CmdScanWhitespace();
    filename = CmdScanFileName();
    if( filename != NULL ) {
        filename = VerifyDot(filename);
        add_string( &cmdOpts->fo_value, filename, '\0' );
    } else {
        cmdOpts->fo=0;
        cmdOpts->o=0;
    }
}


/*
 * Parse the /Fo and /o option. Does nothing because handle_Fo does the job.
 */
static bool parse_Fo( OPT_STRING **p )
/************************************/
{
    /* unused parammeters */ (void)p;

    return( true );
}


/*
 * Parse the /I option.
 */
static bool parse_I( OPT_STRING **p )
/***********************************/
{
    char *              filename;

    CmdScanWhitespace();
    filename = CmdScanFileName();
    if( filename != NULL ) {
        add_string( p, filename, '\0' );
    } else {
        OPT_CLEAN_STRING( p );
    }
    return( true );

}


/*
 * Parse the /U option.
 */
static bool parse_U( OPT_STRING **p )
/***********************************/
{
    return( do_string_parse( p, "U", false, '\0' ) );
}


/*
 * Parse the /passwopts option.
 */
static bool parse_passwopts( OPT_STRING **p )
/*******************************************/
{
    char *str;
    char *src;
    char *dst;

    if( !CmdScanRecogChar( ':' ) )
    {
        FatalError("/passwopts:{argument} requires an argument");
        return( false );
    }

    str = CmdScanString();
    if (str == NULL)
    {
        FatalError("/passwopts requires an argument");
        return( false );
    }

    /*
     * If quoted, stip out the quote characters.
     */
    if (*str == '\"')
    {
        for (dst = str, src = str + 1; *src && (*src != '\"'); )
        {
            *dst++ = *src++;
        }

        if (*src != '\"')
        {
            FatalError("/passwopts argument is missing closing quote");
            return( false );
        }

        *dst = 0x00;
    }

    add_string(p, str, '\0');
    return( true );
} /* parse_passwopts() */


/*
 * Return the next character (forced to lowercase since LINK's options are
 * not case-sensitive) and advance to the next one.
 */
int OPT_GET_LOWER( void )
/***********************/
{
    return( tolower( (unsigned char)GetCharContext() ) );
}


/*
 * If the next character is ch (in either uppercase or lowercase form), it
 * is consumed and a non-zero value is returned; otherwise, it is not
 * consumed and zero is returned.
 */
bool OPT_RECOG_LOWER( int ch )
/****************************/
{
    return( CmdScanRecogLowerChar( ch ) );
}


/*
 * Back up one character.
 */
void OPT_UNGET( void )
/********************/
{
    UngetCharContext();
}


/*
 * Scan a number. Leading whitespace is permitted.
 */
bool OPT_GET_NUMBER( unsigned *p )
/********************************/
{
    unsigned            value;

    CmdScanWhitespace();
    if( CmdScanNumber( &value ) ) {
        *p = value;
        return( true );
    } else {
        return( false );
    }
}


/* Include after all static functions were declared */
#include "cmdlnprs.gc"
