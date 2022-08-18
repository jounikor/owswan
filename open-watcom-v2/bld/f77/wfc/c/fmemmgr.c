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
* Description:  FORTRAN compiler memory manager
*
****************************************************************************/


#include "ftnstd.h"
#include "errcod.h"
#include "stmtsw.h"
#include "global.h"
#include "fmemmgr.h"
#include "ferror.h"
#include "frl.h"
#include "inout.h"
#include "cle.h"
#include "fmeminit.h"
#include "utility.h"
#include "cspawn.h"
#include "wresmem.h"
#include "trmemcvr.h"


void    FMemInit( void ) {
//========================

    UnFreeMem = 0;
#if defined( TRMEM )
    TRMemOpen();
#else
    SysMemInit();
#endif
}

void    FMemErrors( void ) {
//========================

    ProgSw &= ~PS_ERROR; // we always want to report memory problems
    if( UnFreeMem > 0 ) {
        CompErr( CP_MEMORY_NOT_FREED );
    } else if( UnFreeMem < 0 ) {
        CompErr( CP_FREEING_UNOWNED_MEMORY );
    }
}


void    FMemFini( void ) {
//========================

#if defined( TRMEM )
    TRMemClose();
#else
    SysMemFini();
#endif
}


void    *FMemAlloc( size_t size ) {
//=================================

    void        *p;

#if defined( TRMEM )
    p = TRMemAlloc( size );
#else
    p = malloc( size );
#endif
    if( p == NULL ) {
        FrlFini( &ITPool );
#if defined( TRMEM )
        p = TRMemAlloc( size );
#else
        p = malloc( size );
#endif
        if( p == NULL ) {
            if( (ProgSw & PS_STMT_TOO_BIG) == 0
              && (StmtSw & SS_SCANNING) && (ITHead != NULL) ) {
                FreeITNodes( ITHead );
                ITHead = NULL;
                Error( MO_LIST_TOO_BIG );
                ProgSw |= PS_STMT_TOO_BIG;
            } else {
                ProgSw |= PS_FATAL_ERROR;
                PurgeAll(); // free up memory so we can process the error
                Error( MO_DYNAMIC_OUT );
                CSuicide();
            }
        } else {
            UnFreeMem++;
        }
    } else {
        UnFreeMem++;
    }
    return( p );
}

void    *wres_alloc( size_t size )
//================================
{
#if defined( TRMEM )
    return( TRMemAlloc( size ) );
#else
    return( malloc( size ) );
#endif
}

void    FMemFree( void *p ) {
//===========================

#if defined( TRMEM )
    TRMemFree( p );
#else
    free( p );
#endif
    UnFreeMem--;
}

void    wres_free( void *p )
//==========================
{
#if defined( TRMEM )
    TRMemFree( p );
#else
    free( p );
#endif
}

char *FStrDup( const char *buf )
//==============================
{
    char    *new;
    size_t  len;

    len = strlen( buf ) + 1;
    new = FMemAlloc( len );
    if( new != NULL ) {
        memcpy( new, buf, len );
    }
    return( new );
}

