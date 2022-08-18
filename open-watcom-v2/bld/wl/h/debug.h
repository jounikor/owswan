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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#define _ON             1
#define _OFF            0

#define DBG_ALWAYS      0x00
#define DBG_BASE        0x01
#define DBG_OLD         0x02
#define DBG_NEW         0x04
#define DBG_LOADDOS     0x08
#define DBG_VIRTMEM     0x10
#define DBG_DEADCODE    0x20
#define DBG_DBGINFO     0x40
#define DBG_NOCRLF      0x8000
#define DBG_INFO_MASK   0x7FFF

#ifdef _INT_DEBUG

#include <stdio.h>

    extern void _Debug( unsigned int, const char *, ... );
    extern void LPrint( const char *str, ... ); // link print
    #define DEBUG( x ) _Debug x ;
    #define PRINTLOC   printf("%s(%d)\n", __FILE__, __LINE__);
    extern bool CanReadWord( void *p );  // Can we read from p?

    typedef enum {
        DUMP_BYTE,
        DUMP_WORD,
        DUMP_DWORD,
        DUMP_MAX
    } DbgDumpType;

    extern void             PrintMemDump( const void *p, unsigned long size, DbgDumpType type );
    // Trec: will print a history of last few calls to it as soon as called
    // with traceHit being true.
    extern long unsigned    TrecCount;
    extern bool             TrecHit;
    extern void             Trec( const char *s, ... );  // trace record
    #define TREC Trec("%s(%d)", __FILE__, __LINE__);

    // SpyWrite: Set this variable to an offset of a file you want to spy
    // at. When a block at that particular offset is about to be written out,
    // the block will be displayed.
    extern long unsigned    SpyWrite;

    extern unsigned         Debug;

#else

    #define PRINTLOC
    #define DEBUG( x )
    #define CanReadWord( x )    false
    #define PrintMemDump( p, size, type )

#endif
