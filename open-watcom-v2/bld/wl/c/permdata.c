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
* Description:  Routines for making some linker data permanent.
*
****************************************************************************/


#include <string.h>
#include <stdio.h>
#include "linkstd.h"
#include "strtab.h"
#include "dbgcomm.h"
#include "dbgall.h"
#include "carve.h"
#include "alloc.h"
#include "reloc.h"
#include "fileio.h"
#include "virtmem.h"
#include "impexp.h"
#include "loadfile.h"
#include "msg.h"
#include "objio.h"
#include "ring.h"
#include "wlnkmsg.h"
#include "objcalc.h"
#include "permdata.h"

#include "clibext.h"

stringtable             PermStrings;
stringtable             PrefixStrings;  /* these are NetWare prefix strings of which there could possibly be several */
carve_t                 CarveLeader;
carve_t                 CarveModEntry;
carve_t                 CarveSymbol;
carve_t                 CarveSegData;
carve_t                 CarveClass;
carve_t                 CarveGroup;
carve_t                 CarveDLLInfo;
carve_t                 CarveExportInfo;
char                    *IncFileName;
incgroupdef             *IncGroupDefs;
group_entry             **IncGroups;
libnamelist             *SavedUserLibs;
libnamelist             *SavedDefLibs;

static stringtable      StoredRelocs;
static char             *ReadRelocs;
static size_t           SizeRelocs;
static char             *OldExe;
static char             *OldSymFile;
static void             *AltDefData;
static char             *IncStrTab;

#define SEG_CARVE_SIZE          _2KB
#define MOD_CARVE_SIZE          _5KB
#define SDATA_CARVE_SIZE        _16KB
#define SYM_CARVE_SIZE          _32KB

void ResetPermData( void )
/************************/
{
    IncFileName = NULL;
    IncStrTab = NULL;
    ReadRelocs = NULL;
    OldExe = NULL;
    AltDefData = NULL;
    OldSymFile = NULL;
    IncGroupDefs = NULL;
    IncGroups = NULL;
    SavedUserLibs = NULL;
    SavedDefLibs = NULL;
    CarveClass = CarveCreate( sizeof( class_entry ), 20 * sizeof( class_entry ) );
    CarveGroup = CarveCreate( sizeof( group_entry ), 20 * sizeof( group_entry ) );
    CarveDLLInfo = CarveCreate( sizeof( dll_sym_info ), 100 * sizeof( dll_sym_info ) );
    CarveExportInfo = CarveCreate( sizeof( entry_export ), 20 * sizeof( entry_export ) );
    CarveLeader = CarveCreate( sizeof( seg_leader ), SEG_CARVE_SIZE );
    CarveModEntry = CarveCreate( sizeof( mod_entry ), MOD_CARVE_SIZE );
    CarveSegData = CarveCreate( sizeof( segdata ), SDATA_CARVE_SIZE );
    CarveSymbol = CarveCreate( sizeof( symbol ), SYM_CARVE_SIZE );
    InitStringTable( &PermStrings, true );
    InitStringTable( &PrefixStrings, true );
    InitStringTable( &StoredRelocs, false );
}


static void MarkDLLInfo( void *dll )
/**********************************/
{
    ((dll_sym_info *)dll)->isfree = true;
}

static void MarkExportInfo( void *exp )
/*************************************/
{
    ((entry_export *)exp)->isfree = true;
}

static void MarkModEntry( void *mod )
/***********************************/
{
    ((mod_entry *)mod)->modinfo |= MOD_IS_FREE;
}

static void MarkSegData( void *sdata )
/************************************/
{
    ((segdata *)sdata)->isfree = true;
}

static void MarkSymbol( void *sym )
/*********************************/
{
    ((symbol *)sym)->info |= SYM_IS_FREE;
}

static size_t GetString( perm_write_info *info, const char *str )
/***************************************************************/
{
    return( AddStringStringTableOffs( &info->strtab, str ) );
}

static void DoWritePermFile( perm_write_info *info, const char *data, unsigned len, bool isvmem )
/***********************************************************************************************/
{
    unsigned modpos;
    unsigned adjust;

    modpos = info->currpos % MAX_HEADROOM;
    info->currpos += len;
    while( modpos + len >= MAX_HEADROOM ) {
        adjust = MAX_HEADROOM - modpos;
        if( !isvmem ) {
            memcpy( TokBuff + modpos, data, adjust );
        } else {
            ReadInfo( (virt_mem)data, TokBuff + modpos, adjust );
        }
        QWrite( info->incfhdl, TokBuff, MAX_HEADROOM, IncFileName );
        data += adjust;
        len -= adjust;
        modpos = 0;
    }
    if( len > 0 ) {
        if( !isvmem ) {
            memcpy( TokBuff + modpos, data, len );
        } else {
            ReadInfo( (virt_mem)data, TokBuff + modpos, len );
        }
    }
}

