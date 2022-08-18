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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "confldef.h"
#include "model.h"
#include "stackcg.h"
#include "nullprop.h"
#include "flood.h"


typedef struct stupid_struct_so_I_can_use_safe_recurse {
    block       *blk;
    flood_func  func;
    void        *parm;
} flood_parms;

static void *doFloodForward( flood_parms *fp )
{
    block       *next;
    block_num   i;
    block_num   n;
    flood_parms new_parms;

    new_parms = *fp;
    n = fp->blk->targets;
    for( i = 0; i < n; i++ ) {
        next = fp->blk->edge[i].destination.u.blk;
        if( _IsBlkVisited( next ) )
            continue;
        if( !fp->func( next, fp->parm ) )
            break;
        _MarkBlkVisited( next );
        new_parms.blk = next;
        SafeRecurseCG( (func_sr)doFloodForward, &new_parms );
    }
    return NULL;
}

void FloodForward( block *blk, flood_func func, void *parm )
{
    flood_parms parms;

    _MarkBlkAllUnVisited();
    parms.blk = blk;
    parms.func = func;
    parms.parm = parm;
    doFloodForward( &parms );
}

static void *doFloodBackward( flood_parms *fp )
{
    block       *next;
    block_edge  *edge;
    flood_parms new_parms;

    new_parms = *fp;
    for( edge = fp->blk->input_edges; edge != NULL; edge = edge->next_source ) {
        next = edge->source;
        if( _IsBlkVisited( next ) )
            continue;
        if( !fp->func( next, fp->parm ) )
            break;
        _MarkBlkVisited( next );
        new_parms.blk = next;
        SafeRecurseCG( (func_sr)doFloodBackward, &new_parms );
    }
    return NULL;
}

void FloodBackwards( block *start, flood_func func, void *parm )
/************************************************************************
    Flood backwards in the flow graph, calling the given function for each
    block as it is visited
*/
{
    flood_parms parms;

    _MarkBlkAllUnVisited();
    parms.blk = start;
    parms.func = func;
    parms.parm = parm;
    doFloodBackward( &parms );
}
