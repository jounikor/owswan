/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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


#include "plusplus.h"
#include "callgrph.h"
#include "stats.h"
#include "cgfront.h"
#ifndef NDEBUG
    #include "dbg.h"
#endif


ExtraRptCtr( ctr_nodes );       // # nodes
ExtraRptCtr( ctr_edges );       // # edges


static void cgrfInit(           // INITIALIZATION FOR CALL GRAPHING INFO.
    CALLGRAPH *ctl )            // - control information
{
    ctl->carve_nodes = CarveCreate( sizeof( CALLNODE ), 64 );
    ctl->carve_edges = CarveCreate( sizeof( CALLEDGE ), 64 );
    ctl->pruned = false;
}


static void cgrfFini(           // COMPLETION OF CALL GRAPHING INFO.
    CALLGRAPH *ctl )            // - control information
{
    CarveDestroy( ctl->carve_nodes );
    CarveDestroy( ctl->carve_edges );
}


static DIRGRAPH_NODE *cgrfAllocNode( // ALLOCATE A NODE
    CALLGRAPH *ctl )            // - control information
{
    ExtraRptIncrementCtr( ctr_nodes );
    return( CarveAlloc( ctl->carve_nodes ) );
}


static DIRGRAPH_EDGE *cgrfAllocEdge( // ALLOCATE AN EDGE
    CALLGRAPH *ctl )            // - control information
{
    ExtraRptIncrementCtr( ctr_edges );
    return( CarveAlloc( ctl->carve_edges ) );
}


static void cgrfFreeNode(       // FREE A NODE
    CALLGRAPH *ctl,             // - control information
    CALLNODE *node )            // - node to be freed
{
    CarveFree( ctl->carve_nodes, node );
}


static void cgrfFreeEdge(       // ALLOCATE AN EDGE
    CALLGRAPH *ctl,             // - control information
    CALLEDGE *edge )            // - edge to be freed
{
    CarveFree( ctl->carve_edges, edge );
}


static DIRGRAPH_NODE *cgrfInitNode( // INIT A NODE
    CALLGRAPH *ctl,             // - control information
    CALLNODE *node )            // - node to be initialized
{
    /* unused parameters */ (void)ctl;

    node->refs = 0;
    node->depth = 0;
    node->opcodes = 0;
    node->addrs = 0;
    node->state_table = false;
    node->calls_done = false;
    node->cond_flags = 0;
    node->is_vft = false;
    node->inline_fun = false;
    node->inlineable = false;
    node->inlineable_oe = false;
    node->flowed_recurse = false;
    node->rescan = false;
    node->stab_gen = false;
    node->cgfile = NULL;
    node->unresolved = NULL;
    node->stmt_state = STS_NONE;
    node->dtor_method = DTM_DIRECT;
    return( &node->base );
}


static void changeNodeReference(// INCREMENT REFERENCE COUNT FOR AN EDGE
    int incr_refs,              // - signed amount to increment refs
    int incr_addrs,             // - signed amount to increment addrs
    CALLEDGE *edge )            // - edge to be updated
{
    CALLNODE *node;             // - node referenced by edge

    edge->refs = edge->refs + incr_refs;
    edge->addrs = edge->addrs + incr_addrs;
    node = (CALLNODE*)edge->base.target;
    node->refs = node->refs + incr_refs;
    node->addrs = node->addrs + incr_addrs;
}


static void changeCtlCounts(    // UPDATE COUNTS FROM CONTROL INFORMATION
    CALLGRAPH *ctl,             // - control information
    CALLEDGE *edge )            // - edge to be initialized
{
    changeNodeReference( ctl->incr_refs, ctl->incr_addrs, edge );
}


static DIRGRAPH_EDGE *cgrfInitEdge( // INIT AN EDGE
    CALLGRAPH *ctl,             // - control information
    CALLEDGE *edge )            // - edge to be initialized
{
    edge->refs = 0;
    edge->addrs = 0;
    changeCtlCounts( ctl, edge );
    return( &edge->base );
}


static DIRGRAPH_EDGE *cgrfDupEdge( // PROCESS DUPLICATED EDGE
    CALLGRAPH *ctl,             // - control information
    CALLEDGE *edge )            // - duplicated edge
{
    changeCtlCounts( ctl, edge );
    return( &edge->base );
}


static void cgrfPruneNode(      // PRUNE A NODE
    CALLGRAPH *ctl,             // - control information
    DIRGRAPH_NODE *node )       // - node to be pruned
{
    /* unused parameters */ (void)ctl; (void)node;
}


static void cgrfPruneEdge(      // PRUNE AN EDGE
    CALLGRAPH *ctl,             // - control information
    CALLEDGE *edge )            // - edge to be pruned
{
    /* unused parameters */ (void)ctl;

    changeNodeReference( -(int)edge->refs, -(int)edge->addrs, edge );
}


static DIRGRAPH_VFT cgrfVft =   // VIRTUAL FUNCTIONS FOR CALL GRAPHING
{ (fn_init_ctl)cgrfInit,            // - init call-graph info.
  (fn_fini_ctl)cgrfFini,            // - fini call-graph info.
  (fn_alloc_node)cgrfAllocNode,     // - allocate node
  (fn_alloc_edge)cgrfAllocEdge,     // - allocate edge
  (fn_free_node)cgrfFreeNode,       // - allocate node
  (fn_free_edge)cgrfFreeEdge,       // - allocate edge
  (fn_init_node)cgrfInitNode,       // - init node
  (fn_init_edge)cgrfInitEdge,       // - init edge
  (fn_dup_edge)cgrfDupEdge,         // - duplicated edge
  (fn_prune_node)cgrfPruneNode,     // - prune node
  (fn_prune_edge)cgrfPruneEdge,     // - prune edge
};