static void VMemWritePermFile( perm_write_info *info, virt_mem data, unsigned len )
/*********************************************************************************/
{
    DoWritePermFile( info, (void *)data, len, true );
}

static void U32WritePermFile( perm_write_info *info, unsigned_32 data )
/*********************************************************************/
{
    DoWritePermFile( info, (char *)&data, sizeof( data ), false );
}

static void BufWritePermFile( perm_write_info *info, void *data, unsigned len )
/*****************************************************************************/
{
    DoWritePermFile( info, data, len, false );
}

static bool WriteLeaderName( void *_leader, void *info )
/******************************************************/
{
    seg_leader *leader = _leader;

    U32WritePermFile( info, leader->class->name.u.offs );
    U32WritePermFile( info, leader->segname.u.offs );
    return( false );
}

static unsigned WriteGroupsList( perm_write_info *info )
/******************************************************/
{
    group_entry *group;
    unsigned    num;

    num = 0;
    for( group = Groups; group != NULL; group = group->next_group ) {
        if( !group->isautogrp && group->leaders != NULL ) {
            num++;
            U32WritePermFile( info, (unsigned_32)Ring2Count( group->leaders ) );
            group->sym->name.u.offs = GetString( info, group->sym->name.u.ptr );
            U32WritePermFile( info, group->sym->name.u.offs );
            Ring2Lookup( group->leaders, WriteLeaderName, info );
        }
    }
    return( num );
}

static bool CheckFree( bool isfree, perm_write_info *info )
/*********************************************************/
{
    if( isfree ) {
        U32WritePermFile( info, CARVE_INVALID_INDEX );
    }
    return( isfree );
}

static void WriteDLLInfo( void *_dll, void *info )
/************************************************/
{
    dll_sym_info *dll = _dll;

    if( !CheckFree( dll->isfree, info ) ) {
        dll->m.modname.u.ptr = dll->m.modnum->name.u.ptr;
        if( !dll->isordinal ) {
            dll->u.entname.u.ptr = dll->u.entry->name.u.ptr;
        }
        BufWritePermFile( info, dll, offsetof( dll_sym_info, iatsym ) );
    }
}

static void WriteExportInfo( void *_exp, void *info )
/***************************************************/
{
    entry_export *exp = _exp;

    if( !CheckFree( exp->isfree, info ) ) {
        exp->next = CarveGetIndex( CarveExportInfo, exp->next );
        if( exp->name.u.ptr != NULL ) {
            exp->name.u.offs = GetString( info, exp->name.u.ptr );
        }
        BufWritePermFile( info, exp, offsetof( entry_export, sym ) );
    }
}

static void FixSymAddr( void *_sym )
/**********************************/
{
    symbol *sym = _sym;

    if( !IS_SYM_IMPORTED( sym ) && (sym->info & SYM_DEAD) == 0 && sym->addr.off > 0 && sym->p.seg != NULL ) {
        sym->addr.off -= sym->p.seg->u.leader->seg_addr.off;
        sym->addr.off -= sym->p.seg->a.delta;
    }
}

static void PrepModEntry( void *_mod, void *info )
/************************************************/
{
    mod_entry *mod = _mod;

    if( mod->modinfo & MOD_IS_FREE ) {
        *((unsigned_32 *)mod) = CARVE_INVALID_INDEX;
        return;
    }
    Ring2Walk( mod->publist, FixSymAddr );
    mod->n.next_mod = CarveGetIndex( CarveModEntry, mod->n.next_mod );
    mod->name.u.offs = GetString( info, mod->name.u.ptr );
    mod->publist = CarveGetIndex( CarveSymbol, mod->publist );
    mod->segs = CarveGetIndex( CarveSegData, mod->segs );
    mod->modinfo &= ~MOD_CLEAR_ON_INC;
    if( mod->f.source != NULL ) {
        mod->f.fname = mod->f.source->infile->name;
    }
}

static void PrepSegData( void *_sdata, void *info )
/*************************************************/
{
    segdata *sdata = _sdata;

    /* unused parameters */ (void)info;

    if( sdata->isfree ) {
        *((unsigned_32 *)sdata) = CARVE_INVALID_INDEX;
        return;
    }
    sdata->next = CarveGetIndex( CarveSegData, sdata->next );  // not used
    sdata->mod_next = CarveGetIndex( CarveSegData, sdata->mod_next );
    sdata->o.clname.u.ptr = sdata->u.leader->class->name.u.ptr;
    sdata->u.name.u.ptr = sdata->u.leader->segname.u.ptr;
    sdata->vm_data = 0;
}

