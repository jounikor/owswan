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
* Description:  Command line parsing for LINK clone tool.
*
****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "bool.h"
#include "link.h"
#include "cmdline.h"
#include "cmdscan.h"
#include "context.h"
#include "error.h"
#include "file.h"
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
        } else if( ch == '@' ) {                /* command file */
            filename = CmdScanFileNameWithoutQuotes();
            PushContext();
            if( OpenFileContext( filename ) ) {
                FatalError( "Cannot open '%s'.", filename );
            }
            FreeMem( filename );
            CmdStringParse( cmdOpts, itemsParsed );
            PopContext();
        } else {                                /* input file */
            UngetCharContext();
            filename = CmdScanFileName();
            AddFile( TYPE_DEFAULT_FILE, filename );
            FreeMem( filename );
        }
        (*itemsParsed)++;
    }
    CloseContext();
}


/*
 * Ensure argument to /ALIGN was valid.
 */
static void check_align( unsigned *p )
/************************************/
{
    int                 numFound = 0;
    unsigned            temp = *p;

    /*** Count the number of bits that are on ***/
    while( temp != 0 ) {
        if( temp & 0x0001 )  numFound++;
        temp >>= 1;
    }

    /*** Gripe if the number isn't a power of two ***/
    if( numFound != 1 ) {
        Warning( "Invalid argument %u to /ALIGN -- assuming 4096", *p );
        *p = 4096;
    }
}


#define skip_white(s)      while( *s && isspace( *s ) ) ++s;
#define skip_nonwhite(s)   while( *s && !isspace( *s ) ) ++s;

/*
 * Parse the /BASE:@{filename} option.
 */
static bool parse_base_from_file(OPT_STRING **p, char *str)
{
    #define MAX_LEN     255

    FILE    *stream;
    char    *buffer;
    char    *keyword;
    char    *s;
    char    *b;
    size_t  keylen;

    buffer = AllocMem( MAX_LEN );

    for( s = str + 1, b = buffer; *s && (*s != ',') && !isspace(*s); )
        *b++ = *s++;

    if( *s++ != ',' )
    {
        FreeMem(buffer);
        FatalError("/BASE:@{filename} requires valid filename");
        return( false );
    }

    *b      = 0x00;
    keyword = s;

    skip_nonwhite(s);

    keylen = s - keyword;

    stream = fopen(buffer, "r");
    if( stream == NULL ) {
        FreeMem(buffer);
        FatalError("/BASE:@{filename} requires a valid filename");
        return( false );
    }

    while( fgets(buffer, MAX_LEN, stream) != NULL ) {
        s = buffer;
        skip_white(s);

        if( *s == ';' || *s == '\0' )
            continue;

        if( strnicmp(s, keyword, keylen) == 0 && isspace( s[keylen] ) ) {
            s += keylen + 1;
            skip_white(s);

            if( *s == '\0' ) {
                FreeMem( buffer );
                fclose( stream );
                FatalError("/BASE file contains invalid offset");
                return( false );
            }

            b = s;
            skip_nonwhite(b);
            *b = 0x00;

            OPT_CLEAN_STRING( p );
            add_string( p, s, '\0' );
            FreeMem(buffer);
            fclose(stream);

            return( true );
        }
    }

    FreeMem(buffer);
    fclose(stream);
    FatalError("/BASE file does not contain keyword");
    return( false );
} /* parse_base_from_file() */


/*
 * Parse the /BASE option.
 */
static bool parse_base( OPT_STRING **p )
/**************************************/
{
    char *str;

    if( !CmdScanRecogChar( ':' ) )
    {
        FatalError("/BASE requires an argument");
        return( false );
    }

    str = CmdScanString();
    if( str == NULL )
    {
        FatalError("/BASE requires an argument");
        return( false );
    }

    if( *str == '@' )
    {
        return parse_base_from_file( p, str );
    }
    else
    {
        OPT_CLEAN_STRING( p );
        add_string( p, str, '\0' );
    }

    return( true );
} /* parse_base() */


