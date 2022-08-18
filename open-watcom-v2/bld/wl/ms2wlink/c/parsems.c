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
* Description:  Routines for parsing Microsoft command files.
*
****************************************************************************/


#include <string.h>
#include <ctype.h>
#if defined( __WATCOMC__ ) || !defined( __UNIX__ )
#include <process.h>
#endif
#include "wio.h"
#include "walloca.h"
#include "ms2wlink.h"
#include "command.h"

#include "clibext.h"


cmdfilelist         *CmdFile;

static int          OverlayLevel = 0;

void EatWhite( void )
/*******************/
{
    while( *CmdFile->current == ' ' || *CmdFile->current == '\t' || *CmdFile->current == '\r' ) {
        CmdFile->current++;
    }
}

bool InitParsing( void )
/**********************/
// initialize the parsing stuff, and see if there is anything on the command
// line.
{
    CmdFile = MemAlloc( sizeof( cmdfilelist ) );
    CmdFile->next = NULL;
    CmdFile->buffer = MemAlloc( MAX_LINE );   // maximum size of a command line.
    CmdFile->how = COMMANDLINE;
    CmdFile->oldhow = COMMANDLINE;
    CmdFile->where = MIDST;
    CmdFile->name = NULL;
    CmdFile->fp = NULL;
    getcmd( CmdFile->buffer );
    CmdFile->current = CmdFile->buffer;
    EatWhite();
    return( true );
}

void FreeParserMem( void )
/************************/
{
    cmdfilelist     *curr;

    while( (curr = CmdFile) != NULL ) {
        CmdFile = CmdFile->next;
        if( curr->fp != NULL ) {
            QClose( curr->fp, curr->name );
        }
        MemFree( curr->buffer );
        MemFree( curr->name );
        MemFree( curr );
    }
}

static void GetNewLine( prompt_slot slot )
/****************************************/
{
    if( CmdFile->how == NONBUFFERED ) {
        if( QReadStr( CmdFile->fp, CmdFile->buffer, MAX_LINE, CmdFile->name ) ) {
            CmdFile->where = ENDOFFILE;
        } else {
            CmdFile->where = MIDST;
        }
        CmdFile->current = CmdFile->buffer;
    } else if( CmdFile->how == BUFFERED ) {               // interactive.
        CmdFile->where = MIDST;
        //go until next line found;
        for( ; *CmdFile->current != '\n'; CmdFile->current++ ) {
            if( *CmdFile->current == '\0' ) {
                CmdFile->where = ENDOFFILE;
                break;
            }
        }
    } else {
        CmdFile->how = INTERACTIVE;
        CmdFile->oldhow = INTERACTIVE;
        OutPutPrompt( slot );
        if( QReadStr( stdin, CmdFile->buffer, MAX_LINE, "console" ) ) {
            CmdFile->where = ENDOFCMD;
        } else {
            CmdFile->where = MIDST;
        }
        CmdFile->current = CmdFile->buffer;
    }
}

static void RestoreCmdLine( void )
/********************************/
// Restore a saved command line.
{
    cmdfilelist *temp;

    temp = CmdFile;
    CmdFile = CmdFile->next;
    QClose( temp->fp, temp->name );
    MemFree( temp->name );
    MemFree( temp->buffer );
    MemFree( temp );
}

void ParseDefFile( void )
/***********************/
// parse a .def file
{
    char        hmm;

    while( CmdFile->where != ENDOFFILE ) {
        switch( CmdFile->where ) {
        case MIDST:
            EatWhite();
            hmm = *CmdFile->current;
            switch( hmm ) {
            case '\0':
                if( CmdFile->how == BUFFERED ) {
                    CmdFile->where = ENDOFFILE;
                    break;
                }                // NOTE the fall through.
            case '\n':
                if( CmdFile->how == BUFFERED ) {
                    CmdFile->current++;
                } else {
                    CmdFile->where = ENDOFLINE;
                }
                break;
            case ';':
                CmdFile->where = ENDOFLINE;    // a comment.
                break;
            default:          // must be a command
                ProcessDefCommand();
                break;
            }
            break;
        case ENDOFLINE:
            GetNewLine( DEF_SLOT );
            break;
        }
    }
    RestoreCmdLine();
}

