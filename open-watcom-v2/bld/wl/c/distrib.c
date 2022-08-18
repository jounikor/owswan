/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  routines for distributing libraries over overlays
*
****************************************************************************/

#include <string.h>
#include "linkstd.h"
#include "alloc.h"
#include "msg.h"
#include "pcobj.h"
#include "wlnkmsg.h"
#include "objpass1.h"
#include "objpass2.h"
#include "objfree.h"
#include "wcomdef.h"
#include "overlays.h"
#include "ring.h"
#include "distrib.h"
#include "specials.h"
#include "load16m.h"


#ifdef _EXE

#define LowestAncestorMap(o,i)  LowestAncestor( o, SectOvlTab[i] )

#define INITIAL_MOD_ALLOC   32
#define INITIAL_ARC_ALLOC   32
#define MAX_NUM_MODULES     _8KB

static section      **SectOvlTab;
static arcdata      *ArcList;
static unsigned_32  ArcListMaxLen;
static mod_entry    **ModTable;
static unsigned_16  ModTableMaxLen;
static unsigned_16  CurrModHandle;

/* forward declarations */

static void     ScanArcs( mod_entry *mod );

static void DistribNumASect( section *sect )
/******************************************/
{
    SectOvlTab[OvlSectNum++] = sect;
}

void DistribNumberSections( void )
/********************************/
{
    _ChkAlloc( SectOvlTab, sizeof( section * ) * ( OvlSectNum + 1 ) );
    /* OvlSectNum value 0 is reserved for Root */
    /* Overlayed sections start at 1 */
    OvlSectNum = 0;
    DistribNumASect( Root );
    WalkAreas( Root->areas, DistribNumASect );
}

void ResetDistribSupp( void )
/***************************/
{
    ArcList = NULL;
    ArcListMaxLen = 0;
    ModTable = NULL;
    ModTableMaxLen = 0;
    SectOvlTab = NULL;
}

void DistribInitMods( void )
/**************************/
{
    ModTableMaxLen = INITIAL_MOD_ALLOC;
    _ChkAlloc( ModTable, INITIAL_MOD_ALLOC * sizeof( mod_entry * ) );
    CurrModHandle = 0;
    ArcListMaxLen = INITIAL_ARC_ALLOC;
    _ChkAlloc( ArcList, offsetof( arcdata, arcs ) + INITIAL_ARC_ALLOC * sizeof( dist_arc ) );
    ArcList->numarcs = 0;
    ResetPass1Blocks();
}

void DistribAddMod( mod_entry *lp, overlay_ref ovlref )
/******************************************************
 * add this module to the table, and make the arclist field point to a
 * scratch buffer
 * NYI: segdata changes have completely broken distributing libraries.
 * fix this!
 */
{
    mod_entry   **new;

    CurrModHandle++;
    if( CurrModHandle == ModTableMaxLen ) {
        if( ModTableMaxLen > MAX_NUM_MODULES ) {
            LnkMsg( FTL+MSG_TOO_MANY_LIB_MODS, NULL );
        }
        _ChkAlloc( new, sizeof( mod_entry * ) * ModTableMaxLen * 2 );
        memcpy( new, ModTable, sizeof( mod_entry * ) * ModTableMaxLen );
        _LnkFree( ModTable );
        ModTable = new;
        ModTableMaxLen *= 2;
    }
    ModTable[CurrModHandle] = lp;
    lp->x.arclist = ArcList;
    ArcList->numarcs = 0;
    if( lp->modinfo & MOD_FIXED ) {
        ArcList->ovlref = ovlref;
    } else {
        ArcList->ovlref = NO_ARCS_YET;
    }
}

void InitArcList( mod_entry *mod )
/*********************************
 * set up the mod_entry arcdata field for dead code elimination
 */
{
    if( (FmtData.type & MK_OVERLAYS) && FmtData.u.dos.distribute && (LinkState & LS_SEARCHING_LIBRARIES) ) {
    } else {
        _PermAlloc( mod->x.arclist, offsetof( arcdata, arcs ) );
    }
}

static void MarkDead( void *_seg )
/********************************/
{
    segdata     *seg = _seg;

    if( seg->isrefd )
        return;
    if( seg->isdead )
        return;

    if( seg->iscode ) {
        seg->isdead = true;
    } else {
        if( FmtData.type & MK_PE ) {
            char *segname = seg->u.leader->segname.u.ptr;
            if( ( strcmp( segname, CoffPDataSegName ) == 0 )
                || ( strcmp( segname, CoffReldataSegName ) == 0 ) ) {
                seg->isdead = true;
            }
        }
    }
}

