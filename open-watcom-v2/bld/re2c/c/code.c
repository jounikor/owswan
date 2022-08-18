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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "substr.h"
#include "globals.h"
#include "dfa.h"
#include "parser.h"

// there must be at least one span in list;  all spans must cover
// same range

typedef struct BitMap {
    Go              *go;
    State           *on;
    struct BitMap   *next;
    uint            i;
    uchar           m;
} BitMap;

typedef struct SCC {
    State       **top, **stk;
} SCC;

static BitMap       *BitMap_first = NULL;
static const uint   cInfinity = ~0U;

static char *prtCh( uchar c )
{
    static char b[5];

    switch( c ) {
    case '\'': return( "\\'" );
    case '\n': return( "\\n" );
    case '\t': return( "\\t" );
    case '\v': return( "\\v" );
    case '\b': return( "\\b" );
    case '\r': return( "\\r" );
    case '\f': return( "\\f" );
    case '\a': return( "\\a" );
    case '\\': return( "\\\\" );
    default:
        if( isprint( c ) ) {
            b[0] = c;
            b[1] = '\0';
        } else {
            b[0] = '\\';
            b[1] = ( ( c / 64 ) % 8 ) + '0';
            b[2] = ( ( c / 8 ) % 8 ) + '0';
            b[3] = ( c % 8 ) + '0';
            b[4] = '\0';
        }
        return( b );
    }
}

static void Go_unmap( Go *g, Go *base, State *x )
{
    Span *s;
    Span *b;
    Span *e;
    uint lb;

    s = g->span;
    b = base->span;
    e = &b[base->nSpans];
    lb = 0;
    s->ub = 0;
    s->to = NULL;
    for( ; b != e; ++b ) {
        if( b->to == x ) {
            if( ( s->ub - lb ) > 1 ) {
                s->ub = b->ub;
            }
        } else {
            if( b->to != s->to ) {
                if( s->ub ) {
                    lb = s->ub;
                    ++s;
                }
                s->to = b->to;
            }
            s->ub = b->ub;
        }
    }
    s->ub = e[-1].ub;
    ++s;
    g->nSpans = s - g->span;
}

static void doGen( Go *g, State *s, uchar *bm, uchar m )
{
    Span *b;
    Span *e;
    uint lb;

    b = g->span;
    e = &b[g->nSpans];
    lb = 0;
    for( ; b < e; ++b ) {
        if( b->to == s ) {
            for( ; lb < b->ub; ++lb ) {
                bm[lb] |= m;
            }
        }
        lb = b->ub;
    }
}

static bool matches( Go *g1, State *s1, Go *g2, State *s2 )
{
    Span *b1;
    Span *e1;
    uint lb1;
    Span *b2;
    Span *e2;
    uint lb2;

    b1 = g1->span;
    e1 = &b1[g1->nSpans];
    lb1 = 0;
    b2 = g2->span;
    e2 = &b2[g2->nSpans];
    lb2 = 0;
    for( ;; ) {
        for( ; b1 < e1 && b1->to != s1; ++b1 )
            lb1 = b1->ub;
        for( ; b2 < e2 && b2->to != s2; ++b2 )
            lb2 = b2->ub;
        if( b1 == e1 )
            return( b2 == e2 );
        if( b2 == e2 )
            return( false );
        if( lb1 != lb2 || b1->ub != b2->ub )
            return( false );
        ++b1; ++b2;
    }
}

static BitMap *BitMap_new( Go *g, State *x )
{
    BitMap  *b;

    b = malloc( sizeof( BitMap ) );
    b->go = g;
    b->on = x;
    b->next = BitMap_first;
    BitMap_first = b;
    return( b );
}

static BitMap *BitMap_find_go( Go *g, State *x )
{
    BitMap  *b;

    for( b = BitMap_first; b != NULL; b = b->next ) {
        if( matches( b->go, b->on, g, x ) ) {
            return( b );
        }
    }
    return( BitMap_new( g, x ) );
}

static BitMap *BitMap_find( State *x )
{
    BitMap  *b;

    for( b = BitMap_first; b != NULL; b = b->next ) {
        if( b->on == x ) {
            return( b );
        }
    }
    return( NULL );
}

