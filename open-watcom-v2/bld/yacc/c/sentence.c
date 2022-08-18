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
* Description:  Produce a sentence that illustrates the conflict.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yacc.h"
#include "yaccins.h"
#include "alloc.h"

typedef struct traceback traceback;
struct traceback {
    traceback   *next;
    a_state     *state;
    a_sym       *sym;
};

static void pushTrace( traceback **h, a_state *state, a_sym *sym )
{
    traceback   *token;

    token = MALLOC( 1, traceback );
    token->next = *h;
    token->state = state;
    token->sym = sym;
    *h = token;
}

static void popTrace( traceback **h )
{
    traceback   *token;

    token = *h;
    if( token != NULL ) {
        *h = token->next;
        FREE( token );
    }
}

static a_state *findNewShiftState( a_state *state, a_sym *sym )
{
    a_shift_action  *saction;
    a_sym           *shift_sym;

    for( saction = state->trans; (shift_sym = saction->sym) != NULL; ++saction ) {
        if( shift_sym == sym ) {
            return( saction->state );
        }
    }
    return( NULL );
}

static void performShift( traceback **h, a_sym *sym )
{
    a_state     *state;

    state = findNewShiftState( (*h)->state, sym );
    pushTrace( h, state, sym );
}

static void performReduce( traceback **h, a_pro *pro )
{
    an_item     *p;

    for( p = pro->items; p->p.sym != NULL; ++p ) {
        popTrace( h );
    }
    performShift( h, pro->sym );
}


static a_sym *terminalInKernel( an_item *p )
{
    a_sym       *sym_after_dot;
    a_sym       *post_sym;
    an_item     *q;
    a_pro       *pro;

    for( q = p; q->p.sym != NULL; ) {
        ++q;
    }
    pro = q[1].p.pro;
    sym_after_dot = NULL;
    for( q = pro->items; (post_sym = q->p.sym) != NULL; ++q ) {
        if( q == p ) {
            if( post_sym->pro == NULL ) {
                sym_after_dot = post_sym;
            }
            break;
        }
    }
    return( sym_after_dot );
}

static bool notInTraceback( traceback **h, a_sym *sym )
{
    traceback   *t;

    for( t = *h; t != NULL; t = t->next ) {
        if( t->sym == sym ) {
            return( false );
        }
    }
    return( true );
}

static a_sym *findNewShiftSym( a_state *state, traceback **h )
{
    a_shift_action  *saction;
    a_sym           *shift_sym;
    an_item         **item;

    if( state->trans[0].sym != NULL && state->trans[1].sym == NULL ) {
        shift_sym = state->trans[0].sym;
        if( notInTraceback( h, shift_sym ) ) {
            return( shift_sym );
        }
    }
    for( item = state->items; *item != NULL; ++item ) {
        shift_sym = terminalInKernel( *item );
        if( shift_sym != NULL ) {
            if( notInTraceback( h, shift_sym ) ) {
                return( shift_sym );
            }
        }
    }
    for( saction = state->trans; (shift_sym = saction->sym) != NULL; ++saction ) {
        if( notInTraceback( h, shift_sym ) ) {
            return( shift_sym );
        }
    }
    return( NULL );
}

static void flushStack( traceback **h )
{
    while( *h != NULL ) {
        popTrace( h );
    }
}

static void doRunUntilShift( traceback **h, a_sym *sym, traceback **ht, unsigned count )
{
    index_n             sidx;
    a_sym               *chk_sym;
    a_state             *state;
    a_state             *top;
    a_reduce_action     *raction;

    for( ; *h != NULL; ) {
        top = (*h)->state;
        if( top == NULL ) {
            flushStack( h );
            break;
        }
        state = findNewShiftState( top, sym );
        if( state != NULL ) {
            pushTrace( h, state, sym );
            pushTrace( ht, NULL, sym );
            if( sym == eofsym ) {
                break;
            }
            for( ; *h != NULL; ) {
                top = (*h)->state;
                if( top->redun->pro == NULL )
                    break;
                performReduce( h, top->redun->pro );
            }
            break;
        }
        sidx = sym->idx;
        for( raction = top->redun; raction->pro != NULL; ++raction ) {
            if( IsBitSet( raction->follow, sidx ) ) {
                performReduce( h, raction->pro );
                break;
            }
        }
        if( raction->pro == NULL ) {
            if( sym != eofsym ) {
                /* a syntax error will result */
                flushStack( h );
                break;
            }
            if( top->redun->pro != NULL ) {
                performReduce( h, top->redun->pro );
            } else {
                if( count ) {
                    --count;
                    chk_sym = findNewShiftSym( top, ht );
                } else {
                    chk_sym = NULL;
                }
                if( chk_sym != NULL ) {
                    doRunUntilShift( h, chk_sym, ht, count );
                } else {
                    /* a syntax error will result */
                    flushStack( h );
                    break;
                }
            }
        }
    }
}