static void KillUnrefedSyms( void *_sym )
/***************************************/
{
    symbol      *sym = _sym;
    segdata     *seg;

    seg = sym->p.seg;
    if( ( seg != NULL ) && !IS_SYM_IMPORTED( sym ) && !IS_SYM_ALIAS( sym ) && seg->isdead ) {
        if( seg->u.leader->combine == COMBINE_COMMON ) {
            seg = RingFirst( seg->u.leader->pieces );
            if( !seg->isdead ) {
                return;
            }
        }
        if( sym->e.def != NULL ) {
            WeldSyms( sym, sym->e.def );
        } else {
            sym->info |= SYM_DEAD;
        }
        if( LinkFlags & LF_SHOW_DEAD ) {
            LnkMsg( MAP+MSG_SYMBOL_DEAD, "S", sym );
        }
    }
}

static void DefineOvlSegments( mod_entry *mod )
/**********************************************
 * figure out which of the segments are live
 */
{
    Ring2Walk( mod->segs, MarkDead );
    Ring2Walk( mod->publist, KillUnrefedSyms );
}

void DistribSetSegments( void )
/******************************
 * now that we know where everything is, do all the processing that has been
 * postponed until now.
 */
{
    if( (LinkFlags & LF_STRIP_CODE) == 0 )
        return;
    LinkState &= ~LS_CAN_REMOVE_SEGMENTS;
    ObjFormat |= FMT_DEBUG_COMENT;
    if( (FmtData.type & MK_OVERLAYS) && FmtData.u.dos.distribute ) {
        _LnkFree( ArcList );
        ArcList = NULL;
    }
    if( LinkFlags & LF_STRIP_CODE ) {
        WalkMods( DefineOvlSegments );
    }
#if 0           // NYI: distributing libraries completely broken.
    unsigned        index;
    mod_entry       *mod;
    overlay_ref     ovlref;
    mod_entry       **currmod;
    unsigned        num_segdefs;

    if( (FmtData.type & MK_OVERLAYS) && FmtData.u.dos.distribute ) {
        for( index = 1; index <= CurrModHandle; index++ ) {
            mod = ModTable[index];
            CurrMod = mod;
            ovlref = mod->x.arclist->ovlref;
            if( ovlref == NO_ARCS_YET ) {       // only data referenced
                CurrSect = Root;
            } else {
                CurrSect = SectOvlTab[ovlref];
            }
            DefModSegments( mod );
            mod->x.next = NULL;
            for( currmod = &CurrSect->u.dist_mods; *currmod != NULL; ) {
                currmod = &((*currmod)->x.next);
            }
            *currmod = mod;
            mod->n.sect = CurrSect;
        }
    }
    FixGroupProblems();
    FindRedefs();
    if( (FmtData.type & MK_OVERLAYS) && FmtData.u.dos.distribute ) {
        _LnkFree( SectOvlTab );
        SectOvlTab = NULL;
    }
#endif
    if( (FmtData.type & MK_OVERLAYS) && FmtData.u.dos.distribute ) {
        _LnkFree( SectOvlTab );
        SectOvlTab = NULL;
    }
    ReleasePass1Blocks();
}

void FreeDistribSupp( void )
/**************************/
{
    unsigned    index;

    for( index = 1; index <= CurrModHandle; index++ ) {
        FreeAMod( ModTable[index] );
    }
    _LnkFree( ModTable );
    _LnkFree( ArcList );
    _LnkFree( SectOvlTab );
    ReleasePass1Blocks();
}

void DistribProcMods( void )
/**************************/
{
    unsigned_16 index;
    mod_entry   *mod;

    for( index = 1; index <= CurrModHandle; index++ ) {
        mod = ModTable[index];
        CurrSect = mod->n.sect;
        PModule( mod );
    }
}

overlay_ref LowestAncestor( overlay_ref ovlref, section *sect )
/**************************************************************
 * find the lowest common ancestor of the two overlay values by marking all of
 * the ancestors of the first overlay, and then looking for marked ancestors
 * of the other overlay
 */
{
    section     *list;

    for( list = sect; list != NULL; list = list->parent ) {
        // set visited flag
        list->next_sect = (void *)((pointer_int)list->next_sect | 1);
    }
    for( list = SectOvlTab[ovlref]; list != NULL; list = list->parent ) {
        // check visited flag
        if( (pointer_int)list->next_sect & 1 ) {
            break;
        }
    }
    for( ; sect != NULL; sect = sect->parent ) {
        // reset visited flag
        sect->next_sect = (void *)((pointer_int)sect->next_sect & ~1);
    }
    return( list->ovlref );
}