static void BitMap_gen( FILE *o, uint lb, uint ub )
{
    BitMap *b;

    b = BitMap_first;
    if( b != NULL ) {
        uint    n;
        uchar   *bm;
        uint    i;
        uint    j;
        uchar   m;

        n = ub - lb;
        bm = malloc( n );
        fputs( "\tstatic unsigned char yybm[] = {", o );
        memset( bm, 0, n );
        for( i = 0; b != NULL; i += n ) {
            for( m = 0x80; b != NULL && m; b = b->next, m >>= 1 ) {
                b->i = i; b->m = m;
                doGen( b->go, b->on, &bm[-lb], m );
            }
            for( j = 0; j < n; ++j ) {
                if( j % 8 == 0 ) {
                    fputs( "\n\t", o );
                    ++oline;
                }
                fprintf( o, "%3u, ", (uint)bm[j] );
            }
        }
        fputs( "\n\t};\n", o );
        oline += 2;
        free( bm );
    }
}

static void genGoTo( FILE *o, State *to )
{
    fprintf( o, "\tgoto yy%u;\n", to->label );
    ++oline;
}

static void genIf( FILE *o, char *cmp, uint v )
{
    fprintf( o, "\tif(yych %s '%s')", cmp, prtCh( v ) );
}

static void indent( FILE *o, uint i )
{
    while( i-- > 0 ) {
        fputc( '\t', o );
    }
}

static void need( FILE *o, uint n )
{
    if( n == 1 ) {
        fputs( "\tif(YYLIMIT == YYCURSOR) YYFILL(1);\n", o );
    } else {
        fprintf( o, "\tif((YYLIMIT - YYCURSOR) < %d) YYFILL(%d);\n", n, n );
    }
    ++oline;
    fputs( "\tyych = *YYCURSOR;\n", o );
    ++oline;
}

static void Action_emit( Action *a, FILE *o )
{
    uint    i;

    switch( a->type ) {
    case MATCHACT:
        if( a->state->link != NULL ) {
            fputs( "\t++YYCURSOR;\n", o );
            need( o, a->state->depth );
        } else {
            fputs("\tyych = *++YYCURSOR;\n", o);
        }
        oline++;
        break;
    case ENTERACT:
        if( a->state->link != NULL ) {
            need( o, a->state->depth );
        }
        break;
    case SAVEMATCHACT:
        fprintf(o, "\tyyaccept = %u;\n", a->u.SaveMatch.selector);
        oline++;
        if( a->state->link != NULL ) {
            fputs("\tYYMARKER = ++YYCURSOR;\n", o);
            oline++;
            need( o, a->state->depth );
        } else {
            fputs("\tyych = *(YYMARKER = ++YYCURSOR);\n", o);
            oline++;
        }
        break;
    case MOVEACT:
        break;
    case ACCEPTACT:
        {
            bool    first = true;

            for( i = 0; i < a->u.Accept.nRules; ++i ) {
                if( a->u.Accept.saves[i] != ~0u ) {
                    if( first ) {
                        first = false;
                        fputs( "\tYYCURSOR = YYMARKER;\n\tswitch(yyaccept){\n", o );
                        oline += 2;
                    }
                    fprintf( o, "\tcase %u:", a->u.Accept.saves[i] );
                    genGoTo( o, a->u.Accept.rules[i] );
                }
            }
            if( !first ) {
                fputs( "\t}\n", o );
                oline++;
            }
        }
        break;
    case RULEACT:
        {
            uint    back = RegExp_fixedLength( a->u.Rule.rule->u.RuleOp.ctx );

            if( back != ~0 && back > 0 )
                fprintf( o, "\tYYCURSOR -= %u;", back );
            fputc( '\n', o );
            oline++;
            if( !iFlag ) {
                fprintf( o, "#line %u\n", a->u.Rule.rule->u.RuleOp.code->line );
                oline++;
            }
            fputc( '\t', o );
            SubStr_out( &a->u.Rule.rule->u.RuleOp.code->text, o );
            fputc( '\n', o );
            oline++;
        }
        break;
    }
}

static void doLinear( FILE *o, uint i, Span *s, uint n, State *next )
{
    for( ;; ) {
        State *bg = s[0].to;
        while( n >= 3 && s[2].to == bg && ( s[1].ub - s[0].ub ) == 1 ) {
            if( s[1].to == next && n == 3 ) {
                indent( o, i ); genIf( o, "!=", s[0].ub ); genGoTo( o, bg );
                return;
            } else {
                indent( o, i ); genIf( o, "==", s[0].ub ); genGoTo( o, s[1].to );
            }
            n -= 2; s += 2;
        }
        if( n == 1 ) {
            if( bg != next ) {
                indent( o, i); genGoTo(o, s[0].to );
            }
            return;
        } else if( n == 2 && bg == next ) {
            indent( o, i ); genIf( o, ">=", s[0].ub ); genGoTo( o, s[1].to );
            return;
        } else {
            indent( o, i ); genIf( o, ( s[0].ub > 1 ) ? "<=" : "==", s[0].ub - 1 ); genGoTo( o, bg );
            n -= 1; s += 1;
        }
    }
}

