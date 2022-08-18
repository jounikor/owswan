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
* Description:  Routines for creation of DOS EXE and COM files.
*
****************************************************************************/


#include <string.h>
#include "linkstd.h"
#include <exedos.h>
#include "pcobj.h"
#include "newmem.h"
#include "msg.h"
#include "alloc.h"
#include "reloc.h"
#include "wlnkmsg.h"
#include "virtmem.h"
#include "fileio.h"
#include "overlays.h"
#include "loadfile.h"
#include "objcalc.h"
#include "ring.h"
#include "dbgall.h"
#include "loaddos.h"


#ifdef _EXE

unsigned_32             OvlTabOffset;

static unsigned_32 WriteDOSRootRelocs( unsigned_32 mz_hdr_size )
/***************************************************************
 * write all relocs to the file
 */
{
    unsigned long       header_size;

    DumpRelocList( Root->reloclist );
    NullAlign( 0x10 );
    header_size = (unsigned long)Root->relocs * sizeof( dos_addr )
                    + mz_hdr_size;
    return( MAKE_PARA( header_size ) );
}

static void WriteDOSSectRelocs( section *sect, bool repos )
/**********************************************************
 * write all relocs associated with sect to the file
 */
{
    unsigned long       file_loc;
    OUTFILELIST         *out;

    if( sect->relocs != 0 ) {
        file_loc = sect->u.file_loc + MAKE_PARA( sect->size );
        out = sect->outfile;
        if( out->file_loc > file_loc ) {
            SeekLoad( file_loc );
        } else {
            if( repos ) {
                SeekLoad( out->file_loc );
            }
            if( out->file_loc < file_loc ) {
                PadLoad( file_loc - out->file_loc );
                out->file_loc = file_loc;
            }
        }
        file_loc += sect->relocs * sizeof( dos_addr );
        DumpRelocList( sect->reloclist );
        if( file_loc > out->file_loc ) {
            out->file_loc = file_loc;
        }
    }
}

static void AssignFileLocs( section *sect )
/*****************************************/
{
    if( FmtData.u.dos.pad_sections ) {
        sect->outfile->file_loc = ROUND_UP( sect->outfile->file_loc, SECTOR_SIZE );
    }
    sect->u.file_loc = sect->outfile->file_loc;
    sect->outfile->file_loc += MAKE_PARA( sect->size )
                            + MAKE_PARA( sect->relocs * sizeof( dos_addr ) );
    DEBUG((DBG_LOADDOS, "section %d assigned to %l in %s",
            sect->ovlref, sect->u.file_loc, sect->outfile->fname ));
}

static unsigned long WriteDOSData( unsigned_32 mz_hdr_size )
/***********************************************************
 * copy code from extra memory to loadfile
 */
{
    group_entry         *group;
    group_entry         *next_group;
    SECTION             *sect;
    unsigned long       header_size;
    outfilelist         *fnode;
    bool                repos;
    unsigned long       root_size;

    DEBUG(( DBG_BASE, "Writing data" ));
    OrderGroups( CompareDosSegments );
    CurrSect = Root;        // needed for WriteInfo.
    header_size = WriteDOSRootRelocs( mz_hdr_size );

    Root->u.file_loc = header_size;
    if( Root->areas != NULL ) {
        Root->outfile->file_loc = header_size + Root->size;
        WalkAreas( Root->areas, AssignFileLocs );
        OvlEmitTable();
    }

    /* keep track of positions within the file. */
    for( fnode = OutFiles; fnode != NULL; fnode = fnode->next ) {
        fnode->file_loc = 0;
    }
    Root->outfile->file_loc = Root->u.file_loc;
    Root->sect_addr = Groups->grp_addr;

    /* write groups and relocations */
    root_size = 0;
    for( group = Groups; group != NULL; group = next_group ) {
        next_group = group->next_group;
        sect = group->section;
        CurrSect = sect;
        fnode = sect->outfile;
        repos = WriteGroup( group );
        if( ( next_group == NULL ) || ( sect != next_group->section ) ) {
            if( sect == Root ) {
                root_size = fnode->file_loc;
            } else {
                WriteDOSSectRelocs( sect, repos );
            }
        }
        if( repos ) {
            SeekLoad( fnode->file_loc );
        }
    }
    return( root_size );
}

/*
 * the next set of 3 routines are a big kludge.  This is a lot of copycat
 * code from loadfile to allow the front chunk of a group to be split off
*/

static soffset      COMAmountWritten;

static bool WriteSegData( void *_sdata, void *_start )
/****************************************************/
{
    segdata         *sdata = _sdata;
    soffset         *start = _start;
    soffset         newpos;
    size_t          pad;

    if( !sdata->isuninit && !sdata->isdead ) {
        newpos = *start + sdata->a.delta;
        if( newpos + (soffset)sdata->length > 0 ) {
            if( newpos > COMAmountWritten ) {
                pad = newpos - COMAmountWritten;
                PadLoad( pad );
                WriteInfoLoad( sdata->u1.vm_ptr, sdata->length );
                COMAmountWritten += sdata->length + pad;
            } else {
                pad = COMAmountWritten - newpos;
                if( sdata->length > 0 ) {
                    WriteInfoLoad( sdata->u1.vm_ptr + pad, sdata->length - pad );
                }
                COMAmountWritten += sdata->length - pad;
            }
        }
    }
    return( false );
}