static void PrepSymbol( void *_sym, void *info )
/**********************************************/
{
    symbol      *sym = _sym;
    char        *save;
    symbol      *mainsym;

    if( sym->info & SYM_IS_FREE ) {
        *((unsigned_32 *)sym) = CARVE_INVALID_INDEX;
        return;
    }
    sym->hash = CarveGetIndex( CarveSymbol, sym->hash );
    sym->link = CarveGetIndex( CarveSymbol, sym->link );
    sym->publink = CarveGetIndex( CarveSymbol, sym->publink );
    if( sym->info & SYM_IS_ALTDEF ) {
        mainsym = sym->e.mainsym;
        if( (mainsym->info & SYM_NAME_XLATED) == 0 ) {
            mainsym->name.u.offs = GetString( info, mainsym->name.u.ptr );
            mainsym->info |= SYM_NAME_XLATED;
        }
        sym->name = mainsym->name;
    } else if( (sym->info & SYM_NAME_XLATED) == 0 ) {
        sym->name.u.offs = GetString( info, sym->name.u.ptr );
        sym->info |= SYM_NAME_XLATED;
    }
    sym->mod = CarveGetIndex( CarveModEntry, sym->mod );
    if( IS_SYM_ALIAS( sym ) ) {
        save = sym->p.alias.u.ptr;
        sym->p.alias.u.offs = GetString( info, sym->p.alias.u.ptr );
        if( sym->info & SYM_FREE_ALIAS ) {
            _LnkFree( save );
        }
    } else if( IS_SYM_IMPORTED( sym ) ) {
        if( FmtData.type & (MK_OS2 | MK_PE) ) {
            sym->p.import = CarveGetIndex( CarveDLLInfo, sym->p.import );
        }
    } else if( (sym->info & SYM_IS_ALTDEF) == 0 || IS_SYM_COMDAT( sym ) ) {
        sym->p.seg = CarveGetIndex( CarveSegData, sym->p.seg );
        sym->u.altdefs = CarveGetIndex( CarveSymbol, sym->u.altdefs );
    }
    if( sym->info & SYM_EXPORTED ) {
        if( FmtData.type & (MK_OS2 | MK_PE | MK_WIN_VXD) ) {
            sym->e.export = CarveGetIndex( CarveExportInfo, sym->e.export );
        }
    } else if( sym->e.def != NULL ) {
        sym->e.def = CarveGetIndex( CarveSymbol, sym->e.def );
    }
}

static void PrepNameTable( obj_name_list *list, perm_write_info *info )
/*********************************************************************/
{
    for( ; list != NULL; list = list->next ) {
        list->name.u.offs = AddStringStringTableOffs( &info->strtab, list->name.u.ptr );
    }
}

static void PrepFileList( perm_write_info *info )
/***********************************************/
{
    infilelist  *infile;
    char        new_name[PATH_MAX];

    for( infile = CachedFiles; infile != NULL; infile = infile->next ) {
        MakeFileName( infile, new_name );
        infile->name.u.offs = GetString( info, new_name );
    }
}

static bool PrepLeaders( void *_leader, void *info )
/**************************************************/
{
    seg_leader *leader = _leader;

    leader->segname.u.offs = GetString( info, leader->segname.u.ptr );
    return( false );
}

static void PrepClasses( perm_write_info *info )
/**********************************************/
{
    class_entry *class;

    for( class = Root->classlist; class != NULL; class = class->next_class ) {
        class->name.u.offs = GetString( info, class->name.u.ptr );
        RingLookup( class->segs, PrepLeaders, info );
    }
}

void WritePermFile( perm_write_info *info, const void *data, size_t len )
/***********************************************************************/
{
    QWrite( info->incfhdl, data, len, IncFileName );
}

static void FlushPermBuf( perm_write_info *info )
/***********************************************/
{
    size_t      modpos;
    size_t      adjust;

    modpos = info->currpos % MAX_HEADROOM;
    if( modpos == 0 )
        return;
    adjust = SECTOR_SIZE - ( info->currpos % SECTOR_SIZE );
    if( adjust != SECTOR_SIZE ) {
        memset( TokBuff + modpos, 0, adjust );
        info->currpos += adjust;
        modpos += adjust;
    }
    QWrite( info->incfhdl, TokBuff, modpos, IncFileName );
}

static void WriteStringBlock( void *info, const char *data, size_t size )
/***********************************************************************/
{
    QWrite( ((perm_write_info *)info)->incfhdl, data, size, IncFileName );
}

static void FiniStringBlock( stringtable *strtab, size_t *size, void *info,
                                            write_strtable_fn *writefn )
/************************************************************************/
{
    size_t      rawsize;

    rawsize = GetStringTableSize( strtab );
    *size = ROUND_UP( rawsize, SECTOR_SIZE );
    if( *size != rawsize ) {
        ZeroStringTable( strtab, *size - rawsize );
    }
    WriteStringTable( strtab, writefn, info );
    FiniStringTable( strtab );
}

