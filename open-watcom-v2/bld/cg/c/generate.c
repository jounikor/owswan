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
* Description:  High level code generation routines. Lots of action here.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "memcheck.h"
#include "data.h"
#include "types.h"
#include "addrcnst.h"
#include "fpu.h"
#include "makeins.h"
#include "foldins.h"
#include "rtrtn.h"
#include "objout.h"
#include "makeaddr.h"
#include "objio.h"
#include "dominate.h"
#include "inssched.h"
#include "peepopt.h"
#include "memlimit.h"
#include "blips.h"
#include "optmain.h"
#include "opttell.h"
#include "object.h"
#include "regalloc.h"
#include "targetin.h"
#include "makeblk.h"
#include "indvars.h"
#include "loopopts.h"
#include "nullprop.h"
#include "rgtbl.h"
#include "expand.h"
#include "insutil.h"
#include "insdead.h"
#include "namelist.h"
#include "typemap.h"
#include "blktrim.h"
#include "optimize.h"
#include "index.h"
#include "fixindex.h"
#include "conflict.h"
#include "scmain.h"
#include "generate.h"
#include "inline.h"
#include "cse.h"
#include "dataflo.h"
#include "flograph.h"
#include "multiply.h"
#include "splitvar.h"
#include "temps.h"
#include "varusage.h"
#include "liveinfo.h"
#include "regtree.h"
#include "breakrtn.h"
#include "condcode.h"
#include "trecurse.h"
#include "feprotos.h"


static  bool            abortCG;

void    InitCG( void )
/********************/
{
    InOptimizer = 0;
    InsId = 0;
    CurrProc = NULL;
    CurrBlock = NULL;
    BlockList = NULL;
    HeadBlock = NULL;
    BlockByBlock = false;
    abortCG = false;
    FPInit();/* must be before InitRegTbl */
    InitRegTbl();
    ScoreInit();
    InitQueue();
    InitMakeAddr();
    RegTreeInit();
    InitIns();
    InitConflict();
    InitRT();
    InitNames();
    ObjInit();
    ClassPointer = TypeClass( TypePtr );
    InitSegment();
}


void    AbortCG( void )
/*********************/
{
    abortCG = true;
}


void    FiniCG( void )
/********************/
{
    FiniQueue();
    FiniSegment();
    if( abortCG ) {
        AbortObj();
    }
    ObjFini();
    TellFreeAllLabels();
    BlowAwayFreeLists();
    ScoreFini();
}


static  void    AddANop( void )
/*****************************/
{
    AddAnIns( BlockList, MakeNop() );
}


static  void            PreOptimize( void )
/*****************************************/
{
    bool        change;

    if( _IsntModel( NO_OPTIMIZATION ) ) {
//      CharsAndShortsToInts();
        MakeMovAddrConsts();
        PushPostOps();
        DeadTemps();
        InsDead();
        MakeFlowGraph();
        BlockTrim();
        CommonSex( _IsModel( LOOP_OPTIMIZATION ) );
        SetOnCondition();
        BlockTrim();
        AddANop();
        if( _IsModel( LOOP_OPTIMIZATION ) ) {
            change = false;
            if( TransLoops( false ) ) {
                change = true;
            }
            if( LoopInvariant() ) {
                change = true;
            }
            if( change ) {
                CommonSex(true);
                InsDead();
                CommonInvariant();
            }
            if( IndVars() ) {
                CommonSex(false);
                InsDead();
                change = true;
            }
            BlockTrim();
            if( TransLoops( true ) ) {
                BlockTrim();
                CommonSex( false );
                change = true;
            }
            if( change ) {
                ReConstFold();
            }
            LoopEnregister();
            if( change ) {
                BlockTrim();
            }
        }
        MulToShiftAdd();
        KillMovAddrConsts();
        FindReferences();
    } else {
        MakeFlowGraph();
        AddANop();
        FindReferences();
    }
}


