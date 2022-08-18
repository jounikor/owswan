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
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include "yacc.h"
#include "alloc.h"

static void putnum( FILE *fp, char *name, int i )
{
    fprintf( fp, "#define\t%-20s\t%d\n", name, i );
}

static void preamble( FILE *fp )
{
    int         i;

    putnum( fp, "SHIFT", -1 );
    putnum( fp, "ERROR", -2 );
    fprintf( fp, "\n"
                     "#ifndef YYSTYPE\n"
                     "    #define YYSTYPE int\n"
                     "#endif\n"
                     "\n"
                     "typedef struct parse_stack {\n"
                     "    int (YYNEAR *state) (struct parse_stack *, unsigned );\n"
                     "    YYSTYPE v;\n"
                     "} parse_stack;\n\n"
                     "\n"
                     );

    for( i = 0; i < nstate; ++i ) {
        fprintf( fp, "static int YYNEAR state%d( struct parse_stack *, unsigned );\n", i );
    }
}

static void prolog( FILE *fp, int i )
{
    a_name              name;
    a_state *           x;

    fprintf( fp, "\nint YYNEAR state%d( parse_stack * yysp, unsigned token )\n/*\n", i );
    x = statetab[i];
    for( name.item = x->name.item; *name.item != NULL; ++name.item ) {
        showitem( fp, *name.item, " ." );
    }
    fprintf( fp, "*/\n{\n" );
}

static void copyact( a_pro * pro, char * indent )
{
    char        *s;
    char        *type;
    int         i;
    a_sym *     lhs;
    an_item *   rhs;
    unsigned    n;
    int         only_default_type;

    if( pro->action == NULL )
        return;   // Default action is noop
    lhs = pro->sym;
    rhs = pro->item;
    fprintf( fp, "%s/* %s <-", indent, lhs->name );
    for( n = 0; rhs[n].p.sym != NULL; ++n ) {
        fprintf( fp, " %s", rhs[n].p.sym->name );
    }
    fprintf( fp, " */\n%s{ YYSTYPE yyval;\n%s\t", indent, indent );
    only_default_type = true;
    for( s = pro->action; *s != '\0'; ) {
        if( *s == '$' ) {
            if( *++s == '<' ) {
                for( type = ++s; *s != '>'; ++s ) {
                    if( *s == '\0' ) {
                        msg( "Bad type specifier.\n" );
                    }
                }
                *s++ = '\0';
                only_default_type = false;
            } else {
                type = NULL;
            }
            if( *s == '$' ) {
                fprintf( fp, "yyval", *s );
                if( type == NULL ) {
                    type = lhs->type;
                }
                ++s;
            } else {
                if( *s == '-' || isdigit( *s ) ) {
                    i = strtol( s, &s, 10 );
                }
                if( i >= 0 && i > n ) {
                    msg( "Invalid $ parameter.\n" );
                }
                fprintf( fp, "yysp[%d].v", i - 1 ); // Adjust yysp first
                if( type == NULL && i >= 1 ) {
                    type = rhs[i - 1].p.sym->type;
                }
            }
            if( type != NULL ) {
                fprintf( fp, ".%s", type );
            }
        } else if( *s == '\n' ) {
            fprintf( fp, "\n\t%s", indent );
            ++s;
        } else {
            fputc( *s++, fp );
        }
    }
    type = lhs->type;
    if( only_default_type && type != NULL ) {
        fprintf( fp, "\n\t%syysp[0].v.%s = yyval.%s;", indent, type, type );
    } else {
        fprintf( fp, "\n\t%syysp[0].v = yyval;", indent );
    }
    fprintf( fp, "\n%s};\n", indent );
}

static a_state * unique_shift( a_pro * reduced )
{
    // See if there is a unique shift when this state is reduced

    a_state *           shift_to;
    a_state *           test;
    a_shift_action *    tx;
    int                 i;

    shift_to = NULL;
    for( i = 0; i < nstate; ++i ) {
        test = statetab[i];
        for( tx = test->trans; tx->sym != NULL; ++tx ) {
            if( tx->sym == reduced->sym ) {
                // Found something that uses this lhs
                if( shift_to == NULL || shift_to == tx->state ) {
                    // This is the first one or it matches the first one
                    shift_to = tx->state;
                } else {
                    return( NULL );     // Not unique
                }
            }
        }
    }
    return( shift_to );
}

