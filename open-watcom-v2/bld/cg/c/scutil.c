/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Scoreboarding utilities.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "score.h"
#include "cgmem.h"
#include "memcheck.h"
#include "freelist.h"
#include "data.h"
#include "namelist.h"
#include "rgtbl.h"


pointer ScAlloc( size_t size )
/****************************/
{
    pointer     chunk;

    chunk = CGAlloc( size );
    return( chunk );
}

void    ScFree( pointer chunk )
/*****************************/
{
    if( chunk != NULL ) {
        CGFree( chunk );
    }
}

void    ScoreCalcList( void )
/***************************/
{
    score_reg   *curr;
    name        *reg_name;
    int         i;
    hw_reg_set  reg;

    ScoreCount = 0;
    AddRegs();
    for( reg_name = Names[N_REGISTER]; reg_name != NULL; reg_name = reg_name->n.next_name ) {
        if( ScRealRegister( reg_name ) ) {
            reg = HighOffsetReg( reg_name->r.reg );
            if( !HW_CEqual( reg, HW_EMPTY ) ) {
                AllocRegName( reg );
            }
            reg = LowOffsetReg( reg_name->r.reg );
            if( !HW_CEqual( reg, HW_EMPTY ) ) {
                AllocRegName( reg );
            }
        }
    }
    for( reg_name = Names[N_REGISTER]; reg_name != NULL; reg_name = reg_name->n.next_name ) {
        if( ScRealRegister( reg_name ) ) {
            ++ScoreCount;
        }
    }
    if( ScoreCount > 0 ) {
        ScoreList = ScAlloc( ScoreCount * sizeof( score_reg * ) + ScoreCount * sizeof( score_reg ) );
        curr = (score_reg *)&ScoreList[ScoreCount];
        i = 0;
        for( reg_name = Names[N_REGISTER]; reg_name != NULL; reg_name = reg_name->n.next_name ) {
            if( ScRealRegister( reg_name ) ) {
                curr->reg_name = reg_name;
                curr->reg = reg_name->r.reg;
                curr->type_class = reg_name->n.type_class;
                curr->size  = reg_name->n.size;
                curr->high_of = NO_INDEX;
                curr->low_of = NO_INDEX;
                reg_name->r.reg_index = i;
                ScoreList[i] = curr; /*% This avoids MANY multiplies!*/
                ++curr;
                ++i;
            }
        }
        i = ScoreCount;
        while( curr != ScoreList[0] ) {
            --curr;
            --i;
            reg = HighOffsetReg( curr->reg );
            if( !HW_CEqual( reg, HW_EMPTY ) ) {
                reg_name = AllocRegName( reg );
                if( !ScRealRegister( reg_name )
                  || reg_name->r.reg_index == NO_INDEX ) {
                    curr->high = NO_INDEX;
                } else {
                    curr->high = reg_name->r.reg_index;
                    ScoreList[curr->high]->high_of = i;
                }
            } else {
                curr->high = NO_INDEX;
            }
            reg = LowOffsetReg( curr->reg );
            if( !HW_CEqual( reg, HW_EMPTY ) ) {
                reg_name = AllocRegName( reg );
                if( !ScRealRegister( reg_name )
                  || reg_name->r.reg_index == NO_INDEX ) {
                    curr->low = NO_INDEX;
                } else {
                    curr->low = reg_name->r.reg_index;
                    ScoreList[curr->low]->low_of = i;
                }
            } else {
                curr->low = NO_INDEX;
            }
        }
    }
}


void    ScoreClear( score *scoreboard )
/*************************************/
{
    int         i;
    list_head   *list_heads;

    list_heads = (list_head *)&scoreboard[ScoreCount];
    *list_heads = NULL;  /* initialize free list*/
    for( i = 0; i < ScoreCount; ++i ) {
        ++list_heads;
        scoreboard->list = list_heads;
        *list_heads = NULL;
        scoreboard->next_reg = scoreboard;
        scoreboard->prev_reg = scoreboard;
        scoreboard->index = i;
        scoreboard->generation = 0;
        ++scoreboard;
    }
}


void    FreeScListEntry( score_list *list )
/*****************************************/
{
    FrlFreeSize( &ScListFrl, (pointer *)list, sizeof( score_list ) );
}


void    ScoreFreeList( score *scoreitem )
/***************************************/
{
    score_list  *curr;
    score_list  *next;

    if( scoreitem->list != NULL ) {
        for( curr = *scoreitem->list; curr != NULL; curr = next ) {
            next = curr->next;
            FreeScListEntry( curr );
        }
        *scoreitem->list = NULL;
    }
}


void    FreeScoreBoard( score *scoreboard )
/*****************************************/
{
    int         i;
    list_head   *list_heads;

    if( scoreboard != NULL ) {
        for( i = 0; i < ScoreCount; i++ ) {
            ScoreFreeList( &scoreboard[i] );
        }
        list_heads = (list_head *)&scoreboard[ScoreCount];
        *list_heads = NULL;  /* initialize free list*/
        for( i = 0; i < ScoreCount; i++ ) {
            scoreboard[i].list = ++list_heads;
            scoreboard[i].next_reg = &scoreboard[i];
            scoreboard[i].prev_reg = &scoreboard[i];
            scoreboard[i].generation = 0;
            *list_heads = NULL;
        }
    }
}


void    MemChanged( score *scoreboard, bool statics_too )
/*******************************************************/
{
    int         i;
    score_list  **owner;
    score_list  *curr;
    bool        changed;

    for( i = ScoreCount; i > 0; --i ) {
        if( scoreboard->list != NULL ) {
            for( owner = scoreboard->list; (curr = *owner) != NULL; ) {
                changed = false;
                switch( curr->info.class ) {
                case SC_N_CONSTANT:
                    changed = true;
                    break;
                case SC_N_TEMP:
                    if( (curr->info.symbol.v->usage & USE_ADDRESS) == 0 ) {
                        changed = true;
                    }
                    break;
                case SC_N_MEMORY:
                    if( !statics_too ) {
                        changed = true;
                    }
                    break;
                case SC_N_INDEXED:
                    if( curr->info.base != NULL ) {
                        if( curr->info.base->n.class == N_MEMORY ) {
                            if( !statics_too ) {
                                changed = true;
                            }
                        }
                    } else {
                        if( !statics_too ) {
                            changed = true;
                        }
                    }
                    break;
                }
                if( changed ) {
                    owner = &curr->next;
                } else {
                    *owner = curr->next;
                    FreeScListEntry( curr );
                }
            }
        }
        ++scoreboard;
    }
}


score_list      *NewScListEntry( void )
/*************************************/
{
    score_list  *list;

    list = AllocFrl( &ScListFrl, sizeof( score_list ) );
    return( list );
}


bool    ScoreFrlFree( void )
/**************************/
{
    return( FrlFreeAll( &ScListFrl, sizeof( score_list ) ) );
}
