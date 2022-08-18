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
* Description:  Intel x86 indexed addressing instruction processing.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "data.h"
#include "makeins.h"
#include "namelist.h"
#include "rgtbl.h"
#include "split.h"
#include "x86objd.h"
#include "insutil.h"
#include "index.h"
#include "fixindex.h"
#include "conflict.h"
#include "x86data.h"
#include "x86segs.h"
#include "x86table.h"
#include "x86rtrn.h"
#include "x86tls.h"
#include "feprotos.h"


static  void            Merge( name **pname, instruction *ins );
static  void            PropSegments(void);

instruction     *NeedIndex( instruction *ins ) {
/*******************************************************
    Mark conflicts for any name used in instruction as as segment as
    NEEDS_SEGMENT, or split out the segment if it is marked as
    NEEDS_SEGMENT_SPLIT (move the segment operand to a temp and use the
    temp as the segment override).  Also, if any index conflicts are
    marked as NEEDS_INDEX_SPLIT, split them out into a temp as well.
*/

    name                *temp;
    name                *index;
    conflict_node       *conf;
    name                *name;

    if( ins->num_operands > OpcodeNumOperands( ins ) ) {
        name = ins->operands[ins->num_operands - 1];
        conf = NameConflict( ins, name );
        if( conf != NULL && _Isnt( conf, CST_NEEDS_SEGMENT_SPLIT ) ) {
            _SetTrue( conf, CST_NEEDS_SEGMENT );
            MarkSegment( ins, name );
        } else if( name->n.class != N_REGISTER ) {
            if( conf != NULL ) {
                _SetFalse( conf, CST_NEEDS_SEGMENT );
                _SetTrue( conf, CST_WAS_SEGMENT );
            }
            temp = AllocTemp( U2 );
            ins->operands[ins->num_operands - 1] = temp;
            PrefixIns( ins, MakeMove( name, temp, U2 ) );
            MarkSegment( ins, temp );
            _SetTrue( NameConflict( ins, temp ), CST_SEGMENT_SPLIT );
            ins = ins->head.prev;
        }
    }
    index = FindIndex( ins );
    if( index != NULL ) {
        temp = IndexToTemp( ins, index );
        conf = NameConflict( ins, temp );
        _SetTrue( conf, CST_INDEX_SPLIT );
        if( HasTrueBase( index ) && index->i.base->n.class == N_TEMP ) {
            MarkIndex( ins, temp, true );
        } else {
            MarkIndex( ins, temp, false );
        }
        ins = ins->head.prev;
    }
    return( ins );
}


bool    IndexOkay( instruction *ins, name *index ) {
/***********************************************************
    return true if "index" needs to be split out of instruction and a
    short lived temporary used instead.
*/

    name                *name;
    bool                is_temp_index;
    conflict_node       *conf;

    if( HasTrueBase( index ) && index->i.base->n.class == N_TEMP ) {
        is_temp_index = true;
    } else {
        is_temp_index = false;
    }
    name = index->i.index;
    if( IsString( ins->table ) )
        return( true );
    if( name->n.class == N_REGISTER ) {
        return( IsIndexReg( name->r.reg, name->n.type_class, is_temp_index ) );
    }
/* The next two lines require some explanation. If there is a CP/PT
   index still hanging around, it is because a reduction routine
   created it, so it can be handled. Normally, all CP/PT indecies are broken
   up into seg:foo[offset] before we ever get to register allocation */
    if( name->n.type_class == CP )
        return( true );
    if( name->n.type_class == PT )
        return( true );
    if( name->v.conflict == NULL )
        return( false );
    if( name->v.usage & USE_MEMORY )
        return( false );
    if( name->n.class != N_TEMP )
        return( false );
    conf = NameConflict( ins, name );
    if( conf == NULL )
        return( false );
    if( _Is( conf, CST_NEEDS_INDEX_SPLIT ) ) {
        _SetFalse( conf, CST_NEEDS_INDEX );
        return( false );
    } else {
        _SetTrue( conf, CST_NEEDS_INDEX );
        ins->head.state = OPERANDS_NEED_WORK;
        ins->t.index_needs = MarkIndex( ins, name, is_temp_index );
        return( true );
    }
}


static  bool    SegIsBase( name **index ) {
/******************************************
    return true if the segment override for N_INDEXED "*index" is the
    same as the segment override for its N_MEMORY base location, and if
    it is, change *index to point to the N_MEMORY base location.
*/

    if( (*index)->n.class == N_MEMORY )
        return( true );
    if( (*index)->i.index->n.size == WORD_SIZE ) {
        (*index) = (*index)->i.base;
        return( true );
    }
    return( false );
}

