/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  Support for FORTRAN 77 syntax.
*
****************************************************************************/


#include "vi.h"
#include "sstyle.h"
#include "sstyle_f.h"

#include "clibext.h"


/*----- LOCALS -----*/
static int          firstChar;
static linenum      thisLine;
static ss_flags_f   flags;

enum getFloatCommands {
    AFTER_DOT,
    AFTER_EXP,
    BADTEXT,
};

static int issymbol( int c )
{
    /* symbol list taken from Watcom Fortran 77 Lang Ref. pg 3
    */
    if( c == '=' ||
        c == '+' ||
        c == '-' ||
        c == '*' ||
        c == '/' ||
        c == '(' ||
        c == ')' ||
        c == ',' ||
        c == '.' ||
        c == '$' ||
        c == ':' ||
        c == '%' ||
        c == '\\' ) {
        return( 1 );
    } else {
        return( 0 );
    }
}

static int iscomment( int c )
{
    if( c == '!' ||
        c == 'c' ||
        c == 'C' ||
        c == 'd' ||
        c == 'D' ||
        c == '*' ) {
        return( 1 );
    } else {
        return( 0 );
    }
}

static int islogical( char *string )
{
    if( strnicmp( string, ".ne.", 4 ) == 0 ||
        strnicmp( string, ".eq.", 4 ) == 0 ||
        strnicmp( string, ".ge.", 4 ) == 0 ||
        strnicmp( string, ".le.", 4 ) == 0 ||
        strnicmp( string, ".lt.", 4 ) == 0 ||
        strnicmp( string, ".gt.", 4 ) == 0 ||
        strnicmp( string, ".or.", 4 ) == 0 ) {
        return( 4 );
    } else if( strnicmp( string, ".and.", 5 ) == 0 ||
                strnicmp( string, ".xor.", 5 ) == 0 ||
                strnicmp( string, ".not.", 5 ) == 0 ) {
        return( 5 );
    } else {
        return( 0 );
    }
}

static bool isInitialLine( line *line )
{
    char    *data;
    if( line->len < 6 ) {
        return( true );
    }
    data = (line->u.ld.nolinedata) ? WorkLine->data : line->data;
    if( isspace( data[5] ) || data[5] == '0' ) {
        return( true );
    } else {
        return( false );
    }
}

void InitFORTRANLine( char *text, linenum line_no )
{
    firstChar = *text;
    thisLine = line_no;
}

static void getFloat( ss_block *ss_new, char *start, int skip, int command )
{
    char    *end = start + skip;

    ss_new->type = SE_FLOAT;

    switch( command ) {
    case AFTER_DOT:
        if( !isdigit( *end ) ) {
            if( *end == 'E' || *end == 'D' ) {
                getFloat( ss_new, start, end - start + 1, AFTER_EXP );
                return;
            }
            if( *end != '\0' && !isspace( *end ) && !issymbol( *end ) ) {
                if( *end != '\0' ) {
                    end++;
                }
                ss_new->type = SE_INVALIDTEXT;
            }
            break;
        }
        end++;
        SKIP_DIGITS( end );
        if( *end != 'E' && *end != 'D' ) {
            break;
        }
        end++;
        // fall through
    case AFTER_EXP:
        if( *end == '+' || *end == '-' ) {
            end++;
        }
        if( !isdigit( *end ) ) {
            if( *end != '\0' ) {
                end++;
            }
            ss_new->type = SE_INVALIDTEXT;
            break;
        }
        end++;
        SKIP_DIGITS( end );
        if( *end != '\0' && !isspace( *end ) && !issymbol( *end ) ) {
            ss_new->type = SE_INVALIDTEXT;
            end++;
        }
    }
    ss_new->len = end - start;
}