static  block           *NextBlock( block *blk, void *parm )
/**********************************************************/
{
    /* unused parameters */ (void)parm;

    return( blk->next_block );
}


static  void            PostOptimize( void )
/******************************************/
{
    if( _IsntModel( NO_OPTIMIZATION ) ) {
        // Run peephole optimizer again. Important: It is critical that the
        // new instructions can be directly generated because RegAlloc is
        // done by now. PeepOpt() is responsible for verifying that.
        if( PeepOpt( HeadBlock, NextBlock, NULL, true ) ) {
            LiveInfoUpdate();
        }
        // this is important as BuildIndex cannot handle instructions with no operands
        DeadInstructions();
        BuildIndex();
        DeadInstructions();
    }
    MergeIndex();
    if( _IsntModel( NO_OPTIMIZATION ) ) {
    #if (_TARGET & _TARG_RISC) == 0
        //
        // Calling Conditions() at this point has nice optimization effect,
        // but doesn't working correctly right now. It optimizes conditions
        // making them dependent from previous conditions codes, but riscifier
        // generates XOR's which will trash cond. codes. Either Conditions()
        // either riscifier must be fixed to handle this situation.
        //
    #if 0
        // Get rid of unused conditions on variables level
        // to decrease number of referenced vars in LdStAlloc() and Score()
        if( _IsntTargetModel( STATEMENT_COUNTING ) ) {
            Conditions();
            DeadInstructions(); // cleanup junk after Conditions()
        }
    #endif
    #endif
        // OptCloseMoves();  // todo: merge constant moves before riscifier
        LdStAlloc();
        Score();
        DeadInstructions(); // cleanup junk after Score()
        // deRISCify before LoopRegInvariant() or Shedule() are run:
        // they're moving RISCified pair.
        LdStCompress();
        // Reuse registers freed by deriscifier
        Score();
        DeadInstructions(); // cleanup junk after Score()
        if( !BlockByBlock )
            LoopRegInvariant();
    #if (_TARGET & _TARG_RISC) == 0
        // Get rid of remaining unused conditions on register level.
        if( _IsntTargetModel( STATEMENT_COUNTING ) ) {
            Conditions();
        }
    #endif
    }
    FPExpand();
    if( _IsntModel( NO_OPTIMIZATION ) ) {
        DeadInstructions();   // cleanup junk after Conditions()
        // Run scheduler last, when all instructions are stable
        if( _IsModel( INS_SCHEDULING ) ) {
            HaveLiveInfo = false;
            Schedule(); /* NOTE: Schedule messes up live information */
            LiveInfoUpdate();
            HaveLiveInfo = true;
        }
        // run this again in case Scheduler messed around with indices
        if( PeepOpt( HeadBlock, NextBlock, NULL, true ) ) {
            LiveInfoUpdate();
        }
    }
    FPOptimize();
}


static  void    FreeExtraTemps( name *last, block_num id )
/********************************************************/
{
    name        **owner;
    name        *temp;

    owner = &Names[N_TEMP];
    for( ;; ) {
        temp = *owner;
        if( temp == last )
            break;
        if( ( temp->v.usage & USE_IN_ANOTHER_BLOCK ) == 0
         && !_FrontEndTmp( temp )
         && temp->t.u.block_id == id ) {
            *owner = temp->n.next_name;
            FreeAName( temp );
        } else {
            owner = &temp->n.next_name;
        }
    }
}


static  void    ForceTempsMemory( void )
/**************************************/
{
    name        *op;
    name        *next;

    ParmPropagate();
    for( op = Names[N_TEMP]; op != LastTemp; op = next ) {
        next = op->n.next_name;
        if( ( op->v.usage & USE_IN_ANOTHER_BLOCK ) || _FrontEndTmp( op ) ) {
            op = DeAlias( op );
            op->v.usage |= NEEDS_MEMORY | USE_MEMORY;
            while( op->v.conflict != NULL ) {
                FreeAConflict( op->v.conflict );
            }
        }
    }
    AssignOtherLocals();
    for( op = Names[N_MEMORY]; op != NULL; op = op->n.next_name ) {
        op->v.usage |= USE_IN_ANOTHER_BLOCK | USE_MEMORY;
        if( op->v.conflict != NULL ) {
            FreeAConflict( op->v.conflict );
            op->v.conflict = NULL;
        }
    }
    LastTemp = Names[N_TEMP];
}