static  name    *FindSegment( instruction *ins ) {
/*************************************************
    return a pointer to a name within "ins" which requires a segment
    override but does not already have one.
*/

    name        *index;
    opcnt       i;

    if( ins->num_operands > OpcodeNumOperands( ins ) )
        return( NULL );
    if( ins->type_class == XX ) {
        if( ins->head.opcode != OP_CALL_INDIRECT ) {
            if( ins->head.opcode != OP_PUSH )
                return( NULL );
            /* careful ... this assumes small pushes split not movsb'd */
            /* see i86split */
            if( ins->operands[0]->n.size > 4 * WORD_SIZE ) {
                return( NULL );
            }
        }
    }
    if( ins->head.opcode != OP_LA && ins->head.opcode != OP_CAREFUL_LA ) {
        for( i = ins->num_operands; i-- > 0; ) {
            index = ins->operands[i];
            if( SegOver( index ) ) {
                return( index );
            }
        }
    }
    if( ins->result != NULL && SegOver( ins->result ) )
        return( ins->result );
    return( NULL );
}


void    AddSegment( instruction *ins ) {
/***********************************************
    Look at instruction "ins" and if any of its operands need a segment
    override, add it to the instruction as an extra operand.
*/

    opcnt               i;
    name                *index;
    name                *new_index;
    instruction         *new_ins;
    name                **seg_ptr;
    name                *seg;
    conflict_node       *conf;


    index = FindSegment( ins );
    conf = NULL;
    if( index != NULL ) {
        seg_ptr = &ins->operands[ins->num_operands];
        if( SegIsBase( &index ) ) {
            if( index != NULL ) {
                seg = GetSegment( index );
                if( seg->n.class != N_REGISTER ) {
                    new_ins = MakeMove( seg, AllocTemp( U2 ), U2 );
                    *seg_ptr = new_ins->result;
                    ins->num_operands++;
                    PrefixIns( ins, new_ins );
                    MarkSegment( ins, *seg_ptr );
                    conf = NameConflict( new_ins, *seg_ptr );
                }
            } else {
                *seg_ptr = NearSegment();
                ins->num_operands++;
                if( (*seg_ptr)->n.class == N_CONSTANT ) {
                    new_ins = MakeMove( *seg_ptr, AllocTemp( U2 ), U2 );
                    *seg_ptr = new_ins->result;
                    PrefixIns( ins, new_ins );
                    MarkSegment( ins, *seg_ptr );
                    conf = NameConflict( new_ins, *seg_ptr );
                }
            }
        } else { /* segment is the high part of the index */
            new_index = ScaleIndex( OffsetPart( index->i.index ),
                                    index->i.base,
                                    index->i.constant,
                                    index->n.type_class,
                                    index->n.size,
                                    index->i.scale,
                                    index->i.index_flags | X_SEGMENTED );
            if( ins->result == index ) {
                ins->result = new_index;
            }
            for( i = ins->num_operands; i-- > 0; ) {
                if( ins->operands[i] == index ) {
                    ins->operands[i] = new_index;
                }
            }
            *seg_ptr = SegmentPart( index->i.index );
            MarkSegment( ins, *seg_ptr );
            ins->num_operands++;
        }
        if( conf != NULL ) {
            _SetTrue( conf, CST_SEGMENT_SPLIT );
        }
    }
}

void    FixMemRefs( void ) {
/*****************************
    Make sure that no N_MEMORY names are used as indecies.  This would
    cause problems since we might need a segment override for the INDEX
    as well as the memory which we're indexing to get to.  This is no
    good since each instruction can only have one segment override,
    hence one memory reference.
*/

    block       *blk;
    instruction *ins;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
#if _TARGET & _TARG_80386
            ExpandThreadDataRef( ins );
#endif
            NoMemIndex( ins );
        }
    }
}


void    FixSegments( void ) {
/******************************
    Add segment overrides to any instruction in the routine that needs
    them.
*/

    block       *blk;
    instruction *ins;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            AddSegment( ins );
            /* Generate an error if segment override is requested and all segment
             * registers are pegged. However we do NOT want to generate an error
             * if this is a CS override in flat model - that particular case is not
             * an error because the CS override is essentially a no-op.  MN
             */
#define ANY_FLOATING (FLOATING_DS|FLOATING_ES|FLOATING_FS|FLOATING_GS)
#if _TARGET & _TARG_80386
            if( _IsntTargetModel( ANY_FLOATING ) && ins->num_operands > OpcodeNumOperands( ins )
                 && !(_IsTargetModel( FLAT_MODEL ) &&
                (ins->operands[ins->num_operands - 1]->n.class == N_REGISTER) &&
                HW_CEqual( ins->operands[ins->num_operands - 1]->r.reg, HW_CS )) ) {
#else
            if( _IsntTargetModel( ANY_FLOATING ) && ins->num_operands > OpcodeNumOperands( ins ) ) {
#endif
                /* throw away override */
                ins->num_operands--;
                FEMessage( MSG_NO_SEG_REGS, AskForLblSym( CurrProc->label ) );
            }
        }
    }
}


