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
* Description:  I/O routines for wpack.
*
****************************************************************************/


#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__UNIX__) || defined( __WATCOMC__ )
    #include <utime.h>
#else
    #include <sys/utime.h>
#endif
#include "wio.h"
#include "wpack.h"
#include "txttable.h"
#include "common.h"
#include "message.h"
#include "wpackio.h"

#include "clibext.h"


int IOStatus;
static unsigned long infile_posn;

void WriteMsg( const char *msg )
/******************************/
{
    write( STDOUT_FILENO, msg, strlen( msg ) );
}

#if _WPACK
void WriteLen( const char *msg, int len1 )
/****************************************/
{
    write( STDOUT_FILENO, msg, len1 );
}

void IndentLine( int amount )
/**********************************/
// grossly inefficient, but it doesn't really matter
{
    while( amount > 0 ) {
        write( STDOUT_FILENO, " ", 1 );
        amount--;
    }
}

void WriteNumeric( const char *msg, unsigned long num )
/*****************************************************/
{
    char    buf[ 11 ];

    WriteMsg( msg );
    ultoa( num, buf, 10 );
    WriteMsg( buf );
    write( STDOUT_FILENO, "\n", 1 );
}
#endif

static void IOError( unsigned errnum, const char *name )
/******************************************************/
// also ineffiecient, but it doesn't matter.
{
    char errstr[ 160 ];

    strcpy( errstr, LookupText( NULL, errnum ) );
    if( name != NULL ) {
        strcat( errstr, name );
    }
    strcat( errstr, ": " );
    strcat( errstr, strerror( errno ) );
    Error( errnum, errstr );
}

int QRead( int file, void *buffer, int amount )
/****************************************************/
{
    int amtread;

    amtread = read( file, buffer, amount );
    if( amtread == -1 ) {
        IOError( TXT_ERR_READ, NULL );
    }
    return( amtread );
}

int QWrite( int file, void *buffer, int amount )
/*****************************************************/
{
    int result;
    char *msg;

    result = write( file, buffer, amount );
    if( result == -1 ) {
        IOError( TXT_ERR_WRIT, NULL );
    } else if( result != amount ) {
        msg = LookupText( NULL, TXT_DISK_FULL );
        Error( TXT_DISK_FULL, msg );
        result = -1;
    }
    BumpStatus( amount );
    return( result );
}

int QOpenR( const char *filename )
/********************************/
{
    int result;

    result = open( filename, O_BINARY | O_RDONLY, 0 );
    if( result == -1 ) {
        IOError( TXT_NOT_OPEN, filename );
    }
    return( result );
}

int NoErrOpen( const char *filename )
/***********************************/
{
    infile_posn = ~0L;
    return( open( filename, O_BINARY | O_RDONLY, 0 ) );
}

int QOpenW( const char *filename )
/********************************/
{
    int result;

    result = open( filename, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, PMODE_RWX );
    if( result == -1 ) {
        IOError( TXT_NOT_OPEN, filename );
    }
    return( result );
}

#if _WPACK
int QOpenM( const char *filename )
/********************************/
{
    int result;

    result = open( filename, O_BINARY | O_RDWR, 0 );
    if( result == -1 ) {
        IOError( TXT_NOT_OPEN, filename );
    }
    return( result );
}

unsigned long QFileLen( int file )
/***************************************/
{
    return( filelength( file ) );
}

unsigned long QGetDate( int handle )
/*********************************************/
{
    struct stat     statblk;

    fstat( handle, &statblk );
    return( statblk.st_mtime );
}
#endif

void QSeek( int file, long position, int seektype )
/*************************************************/
{
    lseek( file, position, seektype );
}

void QClose( int file )
/****************************/
{
    close( file );
}

void QSetDate( const char *fname, unsigned long stamp )
/******************************************************/
{
    struct stat     statblk;
    struct utimbuf  timebuf;

    stat( fname, &statblk );
    timebuf.modtime = stamp;
    timebuf.actime = statblk.st_atime;
    utime( fname, &timebuf );
}

/*
 * i/o under DOS is much faster if all reads and writes are a multiple of the
 * sector size and occur on a sector boundary. This buffering stuff is used to
 * make this happen.
*/

#define SECTOR_SIZE 512         // has to be a power of 2
#define READ_SIZE (SECTOR_SIZE * 32)
#define WRITE_SIZE (SECTOR_SIZE * 32)