static  void    BlockToCode( bool partly_done )
/*********************************************/
{
    block_num           inputs;
    block_num           targets;
    block_edge          *input_edges;
    conflict_node       *curr;
    conflict_node       **owner;
    conflict_node       *conflist;
    block_num           id;



    /* try to get back some memory*/
    _MemLow;

    /* make the block look like an entire procedure*/

    HeadBlock = CurrBlock;
    BlockList = CurrBlock;
    HeadBlock->prev_block = NULL;
    if( HeadBlock->next_block != NULL ) {
        HeadBlock->next_block->prev_block = NULL;
    }
    /* Kludge - need a pointer to the next block for CALL_LABEL - puke! */
    if( _IsBlkAttr( HeadBlock, BLK_CALL_LABEL ) ) {
        HeadBlock->v.next = HeadBlock->next_block;
    }
    HeadBlock->next_block = NULL;

    /* force anything that spans blocks to memory */

    HeadBlock->u.partition = HeadBlock;
    ConstFold( HeadBlock );
    HeadBlock->u.partition = NULL;

    ForceTempsMemory();
    if( !partly_done ) {
        FixMemRefs();
        HaveLiveInfo = false;
        if( _IsntModel( NO_OPTIMIZATION | FORTRAN_ALIASING ) ) {
            FindReferences();
            CommonSex(false);
            PushPostOps();
        }
        FindReferences();
        DeadTemps();
        if( _IsModel( NO_OPTIMIZATION ) ) {
            SetInOut( HeadBlock );
        } else {
            MakeConflicts();
        }
        MakeLiveInfo();
        HaveLiveInfo = true;
        AxeDeadCode();
        FixIndex();
        FixSegments();
        FPRegAlloc();
        RegAlloc( true );
        FreeConflicts();
        HaveLiveInfo = false;
    } else {
        conflist = NULL;
        owner = &ConfList;
        for( ;; ) {
            curr = *owner;
            if( curr == NULL )
                break;
            if( curr->start_block == HeadBlock ) {
                *owner = curr->next_conflict;
                curr->next_conflict = conflist;
                conflist = curr;
            } else {
                owner = &curr->next_conflict;
            }
        }
        curr = ConfList;
        ConfList = conflist;
        RegAlloc( true );
        FreeConflicts();
        ConfList = curr;
    }
    input_edges = HeadBlock->input_edges;
    inputs = HeadBlock->inputs;
    targets = HeadBlock->targets;
    HeadBlock->inputs = 0;
    HeadBlock->input_edges = NULL;
    HeadBlock->targets = 0;
    FPParms();
    PostOptimize();
    HeadBlock->input_edges = input_edges;
    HeadBlock->inputs = inputs;
    HeadBlock->targets = targets;

    /* generate a prolog that saves all registers*/

    if( ( CurrProc->prolog_state & GENERATED_PROLOG ) == 0 ) {
        CurrProc->state.used = AllCacheRegs();
        GenProlog();
    }

    id = CurrBlock->id;
    AssgnMoreTemps( id );
    OptSegs();

    /* generate the code for the block*/

    if( _IsBlkAttr( CurrBlock, BLK_RETURN ) ) {
        GenObject();
        FiniStackMap();
        FreeProc();
    } else {
        GenObject();
    }
    if( partly_done ) {
        FreeExtraTemps( NULL, id );
    } else {
        FreeExtraTemps( LastTemp, id );
    }
}


