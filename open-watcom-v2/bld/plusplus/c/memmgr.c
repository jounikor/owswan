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


#define USE_CG_MEMMGT

#include "plusplus.h"
#include <stddef.h>
#include "memmgr.h"
#include "ring.h"
#include "initdefs.h"
#include "pragdefn.h"
#include "codegen.h"
#ifndef NDEBUG
    #include "togglesd.h"
#endif

#ifdef TRMEM
    #include "trmem.h"
#endif


typedef struct cleanup *CLEANPTR;
struct cleanup {
    CLEANPTR            next;
    void                (*rtn)( void );
};

typedef struct perm_blk *PERMPTR;
typedef struct perm_blk {
    PERMPTR             next;
    size_t              amt_left;
    size_t              size;
    char                mem[1];
} perm_blk;
#define PERM_MAX_ALLOC  (1024)
#define PERM_MIN_ALLOC  (128)

static CLEANPTR cleanupList;
static PERMPTR permList;

#ifndef NDEBUG
static void *deferredFreeList;
#endif

#ifdef TRMEM

static _trmem_hdl   trackerHdl;

static void printLine( void *dummy, const char *buf, size_t len )
/***************************************************************/
{
    /* unused parameters */ (void)dummy; (void)len;

    fprintf( stdout, "%s\n", buf );
}

#define alloc_mem( size ) _trmem_alloc( size, _trmem_guess_who(), trackerHdl )

#else

  #ifdef USE_CG_MEMMGT
    #define alloc_mem( size ) BEMemAlloc( size )
  #else
    #define alloc_mem( size ) malloc( size )
  #endif

#endif



static void *alloc_from_cleanup( size_t amt )
/*******************************************/
{
    PERMPTR p;
    CLEANPTR curr;

    RingIterBeg( cleanupList, curr ) {
        curr->rtn();
        p = alloc_mem( amt );
        if( p != NULL ) {
            return( p );
        }
    } RingIterEnd( curr )
    return( NULL );
}

void CMemRegisterCleanup( void (*cleanup)( void ) )
/*************************************************/
{
    CLEANPTR new_cleanup;

    new_cleanup = (CLEANPTR)RingAlloc( &cleanupList, sizeof( *new_cleanup ) );
    new_cleanup->rtn = cleanup;
}

void *CMemAlloc( size_t size )
/****************************/
{
    void *p;

    if( size == 0 ) {
        return( NULL );
    }
#ifndef NDEBUG
    if( !TOGGLEDBG( no_mem_cleanup ) ) {
        CLEANPTR curr;
        static unsigned test_cleanup;
        static unsigned test_inc = 1;

        test_cleanup += test_inc;
        if( test_cleanup > 1000 ) {
            test_cleanup = 0;
            RingIterBeg( cleanupList, curr ) {
                curr->rtn();
            } RingIterEnd( curr )
        }
    }
#endif
    p = alloc_mem( size );
    if( p != NULL ) {
        return( p );
    }
    p = alloc_from_cleanup( size );
    if( p == NULL ) {
        CErr1( ERR_OUT_OF_MEMORY );
        CSuicide();
    }
    return( p );
}

#ifdef TRMEM
    #define _doFree( p )    _trmem_free( p, _trmem_guess_who(), trackerHdl );
#else
  #ifdef USE_CG_MEMMGT
    #define _doFree( p )    BEMemFree( p );
  #else
    #define _doFree( p )    free( p );
  #endif
#endif

void CMemFree( void *p )
/**********************/
{
    if( p != NULL ) {
        _doFree( p );
    }
}

void CMemFreePtr( void *pp )
/**************************/
{
    void *p;

    p = *(void **)pp;
    if( p != NULL ) {
        _doFree( p );
        *(void **)pp = NULL;
    }
}

#ifndef NDEBUG
void CMemDeferredFree( void *p )
/******************************/
{
    if( p != NULL ) {
        RingPush( &deferredFreeList, p );
    }
}
#endif

static void linkPerm( PERMPTR p, size_t amt )
{
    p->size = amt;
    p->amt_left = amt;
    RingPush( &permList, p );
}

static void addPerm( size_t size )
{
    PERMPTR p;
    size_t amt;

    if( size > PERM_MAX_ALLOC ) {
        p = CMemAlloc( offsetof( perm_blk, mem ) + size );
        linkPerm( p, size );
        return;
    }
    amt = PERM_MAX_ALLOC;
    for(;;) {
        p = alloc_mem( offsetof( perm_blk, mem ) + amt );
        if( p != NULL ) {
            linkPerm( p, amt );
            return;
        }
        p = alloc_from_cleanup( offsetof( perm_blk, mem ) + amt );
        if( p != NULL ) {
            linkPerm( p, amt );
            return;
        }
        if( amt == PERM_MIN_ALLOC )
            break;
        amt >>= 1;
    }
}

static void *cutPerm( PERMPTR find, size_t size )
{
    void *p;

    if( size <= find->amt_left ) {
        p = &(find->mem[find->size - find->amt_left]);
        find->amt_left -= size;
        return( p );
    }
    return( NULL );
}

void *CPermAlloc( size_t size )
/*****************************/
{
    void *p;
    PERMPTR find;

    size = _RoundUp( size, sizeof( int ) );
    RingIterBeg( permList, find ) {
        p = cutPerm( find, size );
        if( p != NULL ) {
            return( p );
        }
    } RingIterEnd( find )
    addPerm( size );
    RingIterBeg( permList, find ) {
        p = cutPerm( find, size );
        if( p != NULL ) {
            return( p );
        }
    } RingIterEnd( find )
    CErr1( ERR_OUT_OF_MEMORY );
    CSuicide();
    return( NULL );
}


static void cmemInit(           // INITIALIZATION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

#ifdef TRMEM
    {
        unsigned trmem_flags;

        trmem_flags = _TRMEM_ALLOC_SIZE_0 | _TRMEM_OUT_OF_MEMORY;
        if( CppGetEnv( "TRQUIET" ) == NULL ) {
            trmem_flags |= _TRMEM_CLOSE_CHECK_FREE;
        }
        trackerHdl = _trmem_open( malloc, free, NULL, NULL, NULL, printLine, trmem_flags );
    }
#elif defined( USE_CG_MEMMGT )
    BEMemInit();
#endif
#ifndef NDEBUG
    deferredFreeList = NULL;
#endif
    cleanupList = NULL;
    permList = NULL;
    addPerm( 0 );
}

static void cmemFini(           // COMPLETION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    RingFree( &permList );
#ifndef NDEBUG
    RingFree( &deferredFreeList );
#endif
#ifdef TRMEM
 #ifndef NDEBUG
    if( TOGGLEDBG( dump_memory ) ) {
        _trmem_prt_list( trackerHdl );
    }
    if( _trmem_close( trackerHdl ) != 0 && !CompFlags.compile_failed ) {
        // we can't print an error message since we have no more memory
  #if defined( __WWATCOMC__ )
        __trap();
  #endif
    }
 #endif
#elif defined( USE_CG_MEMMGT )
    BEMemFini();
#endif
}


INITDEFN( memmgr, cmemInit, cmemFini )


static void cleanupInit(        // INITIALIZATION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    cleanupList = NULL;
#if 0
    BEMemInit();
#endif
}


static void cleanupFini(        // COMPLETION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    RingFree( &cleanupList );
}

INITDEFN( mem_cleanup, cleanupInit, cleanupFini )
