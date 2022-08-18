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


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "yacc.h"
#include "alloc.h"

#define INFINITY        (unsigned short)-1

static a_look **stk, **top;

static void Reads( a_look *x )
{
    a_shift_action  *tx;
    a_look          *y;
    unsigned short  k;

    *top++ = x;
    k = (unsigned short)( top - stk );
    x->depth = k;
    for( tx = x->trans->state->trans; tx->sym != NULL; ++tx ) {
        if( tx->sym->pro == NULL ) {
            SetBit( x->follow, tx->sym->idx );
        }
    }
    for( y = x->trans->state->look; y->trans != NULL; ++y ) {
        if( y->trans->sym->nullable ) {
            if( y->depth == 0 ) {
                Reads( y );
            }
            if( y->depth < x->depth ) {
                x->depth = y->depth;
            }
            Union( x->follow, y->follow );
        }
    }
    if( x->depth == k ) {
        do {
            --top;
            (*top)->depth = INFINITY;
            Assign( (*top)->follow, x->follow );
        } while( *top != x );
    }
}

static void CalcReads( void )
{
    a_state     *x;
    a_look      *p;

    for( x = statelist; x != NULL; x = x->next ) {
        for( p = x->look; p->trans != NULL; ++p ) {
            if( p->depth == 0 ) {
                Reads( p );
            }
        }
    }
}

static void Nullable( void )
{
    a_sym       *sym;
    a_pro       *pro;
    an_item     *p;
    bool        nullable_added;

    for( sym = symlist; sym != NULL; sym = sym->next ) {
        sym->nullable = false;
    }
    do {
        nullable_added = false;
        for( sym = symlist; sym != NULL; sym = sym->next ) {
            if( !sym->nullable ) {
                for( pro = sym->pro; pro != NULL; pro = pro->next ) {
                    for( p = pro->items; p->p.sym != NULL; ++p ) {
                        if( !p->p.sym->nullable ) {
                            break;
                        }
                    }
                    if( p->p.sym == NULL ) {
                        /* all of the RHS symbols are nullable */
                        /* (the vacuous case means the LHS is nullable) */
                        sym->nullable = true;
                        nullable_added = true;
                    }
                }
            }
        }
    } while( nullable_added );
}

static void Includes( a_look *x )
{
    a_look          *y;
    a_link          *link;
    unsigned short  k;

    *top++ = x;
    k = (unsigned short)( top - stk );
    x->depth = k;
    for( link = x->include; link != NULL; link = link->next ) {
        y = link->el;
        if( y->depth == 0 ) {
            Includes( y );
        }
        if( y->depth < x->depth ) {
            x->depth = y->depth;
        }
        Union( x->follow, y->follow );
    }
    if( x->depth == k ) {
        do {
            --top;
            (*top)->depth = INFINITY;
            Assign( (*top)->follow, x->follow );
        } while( *top != x );
    }
}

static void CalcIncludes( void )
{
    a_state         *x, *y;
    a_shift_action  *tx;
    a_look          *p, *q;
    a_sym           *sym;
    a_pro           *pro;
    an_item         *nullable, *item;
    a_link          *free;

    for( x = statelist; x != NULL; x = x->next ) {
        for( p = x->look; p->trans != NULL; ++p ) {
            p->depth = 0;
            for( pro = p->trans->sym->pro; pro != NULL; pro = pro->next ) {
                nullable = pro->items;
                for( item = pro->items; (sym = item->p.sym) != NULL; ++item ) {
                    if( !sym->nullable ) {
                        nullable = item;
                    }
                }
                y = x;
                for( item = pro->items; (sym = item->p.sym) != NULL; ++item ) {
                    if( sym->pro == NULL ) {
                        for( tx = y->trans; tx->sym != sym; ) {
                            ++tx;
                        }
                        y = tx->state;
                    } else {
                        for( q = y->look; q->trans->sym != sym; ) {
                            ++q;
                        }
                        if( item >= nullable ) {
                            free = CALLOC( 1, a_link );
                            free->el = p;
                            free->next = q->include;
                            q->include = free;
                        }
                        y = q->trans->state;
                    }
                }
            }
        }
    }
    for( x = statelist; x != NULL; x = x->next ) {
        for( p = x->look; p->trans != NULL; ++p ) {
            if( p->depth == 0 ) {
                Includes( p );
            }
        }
    }
}