static void DumpBlock( carve_t carver, void *block, void *_info )
/***************************************************************/
{
    perm_write_info *info = _info;
    size_t          size;

    size = CarveBlockSize( carver );
    CarveBlockScan( carver, block, info->prepfn, info );
    QWrite( info->incfhdl, CarveBlockData( block ), size, IncFileName );
}

static void DumpBlockStruct( carve_t carver, void (*markfn)(void *),
                             carve_info *loc, perm_write_info *info )
/*******************************************************************/
{
    CarveWalkAllFree( carver, markfn );
    loc->num = CarveNumElements( carver );
    CarveWalkBlocks( carver, DumpBlock, info );
}

static unsigned WriteSmallCarve( carve_t carver, void (*markfn)(void *),
                                 void (*writefn)(void *, void *),
                                 perm_write_info *info )
/**************************************************************************/
{
    CarveWalkAllFree( carver, markfn );
    CarveWalkAll( carver, writefn, info );
    return( CarveNumElements( carver ) );
}

static void PrepStartValue( inc_file_header *hdr )
/************************************************/
{
    if( StartInfo.mod != NULL && !StartInfo.user_specd ) {
        hdr->startmodidx = (cv_index)(pointer_uint)CarveGetIndex( CarveModEntry, StartInfo.mod );
        if( StartInfo.type == START_IS_SDATA ) {
            hdr->flags |= INC_FLAG_START_SEG;
            hdr->startidx = (cv_index)(pointer_uint)CarveGetIndex( CarveSegData, StartInfo.targ.sdata );
        } else {
            DbgAssert( StartInfo.type == START_IS_SYM );
            hdr->startidx = (cv_index)(pointer_uint)CarveGetIndex( CarveSymbol, StartInfo.targ.sym );
        }
        hdr->startoff = StartInfo.off;
    } else {
        hdr->startmodidx = 0;
    }
}

static void WriteAltData( perm_write_info *info )
/***********************************************/
{
    symbol      *sym;
    size_t      savepos;

    info->currpos = 0;
    for( sym = HeadSym; sym != NULL; sym = sym->link ) {
        if( (sym->info & SYM_IS_ALTDEF) && (sym->info & SYM_HAS_DATA) ) {
            savepos = info->currpos;
            VMemWritePermFile( info, sym->p.seg->u1.vm_ptr, sym->p.seg->length );
            sym->p.seg->u1.vm_offs = savepos;
        }
    }
    FlushPermBuf( info );
}

static unsigned_32 WriteLibList( perm_write_info *info, bool douser )
/*******************************************************************/
{
    unsigned_32 numlibs;
    file_list   *file;

    numlibs = 0;
    for( file = ObjLibFiles; file != NULL; file = file->next_file ) {
        if( (((file->flags & STAT_USER_SPECD) != 0) ^ douser) == 0 ) {
            U32WritePermFile( info, GetString( info, file->infile->name.u.ptr ) );
            BufWritePermFile( info, &file->priority, sizeof( file->priority ) );
            numlibs++;
        }
    }
    return( numlibs );
}

void WritePermData( void )
/*******************************/
{
    inc_file_header     hdr;
    perm_write_info     info;
    size_t              strsize;

    if( (LinkFlags & LF_INC_LINK_FLAG) == 0 || (LinkState & LS_LINK_ERROR) )
        return;
    InitStringTable( &info.strtab, false );
    AddCharStringTable( &info.strtab, '\0' );   // make 0 idx not valid
    info.incfhdl = QOpenRW( IncFileName );
    hdr.flags = 0;
    hdr.exename = GetString( &info, Root->outfile->fname );
    QModTime( Root->outfile->fname, &hdr.exemodtime );
    if( SymFileName != NULL ) {
        hdr.symname = GetString( &info, SymFileName );
        QModTime( SymFileName, &hdr.symmodtime );
    } else {
        hdr.symname = 0;
    }
    info.currpos = 0;
    BufWritePermFile( &info, &hdr, sizeof( inc_file_header ) ); // reserve space
    PrepClasses( &info );
    hdr.numgroups = WriteGroupsList( &info );
    hdr.numuserlibs = WriteLibList( &info, true );
    hdr.numdeflibs = WriteLibList( &info, false );
    if( FmtData.type & (MK_OS2 | MK_PE | MK_WIN_VXD) ) {
        PrepNameTable( FmtData.u.os2fam.mod_ref_list, &info );
        PrepNameTable( FmtData.u.os2fam.imp_tab_list, &info );
        hdr.numdllsyms = WriteSmallCarve( CarveDLLInfo, MarkDLLInfo, WriteDLLInfo, &info );
        hdr.numexports = WriteSmallCarve( CarveExportInfo, MarkExportInfo, WriteExportInfo, &info );
    }
    FlushPermBuf( &info );
    WriteHashPointers( &info );
    hdr.hdrsize = info.currpos;
    WriteAltData( &info );
    hdr.altdefsize = info.currpos;
    PrepFileList( &info );
    info.prepfn = PrepModEntry;
    DumpBlockStruct( CarveModEntry, MarkModEntry, &hdr.mods, &info );
    info.prepfn = PrepSegData;
    DumpBlockStruct( CarveSegData, MarkSegData, &hdr.segdatas, &info );
    info.prepfn = PrepSymbol;
    DumpBlockStruct( CarveSymbol, MarkSymbol, &hdr.symbols, &info );
    FiniStringBlock( &info.strtab, &strsize, &info, WriteStringBlock );
    hdr.strtabsize = strsize;
    QWrite( info.incfhdl, ReadRelocs, SizeRelocs, IncFileName );
    memcpy( hdr.signature, INC_FILE_SIG, INC_FILE_SIG_SIZE );
    hdr.rootmodidx = (cv_index)(pointer_uint)CarveGetIndex( CarveModEntry, Root->mods );
    hdr.headsymidx = (cv_index)(pointer_uint)CarveGetIndex( CarveSymbol, HeadSym );
    hdr.libmodidx = (cv_index)(pointer_uint)CarveGetIndex( CarveModEntry, LibModules );
    hdr.linkstate = (unsigned_32)( LinkState & ~LS_CLEAR_ON_INC );
    hdr.relocsize = SizeRelocs;
    PrepStartValue( &hdr );
    QSeek( info.incfhdl, 0, IncFileName );
    QWrite( info.incfhdl, &hdr, sizeof( inc_file_header ), IncFileName );
    QClose( info.incfhdl, IncFileName );
}