static void Go_genLinear( Go *g, FILE *o, State *next )
{
    doLinear( o, 0, g->span, g->nSpans, next );
}

static void genCases( FILE *o, uint lb, Span *s )
{
    if( lb < s->ub ) {
        for( ;; ) {
            fprintf( o, "\tcase '%s':", prtCh( lb ) );
            if( ++lb == s->ub )
                break;
            fputc( '\n', o );
            oline++;
        }
    }
}

static void Go_genSwitch( Go *g, FILE *o, State *next )
{
    uint    i;

    if( g->nSpans <= 2 ) {
        Go_genLinear( g, o, next );
    } else {
        State   *def = g->span[g->nSpans - 1].to;
        Span    **r, **s, **t;
        Span    **sP = malloc( ( g->nSpans - 1 ) * sizeof( Span * ) );

        t = &sP[0];
        for( i = 0; i < g->nSpans; ++i ) {
            if( g->span[i].to != def ) {
                *(t++) = &g->span[i];
            }
        }
        fputs( "\tswitch(yych){\n", o );
        ++oline;
        while( t != &sP[0] ) {
            State   *to;

            r = s = &sP[0];
            if( *s == &g->span[0] ) {
                genCases( o, 0, *s );
            } else {
                genCases( o, (*s)[-1].ub, *s );
            }
            to = (*s)->to;
            while( ++s < t ) {
                if( (*s)->to == to ) {
                    genCases( o, (*s)[-1].ub, *s );
                } else {
                    *(r++) = *s;
                }
            }
            genGoTo( o, to );
            t = r;
        }
        fputs( "\tdefault:", o );
        genGoTo(o, def);
        fputs( "\t}\n", o );
        ++oline;
        free( sP );
    }
}

static void doBinary( FILE *o, uint i, Span *s, uint n, State *next )
{
    if( n <= 4 ) {
        doLinear( o, i, s, n, next );
    } else {
        uint    h = n/2;

        indent( o, i ); genIf( o, "<=", s[h-1].ub - 1 ); fputs( "{\n", o ); ++oline;
        doBinary( o, i + 1, &s[0], h, next );
        indent( o, i ); fputs( "\t} else {\n", o ); ++oline;
        doBinary( o, i + 1, &s[h], n - h, next );
        indent( o, i ); fputs( "\t}\n", o ); ++oline;
    }
}

static void Go_genBinary( Go *g, FILE *o, State *next )
{
    doBinary( o, 0, g->span, g->nSpans, next );
}

static void Go_genBase( Go *g, FILE *o, State *next )
{
    if( g->nSpans == 0 )
        return;
    if( !sFlag ) {
        Go_genSwitch( g, o, next );
        return;
    }
    if( g->nSpans > 8 ) {
        Span *bot = &g->span[0], *top = &g->span[g->nSpans - 1];
        uint util;
        if( bot[0].to == top[0].to ) {
            util = ( top[-1].ub - bot[0].ub ) / ( g->nSpans - 2 );
        } else {
            if( bot[0].ub > ( top[0].ub - top[-1].ub ) ) {
                util = ( top[0].ub - bot[0].ub ) / ( g->nSpans - 1 );
            } else {
                util = top[-1].ub / ( g->nSpans - 1 );
            }
        }
        if( util <= 2 ) {
            Go_genSwitch( g, o, next );
            return;
        }
    }
    if( g->nSpans > 5 ) {
        Go_genBinary( g, o, next );
    } else {
        Go_genLinear( g, o, next );
    }
}

static void Go_genGoto( Go *g, FILE *o, State *next )
{
    uint    i;

    if( bFlag ) {
        for( i = 0; i < g->nSpans; ++i ) {
            State *to = g->span[i].to;
            if( to != NULL && to->isBase ) {
                BitMap *b = BitMap_find( to );
                if( b != NULL && matches( b->go, b->on, g, to ) ) {
                    Go go;
                    go.span = malloc( g->nSpans * sizeof( Span ) );
                    Go_unmap( &go, g, to );
                    fprintf( o, "\tif(yybm[%u+yych] & %u)", b->i, (uint)b->m );
                    genGoTo( o, to );
                    Go_genBase( &go, o, next );
                    free( go.span );
                    return;
                }
            }
        }
    }
    Go_genBase( g, o, next );
}