static void runUntilShift( traceback **h, a_sym *sym, traceback **ht )
{
    doRunUntilShift( h, sym, ht, 16 );
}

static traceback *reverseStack( traceback *s )
{
    traceback   *h;
    traceback   *n;

    h = NULL;
    while( s != NULL ) {
        n = s->next;
        s->next = h;
        h = s;
        s = n;
    }
    return( h );
}

static void printAndFreeStack( traceback *top )
{
    size_t      column;
    size_t      len;
    a_sym       *sym;
    char        *min;
    traceback   *token;

    column = 0;
    while( top != NULL ) {
        token = top;
        top = token->next;
        sym = token->sym;
        if( sym != NULL ) {
            min = sym->min;
            len = strlen( min );
            if( column + len > 70 ) {
                fputc( '\n', stdout );
                column = 0;
            }
            fputs( min, stdout );
            column += len;
        }
        FREE( token );
    }
}

static traceback *makeReversedCopy( traceback *top )
{
    traceback   *parse_stack;
    traceback   *curr;

    parse_stack = NULL;
    for( curr = top; curr != NULL; curr = curr->next ) {
        pushTrace( &parse_stack, curr->state, curr->sym );
    }
    return( parse_stack );
}


static traceback *getStatePrefix( a_state *s, a_state *initial_parent )
{
    traceback       *list;
    a_parent        *parent;
    a_state         *min;
    a_state         *min_check;
    a_shift_action  *t;

    list = NULL;
    for( ; (parent = s->parents) != NULL; s = min ) {
        if( initial_parent != NULL ) {
            min = initial_parent;
            initial_parent = NULL;
        } else {
            min = parent->state;
            for( parent = parent->next; parent != NULL; parent = parent->next ) {
                min_check = parent->state;
                if( min_check->sidx < min->sidx ) {
                    min = min_check;
                }
            }
        }
        for( t = min->trans; t->sym != NULL; ++t ) {
            if( t->state == s ) {
                pushTrace( &list, s, t->sym );
            }
        }
    }
    pushTrace( &list, s, NULL );
    return( list );
}

void ShowSentence( a_state *s, a_sym *sym, a_pro *pro, a_state *to_state )
/************************************************************************/
{
    traceback   *list;
    traceback   *parse_stack;
    traceback   *token_stack;
    a_parent    *parent;

    for( parent = s->parents; parent != NULL; parent = parent->next ) {
        if( to_state != NULL ) {
            /* S/R conflict */
            printf( "Sample sentence(s) for shift to state %u:\n", to_state->sidx );
        } else {
            /* R/R conflict */
            printf( "Sample sentence(s) for reduce of rule %u:\n", pro->pidx );
        }
        list = getStatePrefix( s, parent->state );
        parse_stack = makeReversedCopy( list );
        token_stack = NULL;
        if( to_state != NULL ) {
            runUntilShift( &parse_stack, sym, &token_stack );
        } else {
            performReduce( &parse_stack, pro );
            runUntilShift( &parse_stack, sym, &token_stack );
        }
        printf( "  " );
        printAndFreeStack( list );
        if( parse_stack == NULL ) {
            printf( ". %s\n  Will never shift token '%s' in this context\n", sym->name, sym->name );
        } else {
            printf( ".\n" );
            runUntilShift( &parse_stack, eofsym, &token_stack );
            token_stack = reverseStack( token_stack );
            printAndFreeStack( token_stack );
            putchar( '\n' );
        }
        // dump all of the contexts if we have verbose state output
        if( ! showflag )
            break;
        putchar( '\n' );
    }
}