void CgrfFini(                  // FINALIZE FOR CALL-GRAPHING
    CALLGRAPH *ctl )            // - call graph information
{
    DgrfDestruct( &ctl->base );
}


void CgrfInit(                  // INITIALIZE FOR CALL-GRAPHING
    CALLGRAPH *ctl )            // - call graph information
{
    DgrfConstruct( &ctl->base, &cgrfVft );
    ExtraRptRegisterCtr( &ctr_nodes, "# call-graph nodes" );
    ExtraRptRegisterCtr( &ctr_edges, "# call-graph edges" );
}


// note: when a call is added, an addr-of has already been added
//
void CgrfAddCall(               // ADD A CALL
    CALLGRAPH *ctl,             // - call graph information
    CALLNODE *node_src,         // - node for caller
    CALLNODE *node_tgt )        // - node for callee
{
    ctl->incr_refs = 1;
    ctl->incr_addrs = -1;
    DgrfAddEdge( &ctl->base, &node_src->base, &node_tgt->base );
}


void CgrfAddAddrOf(             // ADD AN ADDRESS OF
    CALLGRAPH *ctl,             // - call graph information
    CALLNODE *node_src,         // - node for caller
    CALLNODE *node_tgt )        // - node for callee
{
    ctl->incr_refs = 0;
    ctl->incr_addrs = 1;
    DgrfAddEdge( &ctl->base, &node_src->base, &node_tgt->base );
}


CALLNODE *CgrfAddFunction(      // ADD A FUNCTION
    CALLGRAPH *ctl,             // - call graph information
    SYMBOL func )               // - function
{
    return( (CALLNODE*)DgrfAddNode( &ctl->base, func ) );
}


#if 0
CALLNODE *CgrfFindFunction(     // FIND A FUNCTION
    CALLGRAPH *ctl,             // - call graph information
    SYMBOL func )               // - function
{
    return( (CALLNODE*)DgrfFindNode( &ctl->base, func ) );
}
#endif


void CgrfPruneFunction(         // PRUNE FUNCTION (AND CALLS) FROM GRAPH
    CALLGRAPH *ctl,             // - call graph information
    CALLNODE *node )            // - node for function
{
    if( node->refs == 0 && node->addrs == 0 ) {
        ctl->pruned = true;
        DgrfPruneNode( &ctl->base, &node->base );
    }
}


void CgrfWalkFunctions(         // WALK FUNCTIONS IN GRAPH
    CALLGRAPH *ctl,             // - call graph information
    bool (*walker)              // - walking routine
        ( CALLGRAPH *           // - - control information
        , CALLNODE * ) )        // - - function
{
    DgrfWalkObjects( &ctl->base
                   , (bool (*)( DIRGRAPH_CTL*, DIRGRAPH_NODE*))walker );
}


bool CgrfWalkCalls(             // WALK CALLS FROM NODE IN GRAPH
    CALLGRAPH *ctl,             // - call graph information
    CALLNODE *node,             // - source node
    bool (*walker)              // - walking routine
        ( CALLGRAPH *           // - - control information
        , CALLEDGE * ) )        // - - edge
{
    ctl->curr_node = node;
    return DgrfWalkEdges
            ( &ctl->base
            , &node->base
            , (bool (*)( DIRGRAPH_CTL*, DIRGRAPH_EDGE*))walker );
}

#ifndef NDEBUG

static bool cgrfDumpCall(       // DUMP CALL GRAPH EDGE
    CALLGRAPH *ctl,             // - call graph information
    CALLEDGE *edge )            // - edge in graph
{
    VBUF vbuf;
    ctl = ctl;
    printf( "- calls[%p] refs(%d) addrs(%d) %s\n"
          , edge
          , edge->refs
          , edge->addrs
          , DbgSymNameFull( edge->base.target->object, &vbuf ) );
    VbufFree( &vbuf );
    return( false );
}


static bool cgrfDumpNode(       // DUMP CALL GRAPH NODE
    CALLGRAPH *ctl,             // - call graph information
    CALLNODE *node )            // - node to dump
{
    SYMBOL func;
    VBUF vbuf;

    func = node->base.object;
    if( func == NULL ) {
        func = ModuleInitFuncSym();
    }
    printf( "\nNode[%p] depth(%d) refs(%d) addrs(%d) opcodes(%d) cflags(%d)\n"
            "         inline_fun(%d) inlineable(%d) oe(%d) cgfile(%p)\n"
            "         state_table(%d) rescan(%d) stab_gen(%d)\n"
            "         %s flags(%x)\n"
          , node
          , node->depth
          , node->refs
          , node->addrs
          , node->opcodes
          , node->cond_flags
          , node->inline_fun
          , node->inlineable
          , node->inlineable_oe
          , node->cgfile
          , node->state_table
          , node->rescan
          , node->stab_gen
          , DbgSymNameFull( node->base.object, &vbuf )
          , func->flag );
    CgrfWalkCalls( ctl, node, &cgrfDumpCall );
    VbufFree( &vbuf );
    return( false );
}


void CgrfDump(                  // DUMP CALL GRAPH
    CALLGRAPH *ctl )            // - call graph information
{
    printf( "===================== CALL GRAPH (start) ===============\n" );
    CgrfWalkFunctions( ctl, &cgrfDumpNode );
    printf( "===================== CALL GRAPH (end) ===============\n" );
}

#endif