/*
 * For the /optName option, read in :string and store the string into the
 * given OPT_STRING.  If onlyOne is non-zero, any previous string in p will
 * be deleted.  If quote is non-zero, make sure the string is quoted.
 * Use quote if there aren't any quotes already.
 */
static bool do_string_parse( OPT_STRING **p, char *optName, bool onlyOne,
                            char quote )
/***********************************************************************/
{
    char *              str;

    if( !CmdScanRecogChar( ':' ) ) {
        FatalError( "/%s requires an argument", optName );
        return( false );
    }
    str = CmdScanString();
    if( str == NULL ) {
        FatalError( "/%s requires an argument", optName );
        return( false );
    }
    if( onlyOne )  OPT_CLEAN_STRING( p );
    add_string( p, str, quote );
    return( true );
}


/*
 * Parse the /COMMENT option.
 */
static bool parse_comment( OPT_STRING **p )
/*****************************************/
{
    return( do_string_parse( p, "COMMENT", false, '\0' ) );
}


/*
 * Parse the /DEBUG option.
 */
static bool parse_debug( OPT_STRING **p )
/***************************************/
{
    char *              str;
    char                ch;

    ch = GetCharContext();
    if( ch != ':' ) {                   /* optional :<type> */
        if( ch != '\0' )  UngetCharContext();
    } else {
        str = CmdScanString();
        if( str == NULL ) {
            FatalError( "Missing argument for /DEBUG" );
            return( false );
        } else {
            add_string( p, str, '\0' );
            FreeMem( str );
        }
    }
    return( true );
}


/*
 * Parse the /DEBUGTYPE option.
 */
static bool parse_debugtype( OPT_STRING **p )
/*******************************************/
{
    return( do_string_parse( p, "DEBUGTYPE", false, '\0' ) );
}


/*
 * Parse the /DEF option.
 */
static bool parse_def( OPT_STRING **p )
/*************************************/
{
    return( do_string_parse( p, "DEF", true, '\0' ) );
}


/*
 * Parse the /DEFAULTLIB option.
 */
static bool parse_defaultlib( OPT_STRING **p )
/********************************************/
{
    return( do_string_parse( p, "DEFAULTLIB", false, '\0' ) );
}


/*
 * Parse the /ENTRY option.
 */
static bool parse_entry( OPT_STRING **p )
/***************************************/
{
    return( do_string_parse( p, "ENTRY", true, '\'' ) );
}


/*
 * Parse the /EXETYPE option.
 */
static bool parse_exetype( OPT_STRING **p )
/*****************************************/
{
    return( do_string_parse( p, "EXETYPE", true, '\0' ) );
}


/*
 * Parse the /EXPORT option, which is of the form
 *      /EXPORT:entryname[=internalname][,@ordinal[,NONAME]][,DATA]
 */