static void Lookback( void )
{
    a_state         *x, *y;
    a_shift_action  *tx;
    a_look          *p;
    a_reduce_action *rx;
    a_sym           *sym;
    a_pro           *pro;
    an_item         *item;

    for( x = statelist; x != NULL; x = x->next ) {
        for( p = x->look; p->trans != NULL; ++p ) {
            for( pro = p->trans->sym->pro; pro != NULL; pro = pro->next ) {
                y = x;
                for( item = pro->items; (sym = item->p.sym) != NULL; ++item ) {
                    for( tx = y->trans; tx->sym != sym; ) {
                        ++tx;
                    }
                    y = tx->state;
                }
                for( rx = y->redun; rx->pro != NULL && rx->pro != pro; ) {
                    ++rx;
                }
                Union( rx->follow, p->follow );
            }
        }
    }
}

static a_pro *extract_pro( an_item *p )
{
    an_item     *q;

    for( q = p; q->p.sym != NULL; ) {
        ++q;
    }
    return( q[1].p.pro );
}

static void check_for_user_hooks( a_state *state, a_shift_action *saction, index_n rule )
{
    bool                min_max_set;
    bool                all_match;
    conflict_id         min_id;
    conflict_id         max_id;
    conflict_id         id;
    an_item             **item;
    a_pro               *pro;
    a_SR_conflict       *conflict;
    a_SR_conflict       *last_conflict;
    a_SR_conflict_list  *cx;
    a_sym               *sym;

    if( state->kersize < 2 ) {
        return;
    }
    sym = saction->sym;
    min_max_set = false;
    min_id = CONFLICT_MAX_ID;
    max_id = CONFLICT_MIN_ID;
    for( item = state->items; *item != NULL; ++item ) {
        pro = extract_pro( *item );
        if( pro->SR_conflicts == NULL ) {
            /* production doesn't contain any conflicts */
            return;
        }
        for( cx = pro->SR_conflicts; cx != NULL; cx = cx->next ) {
            conflict = cx->conflict;
            if( conflict->sym != sym ) {
                continue;
            }
            id = conflict->id;
            if( min_id > id ) {
                min_id = id;
                min_max_set = true;
            }
            if( max_id < id ) {
                max_id = id;
                min_max_set = true;
            }
        }
        if( !min_max_set ) {
            /* production doesn't contain a matching conflict */
            return;
        }
    }
    for( id = min_id; id <= max_id; ++id ) {
        last_conflict = NULL;
        all_match = true;
        for( item = state->items; *item != NULL; ++item ) {
            conflict = NULL;
            pro = extract_pro( *item );
            for( cx = pro->SR_conflicts; cx != NULL; cx = cx->next ) {
                conflict = cx->conflict;
                if( conflict->id != id ) {
                    continue;
                }
                if( conflict->sym != sym ) {
                    continue;
                }
                break;
            }
            if( cx == NULL ) {
                all_match = false;
                break;
            }
            last_conflict = conflict;
        }
        if( all_match ) {
            /* found the desired S/R conflict */
            Ambiguous( state );
            last_conflict->state = state;
            last_conflict->shift = saction->state;
            last_conflict->reduce = rule;
            return;
        }
    }
}