static char *my_stpcpy( char *d, char const *s )
{
    size_t  len;

    len = strlen( s );
    memcpy( d, s, len + 1 );
    return( d + len );
}

static size_t symHasMinLen( a_sym *sym, a_pro *pro, a_sym *disallow_error )
{
    size_t      len;
    an_item     *p;

    /* unused parameters */ (void)sym;

    for( p = pro->items; p->p.sym != NULL; ++p ) {
        if( p->p.sym == disallow_error ) {
            // derivations using the error sym are not desirable
            return( 0 );
        }
        if( p->p.sym->min == NULL ) {
            return( 0 );
        }
    }
    len = 0;
    for( p = pro->items; p->p.sym != NULL; ++p ) {
        len += strlen( p->p.sym->min ) + 1;
    }
    ++len;
    return( len );
}

static a_sym *symHasMin( a_sym *sym, a_pro *pro, a_sym *disallow_error )
{
    size_t      len;
    an_item     *p;
    char        *min;
    char        *cat;

    len = symHasMinLen( sym, pro, disallow_error );
    if( len == 0 ) {
        return( NULL );
    }
    min = MALLOC( len, char );
    min[0] = '\0';
    cat = min;
    for( p = pro->items; p->p.sym != NULL; ++p ) {
        if( p->p.sym->min[0] != '\0' ) {
            cat = my_stpcpy( cat, p->p.sym->min );
        }
    }
    if( cat != min && cat[-1] != ' ' ) {
        cat = my_stpcpy( cat, " " );
    }
    sym->min = min;
    return( sym );
}

static void setMinToName( a_sym *sym )
{
    size_t      len;
    char        *min;
    char        *cat;

    len = strlen( sym->name );
    len += 2;
    min = MALLOC( len, char );
    min[0] = '\0';
    cat = min;
    cat = my_stpcpy( cat, sym->name );
    if( cat != min && cat[-1] != ' ' ) {
        cat = my_stpcpy( cat, " " );
    }
    sym->min = min;
}

static void propagateMin( a_sym *disallow_error )
{
    a_sym       *last;
    a_sym       *sym;
    a_sym       *has_min;
    a_pro       *pro;
    unsigned    i;
    size_t      min_len;
    size_t      len;

    do {
        last = NULL;
        for( i = 0; i < nsym; ++i ) {
            sym = symtab[i];
            if( sym->min == NULL ) {
                min_len = (size_t)-1;
                for( pro = sym->pro; pro != NULL; pro = pro->next ) {
                    len = symHasMinLen( sym, pro, disallow_error );
                    if( len != 0 && len < min_len ) {
                        min_len = len;
                    }
                }
                for( pro = sym->pro; pro != NULL; pro = pro->next ) {
                    len = symHasMinLen( sym, pro, disallow_error );
                    if( len != 0 && len == min_len ) {
                        has_min = symHasMin( sym, pro, disallow_error );
                        if( has_min != NULL ) {
                            last = has_min;
                            break;
                        }
                    }
                }
            }
        }
    } while( last != NULL );
}

static void seedWithSimpleMin( void )
{
    a_sym       *sym;
    unsigned    i;

    // set terminals to their name and set nullable syms
    for( i = 0; i < nsym; ++i ) {
        sym = symtab[i];
        if( sym->pro != NULL ) {
            if( sym->nullable ) {
                sym->min = strdup( "" );
            }
        } else {
            setMinToName( sym );
        }
    }
}

static void verifyAllHaveMin( void )
{
    a_sym       *sym;
    unsigned    i;

    for( i = 0; i < nsym; ++i ) {
        sym = symtab[i];
        if( sym->min == NULL ) {
            printf( "%s has no minimum expansion! (mutually recursive?)\n", sym->name );
            setMinToName( sym );
        }
    }
    fflush( stdout );
}

void CalcMinSentence( void )
/**************************/
{
    seedWithSimpleMin();
    // disallow 'error' in the expansions
    propagateMin( errsym );
    // allow 'error' in the expansions
    propagateMin( NULL );
    verifyAllHaveMin();
}