static void reduce( FILE *fp, int production, int error )
{
    int                 plen;
    an_item *           item;
    a_pro *             pro;
    a_state *           shift_to;

    if( production == error ) {
        fprintf( fp, "\treturn( ERROR );\n" );
    } else {
        production -= nstate;           // Convert to 0 base
        pro = protab[production];
        for( item = pro->item, plen = 0; item->p.sym != NULL; ++item ) {
            ++plen;
        }
        if( plen != 0 ) {
            fprintf( fp, "\tyysp -= %d;\n", plen );
        }
        copyact( pro, "\t" );
        // fprintf( fp, "\tactions( %d, yysp );\n", production );
        if( (shift_to = unique_shift( pro )) != NULL ) {
            fprintf( fp, "\tyysp[0].state = state%d;\n", shift_to->sidx );
        } else {
            fprintf( fp, "\t(*yysp[-1].state) ( yysp, %d );\n", pro->sym->token );
        }
        fprintf( fp, "\treturn( %d );\n", plen );
    }
}

static void epilog( FILE *fp )
{
    fprintf( fp, "}\n" );
}

static void gencode( FILE *fp, int statenum, short *toklist, short *s, short *actions,
                        short default_token, short parent_token, short error )
{
    short               default_action;
    short               todo;
    short               token;
    int                 symnum;
    int                 switched;

    prolog( fp, statenum );
    default_action = 0;
    switched = false;
    for( ; toklist < s; ++toklist ) {
        token = *toklist;
        todo = actions[token] ;
        if( token == default_token ) {
            default_action = todo;
        } else if( token != parent_token ) {
            if( ! switched ) {
                fprintf( fp, "    switch( token ) {\n" );
                switched = true;
            }

            for( symnum = 0; symnum < nsym; ++symnum ) {
                if( symtab[symnum]->token == token ) {
                    break;
                }
            }
            if( symnum == nsym ) {
                fprintf( fp, "    case %d:\n", token );
            } else if( symtab[symnum]->name[0] == '\'' ) {
                fprintf( fp, "    case %s:\n", symtab[symnum]->name );
            } else {
                fprintf( fp, "    case %d: /* %s */\n", token, symtab[symnum]->name );
            }
            if( todo >= nstate ) {
                // Reduction or error
                reduce( fp, todo, error );
            } else {
                // Shift
                fprintf( fp, "\tyysp[0].state = state%d;\n", todo );
                fprintf( fp, "\tbreak;\n" );
            }
        }
    }
    if( switched ) {
        fprintf( fp, "    default: ;\n" );
    }
    todo = actions[parent_token];
    if( todo != error ) {
        // There is a parent production
        // For now, try parents only when there is no default action
        fprintf( fp, "\treturn( state%d( yysp, token ) );\n", todo );
    } else if( default_action != 0 ) {
        reduce( default_action, error );
    } else {
        fprintf( fp, "\treturn( ERROR );\n" );
    }
    if( switched ) {
        fprintf( fp, "    }\n    return( SHIFT );\n" );
    }
    epilog();
}

static void putambig( FILE *fp, int i, int state, int token )
{
    fprintf( fp, "#define\tYYAMBIGS%u\t\t%d\n", i, state );
    fprintf( fp, "#define\tYYAMBIGT%u\t\t%d\n", i, token );
}

static void print_token( int token )
{
    int symnum;

    for( symnum = 0; symnum < nsym; ++symnum ) {
        if( symtab[symnum]->token == token ) {
            break;
        }
    }
    if( symnum == nsym ) {
        printf( " %d", token );
    } else {
        printf( " %s", symtab[symnum]->name );
    }
}

