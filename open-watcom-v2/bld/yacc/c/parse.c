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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "alloc.h"
#include "yacc.h"

#define BUF_INCR            500

#define INIT_RHS_SIZE       16

#define TYPENAME_FIRST_CHAR(x) (isalpha(x)||x=='_')
#define TYPENAME_NEXT_CHAR(x) (isalpha(x)||isdigit(x)||x=='_'||x=='.')

typedef enum {
    /* ASCII_MIN = 0x0000 */
    /* ASCII_MAX = 0x00FF */
    T_IDENTIFIER = 0x0100,  /* includes identifiers and literals */
    T_CIDENTIFIER,          /* identifier (but not literal) followed by colon */
    T_NUMBER,               /* -?[0-9]+ */
    T_MARK,                 /* %% */
    T_LCURL,                /* %{ */
    T_RCURL,                /* }% */
    T_AMBIG,                /* %keywords */
    T_KEYWORD_ID,
    T_LEFT,
    T_RIGHT,
    T_NONASSOC,
    T_TOKEN,
    T_PREC,
    T_TYPE,
    T_START,
    T_UNION,
    T_TYPENAME,
    T_EOF
} a_token;

typedef struct y_token {
    struct y_token      *next;
    token_n             value;
    char                name[1];
} y_token;

typedef struct xlat_entry {
    int                 c;
    char                *x;
} xlat_entry;

typedef struct rule_case {
    struct rule_case    *next;
    a_sym               *lhs;
    rule_n              pnum;
} rule_case;

typedef struct uniq_case {
    struct uniq_case    *next;
    char                *action;
    rule_case           *rules;
} uniq_case;

a_SR_conflict           *ambiguousstates;

int                     lineno = { 1 };

static unsigned         bufused;
static unsigned         bufmax;
static char             *buf = { NULL };

static int              ch = { ' ' };
static a_token          token;
static tok_value        value;

static unsigned long    actionsCombined;
static uniq_case        *caseActions;

static y_token          *tokens_head = NULL;
static y_token          *tokens_tail = NULL;

static char             *union_name = NULL;

static xlat_entry       xlat[] = {
    { '~',      "TILDE" },
    { '`',      "BACKQUOTE" },
    { '!',      "EXCLAMATION" },
    { '@',      "AT" },
    { '#',      "SHARP" },
    { '$',      "DOLLAR" },
    { '%',      "PERCENT" },
    { '^',      "XOR" },
    { '&',      "AND" },
    { '*',      "TIMES" },
    { '(',      "LPAREN" },
    { ')',      "RPAREN" },
    { '-',      "MINUS" },
    { '+',      "PLUS" },
    { '=',      "EQUAL" },
    { '[',      "LSQUARE" },
    { ']',      "RSQUARE" },
    { '{',      "LBRACE" },
    { '}',      "RBRACE" },
    { '\\',     "BACKSLASH" },
    { '|',      "OR" },
    { ':',      "COLON" },
    { ';',      "SEMICOLON" },
    { '\'',     "QUOTE" },
    { '"',      "DQUOTE" },
    { '<',      "LT" },
    { '>',      "GT" },
    { '.',      "DOT" },
    { ',',      "COMMA" },
    { '/',      "DIVIDE" },
    { '?',      "QUESTION" },
    { '\0',     NULL }
};

static void addbuf( int c )
{
    if( bufused == bufmax ) {
        bufmax += BUF_INCR;
        if( buf != NULL ) {
            buf = REALLOC( buf, bufmax, char );
        } else {
            buf = MALLOC( bufmax, char );
        }
    }
    buf[bufused++] = (char)c;
}

static void addstr( char *p )
{
    while( *p != '\0' ) {
        addbuf( *(unsigned char *)p );
        ++p;
    }
}

static int nextc( void )
{
    if( (ch = fgetc( yaccin )) == '\r' ) {
        ch = fgetc( yaccin );
    }
    if( ch == '\n' ) {
        ++lineno;
    }
    return( ch );
}