static bool parse_export( OPT_STRING **optStr )
/*********************************************/
{
    char *              strStart;
    char *              str;
    char *              p;
    char *              entryName = NULL;
    char *              internalName = NULL;
    char *              ordinal = NULL;
    size_t              len;
    int                 retcode = 1;

    /*** Extract the export string ***/
    if( !CmdScanRecogChar( ':' ) ) {
        FatalError( "/EXPORT requires an argument" );
        return( false );
    }
    str = CmdScanString();
    strStart = str;
    if( str == NULL ) {
        FatalError( "/EXPORT requires an argument" );
        return( false );
    }

    /*** Extract the entryName ***/
    p = str;
    while( *p != '\0'  &&  *p != '='  &&  *p != ',' )  p++;
    if( p == str ) {
        retcode = 0;
    } else {
        len = p - str + 1;
        if( str[0] != '\'' || str[len-2] != '\'' ) {
            len += 2;
            entryName = AllocMem( len );
            entryName[0] = '\'';
            memcpy( &(entryName[1]), str, len-3 );
            entryName[len-2] = '\'';
        } else {
            entryName = AllocMem( len );
            memcpy( entryName, str, len-1 );
        }
        entryName[len-1] = '\0';
        str = p;
    }

    /*** Extract the internalName ***/
    if( *str == '='  &&  retcode != 0 ) {
        str++;
        p = str;
        while( *p != '\0'  &&  *p != ',' )  p++;
        if( p == str ) {
            retcode = 0;
        } else {
            len = p - str + 1;
            if( str[0] != '\'' || str[len-2] != '\'' ) {
                len += 2;
                internalName = AllocMem( len );
                internalName[0] = '\'';
                memcpy( &(internalName[1]), str, len-3 );
                internalName[len-2] = '\'';
            } else {
                internalName = AllocMem( len );
                memcpy( internalName, str, len-1 );
            }
            internalName[len-1] = '\0';
            str = p;
        }
    }

    /*** Extract the ordinal ***/
    if( *str == ','  &&  retcode != 0 ) {
        if( *(++str) != '@' ) {
            retcode = 0;
        } else {
            str++;
            p = str;
            while( *p != '\0'  &&  isdigit( *p ) )  p++;
            if( p == str ) {
                retcode = 0;
            } else {
                len = p - str + 1;
                ordinal = AllocMem( len );
                memcpy( ordinal, str, len-1 );
                ordinal[len-1] = '\0';
            }
        }
    }

    /*** Abort on error ***/
    if( retcode == 0 ) {
        if( entryName != NULL )
            FreeMem( entryName );
        if( internalName != NULL )
            FreeMem( internalName );
        if( ordinal != NULL )
            FreeMem( ordinal );
        FreeMem( strStart );
        return( false );
    }

    /*** Merge together Watcom-style:  entryName[.ordinal][=internalName] ***/
    FreeMem( strStart );
    len = strlen( entryName );
    if( ordinal != NULL ) {
        len++;                          /* for '.' */
        len += strlen( ordinal );
    }
    if( internalName != NULL ) {
        len++;                          /* for '=' */
        len += strlen( internalName );
    }
    str = AllocMem( len );
    *str = '\0';
    strcat( str, entryName );
    if( ordinal != NULL ) {
        strcat( str, "." );
        strcat( str, ordinal );
    }
    if( internalName != NULL ) {
        strcat( str, "=" );
        strcat( str, internalName );
    }
    add_string( optStr, str, '\0' );
    FreeMem( entryName );
    if( internalName != NULL )
        FreeMem( internalName );
    if( ordinal != NULL )
        FreeMem( ordinal );

    return( true );
}


/*
 * Parse the /HEAP option.
 */
static bool parse_heap( OPT_STRING **p )
/**************************************/
{
    return( do_string_parse( p, "HEAP", true, '\0' ) );
}


/*
 * Parse the /IMPLIB option.
 */
static bool parse_implib( OPT_STRING **p )
/****************************************/
{
    return( do_string_parse( p, "IMPLIB", true, '\0' ) );
}


/*
 * Parse the /INCLUDE option.
 */
static bool parse_include( OPT_STRING **p )
/*****************************************/
{
    return( do_string_parse( p, "INCLUDE", false, '\'' ) );
}


/*
 * Parse the /INCREMENTAL option.
 */
static bool parse_incremental( OPT_STRING **p )
/*********************************************/
{
    char *              str;

    if( !CmdScanRecogChar( ':' ) ) {
        FatalError( "/INCREMENTAL requires an argument" );
        return( false );
    }
    str = CmdScanString();
    if( str == NULL ) {
        FatalError( "/INCREMENTAL requires an argument" );
        return( false );
    }
    if( !stricmp( str, "yes" )  ||  !stricmp( str, "no" ) ) {
        add_string( p, str, '\0' );
        return( true );
    } else {
        FatalError( "Invalid argument '%s' to /INCREMENTAL", str );
        return( false );
    }
}


/*
 * Parse the undocumented /INTERNALDLLNAME option.
 */
static bool parse_internaldllname( OPT_STRING **p )
/*************************************************/
{
    return( do_string_parse( p, "INTERNALDLLNAME", true, '\0' ) );
}