void ReadPermFile( perm_read_info *info, void *data, size_t len )
/***************************************************************/
{
    QRead( info->incfhdl, data, len, IncFileName );
}

static unsigned_32 BufPeekU32( perm_read_info *info )
/***************************************************/
{
    return( *((unsigned_32 *)( info->buffer + info->currpos )) );
}

static unsigned_32 BufReadU32( perm_read_info *info )
/***************************************************/
{
    unsigned_32 retval;

    retval = *((unsigned_32 *)( info->buffer + info->currpos ));
    info->currpos += sizeof( unsigned_32 );
    return( retval );
}

static void BufRead( perm_read_info *info, void *data, size_t len )
/*****************************************************************/
{
    memcpy( data, info->buffer + info->currpos, len );
    info->currpos += len;
}

static char *MapString( size_t off )
/**********************************/
{
    if( off == 0 )
        return( NULL );
    return( IncStrTab + off );
}

static void ReadGroupsList( unsigned count, perm_read_info *info )
/****************************************************************/
{
    incgroupdef         *def;
    unsigned_32         size;
    const char          **p;

    while( count-- ) {
        size = BufReadU32( info );
        _ChkAlloc( def, sizeof( incgroupdef ) + ( size - 1 ) * 2 * sizeof( char * ) );
        RingAppend( &IncGroupDefs, def );
        def->numsegs = size;
        def->grpname = MapString( BufReadU32( info ) );
        p = def->names;
        while( size-- ) {
            *(p++) = MapString( BufReadU32( info ) );
            *(p++) = MapString( BufReadU32( info ) );
        }
    }
}

static void ReadLibList( unsigned count, libnamelist **head, perm_read_info *info )
/*********************************************************************************/
{
    libnamelist         *list;

    while( count-- > 0 ) {
        _ChkAlloc( list, sizeof( libnamelist ) );
        list->name = MapString( BufReadU32( info ) );
        BufRead( info, &list->priority, sizeof( lib_priority ) );
        LinkList( head, list );
    }
}

static void RebuildDLLInfo( void *_dll, perm_read_info *info )
/************************************************************/
{
    dll_sym_info *dll = _dll;

    BufRead( info, dll, offsetof( dll_sym_info, iatsym ) );
    dll->m.modname.u.ptr = MapString( dll->m.modname.u.offs );
    if( !dll->isordinal ) {
        dll->u.entname.u.ptr = MapString( dll->u.entname.u.offs );
    }
}

static void RebuildExportInfo( void *_exp, perm_read_info *info )
/***************************************************************/
{
    entry_export *exp = _exp;

    BufRead( info, exp, offsetof( entry_export, sym ) );
    exp->next = CarveMapIndex( CarveExportInfo, exp->next );
    if( exp->name.u.offs != 0 ) {
        exp->name.u.ptr = MapString( exp->name.u.offs );
    }
    exp->impname = NULL;
}

static void RebuildModEntry( void *_mod, void *info )
/***************************************************/
{
    mod_entry *mod = _mod;

    /* unused parameters */ (void)info;

    if( *((unsigned_32 *)mod) == CARVE_INVALID_INDEX ) {
        CarveInsertFree( CarveModEntry, mod );
        return;
    }
    mod->n.next_mod = CarveMapIndex( CarveModEntry, mod->n.next_mod );
    mod->name.u.ptr = MapString( mod->name.u.offs );
    mod->publist = CarveMapIndex( CarveSymbol, mod->publist );
    mod->segs = CarveMapIndex( CarveSegData, mod->segs );
    mod->f.fname.u.ptr = MapString( mod->f.fname.u.offs );
}

