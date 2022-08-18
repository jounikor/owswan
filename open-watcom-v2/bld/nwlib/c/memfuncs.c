/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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


#include "wlib.h"
#include "wresmem.h"
#ifdef TRMEM
#include "trmem.h"
#endif


static MemPtr   *memPtr;

#ifdef TRMEM
static _trmem_hdl   TRMemHandle;

extern void TRPrintLine( void *parm, const char *buff, size_t len )
/*****************************************************************/
{
    /* unused parameters */ (void)parm; (void)len;

    fprintf( stderr, "%s\n", buff );
}
#endif

void InitMem( void )
/******************/
{
#ifdef TRMEM
    TRMemHandle = _trmem_open( malloc, free, realloc, NULL,
            NULL, TRPrintLine,
            _TRMEM_ALLOC_SIZE_0 | _TRMEM_REALLOC_SIZE_0 |
            _TRMEM_OUT_OF_MEMORY | _TRMEM_CLOSE_CHECK_FREE );
#endif
    memPtr = NULL;
}


void *MemAlloc( size_t size )
/***************************/
{
    MemPtr *ptr;

    if( size == 0 ) {
        return( NULL );
    }
#ifdef TRMEM
    ptr = _trmem_alloc( size + sizeof( MemPtr ), _trmem_guess_who(), TRMemHandle );
#else
    ptr = malloc( size + sizeof( MemPtr ) );
#endif
    if( ptr == NULL )
        FatalError( ERR_NO_MEMORY );
    ptr->next = memPtr;
    ptr->prev = NULL;
    if( memPtr != NULL ) {
        memPtr->prev = ptr;
    }
    memPtr = ptr;
    return( ptr + 1 );
}

void *wres_alloc( size_t size )
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), TRMemHandle ) );
#else
    return( malloc( size ) );
#endif
}

void *MemRealloc( void *ptr, size_t size )
/****************************************/
{
    MemPtr  *mptr;

    if( ptr != NULL ) {
        mptr = ptr;
        mptr--;
        if( mptr == memPtr ) {
            memPtr = mptr->next;
        }
        if( mptr->prev != NULL ) {
            mptr->prev->next = mptr->next;
        }
        if( mptr->next != NULL ) {
            mptr->next->prev = mptr->prev;
        }
        ptr = mptr;
    }
#ifdef TRMEM
    mptr = _trmem_realloc( ptr, size + sizeof( MemPtr ), _trmem_guess_who(), TRMemHandle );
#else
    mptr = realloc( ptr, size + sizeof( MemPtr ) );
#endif
    if( mptr == NULL )
        FatalError( ERR_NO_MEMORY );
    mptr->next = memPtr;
    mptr->prev = NULL;
    if( memPtr != NULL ) {
        memPtr->prev = mptr;
    }
    memPtr = mptr;

    return( mptr + 1 );
}

void MemFree( void *ptr )
/***********************/
{
    MemPtr  *mptr;

    if( ptr == NULL )
        return;
    mptr = ptr;
    mptr--;
    if( mptr == memPtr ) {
        memPtr = mptr->next;
    }
    if( mptr->prev != NULL ) {
        mptr->prev->next = mptr->next;
    }
    if( mptr->next != NULL ) {
        mptr->next->prev = mptr->prev;
    }
#ifdef TRMEM
    _trmem_free( mptr, _trmem_guess_who(), TRMemHandle );
#else
    free( mptr );
#endif
}

void wres_free( void *ptr )
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), TRMemHandle );
#else
    free( ptr );
#endif
}

void *MemAllocGlobal( size_t size )
/*********************************/
{
    void *ptr;

#ifdef TRMEM
    ptr = _trmem_alloc( size, _trmem_guess_who(), TRMemHandle );
#else
    ptr = malloc( size );
#endif
    if( ptr == NULL && size != 0 )
        FatalError( ERR_NO_MEMORY );
    return( ptr );
}

void *MemReallocGlobal( void *ptr, size_t size )
/**********************************************/
{
#ifdef TRMEM
    ptr = _trmem_realloc( ptr, size, _trmem_guess_who(), TRMemHandle );
#else
    ptr = realloc( ptr, size );
#endif
    if( ptr == NULL && size != 0 )
        FatalError( ERR_NO_MEMORY );
    return( ptr );
}

void MemFreeGlobal( void *ptr )
/*****************************/
{
    if( ptr == NULL )
        return;
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), TRMemHandle );
#else
    free( ptr );
#endif
}

void FiniMem( void )
/******************/
{
#ifdef TRMEM
    _trmem_prt_usage( TRMemHandle );
    _trmem_close( TRMemHandle );
#else
    MemPtr  *mptr;

    while( (mptr = memPtr) != NULL ) {
        memPtr = mptr->next;
        free( mptr );
    }
#endif
}

char *DupStr( const char *str )
/*****************************/
{
    char *ptr;

    ptr = MemAlloc( strlen( str ) + 1 );
    strcpy( ptr, str );
    return( ptr );
}

char *DupStrGlobal( const char *str )
/***********************************/
{
    char *ptr;

    ptr = MemAllocGlobal( strlen( str ) + 1 );
    strcpy( ptr, str );
    return( ptr );
}