static bool xlat_char( bool special, int c )
{
    xlat_entry  *t;
    char        buff[16];

    if( isalpha( c ) || isdigit( c ) || c == '_' ) {
        if( special ) {
            addbuf( '_' );
        }
        addbuf( c );
        return( false );
    }
    /* NYI: add %translate 'c' XXXX in case user doesn't like our name */
    addbuf( '_' );
    for( t = xlat; t->x != NULL; ++t ) {
        if( t->c == c ) {
            addstr( t->x );
            return( true );
        }
    }
    warn( "'x' token contains unknown character '%c' (\\x%x)\n", c, c );
    addbuf( 'X' );
    sprintf( buff, "%x", c );
    addstr( buff );
    return( true );
}

static void xlat_token( void )
{
    bool special;

    addbuf( 'Y' );
    special = true;
    for( ;; ) {
        nextc();
        if( ch == EOF || ch == '\n' ) {
            msg( "invalid 'x' token" );
            break;
        }
        if( ch == '\'' )
            break;
        if( ch == '\\' ) {
            special = xlat_char( special, ch );
            nextc();
        }
        special = xlat_char( special, ch );
    }
    addbuf( '\0' );
    value.id = 0;
    token = T_IDENTIFIER;
}

static int eatcrud( void )
{
    int prev;

    for( ;; nextc() ) {
        switch( ch ) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
            break;
        case '/':
            if( nextc() != '*' ) {
                if( ch == '\n' ) {
                    --lineno;
                }
                ungetc( ch, yaccin );
                return( '/' );
            }
            for( prev = '\0'; nextc() != '/' || prev != '*'; prev = ch ) {
                if( ch == EOF ) {
                    return( ch );
                }
            }
            break;
        default:
            return( ch );
        }
    }
}

static void need( char *pat )
{
    while( *pat != '\0' ) {
        if( nextc() != *(unsigned char *)pat++ ) {
            msg( "Expected '%c'\n", pat[-1] );
        }
    }
}

static int lastc( void )
{
    if( bufused > 1 ) {
        return( (unsigned char)buf[bufused - 1] );
    }
    return( '\0' );
}

static void copybal( void )
{
    int         depth;

    depth = 1;
    do {
        addbuf( ch );
        nextc();
        if( lastc() == '/' ) {
            if( ch == '*' ) {
                /* copy a C style comment */
                for( ;; ) {
                    addbuf( ch );
                    nextc();
                    if( ch == EOF )
                        break;
                    if( ch == '/' && lastc() == '*' ) {
                        addbuf( ch );
                        nextc();
                        break;
                    }
                }
            } else if( ch == '/' ) {
                /* copy a C++ style comment */
                for( ;; ) {
                    addbuf( ch );
                    nextc();
                    if( ch == EOF )
                        break;
                    if( ch == '\n' ) {
                        addbuf( ch );
                        nextc();
                        break;
                    }
                }
            }
        }
        if( ch == '"' ) {
            /* copy a string */
            addbuf( ch );
            for( ;; ) {
                nextc();
                if( ch == EOF )
                    break;
                if( ch == '\n' ) {
                    msg( "string literal was not terminated by \" before end of line\n" );
                    break;
                }
                if( ch == '\\' ) {
                    addbuf( ch );
                    nextc();
                    addbuf( ch );
                } else {
                    if( ch == '"' )
                        break;
                    addbuf( ch );
                }
            }
        }
        if( ch == '\'' ) {
            /* copy a character constant */
            addbuf( ch );
            for( ;; ) {
                nextc();
                if( ch == EOF )
                    break;
                if( ch == '\n' ) {
                    msg( "character literal was not terminated by \" before end of line\n" );
                    break;
                }
                if( ch == '\\' ) {
                    addbuf( ch );
                    nextc();
                    addbuf( ch );
                } else {
                    if( ch == '\'' )
                        break;
                    addbuf( ch );
                }
            }
        }
        if( ch == '{' ) {
            ++depth;
        } else if( ch == '}' ) {
            --depth;
        }
    } while( depth > 0 && ch != EOF );
    addbuf( ch );
}