static  void    FlushBlocks( bool partly_done )
/*********************************************/
/* we're in deep trouble. Try to get out of it using as little memory as*/
/* possible*/
{
    block       *blk;
    block       *next;
    block       *curr;
    block_class classes;

    if( !BlockByBlock && _IsntModel( NO_OPTIMIZATION ) ) {
        ProcMessage( MSG_REGALLOC_DIED );
    }
    if( !partly_done ) {
        Renumber();
    }
    curr = CurrBlock;
    BlockByBlock = true;
    classes = 0;
    for( blk = HeadBlock; blk != NULL; blk = next ) {
        next = blk->next_block;
        classes |= blk->class;
        CurrBlock = blk;
        BlockToCode( partly_done );
        FlushOpt();
    }
    CurrBlock = curr;
    HeadBlock = NULL;
    BlockList = NULL;
}


static  void    FreeExtraSyms( name *last )
/*****************************************/
{
    name        **owner;
    name        *temp;
    name        *next;

    owner = &Names[N_TEMP];
    for(;;) {
        temp = *owner;
        if( temp == last )
            break;
        if( ( temp->v.usage & USE_IN_ANOTHER_BLOCK ) == 0
         && !_FrontEndTmp( temp ) ) {
            *owner = temp->n.next_name;
            FreeAName( temp );
        } else {
            owner = &temp->n.next_name;
        }
    }
    for( temp = Names[N_CONSTANT]; temp != NULL; temp = next ) {
        next = temp->n.next_name;
        FreeAName( temp );
    }
    Names[N_CONSTANT] = NULL;
    for( temp = Names[N_INDEXED]; temp != NULL; temp = next ) {
        next = temp->n.next_name;
        FreeAName( temp );
    }
    Names[N_INDEXED] = NULL;
}


static  void    FinishIndex( void )
/*********************************/
{
    block       *blk;
    instruction *ins;
    instruction *next;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
            next = NeedIndex( ins );
            if( next == ins ) {
                next = ins->head.next;
            }
        }
    }
}


static  void    ForceConflictsMemory( void )
/******************************************/
{
    conflict_node       *conf;
    conflict_node       *next;

    ParmPropagate();
    for( conf = ConfList; conf != NULL; conf = conf->next_conflict ) {
        _SetFalse( conf, CST_NEEDS_INDEX_SPLIT | CST_NEEDS_SEGMENT_SPLIT );
    }
    for( conf = ConfList; conf != NULL; conf = next ) {
        next = conf->next_conflict;
        if( !_GBitEmpty( conf->id.out_of_block )
         || ( conf->name->n.class == N_TEMP && _FrontEndTmp( conf->name ) )
         || ( conf->name->n.class == N_MEMORY && conf->name->v.symbol != NULL )
         || ( conf->name->v.usage & USE_IN_ANOTHER_BLOCK ) ) {
            next = InMemory( conf );
        }
    }
}


static  void    Panic( bool partly_done )
/***************************************/
{
    proc_def    *curr_proc;

    if( partly_done ) {
        FinishIndex();
    }
    ForceConflictsMemory();
    SortBlocks();
    UnFixEdges();
    InitStackMap();
    AssgnMoreTemps( NO_BLOCK_ID );
    curr_proc = CurrProc;
    FlushBlocks( partly_done );
    if( curr_proc == CurrProc /* if not freed (dummy!) */
     && ( CurrProc->prolog_state & GENERATED_EPILOG ) == 0 ) {
        GenEpilog();
        FiniStackMap();
        FreeProc();
    }
}


static  void            GenPartialRoutine( bool routine_done )
/************************************************************/
{
    while( routine_done || _MemLow ) {
        if( CreateBreak() ) {
            BlockByBlock = true;
            BlockTrim();
            FindReferences();
            PreOptimize();
            FixBreak();
            SortBlocks();
            /* so the front gets told we're panicking */
            BlockByBlock = false;
            FlushBlocks( false );
            RemoveBreak();
            if( HeadBlock == NULL ) {
                FreeExtraSyms( NULL );
            }
        } else {
            FlushBlocks( false );
            FreeExtraSyms( NULL );
            break;
        }
    }
}