static bool DoCOMGroup( void *_seg, void *chop )
/**********************************************/
{
    seg_leader  *seg = _seg;
    soffset     newstart;

    newstart = *(soffset *)chop + SEG_GROUP_DELTA( seg );
    RingLookup( seg->pieces, WriteSegData, &newstart );
    return( false );
}

static bool WriteCOMGroup( group_entry *group, soffset chop )
/************************************************************
 * write the data for group to the loadfile
 * returns true if the file should be repositioned
 */
{
    unsigned long       file_loc;
    section             *sect;
    bool                repos;
    outfilelist         *finfo;

    repos = false;
    sect = group->section;
    CurrSect = sect;
    finfo = sect->outfile;
    file_loc = GROUP_FILE_LOC( group );
    if( file_loc > finfo->file_loc ) {
        PadLoad( file_loc - finfo->file_loc );
    } else if( file_loc < finfo->file_loc ) {
        SeekLoad( file_loc );
        repos = true;
    }
    DEBUG((DBG_LOADDOS, "group %a section %d to %l in %s",
            &group->grp_addr, sect->ovlref, file_loc, finfo->fname ));
    COMAmountWritten = 0;
    Ring2Lookup( group->leaders, DoCOMGroup, &chop );
    file_loc += COMAmountWritten;
    if( file_loc > finfo->file_loc ) {
        finfo->file_loc = file_loc;
    }
    return( repos );
}

static void WriteCOMFile( void )
/*******************************
 * generate a DOS .COM file.
 */
{
    outfilelist         *fnode;
    group_entry         *group;
    bool                repos;
    soffset             chop;

    if( StartInfo.addr.seg != 0 ) {
        LnkMsg( ERR+MSG_INV_COM_START_ADDR, NULL );
        return;
    }
    if( ( StackAddr.seg != 0 ) || ( StackAddr.off != 0 ) ) {
        LnkMsg( WRN+MSG_STACK_SEG_IGNORED, NULL );
    }
    OrderGroups( CompareDosSegments );
    CurrSect = Root;        // needed for WriteInfo.
    fnode = Root->outfile;
    fnode->file_loc = Root->u.file_loc = 0;
    Root->sect_addr = Groups->grp_addr;

    /* write groups */
    for( group = Groups; group != NULL; group = group->next_group ) {
        chop = SUB_REAL_ADDR( group->grp_addr, StartInfo.addr );
        if( chop > 0 ) {
            chop = 0;
        }
        if( (soffset)group->size + chop > 0 ) {
            repos = WriteCOMGroup( group, chop );
            if( repos ) {
                SeekLoad( fnode->file_loc );
            }
        }
    }
    if( fnode->file_loc > (_64KB - 0x200) ) {
        LnkMsg( ERR+MSG_COM_TOO_LARGE, NULL );
    }
    DBIWrite();
}

void FiniDOSLoadFile( void )
/***************************
 * terminate writing of load file
 */
{
    unsigned_32         hdr_size;
    unsigned_32         mz_hdr_size;
    unsigned_32         temp;
    unsigned_32         min_size;
    unsigned_32         root_size;
    dos_exe_header      exe_head;

    if( FmtData.type & MK_COM ) {
        WriteCOMFile();
        return;
    }
    if( FmtData.u.dos.full_mz_hdr ) {
        mz_hdr_size = 0x40;
    } else {
        mz_hdr_size = sizeof( dos_exe_header ) + sizeof( unsigned_32 );
    }
    SeekLoad( mz_hdr_size );
    root_size = WriteDOSData( mz_hdr_size );
    if( FmtData.type & MK_OVERLAYS ) {
        OvlPadOvlFiles();
    }
    /* output debug info into root main output file */
    CurrSect = Root;
    DBIWrite();
    hdr_size = MAKE_PARA( Root->relocs * sizeof( dos_addr ) + mz_hdr_size );
    DEBUG((DBG_LOADDOS, "root size %l, hdr size %l", root_size, hdr_size ));
    SeekLoad( 0 );
    _HostU16toTarg( DOS_SIGNATURE, exe_head.signature );
    temp = hdr_size / 16U;
    _HostU16toTarg( temp, exe_head.hdr_size );
    _HostU16toTarg( root_size % 512U, exe_head.mod_size );
    temp = ( root_size + 511U ) / 512U;
    _HostU16toTarg( temp, exe_head.file_size );
    _HostU16toTarg( Root->relocs, exe_head.num_relocs );

    min_size = MemorySize() - ( root_size - hdr_size ) + FmtData.SegMask;
    min_size >>= FmtData.SegShift;
    _HostU16toTarg( min_size, exe_head.min_16 );
    _HostU16toTarg( 0xffff, exe_head.max_16 );
    _HostU16toTarg( StartInfo.addr.off, exe_head.IP );
    _HostU16toTarg( StartInfo.addr.seg, exe_head.CS_offset );
    _HostU16toTarg( StackAddr.seg, exe_head.SS_offset );
    _HostU16toTarg( StackAddr.off, exe_head.SP );
    _HostU16toTarg( 0, exe_head.chk_sum );
    _HostU16toTarg( mz_hdr_size, exe_head.reloc_offset );
    _HostU16toTarg( 0, exe_head.overlay_num );
    WriteLoad( &exe_head, sizeof( dos_exe_header ) );
    WriteLoadU32( OvlTabOffset );
}

#endif