static a_token scan( unsigned used )
{
    bufused = used;
    eatcrud();
    if( isalpha( ch ) ) {
        for( ;; ) {
            addbuf( ch );
            nextc();
            if( isalpha( ch ) )
                continue;
            if( isdigit( ch ) )
                continue;
            if( ch == '_' )
                continue;
            if( ch == '.' )
                continue;
            if( ch == '-' )
                continue;
            break;
        }
        addbuf( '\0' );
        if( eatcrud() == ':' ) {
            nextc();
            token = T_CIDENTIFIER;
        } else {
            token = T_IDENTIFIER;
        }
        value.id = 0;
    } else if( isdigit( ch ) || ch == '-' ) {
        do {
            addbuf( ch );
        } while( isdigit( nextc() ) );
        addbuf( '\0' );
        token = T_NUMBER;
        value.number = atoi( buf );
    } else {
        switch( ch ) {
        case '\'':
            if( denseflag && !translateflag ) {
                msg( "cannot use '+' style of tokens with the dense option\n" );
            }
            if( !translateflag ) {
                addbuf( '\'' );
                nextc();
                addbuf( ch );
                if( ch == '\\' ) {
                    nextc();
                    addbuf( ch );
                    switch( ch ) {
                    case 'n':  ch = '\n'; break;
                    case 'r':  ch = '\r'; break;
                    case 't':  ch = '\t'; break;
                    case 'b':  ch = '\b'; break;
                    case 'f':  ch = '\f'; break;
                    case '\\': ch = '\\'; break;
                    case '\'': ch = '\''; break;
                    }
                }
                addbuf( '\'' );
                value.id = (unsigned char)ch;
                token = T_IDENTIFIER;
                need( "'" );
            } else {
                xlat_token();
            }
            break;
        case '{':
            token = ch;
            copybal();
            break;
        case '<': case '>': case '|': case ';': case ',':
            token = ch;
            break;
        case EOF:
            token = T_EOF;
            break;
        case '%':
            switch( nextc() ) {
            case '%':
                token = T_MARK;
                break;
            case '{':
                token = T_LCURL;
                break;
            case '}':
                token = T_RCURL;
                break;
            case 'a':
                need( "mbig" );
                token = T_AMBIG;
                break;
            case 'k':
                need( "eyword_id" );
                token = T_KEYWORD_ID;
                break;
            case 'l':
                need( "eft" );
                token = T_LEFT;
                value.assoc = L_ASSOC;
                break;
            case 'n':
                need( "onassoc" );
                token = T_NONASSOC;
                value.assoc = NON_ASSOC;
                break;
            case 'p':
                need( "rec" );
                token = T_PREC;
                break;
            case 'r':
                need( "ight" );
                token = T_RIGHT;
                value.assoc = R_ASSOC;
                break;
            case 's':
                need( "tart" );
                token = T_START;
                break;
            case 't':
                nextc();
                if( ch == 'o' ) {
                    need( "ken" );
                    token = T_TOKEN;
                } else if( ch == 'y' ) {
                    need( "pe" );
                    token = T_TYPE;
                } else {
                    msg( "Expecting %%token or %%type.\n" );
                }
                break;
            case 'u':
                need( "nion" );
                token = T_UNION;
                break;
            default:
                msg( "Unrecognized %% token.\n" );
            }
            break;
        default:
            msg( "Bad token.\n" );
        }
        addbuf( '\0' );
        nextc();
    }
    return( token );
}

static a_token scan_typename( unsigned used )
{
    bufused = used;
    if( TYPENAME_FIRST_CHAR( ch ) ) {
        do {
            addbuf( ch );
            nextc();
        } while( TYPENAME_NEXT_CHAR( ch ) );
        token = T_TYPENAME;
    }
    addbuf( '\0' );
    value.id = 0;
    return( token );
}

static char *get_typename( char *src )
{
    int             c;

    c = *(unsigned char *)src;
    if( TYPENAME_FIRST_CHAR( c ) ) {
        do {
            c = *(unsigned char *)(++src);
        } while( TYPENAME_NEXT_CHAR( c ) );
    }
    return( src );
}

static a_sym *make_sym( char *name, token_n tokval )
{
    a_sym *p;

    p = addsym( name );
    p->token = tokval;
    return( p );
}