void StartNewFile( char *fname )
/******************************/
{
    cmdfilelist     *newfile;
    unsigned long   long_size;

    newfile = MemAlloc( sizeof( cmdfilelist ) );
    newfile->name = fname;
    newfile->fp = NULL;
    newfile->buffer = NULL;
    newfile->next = CmdFile;
    CmdFile = newfile;
    newfile->fp = QOpenR( newfile->name );
    long_size = QFileSize( newfile->fp );
    if( long_size < 0x10000 - 16 - 1 ) {       // if can alloc a chunk big enough
        size_t  size = (size_t)long_size;

        newfile->buffer = MemAlloc( size + 1 );
        if( newfile->buffer != NULL ) {
            size = QRead( newfile->fp, newfile->buffer, size, newfile->name );
            if( size == IOERROR )
                size = 0;
            *(newfile->buffer + size) = '\0';
            newfile->where = MIDST;
            newfile->how = BUFFERED;
            newfile->current = newfile->buffer;
        }
    }
    if( newfile->buffer == NULL ) {  // if couldn't buffer for some reason.
        newfile->where = ENDOFLINE;
        newfile->how = NONBUFFERED;
        newfile->buffer = MemAlloc( MAX_LINE + 1 );// have to have at least this
        newfile->current = newfile->buffer;        // much RAM or death ensues.
    }
}

static void ProcessDefFile( void )
/********************************/
{
    cmdentry *  cmd;

    if( Commands[DEF_SLOT] == NULL ) {
        return;
    }
    HaveDefFile = 1;
    cmd = Commands[DEF_SLOT];
    StartNewFile( FileName( cmd->command, DEF_SLOT, false ) );
    ParseDefFile();
}

void DirectiveError( void )
/*************************/
{
    char    *msg;
    size_t  len;

    len = 22 + CmdFile->len;
    msg = alloca( len + 2 );
    memcpy( msg, "directive error near '", 22 );
    memcpy( msg + 22, CmdFile->token, CmdFile->len );
    *( msg + len) = '\'';
    *( msg + len + 1) = '\0';
    ErrorExit( msg );
}

bool MakeToken( sep_type separator, bool include_fn )
/***************************************************/
// include_fn == true if '.' and ':' are allowed to be part of the token
// (include filename).
{
    bool    quit;
    char    hmm;
    char    matchchar;
    size_t  len;
    bool    forcematch;
    bool    hitmatch;

    EatWhite();
    hmm = *CmdFile->current;
    CmdFile->token = CmdFile->current;
    if( hmm == '\0' || hmm == '\n' ) {
        CmdFile->len = 0;
        return( false );
    }
    matchchar = '\0';
    CmdFile->len = 1;                 // for error reporting.
    switch( separator ) {
    case SEP_QUOTE:
        if( hmm != '\'' && hmm != '"' ) {
            return( false );
        }
        matchchar = hmm;
        break;
    case SEP_AT:
        if( hmm != '@' ) {
            return( false );
        }
        break;
    case SEP_COLON:
        if( hmm != ':' ) {
            return( false );
        }
        break;
    case SEP_EQUALS:
        if( hmm != '=' ) {
            return( false );
        }
        break;
    case SEP_PERIOD:
        if( hmm != '.' ) {
            return( false );
        }
        break;
    default:
        CmdFile->current--;        // no separator wanted.
    }
    CmdFile->current++;     // skip separator.
    CmdFile->token = CmdFile->current;
    hmm = *CmdFile->current;
    if( hmm == '\0' || hmm == '\n' || hmm == ';' ) {
        CmdFile->len = 0;
        return( false );
    }
    len = 0;
    quit = false;
    forcematch = (separator == SEP_QUOTE );
    hitmatch = false;
    while( !quit ) {
        len++;
        CmdFile->current++;
        hmm = *CmdFile->current;
        switch( hmm ) {
        case '\'':
        case '"':
            if( separator == SEP_QUOTE && hmm == matchchar ) {
                CmdFile->current++;    // don't include end quote in next token.
                hitmatch = true;
                quit = true;
            }
            break;
        case ')':       // Right paren separates only in overlay, left never
            if( OverlayLevel == 0 )
                break;      // NOTE: possible fall through
            /* fall through */
        case '+':              // break a token on any of these.
        case ',':
        case ';':
        case '=':
        case '@':
            if( separator == SEP_SPACE )
                break; // NOTE: possible fall through
            /* fall through */
        case '/':
        case ' ':
        case '\t':
            if( !forcematch ) {
                quit = true;
            }
            break;
        case '.':
        case ':':
            if( !forcematch && !include_fn ) {
                quit = true;
            }
            break;
        case '\0':
        case '\r':
        case '\n':
            quit = true;
            break;
        }
    }
    CmdFile->len = len;
    if( forcematch && !hitmatch ) {
        return( false );
    }
    return( true );
}