static void getNumber( ss_block *ss_new, char *start )
{
    char    *end = start + 1;
    char    save_char;
    char    *save_loc;
    int     nchars;

    SKIP_DIGITS( end );
    switch( *end ) {
    case 'h':
    case 'H':
        /* Hollerith constant (string)
        */
        save_loc = end;
        save_char = *end;
        *end = 0;
        nchars = atoi( start );
        end++;
        while( *end != '\0' && nchars != 0 ) {
            nchars--;
            end++;
        }
        if( *end == '\0' && nchars > 0 ) {
            ss_new->type = SE_INVALIDTEXT;
        } else {
            ss_new->type = SE_STRING;
        }
        *save_loc = save_char;
        break;
    case '.':
        getFloat( ss_new, start, end - start + 1, AFTER_DOT );
        return;
    case 'E':
    case 'D':
        getFloat( ss_new, start, end - start + 1, AFTER_EXP );
        return;
    default:
        ss_new->type = SE_INTEGER;
    }
    ss_new->len = end - start;
}

static void getText( ss_block *ss_new, char *start )
{
    char    *keyword;
    char    *end = start;

    // eliminate leading spaces
    SKIP_SPACES( end );
    // take copy string up to first white space
    keyword = end;
    while( isalnum( *end ) || (*end == '_') || (*end == '$') ) {
        end++;
    }

    // test if string is keyword
    if( IsKeyword( keyword, end, true ) ) {
        char *end2 = end;

        ss_new->type = SE_KEYWORD;

        // attempt to find a "*" associated with type
        SKIP_SPACES( end2 );
        if( *end2 == '*' ) {
            end = ++end2;
        }
    } else {
        ss_new->type = SE_IDENTIFIER;
    }

    ss_new->len = end - start;
}


static void getSymbol( ss_block *ss_new, int length )
{
    ss_new->type = SE_SYMBOL;
    ss_new->len = length;
}

static void getLiteral( ss_block *ss_new, char *start, int skip )
{
    char    *end;
    char    lastchar = '\0';
    bool    empty;
    bool    multiLine = flags.inString;
    line    *line;
    fcb     *fcb;
    char    *data;
    vi_rc   rc;

    empty = true;
    for( end = start + skip; *end != '\0'; ++end ) {
        if( end[0] == '\'' ) {
            if( end[1] != '\'' )
                break;
            ++end;
        }
        empty = false;
    }
    flags.inString = false;
    if( *end == '\0' ) {
        // if next line a continuation line, then flag flags.inString, else
        // flag unterminated string
        rc = CGimmeLinePtr( thisLine + 1, &fcb, &line );
        for( ;; ) {
            if( rc != ERR_NO_ERR ) {
                break;
            }
            data = (line->u.ld.nolinedata) ? WorkLine->data : line->data;
            if( !iscomment( data[0] ) ) {
                break;
            }
            rc = CGimmeNextLinePtr( &fcb, &line );
        }
        ss_new->type = SE_INVALIDTEXT;
        if( rc == ERR_NO_ERR && !isInitialLine( line ) ) {
            ss_new->type = SE_STRING;
            flags.inString = true;
        }
    } else {
        end++;
        lastchar = tolower( *end );
        switch( lastchar ) {
        case 'c':
            end++;
            ss_new->type = SE_STRING;
            break;
        case 'x':
            end++;
            ss_new->type = SE_HEX;
            break;
        case 'o':
            end++;
            ss_new->type = SE_OCTAL;
            break;
        default:
            // hard to say if invalid or not - take a guess
            if( islower( *end ) ) {
                end++;
                ss_new->type = SE_INVALIDTEXT;
            } else {
                ss_new->type = SE_STRING;
            }
            break;
        }
    }
    ss_new->len = end - start;
    if( empty && !multiLine ) {
        ss_new->type = SE_INVALIDTEXT;
    }
}

static void getComment( ss_block *ss_new, char *start )
{
    char    *end = start + 1;

    SKIP_TOEND( end );
    ss_new->type = SE_COMMENT;
    ss_new->len = end - start;
}

static void getInvalidChar( ss_block *ss_new )
{
    ss_new->type = SE_INVALIDTEXT;
    ss_new->len = 1;
}