void genobj( FILE *fp )
{
    short *token, *actions, *base, *other, *parent, *size;
    short *p, *q, *r, *s;
    short error, tokval, redun, *test, *best;
#if 1
    short *same, *diff;
#endif
    set_size *mp;
    a_sym *sym;
    a_pro *pro;
    a_state *x;
    a_shift_action *tx;
    a_reduce_action *rx;
    int i, j, ntoken, dtoken, ptoken;
    unsigned num_default, num_parent;
    set_size max_savings;
    set_size savings;

    num_default = num_parent = 0;
    ntoken = FirstNonTerminalTokenValue();
    dtoken = ntoken++;
    ptoken = ntoken++;
    for( i = nterm; i < nsym; ++i ) {
        symtab[i]->token = ntoken++;
    }

    error = nstate + npro;
    actions = CALLOC( ntoken, short );
    for( i = 0; i < ntoken; ++i ) {
        actions[i] = error;
    }
    preamble( fp );
    token = CALLOC( ntoken, short );
    test = CALLOC( ntoken, short );
    best = CALLOC( ntoken, short );
    base = CALLOC( nstate, short );
    other = CALLOC( nstate, short );
    parent = CALLOC( nstate, short );
    size = CALLOC( nstate, short );
    for( i = nstate; --i >= 0; ) {
        for( s = actions + ntoken; --s >= actions; ) {
            *s = error;
        }
        x = statetab[i];
        q = token;
        for( tx = x->trans; (sym = tx->sym) != NULL; ++tx ) {
            tokval = sym->token;
            *q++ = tokval;
            actions[tokval] = tx->state->sidx;
        }
        max_savings = 0;
        for( rx = x->redun; (pro = rx->pro) != NULL; ++rx ) {
            if( (savings = (mp = Members( rx->follow )) - setmembers) == 0 )
                continue;
            redun = pro->pidx + nstate;
            if( max_savings < savings ) {
                max_savings = savings;
                r = q;
            }
            protab[pro->pidx]->used = true;
            while( mp-- != setmembers ) {
                tokval = symtab[*mp]->token;
                *q++ = tokval;
                actions[tokval] = redun;
            }
        }
        if( max_savings ) {
            tokval = other[i] = actions[*r];
            *q++ = dtoken;
            actions[dtoken] = tokval;
            p = r;
            while( max_savings-- > 0 )
                actions[*p++] = error;
            while( p < q )
                *r++ = *p++;
            q = r;
            ++num_default;
        } else {
            other[i] = error;
        }
        r = q;
        size[i] = r - token;
        max_savings = 0;
        parent[i] = nstate;
        for( j = nstate; --j > i; ) {
            // FOR NOW -- only use parent if no default here or same default
            if( other[i] != error && other[i] != other[j] )
                continue;
            savings = 0;
            x = statetab[j];
            p = test;
            q = test + ntoken;
            for( tx = x->trans; (sym = tx->sym) != NULL; ++tx )
                if( actions[sym->token] == tx->state->sidx ) {
                    ++savings;
                    *p++ = sym->token;
                } else {
                    if( actions[sym->token] == error )
                        --savings;
                    *--q = sym->token;
                }
            for( rx = x->redun; (pro = rx->pro) != NULL; ++rx ) {
                if( (redun = pro->pidx + nstate) == other[j] )
                    redun = error;
                redun = pro->pidx + nstate;
                for( mp = Members( rx->follow ); mp-- != setmembers; ) {
                    tokval = symtab[*mp]->token;
                    if( actions[tokval] == redun ) {
                        ++savings;
                        *p++ = tokval;
                    } else {
                        if( actions[tokval] == error )
                            --savings;
                        *--q = tokval;
                    }
                }
            }
            if( other[j] != error ) {
                if( other[j] == other[i] ) {
                    ++savings;
                    *p++ = dtoken;
                } else {
                    *--q = dtoken;
                }
            }
#if 0
            printf( "state %d calling state %d saves %d:", i, j, savings );
            for( s = test; s < p; ++s ) {
                print_token( *s );
            }
            printf( " costs" );
            for( s = test + ntoken; --s >= q; ) {
                if( actions[*s] == error ) {
                    print_token( *s );
                }
            }
            printf( "\n" );
#endif
            if( max_savings < savings ) {
                max_savings = savings;
                same = p;
                diff = q;
                s = test;  test = best;  best = s;
                parent[i] = j;
            }
        }
        if( max_savings < 1 ) { // Could raise threshold for performance
            s = r;
        } else {
            ++num_parent;
            s = token;
            p = same;
            while( --p >= best )
                actions[*p] = error;
            for( q = token; q < r; ++q ) {
                if( actions[*q] != error ) {
                    *s++ = *q;
                }
            }
            p = best + ntoken;
            while( --p >= diff ) {
                if( actions[*p] == error ) {
                    *s++ = *p;
                }
            }
            tokval = parent[i];
            *s++ = ptoken;
            actions[ptoken] = tokval;
        }
        gencode( fp, i, token, s, actions, dtoken, ptoken, error );
        while( --s >= token ) {
            actions[*s] = error;
        }
    }
    for( i = 0; i < nambig; ++i ) {
        putambig( fp, i, base[ambiguities[i].state], ambiguities[i].token );
    }
    putnum( fp, "YYNOACTION", error - nstate + dtoken );
    putnum( fp, "YYEOFTOKEN", eofsym->token );
    putnum( fp, "YYERRTOKEN", errsym->token );
    putnum( fp, "YYERR", errstate->sidx );
    fprintf( fp, "#define YYSTART   state%d\n", startstate->sidx );
    fprintf( fp, "#define YYSTOP    state%d\n", eofsym->enter->sidx );
    printf( "%u states, %u with defaults, %u with parents\n", nstate, num_default, num_parent );
}