char *ToString( void )
/********************/
{
    char            *src;
    size_t          len;
    char            *str;

    src = CmdFile->token;
    len = CmdFile->len;
    str = MemAlloc( len + 1 );
    memcpy( str, src, len );
    str[len] = '\0';
    return( str );
}

static char     sep_chr = ',';  /* Current separator character. */
static char     last_sep;       /* Previous separator character. */
static bool     is_redir;       /* Input is redirected (not from console). */
static bool     is_inp_done;    /* Input terminator next, no more input. */
static bool     is_term;        /* Input terminator was encountered. */
static bool     is_quoting;     /* Currently reading a quoted name. */
static bool     is_new_line;    /* At the beginning of a new input line. */
static bool     no_prompt;      /* Prompting is disabled (batch mode). */
static bool     more_cmdline;   /* More command line input is available. */
static char     masked_char;
static char     mask_spc_chr;

#define LINE_BUF_SIZE   256

static int ReadNextChar( prompt_slot slot )
/*****************************************/
{
    int     c;

    if( CmdFile->how == BUFFERED || CmdFile->how == NONBUFFERED )
    {
        for( ;; ) {
            c = *CmdFile->current++;
            if( c == '\0' /*EOF*/ || c == '\x1a' )
                break;  /* Quit if end of stream. */
            if( is_new_line ) {
                if( slot != OPTION_SLOT && !no_prompt ) {
                    ; //OutPutPrompt( slot );
                }
                is_new_line = false;
            }
            if( slot != OPTION_SLOT && !no_prompt ) {
                if( c == '\r' ) {
                    continue;
                }
                //@TODO: output 'c' here?
            }
            if( c == ';' ) {
                ;//@TODO: output newline?
            } else if( c == '\n' ) {
                is_new_line = true;
            } else if( c == '\t' ) {
                c = ' ';    /* Tabs to spaces. */
            }
            if( c == '\n' || c >= ' ' ) {
                return( c );
            }
        }
        RestoreCmdLine();
    }

    if( more_cmdline ) {
        for( ;; ) {
            if( *CmdFile->current == '\0' ) {
                more_cmdline = false;
                is_new_line  = true;
                return( '\n' );
            }
            c = *CmdFile->current++;
            //@TODO: escaped quotes?
            if( c >= ' ' ) {
                return( c );
            }
        }
    }
    for( ;; ) {
        if( is_new_line ) {
            if( slot != OPTION_SLOT && ((!is_redir && !no_prompt) || (!is_term && no_prompt)) )
                GetNewLine( slot );
            is_new_line = false;
        }
        c = *CmdFile->current++;
        if( c == '\0' /*EOF*/ ) {
            c = ';';    /* EOF to end-of-input marker. */
        } else if( c == '\t' ) {
            c = ' ';    /* Tabs to spaces. */
        }
        if( c == '\n' )
            is_new_line = true;
        if( c == '\n' || c >= ' ' ) {
            return( c );
        }
    }
}

static char *StrDup( const char *src )
/************************************/
{
    size_t          len;
    char            *str;

    len = strlen( src );
    str = MemAlloc( len + 1 );
    memcpy( str, src, len );
    str[len] = '\0';
    return( str );
}

static int GetNextInputChar( prompt_slot slot )
/*********************************************/
{
    int     c;

    c = ReadNextChar( slot );
    /* Redirect input to a response file; cannot be nested. */
    if( c == '@' && !is_quoting ) {
        if( CmdFile->how == COMMANDLINE || CmdFile->how == INTERACTIVE ) {
            char    fname[LINE_BUF_SIZE];
            int     oi;

            for( oi = 0; oi < sizeof( fname ) - 1; ) {
                c = ReadNextChar( slot );
                if( c == '"' )
                    is_quoting ^= true;
                /* Quit loop if not a valid filename character. */
                if( (!is_quoting && (c == ',' || c == '+' || c == ';' || c == ' ')) ||
                    c == '/' || c < ' ' )
                    break;
                if( c != '"' ) {
                    fname[oi++] = c;
                }
            }
            fname[oi] = '\0';
            if( c > ' ' )
                --CmdFile->current;

            /* Open the response file and read next character. */
            StartNewFile( StrDup( fname ) );
            c = ReadNextChar( slot );
        }
    }
    return( c );
}