/*
 * Parse the /MACHINE option.
 */
static bool parse_machine( OPT_STRING **p )
/*****************************************/
{
    return( do_string_parse( p, "MACHINE", false, '\0' ) );
}


/*
 * Parse the /MAP option.
 */
static bool parse_map( OPT_STRING **p )
/*************************************/
{
    char *              str;

    if( !CmdScanRecogChar( ':' ) )
        return( true );
    str = CmdScanString();
    if( str != NULL ) {
        add_string( p, str, '\0' );
    }
    return( true );
}


/*
 * Parse the /ORDER option.
 */
static bool parse_order( OPT_STRING **p )
/***************************************/
{
    return( do_string_parse( p, "ORDER", false, '\0' ) );
}


/*
 * Parse the /OUT option.
 */
static bool parse_out( OPT_STRING **p )
/*************************************/
{
    return( do_string_parse( p, "OUT", true, '\0' ) );
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
    if( str == NULL )
    {
        FatalError("/passwopts requires an argument");
        return( false );
    }

    /*
     * If quoted, stip out the quote characters.
     */
    if( *str == '\"' )
    {
        for( dst = str, src = str + 1; *src && (*src != '\"'); )
        {
            *dst++ = *src++;
        }

        if( *src != '\"' )
        {
            FatalError("/passwopts argument is missing closing quote");
            return( false );
        }

        *dst = 0x00;
    }

    add_string( p, str, '\0' );
    return( true );
} /* parse_passwopts() */


/*
 * Parse the /PDB option.
 */
static bool parse_pdb( OPT_STRING **p )
/*************************************/
{
    return( do_string_parse( p, "PDB", false, '\0' ) );
}


/*
 * Parse the /SECTION option.
 */
static bool parse_section( OPT_STRING **p )
/*****************************************/
{
    return( do_string_parse( p, "SECTION", true, '\0' ) );
}


/*
 * Parse the /STACK option.
 */
static bool parse_stack( OPT_STRING **p )
/***************************************/
{
    return( do_string_parse( p, "STACK", true, '\0' ) );
}


/*
 * Parse the /STUB option.
 */
static bool parse_stub( OPT_STRING **p )
/**************************************/
{
    return( do_string_parse( p, "STUB", true, '\0' ) );
}


/*
 * Parse the /SUBSYSTEM option.
 */
static bool parse_subsystem( OPT_STRING **p )
/*******************************************/
{
    return( do_string_parse( p, "SUBSYSTEM", true, '\0' ) );
}


/*
 * Parse the /VERSION option.
 */
static bool parse_version( OPT_STRING **p )
/*****************************************/
{
    return( do_string_parse( p, "VERSION", true, '\0' ) );
}


/*
 * Set /OPT:NOREF when /DEBUG is used.
 */
static void handle_debug( OPT_STORAGE *cmdOpts, int x )
/*****************************************************/
{
    /* unused parammeters */ (void)x;

    cmdOpts->opt_level = OPT_ENUM_opt_level_opt_noref;

    if( cmdOpts->debug_value != NULL ) {
        // suppress this diagnostic,
        // it appears that MS ignores this option also.
        //Warning( "Ignoring unsupported argument '%s' to /DEBUG", cmdOpts->debug_value->data );
        OPT_CLEAN_STRING( &cmdOpts->debug_value );
    }
}


/*
 * Suppress warning messages.
 */
static void handle_nowwarn( OPT_STORAGE *cmdOpts, int x )
/*******************************************************/
{
    /* unused parammeters */ (void)cmdOpts; (void)x;

    DisableWarnings( true );
}


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
 * Scan a number.  No leading whitespace is permitted.
 */
bool OPT_GET_NUMBER( unsigned *p )
/********************************/
{
    unsigned            value;

    if( !CmdScanRecogChar( ':' ) )
        return( false );
    if( CmdScanNumber( &value ) ) {
        *p = value;
        return( true );
    } else {
        return( false );
    }
}


/* Include after all static functions were declared */
#include "cmdlnprs.gc"