static a_SR_conflict *make_unique_ambiguity( a_sym *sym, conflict_id id )
{
    a_SR_conflict   *ambig;

    for( ambig = ambiguousstates; ambig != NULL; ambig = ambig->next ) {
        if( ambig->id == id ) {
            if( ambig->sym != sym ) {
                msg( "ambiguity %u deals with %s, not %s.\n", id, ambig->sym->name, sym->name );
                break;
            }
            return( ambig );
        }
    }
    ambig = MALLOC( 1, a_SR_conflict );
    ambig->next = ambiguousstates;
    ambig->sym = sym;
    ambig->id = id;
    ambig->state = NULL;
    ambig->shift = NULL;
    ambig->thread = NULL;
    ambig->reduce = 0;
    ambiguousstates = ambig;
    return( ambig );
}

static void tlist_remove( char *name )
{
    y_token *curr;
    y_token *last;

    last = NULL;
    for( curr = tokens_head; curr != NULL; curr = curr->next ) {
        if( strcmp( name, curr->name ) == 0 ) {
            if( last == NULL ) {
                tokens_head = curr->next;
            } else {
                last->next = curr->next;
            }
            if( curr->next == NULL ) {
                tokens_tail = last;
            }
            FREE( curr );
            break;
        }
        last = curr;
    }
}

static void tlist_add( char *name, token_n tokval )
{
    y_token *tmp;
    y_token *curr;

    for( curr = tokens_head; curr != NULL; curr = curr->next ) {
        if( strcmp( name, curr->name ) == 0 ) {
            curr->value = tokval;
            return;
        }
    }
    tmp = (y_token *)MALLOC( strlen( name ) + sizeof( y_token ), char );
    strcpy( tmp->name, name );
    tmp->value = tokval;
    tmp->next = NULL;
    if( tokens_head == NULL ) {
        tokens_head = tmp;
    }
    if( tokens_tail != NULL ) {
        tokens_tail->next = tmp;
    }
    tokens_tail = tmp;
}

static bool scanambig( unsigned used, a_SR_conflict_list **list )
{
    bool                    absorbed_something;
    conflict_id             id;
    a_sym                   *sym;
    a_SR_conflict           *ambig;
    a_SR_conflict_list      *en;

    absorbed_something = false;
    for( ; token == T_AMBIG; ) {
        /* syntax is "%ambig <number> <token>" */
        /* token has already been scanned by scanprec() */
        if( scan( used ) != T_NUMBER || value.number < 0 ) {
            msg( "Expecting a non-negative number after %ambig.\n" );
            break;
        }
        id = value.number;
        if( scan( used ) != T_IDENTIFIER ) {
            msg( "Expecting a token name after %ambig <number>.\n" );
            break;
        }
        sym = findsym( buf );
        if( sym == NULL ) {
            msg( "Unknown token specified in %ambig directive.\n" );
            break;
        }
        if( sym->token == 0 ) {
            msg( "Non-terminal specified in %ambig directive.\n" );
            break;
        }
        scan( used );
        absorbed_something = true;
        ambig = make_unique_ambiguity( sym, id );
        en = MALLOC( 1, a_SR_conflict_list );
        en->next = *list;
        en->thread = ambig->thread;
        en->pro = NULL;
        en->conflict = ambig;
        ambig->thread = en;
        *list = en;
    }
    return( absorbed_something );
}

static bool scanprec( unsigned used, a_sym **precsym )
{
    if( token != T_PREC )
        return( false );
    if( scan( used ) != T_IDENTIFIER || (*precsym = findsym( buf )) == NULL || (*precsym)->token == 0 ) {
        msg( "Expecting a token after %prec.\n" );
    }
    scan( used );
    return( true );
}

static void scanextra( unsigned used, a_sym **psym, a_SR_conflict_list **pSR )
{
    scan( used );
    for( ;; ) {
        if( !scanprec( used, psym ) && !scanambig( used, pSR ) ) {
            break;
        }
    }
}

static char *type_name( char *type )
{
    if( type == NULL ) {
        return( "** no type **" );
    }
    return( type );
}

static void copycurl( FILE *fp )
{
    do {
        while( ch != '%' && ch != EOF ) {
            fputc( ch, fp );
            nextc();
        }
    } while( nextc() != '}' && ch != EOF );
    nextc();
}

