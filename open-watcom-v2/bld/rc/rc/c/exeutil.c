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
* Description:  Resource Compiler executable processing utility functions.
*
****************************************************************************/


#include "global.h"
#include "rcrtns.h"
#include "rccore.h"
#include "exeutil.h"

#include "clibext.h"


/*
 * CopyExeData
 * NB When an error occurs the function MUST return without altering errno
 */
RcStatus CopyExeData( FILE *in_fp, FILE *out_fp, uint_32 length )
/***************************************************************/
{
    size_t          numread;
    size_t          bufflen;

    if( length == 0 ) {
        return( RS_PARAM_ERROR );
    }
    for( bufflen = IO_BUFFER_SIZE; length > 0; length -= bufflen ) {
        if( bufflen > length )
            bufflen = length;
        numread = RESREAD( in_fp, Pass2Info.IoBuffer, bufflen );
        if( numread != bufflen ) {
            return( RESIOERR( in_fp, numread ) ? RS_READ_ERROR : RS_READ_INCMPLT );
        }
        if( RESWRITE( out_fp, Pass2Info.IoBuffer, numread ) != numread ) {
            return( RS_WRITE_ERROR );
        }
    }
    return( RS_OK );
} /* CopyExeData */

/*
 * CopyExeDataTilEOF
 * NB when an error occurs this function MUST return without altering errno
 */
RcStatus CopyExeDataTilEOF( FILE *in_fp, FILE *out_fp )
/*****************************************************/
{
    size_t      numread;

    while( (numread = RESREAD( in_fp, Pass2Info.IoBuffer, IO_BUFFER_SIZE )) != 0 ) {
        if( numread != IO_BUFFER_SIZE && RESIOERR( in_fp, numread ) ) {
            return( RS_READ_ERROR );
        }
        if( RESWRITE( out_fp, Pass2Info.IoBuffer, numread ) != numread ) {
            return( RS_WRITE_ERROR );
        }
    }

    return( RS_OK );
} /* CopyExeDataTilEOF */

long AlignAmount( long offset, uint_16 shift_count )
/**************************************************/
{
    uint_32     low_bits;       /* low shift_count bits of offset */

    low_bits = offset & (0xffffffffUL >> (32 - shift_count));
    if( low_bits == 0 ) {
        return( 0 );
    } else {
        return( (0x1UL << shift_count) - low_bits );
    }
} /* AlignAmount */

static uint_32 FloorLog2( uint_32 value )
/***************************************/
/* This calculates the floor of the log base 2 of value. */
/* modified from binary_log function in wlink */
{
    uint_32 log;

    if( value == 0 ) {
        return( 0U );
    }
    log = 31;
    while( (value & 0x80000000UL) == 0 ) {  /* done if high bit on */
        value <<= 1;            /* shift left and decrease possible log. */
        log--;
    }
    return( log );
} /* FloorLog2 */

uint_16 FindShiftCount( uint_32 filelen, uint_16 numobjs )
/********************************************************/
/* filelen is the length of the file without any padding, numobjs is the */
/* number of objects that must appear on an alignment boundary */
{
    uint_16     shift_old;
    uint_16     shift;

    if( filelen < 0x10000L ) {
        return( 0 );
    }

    shift_old = 16;
    shift = (uint_16)( FloorLog2( filelen + numobjs * (1L << shift_old) ) - 15 );
    /* It is possible for the algorithm to blow up so don't check for != use <*/
    while( shift < shift_old ) {
        shift_old = shift;
        shift = (uint_16)( FloorLog2( filelen + numobjs * (1L << shift_old) ) - 15 );
    }

    /* In event of the rare case that the algorithm blew up take the min */
    if( shift > shift_old )
        shift = shift_old;
    return( shift );
} /* FindShiftCount */

/*
 *PadExeData
 * NB When an error occurs the function MUST return without altering errno
 */
RcStatus PadExeData( FILE *fp, long length )
/******************************************/
{
    size_t  numwrite;

    memset( Pass2Info.IoBuffer, 0, IO_BUFFER_SIZE );
    for( numwrite = IO_BUFFER_SIZE; length > 0; length -= numwrite ) {
        if( numwrite > length )
            numwrite = length;
        if( RESWRITE( fp, Pass2Info.IoBuffer, numwrite ) != numwrite ) {
            return( RS_WRITE_ERROR );
        }
    }
    return( RS_OK );
} /* PadExeData */