static void RebuildSegData( void *_sdata, void *info )
/****************************************************/
{
    segdata *sdata = _sdata;

    /* unused parameters */ (void)info;

    if( *((unsigned_32 *)sdata) == CARVE_INVALID_INDEX ) {
        CarveInsertFree( CarveSegData, sdata );
        return;
    }
    sdata->next = CarveMapIndex( CarveSegData, sdata->next );  // dont use this?
    sdata->mod_next = CarveMapIndex( CarveSegData, sdata->mod_next );
    sdata->u.name.u.ptr = MapString( sdata->u.name.u.offs );
    sdata->o.clname.u.ptr = MapString( sdata->o.clname.u.offs );
}

static void RebuildSymbol( void *_sym, void *info )
/*************************************************/
{
    symbol *sym = _sym;

    /* unused parameters */ (void)info;

    if( *((unsigned_32 *)sym) == CARVE_INVALID_INDEX ) {
        CarveInsertFree( CarveSymbol, sym );
        return;
    }
    sym->hash = CarveMapIndex( CarveSymbol, sym->hash );
    sym->link = CarveMapIndex( CarveSymbol, sym->link );
    sym->publink = CarveMapIndex( CarveSymbol, sym->publink );
    sym->name.u.ptr = MapString( sym->name.u.offs );
    sym->info &= ~SYM_CLEAR_ON_INC;
    sym->mod = CarveMapIndex( CarveModEntry, sym->mod );
    if( IS_SYM_ALIAS( sym ) ) {
        sym->p.alias.u.ptr = MapString( sym->p.alias.u.offs );
    } else if( IS_SYM_IMPORTED( sym ) ) {
        if( FmtData.type & (MK_OS2 | MK_PE) ) {
            sym->p.import = CarveMapIndex( CarveDLLInfo, sym->p.import );
        }
    } else if( (sym->info & SYM_IS_ALTDEF) == 0 || IS_SYM_COMDAT( sym ) ) {
        sym->p.seg = CarveMapIndex( CarveSegData, sym->p.seg );
        sym->u.altdefs = CarveMapIndex( CarveSymbol, sym->u.altdefs );
    }
    if( sym->info & SYM_EXPORTED ) {
        if( FmtData.type & (MK_OS2 | MK_PE | MK_WIN_VXD) ) {
            sym->e.export = CarveMapIndex( CarveExportInfo, sym->e.export );
        }
    } else if( sym->e.def != NULL ) {
        sym->e.def = CarveMapIndex( CarveSymbol, sym->e.def );
    }
}

static void ReadBlockInfo( carve_t cv, void *blk, void *info )
/************************************************************/
{
    QRead( ((perm_read_info *)info)->incfhdl, CarveBlockData( blk ), CarveBlockSize( cv ), IncFileName );
}

static void SmallFreeCheck( void *data, void *_info )
/***************************************************/
{
    perm_read_info *info = _info;
    unsigned_32 freeblk;

    if( info->num > 0 ) {
        info->num--;
        freeblk = BufPeekU32( info );
        if( freeblk == CARVE_INVALID_INDEX ) {
            info->currpos += sizeof( unsigned_32 );
            CarveInsertFree( info->cv, data );
        } else {
            info->cbfn( data, info );
        }
    }
}

static void RebuildSmallCarve( carve_t cv, unsigned num,
                               void (*fn)(void *,perm_read_info *),
                               perm_read_info * info )
/*****************************************************************/
{
    info->cv = cv;
    info->num = num;
    info->cbfn = fn;
    CarveWalkAll( cv, SmallFreeCheck, info );
}

static void PurgeRead( perm_read_info *info )
/*******************************************/
{
    ClearHashPointers();
    CarvePurge( CarveGroup );
    CarvePurge( CarveModEntry );
    CarvePurge( CarveSegData );
    CarvePurge( CarveSymbol );
    if( FmtData.type & (MK_OS2 | MK_PE | MK_WIN_VXD) ) {
        CarvePurge( CarveDLLInfo );
        CarvePurge( CarveExportInfo );
    }
    _LnkFree( info->buffer );
    _LnkFree( IncStrTab );
    _LnkFree( ReadRelocs );
    _LnkFree( AltDefData );
    IncStrTab = NULL;
    ReadRelocs = NULL;
    AltDefData = NULL;
    if( OldExe != NULL ) {
        _LnkFree( OldExe );
        OldExe = NULL;
    }
}