static void State_emit( State *s, FILE *o )
{
    if( s->referenced ) {
        fprintf( o, "yy%u: ", s->label );
    }
    Action_emit( s->action, o );
}

static uint merge( Span *x0, State *fg, State *bg )
{
    Span *x;
    Span *f;
    Span *b;
    uint nf;
    uint nb;
    State *prev;
    State *to;

    // NB: we assume both spans are for same range
    x = x0;
    f = fg->go.span;
    b = bg->go.span;
    nf = fg->go.nSpans;
    nb = bg->go.nSpans;
    prev = NULL;
    for( ;; ) {
        if( f->ub == b->ub ) {
            to = ( f->to == b->to ) ? bg : f->to;
            if( to == prev ) {
                --x;
            } else {
                x->to = prev = to;
            }
            x->ub = f->ub;
            ++x; ++f; --nf; ++b; --nb;
            if( nf == 0 && nb == 0 ) {
                return( x - x0 );
            }
        }
        while( f->ub < b->ub ) {
            to = ( f->to == b->to ) ? bg : f->to;
            if( to == prev ) {
                --x;
            } else {
                x->to = prev = to;
            }
            x->ub = f->ub;
            ++x; ++f; --nf;
        }
        while( b->ub < f->ub ) {
            to = ( b->to == f->to ) ? bg : f->to;
            if( to == prev ) {
                --x;
            } else {
                x->to = prev = to;
            }
            x->ub = b->ub;
            ++x; ++b; --nb;
        }
    }
}

static void SCC_init( SCC *s, uint size )
{
    s->top = s->stk = malloc( size * sizeof( State ) );
}

static void SCC_destroy( SCC *s )
{
    free( s->stk );
}

static void SCC_traverse( SCC *s, State *x )
{
    uint    k;
    uint    i;

    *s->top = x;
    k = ++s->top - s->stk;
    x->depth = k;
    for( i = 0; i < x->go.nSpans; ++i ) {
        State *y = x->go.span[i].to;
        if( y ) {
            if( y->depth == 0 )
                SCC_traverse( s, y );
            if( y->depth < x->depth )
                x->depth = y->depth;
        }
    }
    if( x->depth == k ) {
        do {
            (*--s->top)->depth = cInfinity;
            (*s->top)->link = x;
        } while( *s->top != x );
    }
}

static uint maxDist( State *s )
{
    uint    mm;
    uint    i;

    mm = 0;
    for( i = 0; i < s->go.nSpans; ++i ) {
        State *t = s->go.span[i].to;

        if( t ) {
            uint m = 1;
            if( t->link == NULL )
                m += maxDist( t );
            if( m > mm ) {
                mm = m;
            }
        }
    }
    return( mm );
}

static void calcDepth( State *head )
{
    State   *t;
    State   *s;
    uint    i;

    for( s = head; s != NULL; s = s->next ) {
        if( s->link == s ) {
            for( i = 0; i < s->go.nSpans; ++i ) {
                t = s->go.span[i].to;
                if( t != NULL && t->link == s ) {
                    s->depth = maxDist(s);
                    break;
                }
            }
            if( i == s->go.nSpans ) {
                s->link = NULL;
            }
        } else {
            s->depth = maxDist(s);
        }
    }
}

static void DFA_findSCCs( DFA *d )
{
    SCC     scc;
    State   *s;

    SCC_init( &scc, d->nStates );
    for( s = d->head; s != NULL; s = s->next ) {
        s->depth = 0;
        s->link = NULL;
    }
    for( s = d->head; s != NULL; s = s->next ) {
        if( s->depth == 0 ) {
            SCC_traverse( &scc, s );
        }
    }
    calcDepth( d->head );
    SCC_destroy( &scc );
}

static void DFA_split( DFA *d, State *s )
{
    State *move;

    move = State_new();
    Action_new_Move( move );
    DFA_addState( d, &s->next, move );
    move->link = s->link;
    move->rule = s->rule;
    move->go = s->go;
    s->rule = NULL;
    s->go.nSpans = 1;
    s->go.span = malloc( sizeof( Span ) );
    s->go.span[0].ub = d->ubChar;
    s->go.span[0].to = move;
}

static void tree_reference( State *s, int isBase )
{
    uint    i;

    if( s->referenced == 0 ) {
        if( isBase == 0 )
            s->referenced = 1;
        if( s->action->type == MATCHACT && s->go.nSpans == 1 && s->go.span[0].to->action->type == RULEACT && s->go.span[0].to->go.nSpans == 0 ) {
        } else {
            for( i = 0; i < s->go.nSpans; i++ ) {
                tree_reference( s->go.span[i].to, s->isBase );
            }
        }
    }
}