void CheckDebugOffset( ExeFileInfo * exe )
/****************************************/
{
    uint_32     curroffset;

    curroffset = RESTELL( exe->fp );
    if( curroffset > exe->DebugOffset ) {
        exe->DebugOffset = curroffset;
    }
} /* CheckDebugOffset */

unsigned_32 OffsetFromRVA( ExeFileInfo *exe, pe_va rva )
/******************************************************/
{
    pe_object           *objects;
    unsigned_16         obj_cnt;
    unsigned            i;
    exe_pe_header       *pehdr;

    pehdr = exe->u.PEInfo.WinHead;
    if( IS_PE64( *pehdr ) ) {
        obj_cnt = PE64( *pehdr ).num_objects;
    } else {
        obj_cnt = PE32( *pehdr ).num_objects;
    }
    objects = exe->u.PEInfo.Objects;
    for( i = 0; i < obj_cnt; i++ ) {
        if( objects[i].rva == rva )
            break;
        if( objects[i].rva > rva ) {
            if( i != 0 )
                i--;
            break;
        }
    }
    if( i == obj_cnt )
        i--;
    if( objects[i].rva > rva )
        return( 0xFFFFFFFF );
    return( objects[i].physical_offset + rva - objects[i].rva );
}

/*
 * SeekRead
 * NB When an error occurs the function MUST return without altering errno
 */
RcStatus SeekRead( FILE *fp, long newpos, void *buff, size_t size )
/*****************************************************************/
/* seek to a specified spot in the file, and read some data */
{
    size_t      numread;

    if( RESSEEK( fp, newpos, SEEK_SET ) )
        return( RS_READ_ERROR );
    numread = RESREAD( fp, buff, size );
    if( numread != size ) {
        return( RESIOERR( fp, numread ) ? RS_READ_ERROR : RS_READ_INCMPLT );
    }
    return( RS_OK );

} /* SeekRead */

/* location within a windows executable of the offset of the os2_exe_header */
#define WIN_EXE_HEADER_OFFSET           0x3cL
#define DOS_RELOCATION_OFFSET           0x18L
#define DOS_EXE_SIGNATURE               0x5a4d

/* If the value at DOS_RELOCATION_ADDRESS_OFFSET < */
/* WIN_EXE_HEADER_OFFSET + sizeof( uint_32 ) then the DOS reloction */
/* information starts before the end of the address of the os2_exe_header */
/* so this is not a valid windows EXE file. */

ExeType FindNEPELXHeader( FILE *fp, unsigned_32 *nh_offset )
/**********************************************************/
/* Determine type of executable */
{
    os2_exe_header  ne_header;
    unsigned_16     data;
    RcStatus        rc;

    rc = SeekRead( fp, 0, &data, sizeof( data ) );
    if( rc != RS_OK )
        return( EXE_TYPE_UNKNOWN );
    if( data != DOS_EXE_SIGNATURE )
        return( EXE_TYPE_UNKNOWN );

    rc = SeekRead( fp, DOS_RELOCATION_OFFSET, &data, sizeof( data ) );
    if( rc != RS_OK )
        return( EXE_TYPE_UNKNOWN );

    if( data < WIN_EXE_HEADER_OFFSET + sizeof( uint_32 ) ) {
        return( EXE_TYPE_UNKNOWN );
    }

    rc = SeekRead( fp, WIN_EXE_HEADER_OFFSET, nh_offset, sizeof( uint_32 ) );
    if( rc != RS_OK )
        return( EXE_TYPE_UNKNOWN );

    rc = SeekRead( fp, *nh_offset, &data, sizeof( unsigned_16 ) );
    if( rc != RS_OK )
        return( EXE_TYPE_UNKNOWN );

    switch( data ) {
    case OS2_SIGNATURE_WORD:
        rc = SeekRead( fp, *nh_offset, &ne_header, sizeof( ne_header ) );
        if( rc != RS_OK )
            return( EXE_TYPE_UNKNOWN );
        if( ne_header.target == TARGET_OS2 )
            return( EXE_TYPE_NE_OS2 );
        if( ne_header.target == TARGET_WINDOWS || ne_header.target == TARGET_WIN386 )
            return( EXE_TYPE_NE_WIN );
        return( EXE_TYPE_UNKNOWN );
        break;
    case PE_SIGNATURE:
        return( EXE_TYPE_PE );
        break;
    case OSF_FLAT_LX_SIGNATURE:
        return( EXE_TYPE_LX );
        break;
    default:
        return( EXE_TYPE_UNKNOWN );
        break;
    }
} /* FindNEPEHeader */