static bool NewRefVector( symbol *sym, overlay_ref ovlref, overlay_ref sym_ovlref )
/**********************************************************************************
 * sometimes there can be an overlay vector generated to a routine specified
 * in an .OBJ file caused by a call from a library routine. this checks for
 * this case.
 */
{
    if( ( sym->p.seg == NULL ) || ( (sym->u.d.ovlstate & OVL_VEC_MASK) != OVL_UNDECIDED ) ) {
        return( true );
    }
    /*
     * at this point, we know it has already been defined, but does not have an
     * overlay vector, and is not data
     */
    if( LowestAncestorMap( sym_ovlref, ovlref ) != sym_ovlref ) {
        OvlVectorize( sym );
        return( true );
    }
    return( false );
}

void DefDistribSym( symbol *sym )
/********************************
 * move current module based on where this symbol has been referenced from,
 * and make the symbol point to the current module. All symbols which get
 * passed to this routine are in an overlay class.
 */
{
    arcdata     *arclist;
    segdata     *seg;

    if( sym->info & SYM_REFERENCED ) {
        arclist = CurrMod->x.arclist;
        if( CurrMod->modinfo & MOD_FIXED ) {
            seg = sym->p.seg;
            if( seg->iscode ) {      // if code..
                NewRefVector( sym, sym->u.d.ovlref, arclist->ovlref );
            } else if( (sym->u.d.ovlstate & OVL_FORCE) == 0 ) {
                // don't generate a vector.
                sym->u.d.ovlstate |= OVL_FORCE | OVL_NO_VECTOR;
            }
        } else {
            if( arclist->ovlref == NO_ARCS_YET ) {
                arclist->ovlref = sym->u.d.ovlref;
            } else {
                arclist->ovlref = LowestAncestorMap( arclist->ovlref, sym->u.d.ovlref );
            }
        }
    }
    sym->u.d.modnum = CurrModHandle;
}

static void AddArc( dist_arc arc )
/*********************************
 * add an arc to the arclist for the current module
 */
{
    arcdata     *arclist;

    arclist = CurrMod->x.arclist;
    if( arclist->numarcs >= ArcListMaxLen ) {
        _ChkAlloc( arclist, offsetof( arcdata, arcs ) + 2 * ArcListMaxLen * sizeof( dist_arc ) );
        memcpy( arclist, ArcList, offsetof( arcdata, arcs ) + ArcListMaxLen * sizeof( dist_arc ) );
        _LnkFree( ArcList );
        CurrMod->x.arclist = arclist;
        ArcList = arclist;
        ArcListMaxLen *= 2;
    }
    arclist->arcs[arclist->numarcs] = arc;
    arclist->numarcs++;
}

static bool NotAnArc( dist_arc arc )
/***********************************
 * return true if this is not an arc in the current module
 */
{
    unsigned    index;
    arcdata     *arclist;

    arclist = CurrMod->x.arclist;
    for( index = arclist->numarcs; index-- > 0; ) {
        if( arclist->arcs[index].test == arc.test ) {
            return( false );
        }
    }
    return( true );
}

void RefDistribSym( symbol *sym )
/********************************
 * add an arc to the reference graph if it is not already in the graph
 */
{
    mod_entry   *mod;
    segdata     *seg;
    dist_arc    arc;

    arc.sym = sym;
    if( sym->info & SYM_DEFINED ) {
        if( sym->info & SYM_DISTRIB ) {
            mod = ModTable[sym->u.d.modnum];
            if( mod->modinfo & MOD_FIXED ) {        // add reference, as long
                seg = sym->p.seg;                   // as it is a code ref.
                if( seg->iscode ) {
                    AddArc( arc );
                }
            } else {
                arc.test = sym->u.d.modnum;
                if( NotAnArc( arc ) && ( sym->u.d.modnum != CurrModHandle ) ) {
                    AddArc( arc );
                }
            }
        } else if( (sym->u.d.ovlstate & OVL_VEC_MASK) == OVL_UNDECIDED ) {
            if( NotAnArc( arc ) ) {
                AddArc( arc );
            }
        }
    } else {   // just a reference, so it has to be added to the call graph.
        AddArc( arc );
    }
}