void DFA_emit( DFA *d, FILE *o )
{
    State       *s;
    uint        i;
    uint        nRules;
    uint        nSaves;
    uint        *saves;
    State       **rules;
    State       *accept;
    Span        *span;

    bUsedYYAccept = false;
    DFA_findSCCs( d );
    d->head->link = d->head;
    d->head->depth = maxDist( d->head );

    nRules = 0;
    for( s = d->head; s != NULL; s = s->next ) {
        if( s->rule != NULL && s->rule->u.RuleOp.accept >= nRules ) {
            nRules = s->rule->u.RuleOp.accept + 1;
        }
    }

    saves = malloc( nRules * sizeof( *saves ) );
    memset( saves, ~0, nRules * sizeof( *saves ) );

    // mark backtracking points
    nSaves = 0;
    for( s = d->head; s != NULL; s = s->next ) {
        if( s->rule != NULL ) {
            for( i = 0; i < s->go.nSpans; ++i ) {
                if( s->go.span[i].to != NULL && s->go.span[i].to->rule == NULL ) {
                    free( s->action );
                    if( saves[s->rule->u.RuleOp.accept] == ~0 )
                        saves[s->rule->u.RuleOp.accept] = nSaves++;
                    Action_new_Save( s, saves[s->rule->u.RuleOp.accept] );
                    continue;
                }
            }
        }
    }

    // insert actions
    rules = malloc( nRules * sizeof( *rules ) );
    memset( rules, 0, nRules * sizeof( *rules ) );
    accept = NULL;
    for( s = d->head; s != NULL; s = s->next ) {
        State *ow;
        if( s->rule == NULL ) {
            ow = accept;
        } else {
            if( rules[s->rule->u.RuleOp.accept] == NULL ) {
                State *n = State_new();
                Action_new_Rule( n, s->rule );
                DFA_addState( d, &s->next, n );
                rules[s->rule->u.RuleOp.accept] = n;
            }
            ow = rules[s->rule->u.RuleOp.accept];
        }
        for( i = 0; i < s->go.nSpans; ++i ) {
            if( s->go.span[i].to == NULL ) {
                if( ow == NULL ) {
                    ow = accept = State_new();
                    Action_new_Accept( accept, nRules, saves, rules );
                    DFA_addState( d, &s->next, accept );
                }
                s->go.span[i].to = ow;
            }
        }
    }

    // split ``base'' states into two parts
    for( s = d->head; s != NULL; s = s->next ) {
        s->isBase = false;
        if( s->link != NULL ) {
            for( i = 0; i < s->go.nSpans; ++i ) {
                if( s->go.span[i].to == s ) {
                    s->isBase = true;
                    DFA_split( d, s );
                    if( bFlag )
                        BitMap_find_go( &s->next->go, s );
                    s = s->next;
                    break;
                }
            }
        }
    }

    // find ``base'' state, if possible
    span = malloc( ( d->ubChar - d->lbChar ) * sizeof( *span ) );
    for( s = d->head; s != NULL; s = s->next ) {
        if( s->link == NULL ) {
            for( i = 0; i < s->go.nSpans; ++i ) {
                State *to = s->go.span[i].to;
                if( to != NULL && to->isBase ) {
                    uint    nSpans;

                    to = to->go.span[0].to;
                    nSpans = merge( span, s, to );
                    if( nSpans < s->go.nSpans ) {
                        free( s->go.span );
                        s->go.nSpans = nSpans;
                        s->go.span = malloc( nSpans * sizeof( Span ) );
                        memcpy( s->go.span, span, nSpans * sizeof( Span ) );
                    }
                    tree_reference( s, 0 );
                    break;
                }
            }
        }
    }
    free( span );

    tree_reference( d->head, 0 );

    free( d->head->action );

    Action_new_Enter( d->head );

    fputs( "{\n\tYYCTYPE yych;\n", o );
    oline += 2;
    if( bUsedYYAccept ) {
        fputs( "\tunsigned int yyaccept = 0;\n", o );
        oline += 1;
    }
    if( bFlag ) {
        BitMap_gen( o, d->lbChar, d->ubChar );
    }

    fprintf( o, "\tgoto yy%u;\n", d->head->label );
    ++oline;

    for( s = d->head; s != NULL; s = s->next ) {
        State_emit( s, o );
        Go_genGoto( &s->go, o, s->next );
    }
    fputs( "}\n", o );
    ++oline;

    BitMap_first = NULL;

    free( saves );
    free( rules );
}