static void ReadBinary( char **buf, unsigned_32 nameidx, time_t modtime )
/***********************************************************************/
{
    char                *fname;
    f_handle            hdl;
    size_t              size;

    fname = MapString( nameidx );
    hdl = QObjOpen( fname );
    if( hdl == NIL_FHANDLE ) {
        return;
    }
    if( QFModTime(hdl) != modtime ) {
        LnkMsg( WRN+MSG_EXE_DATE_CHANGED, "s", fname );
        return;
    }
    size = QFileSize( hdl );
    _ChkAlloc( *buf, size );
    QRead( hdl, *buf, size, fname );
    QClose( hdl, fname );
}

static void ReadStartInfo( inc_file_header *hdr )
/***********************************************/
{
    if( hdr->startmodidx != 0 ) {
        StartInfo.from_inc = true;
        StartInfo.mod = CarveMapIndex( CarveModEntry, (void *)(pointer_uint)hdr->startmodidx);
        if( hdr->flags & INC_FLAG_START_SEG ) {
            StartInfo.type = START_IS_SDATA;
            StartInfo.targ.sdata = CarveMapIndex( CarveSegData, (void *)(pointer_uint)hdr->startidx );
        } else {
            StartInfo.type = START_IS_SYM;
            StartInfo.targ.sym = CarveMapIndex( CarveSymbol, (void *)(pointer_uint)hdr->startidx );
        }
        StartInfo.off = hdr->startoff;
    }
}

void ReadPermData( void )
/******************************/
{
    perm_read_info      info;
    inc_file_header     *hdr;

    info.incfhdl = QObjOpen( IncFileName );
    if( info.incfhdl == NIL_FHANDLE )
        return;
    _ChkAlloc( info.buffer, SECTOR_SIZE );
    QRead( info.incfhdl, info.buffer, SECTOR_SIZE, IncFileName );
    hdr = (inc_file_header *)info.buffer;
    if( memcmp( hdr->signature, INC_FILE_SIG, INC_FILE_SIG_SIZE ) != 0 ) {
        LnkMsg( WRN+MSG_INV_INC_FILE, NULL );
        _LnkFree( info.buffer );
        QClose( info.incfhdl, IncFileName );
        return;
    }
    if( hdr->hdrsize > SECTOR_SIZE ) {
        _LnkRealloc( info.buffer, info.buffer, hdr->hdrsize );
        hdr = (inc_file_header *)info.buffer;   /* in case realloc moved it*/
        QRead( info.incfhdl, info.buffer + SECTOR_SIZE, hdr->hdrsize - SECTOR_SIZE, IncFileName );
    }
    info.currpos = sizeof( inc_file_header );
    CarveRestart( CarveModEntry, hdr->mods.num );
    CarveRestart( CarveSegData, hdr->segdatas.num );
    CarveRestart( CarveSymbol, hdr->symbols.num );
    if( FmtData.type & (MK_OS2 | MK_PE | MK_WIN_VXD) ) {
        CarveRestart( CarveDLLInfo, hdr->numdllsyms );
        CarveRestart( CarveExportInfo, hdr->numexports );
    }
    ReadHashPointers( &info );
    if( hdr->altdefsize > 0 ) {
        _ChkAlloc( AltDefData, hdr->altdefsize );
        QRead( info.incfhdl, AltDefData, hdr->altdefsize, IncFileName );
    }
    CarveWalkBlocks( CarveModEntry, ReadBlockInfo, &info );
    CarveWalkBlocks( CarveSegData, ReadBlockInfo, &info );
    CarveWalkBlocks( CarveSymbol, ReadBlockInfo, &info );
    _ChkAlloc( IncStrTab, hdr->strtabsize );
    QRead( info.incfhdl, IncStrTab, hdr->strtabsize, IncFileName );
    _ChkAlloc( ReadRelocs, hdr->relocsize );
    QRead( info.incfhdl, ReadRelocs, hdr->relocsize, IncFileName );
    QClose( info.incfhdl, IncFileName );
    ReadBinary( &OldExe, hdr->exename, hdr->exemodtime );
    if( OldExe == NULL ) {
        PurgeRead( &info );
        return;
    }
    if( hdr->symname != 0 ) {
        ReadBinary( &OldSymFile, hdr->symname, hdr->symmodtime );
        if( OldSymFile == NULL ) {
            PurgeRead( &info );
            return;
        }
    }
    ReadGroupsList( hdr->numgroups, &info );
    ReadLibList( hdr->numuserlibs, &SavedUserLibs, &info );
    ReadLibList( hdr->numdeflibs, &SavedDefLibs, &info );
    if( FmtData.type & (MK_OS2 | MK_PE | MK_WIN_VXD) ) {
        RebuildSmallCarve( CarveDLLInfo, hdr->numdllsyms, RebuildDLLInfo, &info );
        RebuildSmallCarve( CarveExportInfo, hdr->numexports, RebuildExportInfo, &info );
    }
    CarveWalkAll( CarveModEntry, RebuildModEntry, &info );
    CarveWalkAll( CarveSegData, RebuildSegData, &info );
    CarveWalkAll( CarveSymbol, RebuildSymbol, &info );
    Root->mods = CarveMapIndex( CarveModEntry, (void *)(pointer_uint)hdr->rootmodidx );
    HeadSym = CarveMapIndex( CarveSymbol, (void *)(pointer_uint)hdr->headsymidx );
    LibModules = CarveMapIndex( CarveModEntry, (void *)(pointer_uint)hdr->libmodidx );
    LinkState = (stateflag)hdr->linkstate | LS_GOT_PREV_STRUCTS | (LinkState & LS_CLEAR_ON_INC);
    ReadStartInfo( hdr );
    _LnkFree( info.buffer );
}