static char *checkAttrib( char *s, char **ptype, char *buff, int *errs,
                          a_sym *lhs, a_sym **rhs, unsigned base, unsigned n )
{
    char        save;
    char        *type;
    char        *p;
    int         err_count;
    long        il;

    err_count = 0;
    ++s;
    if( *s == '<' ) {
        ++s;
        p = s;
        s = get_typename( p );
        if( p == s || *s != '>' )  {
            ++err_count;
            msg( "Bad type specifier.\n" );
        }
        save = *s;
        *s = '\0';
        type = strdup( p );
        *s = save;
        ++s;
    } else {
        type = NULL;
    }
    if( *s == '$' ) {
        strcpy( buff, "yyval" );
        if( type == NULL && lhs->type != NULL ) {
            type = strdup( lhs->type );
        }
        ++s;
    } else {
        il = n + 1;
        if( *s == '-' || isdigit( *s ) ) {
            il = strtol( s, &s, 10 );
        }
        if( il > (long)n ) {
            ++err_count;
            msg( "Invalid $ parameter (%ld).\n", il );
        }
        il -= base + 1;
        sprintf( buff, "yyvp[%ld]", il );
        if( type == NULL && il >= 0 && rhs[il]->type != NULL ) {
            type = strdup( rhs[il]->type );
        }
    }
    *ptype = type;
    *errs = err_count;
    return( s );
}

static a_pro *findPro( a_sym *lhs, rule_n pnum )
{
    a_pro       *pro;

    for( pro = lhs->pro; pro != NULL; pro = pro->next ) {
        if( pro->pidx == pnum ) {
            return( pro );
        }
    }
    return( NULL );
}

static void copyUniqueActions( FILE *fp )
{
    a_pro       *pro;
    char        *s;
    uniq_case   *c;
    uniq_case   *cnext;
    rule_case   *r;
    rule_case   *rnext;
    an_item     *item;

    for( c = caseActions; c != NULL; c = cnext ) {
        cnext = c->next;
        for( r = c->rules; r != NULL; r = rnext ) {
            rnext = r->next;
            fprintf( fp, "case %d:\n", r->pnum );
            pro = findPro( r->lhs, r->pnum );
            fprintf( fp, "/* %s <-", pro->sym->name );
            for( item = pro->items; item->p.sym != NULL; ++item ) {
                fprintf( fp, " %s", item->p.sym->name );
            }
            fprintf( fp, " */\n" );
            FREE( r );
        }
        for( s = c->action; *s != '\0'; ++s ) {
            fputc( *s, fp );
        }
        fprintf( fp, "\nbreak;\n" );
        FREE( c->action );
        FREE( c );
    }
}

static void addRuleToUniqueCase( uniq_case *p, rule_n pnum, a_sym *lhs )
{
    rule_case   *r;

    r = MALLOC( 1, rule_case );
    r->lhs = lhs;
    r->pnum = pnum;
    r->next = p->rules;
    p->rules = r;
}

static void insertUniqueAction( rule_n pnum, char *action, a_sym *lhs )
{
    uniq_case   **p;
    uniq_case   *c;
    uniq_case   *n;

    p = &caseActions;
    for( c = *p; c != NULL; c = c->next ) {
        if( strcmp( c->action, action ) == 0 ) {
            ++actionsCombined;
            addRuleToUniqueCase( c, pnum, lhs );
            /* promote to front */
            *p = c->next;
            c->next = caseActions;
            caseActions = c;
            FREE( action );
            return;
        }
        p = &(c->next);
    }
    n = MALLOC( 1, uniq_case );
    n->action = action;
    n->rules = NULL;
    n->next = *p;
    *p = n;
    addRuleToUniqueCase( n, pnum, lhs );
}

static char *strpcpy( char *d, char *s )
{
    while( (*d = *s++) != '\0' ) {
        ++d;
    }
    return( d );
}

static void lineinfo( FILE *fp )
{
    if( lineflag ) {
        fprintf( fp, "\n#line %d \"%s\"\n", lineno, srcname );
    }
}