static size_t GetLine( char *line, size_t buf_len, prompt_slot slot )
/*******************************************************************/
{
    bool        first = true;
    unsigned    oi;
    int         c;
    unsigned    idx1, idx2;

    first = true;
    last_sep = sep_chr;
    if( is_inp_done ) {
        line[0] = '\0';
        is_term = true;
        return( 0 );
    }
    c = 0;
    for( ;; ) {
        is_quoting = false;
        /* Read a line from input stream. */
        for( oi = 0; oi < buf_len - 1; ) {
            c = GetNextInputChar( slot );
            if( c == '"' )
                is_quoting ^= true;
            if( c == '\n' || (!is_quoting && (c == ',' || c == ';')) ) {
                /* Done with this line. */
                if( c == ';' )  /* Semicolon terminates input. */
                    more_cmdline = false;
                break;
            }
            if( oi != 0 || c != ' ' ) { /* Ignore leading spaces. */
                if( !is_quoting ) {
                    if( c == '+' ) {    /* A '+' acts as a separator. */
                        if( !masked_char )
                            masked_char = c;
                        c = mask_spc_chr;
                    }
                    if( c == ' ' && !masked_char )
                        masked_char = c;
                }
                line[oi++] = c;
            }
        }
        /* Check for line buffer overflow. */
        if( ( oi == buf_len - 1 ) && (c = GetNextInputChar( slot )) != '\n' && c != ',' && c != ';' ) {
            ErrorExit( "maximum line length exceeded" );
        }
        if( oi ) {  /* Index of the terminating null. */
            /* Strip trailing spaces. */
            while( oi > 0 && line[oi - 1] == ' ' ) {
                --oi;
            }
        }
        line[oi] = '\0';

        idx1 = 0;
        for( idx2 = 0; idx2 < oi; ++idx2 ) {
            if( line[idx2] == '"' ) {
                while( idx2 < oi && line[++idx2] != '"' ) {
                    line[idx1++] = line[idx2];
                }
            } else if( line[idx2] != ' ' || mask_spc_chr != 0 || is_quoting ) {
                if( !is_quoting && line[idx2] == ' ' ) {
                    line[idx1] = mask_spc_chr;
                } else {
                    line[idx1] = line[idx2];
                }
                ++idx1;
            }
        }

        /* Null terminate string. */
        line[idx1] = '\0';
        sep_chr = c;
        if( oi || !first || !((last_sep == ',' && sep_chr == '\n') || (last_sep == '\n' && sep_chr == ',')) )
            break;

        /* Another run through the loop. */
        first = false;
        last_sep = ',';
    }
    is_inp_done = (c == ';');
    is_term = (oi == 0 && is_inp_done);
    return( idx1 ); /* Return strlen( line ) equivalent. */
}

static void TokenizeLine( char *buf, char delim, void (*pfn)( char * ) )
/**********************************************************************/
{
    char    *s1;
    char    *s2;

    for( s1 = buf; *s1 != '\0'; s1 = s2 ) {
        s2 = s1;
        while( *s2 != '\0' && *s2 != delim )
            ++s2;
        if( *s2 != '\0' )
            *s2++ = '\0';
        pfn( s1 );
    }
}

static void DoOneOption( char *opt )
/**********************************/
// process a single option
{
    ProcessOption( opt );
}

static void DoOptions( char *buf )
/********************************/
// process options in a string, if present
{
    char    *opt;
    char    *cmd;

    for( cmd = buf; *cmd != '\0'; ++cmd ) {
        if( *cmd == '/' ) {
            break;
        }
    }
    /* See if option separator was found. */
    if( *cmd == '/' ) {
        *cmd = '\0';
        opt = cmd + 1;
        /* Strip trailing spaces from text preceding the option. */
        --cmd;
        while( cmd > buf && *cmd == ' ' ) {
            *cmd-- = '\0';
        }
        /* Strip trailing junk from the option. */
        //@TODO!
        /* Now handle the option(s). */
        TokenizeLine( opt, '/', DoOneOption );
    }
}

static char *LastStringChar( char *s )
/************************************/
{
    if( *s != '\0' ) {
        while( *s != '\0' )
            ++s;
        --s;
    }
    return( s );
}