void PermSaveFixup( void *fix, size_t size )
/******************************************/
{
    AddBufferStringTable( &StoredRelocs, fix, size );
}

void IterateModRelocs( size_t offset, size_t sizeleft, size_t (*fn)(void *) )
/***************************************************************************/
{
    char        *fixoff;
    size_t      size;

    fixoff = ReadRelocs + offset;
    for( ; sizeleft > 0; sizeleft -= size ) {
        size = fn( fixoff );
        fixoff += size;
    }
}

static void SaveRelocData( void *_curr, const char *data, size_t size )
/*********************************************************************/
{
    char **curr = _curr;

    if( ReadRelocs == NULL && SizeRelocs > 0 ) {
        _ChkAlloc( ReadRelocs, SizeRelocs );
        *curr = ReadRelocs;
    }
    memcpy( *curr, data, size );
    *curr += size;
}

void IncP2Start( void )
/****************************/
{
    char   *spare;

    _LnkFree( ReadRelocs );
    ReadRelocs = NULL;
    _LnkFree( OldExe );
    OldExe = NULL;
    _LnkFree( OldSymFile );
    OldSymFile = NULL;
    _LnkFree( AltDefData );
    AltDefData = NULL;
    FiniStringBlock( &StoredRelocs, &SizeRelocs, &spare, SaveRelocData );
}

void PermStartMod( mod_entry *mod )
/****************************************/
{
    mod->relocs = GetStringTableSize( &StoredRelocs );
}

void PermEndMod( mod_entry *mod )
/**************************************/
{
    mod->sizerelocs = GetStringTableSize( &StoredRelocs ) - mod->relocs;
}

void *GetSegContents( segdata *sdata, virt_mem_size off )
/*******************************************************/
{
    if( OldSymFile != NULL && IS_DBG_INFO( sdata->u.leader ) ) {
        return( OldSymFile + off );
    }
    return( OldExe + off );
}

void *GetAltdefContents( segdata *sdata )
/***********************************************/
{
    return( (char *)AltDefData + sdata->u1.vm_offs );
}

void FreeSavedRelocs( void )
/*********************************/
{
    if( (LinkFlags & LF_INC_LINK_FLAG) == 0 ) {
        _LnkFree( ReadRelocs );
        ReadRelocs = NULL;
    }
}

void CleanPermData( void )
/*******************************/
{
#ifndef NDEBUG
    if( (LinkFlags & LF_INC_LINK_FLAG) == 0 ) {
        CarveVerifyAllGone( CarveLeader, "seg_leader" );
        CarveVerifyAllGone( CarveModEntry, "mod_entry" );
        CarveVerifyAllGone( CarveDLLInfo, "dll_sym_info" );
        CarveVerifyAllGone( CarveExportInfo, "entry_export" );
        CarveVerifyAllGone( CarveSymbol, "symbol" );
        CarveVerifyAllGone( CarveSegData, "segdata" );
        CarveVerifyAllGone( CarveClass, "class_entry" );
        CarveVerifyAllGone( CarveGroup, "group_entry" );
    }
#endif
    if( LinkState & LS_LINK_ERROR ) {
        QDelete( IncFileName );
    }
    CarveDestroy( CarveLeader );
    CarveDestroy( CarveModEntry );
    CarveDestroy( CarveDLLInfo );
    CarveDestroy( CarveExportInfo );
    CarveDestroy( CarveSymbol );
    CarveDestroy( CarveSegData );
    CarveDestroy( CarveClass );
    CarveDestroy( CarveGroup );
    FiniStringTable( &PrefixStrings);
    FiniStringTable( &PermStrings );
    FiniStringTable( &StoredRelocs );
    _LnkFree( IncFileName );
    _LnkFree( IncStrTab );
    _LnkFree( ReadRelocs );
    _LnkFree( OldExe );
    _LnkFree( OldSymFile );
    _LnkFree( AltDefData );
    RingFree( &IncGroupDefs );
    _LnkFree( IncGroups );
    FreeList( SavedUserLibs );
    FreeList( SavedDefLibs );
}