static void copyact( FILE *fp, rule_n pnum, a_sym *lhs, a_sym **rhs, unsigned base, unsigned n )
{
    char        *action;
    char        *p;
    char        *s;
    char        *type;
    unsigned    i;
    int         errs;
    int         total_errs;
    size_t      total_len;
    char        buff[80];

    if( ! lineflag ) {
        /* we don't need line numbers to correspond to the grammar */
        total_errs = 0;
        total_len = strlen( buf ) + 1;
        for( s = buf; *s != '\0'; ) {
            if( *s == '$' ) {
                s = checkAttrib( s, &type, buff, &errs, lhs, rhs, base, n );
                total_len += strlen( buff );
                if( type != NULL ) {
                    total_len += strlen( type ) + 1;
                }
                FREE( type );
                total_errs += errs;
            } else {
                ++s;
            }
        }
        if( total_errs == 0 ) {
            action = MALLOC( total_len, char );
            p = action;
            for( s = buf; *s != '\0'; ) {
                if( *s == '$' ) {
                    s = checkAttrib( s, &type, buff, &errs, lhs, rhs, base, n );
                    p = strpcpy( p, buff );
                    if( type != NULL ) {
                        *p++ = '.';
                        p = strpcpy( p, type );
                        FREE( type );
                    }
                } else {
                    *p++ = *s++;
                }
            }
            *p = '\0';
            insertUniqueAction( pnum, action, lhs );
        }
        return;
    }
    fprintf( fp, "case %d:\n", pnum );
    fprintf( fp, "/* %s <-", lhs->name );
    for( i = 0; i < n; ++i ) {
        fprintf( fp, " %s", rhs[i]->name );
    }
    fprintf( fp, " */\n" );
    lineinfo( fp );
    for( s = buf; *s != '\0'; ) {
        if( *s == '$' ) {
            s = checkAttrib( s, &type, buff, &errs, lhs, rhs, base, n );
            fprintf( fp, "%s", buff );
            if( type != NULL ) {
                fprintf( fp, ".%s", type );
                FREE( type );
            }
        } else {
            fputc( *s++, fp );
        }
    }
    fprintf( fp, "\nbreak;\n" );
}

static char *dupbuf( void )
{
    char *str;

    str = MALLOC( bufused, char );
    memcpy( str, buf, bufused );
    bufused = 0;
    return( str );
}

void dump_header( FILE *fp )
{
    const char  *fmt;
    const char  *ttype;
    y_token     *t;
    y_token     *tmp;

    fprintf( fp, "#ifndef YYTOKENTYPE\n" );
    fprintf( fp, "#define YYTOKENTYPE yytokentype\n" );
    if( fastflag || bigflag || compactflag ) {
        ttype = "unsigned short";
    } else {
        ttype = "unsigned char";
    }
    if( enumflag ) {
        fprintf( fp, "typedef enum yytokentype {\n" );
        fmt = "\t%-20s = 0x%02x,\n";
    } else {
        fmt = "#define %-20s 0x%02x\n";
    }
    t = tokens_head;
    while( t != NULL ) {
        fprintf( fp, fmt, t->name, t->value );
        tmp = t;
        t = t->next;
        FREE( tmp );
    }
    if( enumflag ) {
        fprintf( fp, "\tYTOKEN_ENUMSIZE_SETUP = (%s)-1\n", ttype );
        fprintf( fp, "} yytokentype;\n" );
    } else {
        fprintf( fp, "typedef %s yytokentype;\n", ttype );
    }
    fprintf( fp, "#endif\n" );
    fflush( fp );
}

void close_header( FILE *fp )
{
    if( union_name != NULL ) {
        fprintf( fp, "typedef union %s YYSTYPE;\n", union_name );
        fprintf( fp, "extern YYSTYPE yylval;\n" );
        FREE( union_name );
    }
    fclose( fp );
}