static void DoOneObject( char *obj )
/**********************************/
// process a single object file
{
    bool    l_paren = false;
    bool    r_paren = false;
    char    *end;


    /* Find the end of the string. */
    end = LastStringChar( obj );

    /* The way Microsoft handles overlay syntax is pretty bizarre... */

    /* First remove surrounding parentheses, if present. */
    if( *obj == '(' ) {
        l_paren = true;
        ++obj;
    }
    if( *end == ')' ) {
        r_paren = true;
        *end-- = '\0';
    }
    /* Only then process options. */
    DoOptions( obj );

    /* Now look for a right parenthesis *again*. */
    end = LastStringChar( obj );
    if( *end == ')' ) {
        if( r_paren )
            ErrorExit( "nested right parentheses" );
        r_paren = true;
        *end-- = '\0';
    }
    /* Conditionally open an overlay. */
    if( l_paren ) {
        char    *sect;

        if( OverlayLevel )
            ErrorExit( "nested left parentheses" );

        sect = MemAlloc( 8 );
        memcpy( sect, "section", 8 );
        AddCommand( sect , OVERLAY_SLOT, true );
        ++OverlayLevel;
    }
    /* If anything is left, add it to the list of object files. */
    if( *obj != '\0' ) {
        AddCommand( FileName( obj, OBJECT_SLOT, false ), ( OverlayLevel ? OVERLAY_SLOT : OBJECT_SLOT ), false );
    }
    /* Conditionally close the current overlay. */
    if( r_paren ) {
        if( !OverlayLevel )
            ErrorExit( "unmatched right parenthesis" );
        --OverlayLevel;
    }
}

static void DoOneLib( char *lib )
/*******************************/
// process a single library
{
    DoOptions( lib );

    /* If anything is left, add it to the list of object files. */
    if( *lib != '\0' ) {
        AddCommand( FileName( lib, LIBRARY_SLOT, false ), LIBRARY_SLOT, false );
    }
}

static void GetNextInput( char *buf, size_t len, prompt_slot slot )
/*****************************************************************/
{
    if( !is_term ) {
        GetLine( buf, len, slot );
        DoOptions( buf );
        if( *buf != '\0' ) {
            AddCommand( FileName( buf, slot, false ), slot, false );
        }
    }
}

void ParseMicrosoft( void )
/*************************/
// read in Microsoft linker commands
{
    char        cmd[LINE_BUF_SIZE];
    char        *end;
    size_t      len;
    bool        first;
    bool        more_objs;
    bool        more_libs;


    /* Start with object files. */
    mask_spc_chr = 0x1f;    /* Replace spaces with another character. */
    more_cmdline = ( *CmdFile->current != '\0' );
    first = more_cmdline;
    is_new_line = true;
    do {
        more_objs = false;  /* No more unless we discover otherwise. */
        if( first ) {
            len = GetLine( cmd, sizeof( cmd ), OPTION_SLOT );
        } else {
            len = GetLine( cmd, sizeof( cmd ), OBJECT_SLOT );
        }
        if( len == 0 )
            break;
        end = LastStringChar( cmd );
        if( *end == mask_spc_chr ) {
            more_objs = true;
            *end = '\0';
        }
        TokenizeLine( cmd, mask_spc_chr, DoOneObject );
        first = false;
    } while( more_objs );

    /* Check for possible error conditions. */
    if( OverlayLevel )
        ErrorExit( "unmatched left parenthesis" );
    FindObjectName();   /* This will report an error if no objects. */

    mask_spc_chr = 0;   /* Remove spaces in input. */
    /* Get executable and map file name. */
    GetNextInput( cmd, sizeof( cmd ), RUN_SLOT );
    GetNextInput( cmd, sizeof( cmd ), MAP_SLOT );

    mask_spc_chr = 0x1f;    /* Replace spaces with another character. */
    /* Get library file names. */
    if( !is_term ) {
        do {
            GetLine( cmd, sizeof( cmd ), LIBRARY_SLOT );
            more_libs = false;
            if( is_term || *cmd == '\0' )
                break;
            end = LastStringChar( cmd );
            if( *end == mask_spc_chr ) {
                more_libs = true;
                *end = '\0';
            }
            TokenizeLine( cmd, mask_spc_chr, DoOneLib );
        } while( more_libs );
    }

    mask_spc_chr = 0;   /* Remove spaces in input again. */

    /* Get def file name and process it. */
    GetNextInput( cmd, sizeof( cmd ), DEF_SLOT );
    ProcessDefFile();
}