static void resolve( a_state *x, set_size *work, a_reduce_action **reduce )
{
    a_shift_action  *tx, *ux;
    a_reduce_action *rx;
    set_size        *w;
    set_size        *mp;
    index_n         i;
    a_prec          symprec, proprec, prevprec;


    w = work;
    for( rx = x->redun; rx->pro != NULL; ++rx ) {
        for( mp = Members( rx->follow ); mp-- != setmembers; ) {
            if( reduce[*mp] != NULL ) {
                prevprec = reduce[*mp]->pro->prec;
                proprec = rx->pro->prec;
                if( prevprec.prec == 0 || proprec.prec == 0 || prevprec.prec == proprec.prec ) {
                    *w++ = *mp;
                    /* resolve to the earliest production */
                    if( rx->pro->pidx >= reduce[*mp]->pro->pidx ) {
                        continue;
                    }
                } else if( prevprec.prec > proprec.prec ) {
                    /* previous rule had higher precedence so leave it alone */
                    continue;
                }
            }
            reduce[*mp] = rx;
        }
    }
    while( w != work ) {
        --w;
        if( symtab[*w]->token == errsym->token )
            continue;
        printf( "r/r conflict in state %d on %s:\n", x->sidx, symtab[*w]->name);
        ++RR_conflicts;
        for( rx = x->redun; rx->pro != NULL; ++rx ) {
            if( IsBitSet( rx->follow, *w ) ) {
                showitem( rx->pro->items, "" );
            }
        }
        printf( "\n" );
        for( rx = x->redun; rx->pro != NULL; ++rx ) {
            if( IsBitSet( rx->follow, *w ) ) {
                ShowSentence( x, symtab[*w], rx->pro, NULL );
            }
        }
        printf( "---\n\n" );
    }
    ux = x->trans;
    for( tx = ux; tx->sym != NULL; ++tx ) {
        i = tx->sym->idx;
        if( i >= nterm || reduce[i] == NULL ) {
            *ux++ = *tx;
        } else {
            /* shift/reduce conflict detected */
            check_for_user_hooks( x, tx, reduce[i]->pro->pidx );
            symprec = tx->sym->prec;
            proprec = reduce[i]->pro->prec;
            if( symprec.prec == 0 || proprec.prec == 0 ) {
                if( tx->sym != errsym ) {
                    printf( "s/r conflict in state %d on %s:\n", x->sidx, tx->sym->name );
                    ++SR_conflicts;
                    printf( "\tshift to %d\n", tx->state->sidx );
                    showitem( reduce[i]->pro->items, "" );
                    printf( "\n" );
                    ShowSentence( x, tx->sym, reduce[i]->pro, NULL );
                    ShowSentence( x, tx->sym, NULL, tx->state );
                    printf( "---\n\n" );
                }
                *ux++ = *tx;
                reduce[i] = NULL;
            } else {
                if( symprec.prec > proprec.prec ) {
                    *ux++ = *tx;
                    reduce[i] = NULL;
                } else if( symprec.prec == proprec.prec ) {
                    if( symprec.assoc == R_ASSOC ) {
                        *ux++ = *tx;
                        reduce[i] = NULL;
                    } else if( symprec.assoc == NON_ASSOC ) {
                        ux->sym = tx->sym;
                        ux->state = errstate;
                        ++ux;
                        reduce[i] = NULL;
                    }
                }
            }
        }
    }
    ux->sym = NULL;
    for( rx = x->redun; rx->pro != NULL; ++rx ) {
        Clear( rx->follow );
    }
    for( i = 0; i < nterm; ++i ) {
        if( reduce[i] != NULL ) {
            SetBit( reduce[i]->follow, i );
            reduce[i] = NULL;
        }
    }
}