void defs( FILE *fp )
{
    token_n     gentoken;
    a_sym       *sym;
    a_token     ctype;
    char        *type;
    a_prec      prec;

    eofsym = make_sym( "$eof", TOKEN_EOF );
    nosym = make_sym( "$impossible", TOKEN_IMPOSSIBLE );
    errsym = make_sym( "error", TOKEN_ERROR );
    if( denseflag ) {
        gentoken = TOKEN_DENSE_BASE;
    } else {
        gentoken = TOKEN_SPARSE_BASE;
    }
    scan( 0 );
    prec.prec = 0;
    prec.assoc = NON_ASSOC;
    for( ; token != T_MARK; ) {
        switch( token ) {
        case T_START:
            if( scan( 0 ) != T_IDENTIFIER )
                msg( "Identifier needed after %%start.\n" );
            startsym = addsym( buf );
            scan( 0 );
            break;
        case T_UNION:
            if( scan( 0 ) != '{' ) {
                msg( "Need '{' after %%union.\n" );
            }
            fprintf( fp, "typedef union " );
            lineinfo( fp );
            fprintf( fp, "%s YYSTYPE;\n", buf );
            if( union_name == NULL ) {
                union_name = MALLOC( strlen( buf ) + 1, char );
                strcpy( union_name, buf );
            } else {
                msg( "%union already defined\n" );
            }
            scan( 0 );
            break;
        case T_LCURL:
            lineinfo( fp );
            copycurl( fp );
            scan( 0 );
            break;
        case T_KEYWORD_ID:
            switch( scan( 0 ) ) {
            case T_IDENTIFIER:
                sym = addsym( buf );
                if( sym->token == 0 ) {
                    msg( "Token must be assigned number before %keyword_id\n" );
                }
                value.id = sym->token;
                break;
            case T_NUMBER:
                break;
            default:
                msg( "Expecting identifier or number.\n" );
            }
            keyword_id_low = value.id;
            switch( scan( 0 ) ) {
            case T_IDENTIFIER:
                sym = addsym( buf );
                if( sym->token == 0 ) {
                    msg( "Token must be assigned number before %keyword_id\n" );
                }
                value.id = sym->token;
                break;
            case T_NUMBER:
                break;
            default:
                msg( "Expecting identifier or number.\n" );
            }
            keyword_id_high = value.id;
            scan( 0 );
            break;
        case T_LEFT:
        case T_RIGHT:
        case T_NONASSOC:
            ++prec.prec;
            prec.assoc = value.assoc;
            /* fall through */
        case T_TOKEN:
        case T_TYPE:
            ctype = token;
            if( scan( 0 ) == '<' ) {
                if( scan_typename( 0 ) != T_TYPENAME ) {
                    msg( "Expecting type specifier.\n" );
                }
                type = dupbuf();
                if( scan( 0 ) != '>' ) {
                    msg( "Expecting '>'.\n" );
                }
                scan( 0 );
            } else {
                type = NULL;
            }
            while( token == T_IDENTIFIER ) {
                sym = addsym( buf );
                if( type != NULL ) {
                    if( sym->type != NULL ) {
                        if( strcmp( sym->type, type ) != 0 ) {
                            msg( "'%s' type redeclared from '%s' to '%s'\n", buf, sym->type, type );
                            FREE( sym->type );
                            sym->type = strdup( type );
                        }
                    } else {
                        sym->type = strdup( type );
                    }
                }
                if( ctype == T_TYPE ) {
                    scan( 0 );
                } else {
                    if( sym->token == 0 ) {
                        sym->token = value.id;
                    }
                    if( ctype != T_TOKEN ) {
                        sym->prec = prec;
                    }
                    if( scan( 0 ) == T_NUMBER ) {
                        if( sym->token != 0 ) {
                            if( sym->name[0] != '\'' ) {
                                tlist_remove( sym->name );
                            }
                        }
                        sym->token = (token_n)value.number;
                        scan( 0 );
                    }
                    if( sym->token == 0 ) {
                        sym->token = gentoken++;
                    }
                    if( sym->name[0] != '\'' ) {
                        tlist_add( sym->name, sym->token );
                    }
                }
                if( token == ',' ) {
                    scan( 0 );
                }
            }
            if( type != NULL )
                FREE( type );
            break;
        default:
            msg( "Incorrect syntax.\n" );
        }
    }
    scan( 0 );
}