static void getLabelOrWS( ss_block *ss_new, char *start, int text_col )
{
    char    *end = start;

    while( isspace( *end ) && text_col <= 4 ) {
        end++;
        text_col++;
    }
    if( *end != '\0' && text_col <= 4 ) {
        ss_new->type = SE_JUMPLABEL;
        while( *end != '\0' && text_col <= 4 ) {
            if( !isdigit( *end ) && !isspace( *end ) ) {
                end++;
                ss_new->type = SE_INVALIDTEXT;
                break;
            }
            end++;
            text_col++;
        }
        ss_new->len = end - start;
    } else {
        SSGetWhiteSpace( ss_new, start );
    }
}

static void getContinuationOrWS( ss_block *ss_new, char *start )
{
    if( isspace( *start ) ) {
        SSGetWhiteSpace( ss_new, start );
        return;
    }
    /* this is technically wrong - a ' ' on an initial line is
     * also a label, but that would be ugly
     */
    ss_new->type = SE_JUMPLABEL;
    ss_new->len = 1;
}

void InitFORTRANFlagsGivenValues( ss_flags_f *newFlags )
{
    flags = *newFlags;
}

void GetFORTRANFlags( ss_flags_f *storeFlags )
{
    *storeFlags = flags;
}

void InitFORTRANFlags( linenum line_no )
{
    char    *text;
//    char    *start;
    line    *line;
    fcb     *fcb;
    vi_rc   rc;
    int     numQuotes = 0;

    flags.inString = false;

    rc = CGimmeLinePtr( line_no, &fcb, &line );
    if( rc != ERR_NO_ERR ) {
        // probably past eof
        return;
    }

    if( isInitialLine( line ) ) {
        return;
    }

    for( ;; ) {
        rc = GimmePrevLinePtr( &fcb, &line );
        if( rc != ERR_NO_ERR ) {
            break;
        }
        text = line->u.ld.nolinedata ? WorkLine->data : line->data;
//        start = text;
        if( iscomment( *text ) ) {
            continue;
        }
        while( *text != '\0' ) {
            if( *text == '!' ) {
                // rest of line is part of a comment
                break;
            } else if( *text == '\'' ) {
                numQuotes ^= 1;
            }
            text++;
        }
        if( isInitialLine( line ) ) {
            break;
        }
    }

    if( numQuotes == 1 ) {
        flags.inString = true;
    }
}

void GetFORTRANBlock( ss_block *ss_new, char *start, int text_col )
{
    int length = 0;

    if( start[0] == '\0' ) {
        if( text_col == 0 ) {
            // line is empty -
            // do not flag following line as having anything to do
            // with an unterminated string from previous line
            flags.inString = false;
        }
        SSGetBeyondText( ss_new );
        return;
    }

    if( iscomment( firstChar ) ) {
        getComment( ss_new, start );
        return;
    }

    if( text_col <= 4 ) {
        getLabelOrWS( ss_new, start, text_col );
        return;
    }

    if( text_col == 5 ) {
        getContinuationOrWS( ss_new, start );
        return;
    }

    if( flags.inString ) {
        getLiteral( ss_new, start, 0 );
        return;
    }

    if( isspace( start[0] ) ) {
        SSGetWhiteSpace( ss_new, start );
        return;
    }

    switch( start[0] ) {

    case '!':
        getComment( ss_new, start );
        return;
    case '\'':
        getLiteral( ss_new, start, 1 );
        return;
    case '.':
        if( isdigit( start[1] ) ) {
            getFloat( ss_new, start, 1, AFTER_DOT );
            return;
        }
        length = islogical( start );
        if( length > 0 ) {
            getSymbol( ss_new, length );
            return;
        }
    }

    if( issymbol( start[0] ) ) {
        getSymbol( ss_new, 1 );
        return;
    }

    if( isdigit( *start ) ) {
        getNumber( ss_new, start );
        return;
    }

    if( isalpha( *start ) || (*start == '_') || (*start == '$') ) {
        getText( ss_new, start );
        return;
    }

    getInvalidChar( ss_new );
}