void    ProcMessage( msg_class msg )
/**********************************/
{
    static proc_def *lastproc = NULL;

    if( _IsntModel( NO_OPTIMIZATION ) && lastproc != CurrProc ) {
        FEMessage( msg, AskForLblSym( CurrProc->label ) );
        lastproc = CurrProc;
    }
}


void    Generate( bool routine_done )
/***********************************/
/* The big one - here's where most of code generation happens.
 * Follow this routine to see the transformation of code unfold.
 */
{
    if( BGInInline() )
        return;
    HaveLiveInfo = false;
    HaveDominatorInfo = false;
#if ( _TARGET & ( _TARG_370 | _TARG_RISC ) ) == 0
    /* if we want to go fast, generate statement at a time */
    if( _IsModel( NO_OPTIMIZATION ) ) {
        if( !BlockByBlock ) {
            InitStackMap();
            BlockByBlock = true;
        }
        LNBlip( SrcLine );
        FlushBlocks( false );
        FreeExtraSyms( LastTemp );
        if( _MemLow ) {
            BlowAwayFreeLists();
        }
        return;
    }
#endif

    /* if we couldn't get the whole procedure in memory, generate part of it */
    if( BlockByBlock ) {
        if( _MemLow || routine_done ) {
            GenPartialRoutine( routine_done );
        } else {
            BlkTooBig();
        }
        return;
    }

    /* if we're low on memory, go into BlockByBlock mode */
    if( _MemLow ) {
        InitStackMap();
        GenPartialRoutine( routine_done );
        BlowAwayFreeLists();
        return;
    }

    /* check to see that no basic block gets too unwieldy */
    if( !routine_done ) {
        BlkTooBig();
        return;
    }

    /* The routine is all in memory. Optimize and generate it */
    FixReturns();
    FixEdges();
    Renumber();
    BlockTrim();
    FindReferences();
    TailRecursion();
    NullConflicts( USE_IN_ANOTHER_BLOCK );
    InsDead();
    FixMemRefs();
    FindReferences();
    PreOptimize();
    PropNullInfo();
    MemtoBaseTemp();
    if( _MemCritical ) {
        Panic( false );
        return;
    }
    MakeConflicts();
    if( _IsModel( LOOP_OPTIMIZATION ) ) {
        SplitVars();
    }
    AddCacheRegs();
    MakeLiveInfo();
    HaveLiveInfo = true;
    AxeDeadCode();
    /* AxeDeadCode() may have emptied some blocks. Run BlockTrim() to get rid
     * of useless conditionals, then redo conflicts etc. if any blocks died.
     */
    if( BlockTrim() ) {
        FreeConflicts();
        NullConflicts( EMPTY );
        FindReferences();
        MakeConflicts();
        MakeLiveInfo();
    }
    FixIndex();
    FixSegments();
    FPRegAlloc();
    if( !RegAlloc( false ) ) {
        Panic( true );
        HaveLiveInfo = false;
        return;
    }
    FPParms();
    FixMemBases();
    PostOptimize();
    InitStackMap();
    AssignTemps();
    FiniStackMap();
    FreeConflicts();
    SortBlocks();
    if( CalcDominatorInfo() ) {
        HaveDominatorInfo = true;
    }
    GenProlog();
    UnFixEdges();
    OptSegs();
    GenObject();
    if( ( CurrProc->prolog_state & GENERATED_EPILOG ) == 0 ) {
        GenEpilog();
    }
    FreeProc();
    HaveLiveInfo = false;
#if _TARGET & _TARG_INTEL
    if( _IsModel( NEW_P5_PROFILING ) ) {
        FlushQueue();
    }
#else
    FlushQueue();
#endif
}