#if _WPACK
static char *   CurrWrite;
static char *   WriteBuf;
static char *   SaveBuf;
static unsigned     SaveLen;
static int          SaveHandle;
#else
static char *       CurrWrite;
static char *       WriteBuf;
#endif
static char *   CurrRead;
static char *   ReadBuf;
static int          BytesRead;
static int          ReadLimit;
static unsigned     BytesWrote;
static unsigned_32  CRC;

/*
 * this uses the "CRC-32" polynomial for calculating the CRC (also used by
 * Ethernet, Token Ring, etc..). For a good description of this algorithm,
 * see "C Programmer's Guide to NetBIOS" by W. David Schwaderer.
*/

#define GOOD_CRC    0xDEBB20E3

#define CALC_CRC( data, index ) index = CRC^data;CRC = (CRC>>8)^CRCTable[index];

// there are 256 elements in this array.

static unsigned_32  CRCTable[] = {
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
      0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
      0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
      0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
      0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
      0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
      0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
      0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
      0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
      0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
      0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
      0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
      0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
      0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
      0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
      0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
      0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
      0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
      0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
      0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
      0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
      0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
      0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
      0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
      0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
      0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
      0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
      0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
      0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
      0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
      0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
      0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
      0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
      0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
      0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
      0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
      0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
      0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
      0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
      0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
      0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
      0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
      0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
      0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
      0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
      0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
      0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
      0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
      0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
      0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
      0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
      0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

int InitIO( void )
/************************/
{
//  _nheapgrow();   called in main()
    ReadBuf = (char *)WPMemAlloc( READ_SIZE + 3 ) + 3;  // so we can "unread"
    WriteBuf = WPMemAlloc( WRITE_SIZE ); // 3 bytes
    if( ReadBuf == NULL  ||  WriteBuf == NULL ) {
        return( false );
    }
    BytesRead = READ_SIZE + 1;
    ReadLimit = 0;              // force reloading of buffer.
    CurrWrite = WriteBuf;
    BytesWrote = 0;
    CRC = 0xFFFFFFFF;
    IOStatus = OK;
    infile_posn = ~0L;
    return( true );
}

void FiniIO( void )
/************************/
{
    if( ReadBuf != NULL ) {
        WPMemFree( ReadBuf );
    }
    if( WriteBuf != NULL ) {
        WPMemFree( WriteBuf );
    }
}

int BufSeek( unsigned long position )
/******************************************/
{
    unsigned long   sect_start;
    int             result;

    result = ReadLimit;
    sect_start = position & ~(READ_SIZE - 1);
    /* avoid re-reading buffer if we are at the correct location */
    if( sect_start != infile_posn ) {
        infile_posn = sect_start;
        QSeek( infile, sect_start, SEEK_SET );
        result = QRead( infile, ReadBuf, READ_SIZE );
        ReadLimit = result;
    }
    BytesRead = position - sect_start;
    if( BytesRead < ReadLimit ) {
        CurrRead = ReadBuf + BytesRead;
        IOStatus = OK;
    } else {
        result = -1;
    }
    return( result );
}

#if _WPACK
void CopyInfo( int dstfile, int srcfile, unsigned long len1 )
/*****************************************************************/
// make sure nothing is being buffered when this routine is being called.
{
    while( len1 > READ_SIZE ) {
        QRead( srcfile, ReadBuf, READ_SIZE );
        QWrite( dstfile, ReadBuf, READ_SIZE );
        len1 -= READ_SIZE;
    }
    if( len1 > 0 ) {
        QRead( srcfile, ReadBuf, len1 );
        QWrite( dstfile, ReadBuf, len1 );
    }
}

int WriteSeek( unsigned long position )
/********************************************/
// preload write buffer with the information at the start of the sector
// containing position.
{
    unsigned long   sect_start;
    int             len1;

    sect_start = position & ~(SECTOR_SIZE - 1);
    QSeek( outfile, sect_start, SEEK_SET );
    len1 = position - sect_start;
    if( len1 > 0 ) {
        if( QRead( outfile, WriteBuf, SECTOR_SIZE ) < len1 ) {
            return( -1 );
        }
        QSeek( outfile, sect_start, SEEK_SET );
        BytesWrote = len1;
        CurrWrite += len1;
    }
    return( len1 );
}
#endif

static void ReloadTheBuffer( void )
/*********************************/
{
    int     amtread;

#ifdef GUISETUP
    {
        extern void QueryCancel();

        QueryCancel();
    }
#endif
    infile_posn += (unsigned)ReadLimit;
    amtread = QRead( infile, ReadBuf, READ_SIZE );
    CurrRead = ReadBuf;
    if( amtread <= 0 ) {    // no data, so it is EOF
        CurrRead = ReadBuf;
        if( amtread == -1 ) {
            IOStatus = IO_PROBLEM;
        } else {
            IOStatus = AT_EOF;
        }
    } else {
        ReadLimit = amtread;
    }
    BytesRead = 0;
}

byte EncReadByte( void )
/*****************************/
{
    byte    data;

    if( BytesRead >= ReadLimit ) {
        ReloadTheBuffer();
    }
    data = *CurrRead;
    CurrRead++;
    BytesRead++;
    return( data );
}

void UnReadByte( byte value )
/**********************************/
{
    CurrRead--;
    *CurrRead = value;
    BytesRead--;
}

byte DecReadByte( void )
/*****************************/
{
    byte    index;
    byte    data;

    if( BytesRead >= ReadLimit ) {
        ReloadTheBuffer();
    }
    data = *CurrRead;
    CurrRead++;
    BytesRead++;
    CALC_CRC( data, index );
    return( data );
}

#if _WPACK
void EncWriteByte( byte c )
/********************************/
{
    byte    index;

    DecWriteByte( c );
    CALC_CRC( c, index );
}
#endif

static void WriteTheBuffer( void )
/********************************/
{
    int amtwrote;

    amtwrote = QWrite( outfile, WriteBuf, WRITE_SIZE );
    if( amtwrote == -1 ) {
        IOStatus = IO_PROBLEM;
    }
    CurrWrite = WriteBuf;
    BytesWrote = 0;
}

void DecWriteByte( byte c )
/********************************/
{
    *CurrWrite = c;
    CurrWrite++;
    BytesWrote++;
    if( BytesWrote >= WRITE_SIZE ) {
        WriteTheBuffer();
    }
}

void FlushWrite( void )
/****************************/
{
    if( BytesWrote != 0 ) {
        QWrite( outfile, WriteBuf, BytesWrote );
        CurrWrite = WriteBuf;
        BytesWrote = 0;
    }
    IOStatus = OK;
}

void FlushRead( void )
/***************************/
{
    BytesRead = READ_SIZE + 1;     // force a read next time
    infile_posn = ~0L;
    IOStatus = OK;
}

#if _WPACK
void SwitchBuffer( int handle, bool iswrite, void *buf )
/*****************************************************************/
{
    if( iswrite ) {
        SaveHandle = outfile;
        outfile = handle;
        SaveBuf = WriteBuf;
        SaveLen = BytesWrote;
        BytesWrote = 0;
        WriteBuf = buf;
        CurrWrite = buf;
    } else {
        SaveHandle = infile;
        infile = handle;
        BytesRead = READ_SIZE + 1;
        infile_posn = ~0L;
    }
}

void RestoreBuffer( bool iswrite )
/***************************************/
{
    if( iswrite ) {
        outfile = SaveHandle;
        WriteBuf = SaveBuf;
        BytesWrote = SaveLen;
        CurrWrite = WriteBuf + BytesWrote;
    } else {
        infile = SaveHandle;
        BytesRead = READ_SIZE + 1;
        infile_posn = ~0L;
    }
}

void WriteFiller( unsigned amount )
/*****************************************/
// note this assumes that a write past the end of the buffer won't happen.
{
    BytesWrote += amount;
    CurrWrite += amount;
}

unsigned_32 GetCRC( void )
/*******************************/
{
    unsigned_32 result;

    result = ~CRC;
    CRC = 0xFFFFFFFF;       // reset it for the next file.
    return( result );
}
#endif

void ModifyCRC( unsigned long *value, byte data )
/******************************************************/
// this is used for adjusting the value of the CRC stored in the file to take
// into account the few extra bytes of "read ahead" done by the compression
// routines.
{
    byte            crcidx;
    unsigned long   SaveCRC;

    SaveCRC = CRC;
    CRC = ~(*value);        // undo post conditioning
    CALC_CRC( data, crcidx );
    *value = ~CRC;
    CRC = SaveCRC;
}

bool CheckCRC( unsigned_32 value )
/***************************************/
{
    byte     crcidx;
    int      loopidx;
    byte *   crcptr;
    bool     result;

    crcptr = (byte *) &value;
    for( loopidx = 1; loopidx <= 4; loopidx++ ) {
        CALC_CRC( *crcptr, crcidx );        // NOTE: this assumes intel byte
        crcptr++;                           // ordering.
    }
    result = CRC == GOOD_CRC;
    CRC = 0xFFFFFFFF;           // reset for next file.
    return( result );
}