void rules( FILE *fp )
{
    a_sym               *lhs, *sym, *precsym;
    a_sym               **rhs;
    unsigned            nrhs;
    unsigned            maxrhs;
    a_pro               *pro;
    char                buffer[20];
    unsigned            i;
    int                 numacts;
    bool                action_defined;
    bool                unit_production;
    bool                not_token;
    a_SR_conflict_list  *list_of_ambiguities;
    a_SR_conflict_list  *ambig;

    ambiguousstates = NULL;
    maxrhs = INIT_RHS_SIZE;
    rhs = CALLOC( maxrhs, a_sym * );
    while( token == T_CIDENTIFIER ) {
        int sym_lineno = lineno;
        lhs = addsym( buf );
        if( lhs->token != 0 )
            msg( "%s used on lhs.\n", lhs->name );
        if( startsym == NULL )
            startsym = lhs;
        numacts = 0;
        do {
            action_defined = false;
            precsym = NULL;
            list_of_ambiguities = NULL;
            nrhs = 0;
            scanextra( 0, &precsym, &list_of_ambiguities );
            for( ; token == '{' || token == T_IDENTIFIER; ) {
                if( token == '{' ) {
                    i = bufused;
                    scanextra( bufused, &precsym, &list_of_ambiguities );
                    numacts++;
                    if( token == '{' || token == T_IDENTIFIER ) {
                        sprintf( buffer, "$pro%d", npro );
                        sym = addsym( buffer );
                        copyact( fp, npro, sym, rhs, nrhs, nrhs );
                        addpro( sym, rhs, 0 );
                    } else {
                        copyact( fp, npro, lhs, rhs, 0, nrhs );
                        action_defined = true;
                        break;
                    }
                    bufused -= i;
                    memcpy( buf, &buf[i], bufused );
                } else {
                    sym = addsym( buf );
                    if( value.id != 0 ) {
                        sym->token = value.id;
                    }
                    if( sym->token != 0 )
                        precsym = sym;
                    scanextra( 0, &precsym, &list_of_ambiguities );
                }
                if( nrhs + 1 > maxrhs ) {
                    maxrhs *= 2;
                    rhs = REALLOC( rhs, maxrhs, a_sym * );
                }
                rhs[nrhs++] = sym;
            }
            unit_production = false;
            if( !action_defined ) {
                if( nrhs > 0 ) {
                    /* { $$ = $1; } is default action */
                    if( defaultwarnflag ) {
                        char *type_lhs = type_name( lhs->type );
                        char *type_rhs = type_name( rhs[0]->type );
                        if( strcmp( type_rhs, type_lhs ) != 0 ) {
                            warn("default action would copy '%s <%s>' to '%s <%s>'\n",
                                rhs[0]->name, type_rhs, lhs->name, type_lhs );
                        }
                    }
                    if( nrhs == 1 ) {
                        unit_production = true;
                    }
                } else {
                    if( sym_lineno == lineno && token == '|' ) {
                        warn( "unexpected epsilon reduction for '%s'?\n", lhs->name );
                    }
                }
            }
            pro = addpro( lhs, rhs, nrhs );
            if( unit_production ) {
                pro->unit = true;
            }
            if( precsym != NULL ) {
                pro->prec = precsym->prec;
            }
            if( list_of_ambiguities ) {
                for( ambig = list_of_ambiguities; ambig != NULL; ambig = ambig->next ) {
                    ambig->pro = pro;
                }
                pro->SR_conflicts = list_of_ambiguities;
            }
            if( token == ';' ) {
                do {
                    scan( 0 );
                } while( token == ';' );
            } else if( token != '|' ) {
                if( token == T_CIDENTIFIER ) {
                    msg( "Missing ';'\n" );
                } else {
                    msg( "Incorrect syntax.\n" );
                }
            }
        } while( token == '|' );
    }
    FREE( rhs );

    not_token = false;
    for( sym = symlist; sym != NULL; sym = sym->next ) {
        /* check for special symbols */
        if( sym == eofsym )
            continue;
        if( denseflag ) {
            if( sym->token != 0 && sym->token < TOKEN_DENSE_BASE ) {
                continue;
            }
        } else {
            if( sym->token != 0 && sym->token < TOKEN_SPARSE_BASE ) {
                continue;
            }
        }
        if( sym->pro != NULL && sym->token != 0 ) {
            not_token = true;
            warn( "%s not defined as '%%token'.\n", sym->name );
        }
    }
    if( not_token ) {
        msg( "cannot continue (because of %%token problems)\n" );
    }
    copyUniqueActions( fp );
}

static void copyfile( FILE *fp )
{
    do {
        fputc( ch, fp );
    } while( nextc() != EOF );
}

void tail( FILE *fp )
{
    if( token == T_MARK ) {
        copyfile( fp );
    } else if( token != T_EOF ) {
        msg( "Expected end of file.\n" );
    }
}

void parsestats( void )
{
    dumpstatistic( "actions combined", actionsCombined );
}