static void Conflict( void )
{
    a_word          *set;
    a_state         *x;
    a_shift_action  *tx;
    a_reduce_action *rx;
    a_reduce_action **reduce;
    set_size        *work;
    set_size        i;

    set = AllocSet( 1 );
    reduce = CALLOC( nterm, a_reduce_action * );
    work = CALLOC( nterm, set_size );
    for( x = statelist; x != NULL; x = x->next ) {
        Clear( set );
        for( tx = x->trans; tx->sym != NULL; ++tx ) {
            if( tx->sym->pro == NULL ) {
                SetBit( set, tx->sym->idx );
            }
        }
        for( rx = x->redun; rx->pro != NULL; ++rx ) {
            for( i = 0; i < GetSetSize( 1 ); ++i ) {
                if( rx->follow[i] & set[i] ) {
                    resolve( x, work, reduce );
                    break;
                }
                set[i] |= rx->follow[i];
            }
            if( i < GetSetSize( 1 ) ) {
                break;
            }
        }
    }
    FreeSet( set );
    FREE( reduce );
    FREE( work );
}

void lalr1( void )
{
    a_state         *x;
    a_shift_action  *tx;
    a_reduce_action *rx;
    a_look          *lk, *look;
    a_word          *lp, *lset;
    a_word          *rp, *rset;

    InitSets( nterm );
    lk = look = CALLOC( nvtrans + nstate, a_look );
    lp = lset = AllocSet( nvtrans );
    rp = rset = AllocSet( nredun );
    for( x = statelist; x != NULL; x = x->next ) {
        x->look = lk;
        for( tx = x->trans; tx->sym != NULL; ++tx ) {
            if( tx->sym->pro != NULL ) {
                lk->trans = tx;
                lk->follow = lp;
                lp += GetSetSize( 1 );
                ++lk;
            }
        }
        ++lk;
        for( rx = x->redun; rx->pro != NULL; ++rx ) {
            rx->follow = rp;
            rp += GetSetSize( 1 );
        }
    }
    stk = CALLOC( nvtrans, a_look * );
    top = stk;
    Nullable();
    CalcReads();
    CalcIncludes();
    Lookback();
    if( lk - look != nvtrans + nstate ) {
        puts( "internal error" );
    }
    if( lp - lset != GetSetSize( nvtrans ) ) {
        puts( "internal error" );
    }
    if( rp - rset != GetSetSize( nredun ) ) {
        puts( "internal error" );
    }
    FREE( look );
    FREE( stk );
    Conflict();
    nbstate = nstate;
}

void showstates( void )
{
    int         i;

    for( i = 0; i < nstate; ++i ) {
        printf( "\n" );
        showstate( statetab[i] );
    }
}

void showstate( a_state *x )
{
    a_parent        *parent;
    a_shift_action  *tx;
    a_reduce_action *rx;
    size_t          col, new_col;
    set_size        *mp;
    an_item         **item;

    printf( "state %d:\n", x->sidx );
    col = printf( "  parent states:" );
    for( parent = x->parents; parent != NULL; parent = parent->next ) {
        col += printf( " %d", parent->state->sidx );
        if( col > 79 ) {
            printf( "\n" );
            col = 0;
        }
    }
    printf( "\n" );
    for( item = x->items; *item != NULL; ++item ) {
        showitem( *item, " ." );
    }
    printf( "actions:" );
    col = 8;
    for( tx = x->trans; tx->sym != NULL; ++tx ) {
        new_col = col + 1 + strlen( tx->sym->name ) + 1 + 1 + 3;
        if( new_col > 79 ) {
            putchar('\n');
            new_col -= col;
        }
        col = new_col;
        printf( " %s:s%03d", tx->sym->name, tx->state->sidx );
    }
    putchar( '\n' );
    col = 0;
    for( rx = x->redun; rx->pro != NULL; ++rx ) {
        for( mp = Members( rx->follow ); mp-- != setmembers; ) {
            new_col = col + 1 + strlen( symtab[*mp]->name );
            if( new_col > 79 ) {
                putchar('\n');
                new_col -= col;
            }
            col = new_col;
            printf( " %s", symtab[*mp]->name );
        }
        new_col = col + 1 + 5;
        if( new_col > 79 ) {
            putchar('\n');
            new_col -= col;
        }
        col = new_col;
        printf( ":r%03d", rx->pro->pidx );
    }
    putchar( '\n' );
}