static  type_class_def  FltClass( instruction *ins ) {
/*****************************************************
    return the floating point class (FD or FS) of "ins", or XX if it
    does not involve floating point.
*/

    if( _IsFloating( ins->type_class ) )
        return( ins->type_class );
    if( _IsFloating( _OpClass( ins ) ) )
        return( _OpClass( ins ) );
    return( XX );
}

void    MergeIndex( void ) {
/*****************************
    Merge segment overrides back into the index registers.  This is
    called just before scoreboarding to simplify matters.  For example
    MOV  [DI], ES => AX becomes MOV [ES:DI] => AX.
*/

    block       *blk;
    instruction *ins;
    opcnt       i;
    name        **name;
    bool        dec;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( OpcodeNumOperands( ins ) < ins->num_operands ) {
                dec = false;
                for( i = OpcodeNumOperands( ins ); i-- > 0; ) {
                    name = &ins->operands[i];
                    if( (*name)->n.class == N_INDEXED ) {
                        Merge( name, ins );
                        dec = true;
                    }
                }
                if( ins->result != NULL ) {
                    name = &ins->result;
                    if( (*name)->n.class == N_INDEXED ) {
                        Merge( name, ins );
                        dec = true;
                    }
                }
                if( dec ) {
                    ins->num_operands--;
                }
            }
        }
    }
}

static  void    Merge( name **pname, instruction *ins ) {
/********************************************************
    called by MergeIndex ^
*/

    name        *index;
    name        *reg;
    hw_reg_set  tmp;

    index = *pname;
    tmp = index->i.index->r.reg;
    HW_TurnOn( tmp, ins->operands[ins->num_operands - 1]->r.reg );
    reg = AllocRegName( tmp );
    *pname = ScaleIndex( reg, index->i.base, index->i.constant,
                         index->n.type_class, index->n.size,
                         index->i.scale, index->i.index_flags );
}


void    FixChoices( void ) {
/*****************************
    Run through the conflict list and make sure that no conflict is
    allowed to get cached in a segment register unless it was actually
    used as a segment override somewhere in the program.  This avoids
    loads of bad selectors in protected mode.
*/

    conflict_node       *conf;
#if 0 /* 2007-07-10 RomanT -- This method is not used anymore */
    name                *temp;
    name                *alias;
#else
    possible_for_alias  *aposs;
#endif

    PropSegments();
    for( conf = ConfList; conf != NULL; conf = conf->next_conflict ) {
        if( _Isnt( conf, CST_VALID_SEGMENT ) ) {
            conf->possible = NoSegments( conf->possible );
#if 0 /* 2007-07-10 RomanT -- This method is not used anymore */
            temp = conf->name;
            if( temp->n.class == N_TEMP ) {
                alias = temp;
                for(;;) {
                    alias = alias->t.alias;
                    alias->t.possible = NoSegments( alias->t.possible );
                    if( alias == temp ) {
                        break;
                    }
                }
            }
#else
            for( aposs = conf->possible_for_alias_list; aposs != NULL; aposs = aposs->next ) {
                aposs->possible = NoSegments( aposs->possible );
            }
#endif
        }
    }
}

static  void    PropSegments( void ) {
/*******************************
    For every instruction of the form MOV X => Y, mark the conflict node
    for X as a valid segment if the conflict for Y is marked as a valid
    segment.
*/

    block               *blk;
    instruction         *ins;
    conflict_node       *src;
    conflict_node       *dst;
    bool                change;

    for( src = ConfList; src != NULL; src = src->next_conflict ) {
        if( _Is( src, CST_VALID_SEGMENT ) ) {
            _SetTrue( src, CST_WAS_SEGMENT );
        }
    }
    for( change = true; change; ) {
        change = false;
        for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
            for( ins = blk->ins.hd.prev; ins->head.opcode != OP_BLOCK; ins = ins->head.prev ) {
                if( ins->head.opcode == OP_MOV ) {
                    dst = NameConflict( ins, ins->result );
                    if( dst != NULL && _Is( dst, CST_WAS_SEGMENT ) ) {
                        src = NameConflict( ins, ins->operands[0] );
                        if( src != NULL && _Isnt( src, CST_WAS_SEGMENT ) ) {
                            _SetTrue( src, CST_WAS_SEGMENT );
                            change = true;
                        }
                    }
                }
            }
        }
    }
}


void    FixFPConsts( instruction *ins ) {
/************************************************
    Fix floating point constants, by forcing them into memory if they
    are not going to be segment register addressable.  This is done so that
    OneMemRef, the routine that ensures that we will have only one
    memory reference, (therefore one segment override), will see it as a
    memory reference and treat it accordingly.
*/

    opcnt               i;
    type_class_def      type_class;

    if( !FPCInCode() && (_IsTargetModel( FLOATING_SS ) && _IsTargetModel( FLOATING_DS )) ) {
        type_class = FltClass( ins );
        if( type_class != XX ) {
            for( i = ins->num_operands; i-- > 0; ) {
                ins->operands[i] = Addressable( ins->operands[i], type_class );
            }
        }
    }
}