static void DoRefGraph( overlay_ref ovlref, mod_entry *mod )
/***********************************************************
 * checks to see if the mod has changed position, and if it has, check all
 * of the routines that mod references
 */
{
    arcdata     *arclist;
    overlay_ref anc_ovlref;

    arclist = mod->x.arclist;
    /*
     * this next line is necessary to break cycles in the graph.
     */
    if( (mod->modinfo & MOD_VISITED) && ( ovlref == arclist->ovlref ) || (mod->modinfo & MOD_FIXED) )
        return;
    if( arclist->ovlref == NO_ARCS_YET ) {
        arclist->ovlref = 0;
        anc_ovlref = 0;
        ScanArcs( mod );
    } else {
        anc_ovlref = LowestAncestorMap( ovlref, arclist->ovlref );
        if( anc_ovlref != arclist->ovlref ) {
            arclist->ovlref = anc_ovlref;
            ScanArcs( mod );
        }
    }
    if( anc_ovlref == 0 ) {   // it's at the root, so pull it out of the graph
        arclist->numarcs = 0;
    }
}

static void DeleteArc( arcdata *arclist, unsigned_16 index )
/***********************************************************
 * delete an arc from the specified arc list
 */
{
    arclist->numarcs--;
    if( arclist->numarcs > 0 ) {
        arclist->arcs[index] = arclist->arcs[arclist->numarcs];
    }
}

static void ScanArcs( mod_entry *mod )
/*************************************
 * go through all modules referenced by mod, and see if they need to change
 * position because of the position of mod
 */
{
    arcdata     *arclist;
    symbol      *sym;
    mod_entry   *refmod;
    unsigned_16 index;
    overlay_ref ovlref;
    dist_arc    currarc;

    mod->modinfo |= MOD_VISITED;
    arclist = mod->x.arclist;
    ovlref = arclist->ovlref;
    if( ovlref != NO_ARCS_YET ) {
        for( index = arclist->numarcs; index-- > 0; ) {
            currarc = arclist->arcs[index];
            if( currarc.test <= MAX_NUM_MODULES ) {     // GIANT KLUDGE!
                DoRefGraph( ovlref, ModTable[currarc.mod] );
            } else {
                sym = currarc.sym;
                if( sym->info & SYM_DEFINED ) {
                    if( sym->info & SYM_DISTRIB ) {
                        currarc.test = sym->u.d.modnum;
                        refmod = ModTable[currarc.mod];
                        if( refmod->modinfo & MOD_FIXED ) {
                            if( NewRefVector( sym, ovlref, refmod->x.arclist->ovlref ) ) {
                                DeleteArc( arclist, index );
                            }
                        } else {
                            DoRefGraph( ovlref, refmod );
                            if( !NotAnArc( currarc ) ) {
                                DeleteArc( arclist, index );
                            } else {
                                arclist->arcs[index] = currarc;
                            }
                        }
                    } else {
                        if( ( sym->p.seg == NULL )
                          || NewRefVector( sym, ovlref, sym->p.seg->u.leader->class->section->ovlref ) ) {
                            DeleteArc( arclist, index );
                        }
                    }
                } else {
                    if( (sym->u.d.ovlstate & OVL_REF) == 0 ) {
                        sym->u.d.ovlref = ovlref;
                        sym->u.d.ovlstate |= OVL_REF;
                    } else {
                        sym->u.d.ovlref = LowestAncestorMap( ovlref, sym->u.d.ovlref );
                    }
                }
            } /* if( a module ) */
        } /* for( arcs left ) */
    } /* if( an ovlnum defined ) */
    mod->modinfo &= ~MOD_VISITED;
}

void DistribFinishMod( mod_entry *mod )
/**************************************
 * check the position of the modules referenced by mod, and then make a
 * more permanent copy of the arclist for this module.
 */
{
    arcdata     *arclist;
    unsigned    allocsize;

    ScanArcs( mod );
    if( mod->modinfo & MOD_FIXED ) {    // no need to scan a fixed module more than once
        mod->x.arclist->numarcs = 0;
    }
    allocsize = offsetof( arcdata, arcs ) + mod->x.arclist->numarcs * sizeof( dist_arc );
    _Pass1Alloc( arclist, allocsize );
    memcpy( arclist, mod->x.arclist, allocsize );
    mod->x.arclist = arclist;
}

void DistribIndirectCall( symbol *sym )
/**************************************
 * handle indirect calls and their effect on distributed libs.
 */
{
    arcdata         *arclist;
    overlay_ref     save_ovlref;

    arclist = CurrMod->x.arclist;
    save_ovlref = arclist->ovlref;
    arclist->ovlref = 0;                // make sure current module isn't
    CurrMod->modinfo |= MOD_VISITED;    // visited
    DoRefGraph( 0, ModTable[sym->u.d.modnum] );
    CurrMod->modinfo &= ~MOD_VISITED;
    arclist->ovlref = save_ovlref;
    sym->u.d.ovlstate |= OVL_REF;
    sym->u.d.ovlref = 0;                // make sure current symbol put in root.
}

#endif
