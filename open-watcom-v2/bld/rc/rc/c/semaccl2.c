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
* Description:  OS/2 Accelerator table related semantic actions.
*
****************************************************************************/


#include "global.h"
#include "rcerrors.h"
#include "semantic.h"
#include "semantc2.h"
#include "reserr.h"
#include "rcrtns.h"
#include "rccore.h"


static bool ResOS2WriteAccelEntry( AccelTableEntryOS2 *currentry, FILE *fp )
/**************************************************************************/
{
    bool                error;

    error = ResWriteUint16( currentry->Flags, fp );
    if( !error ) {
        error = ResWriteUint16( currentry->Ascii, fp );
    }
    if( !error ) {
        error = ResWriteUint16( currentry->Id, fp );
    }
    return( error );
}

#define CTRL_EVENT  0x8000

const FullAccelFlagsOS2 DefaultAccelFlagsOS2 = { 0, false };

int SemOS2StrToAccelEvent( char * string )
/*************************************/
{
    if( *string == '^' ) {
        /* control character requested */
        string++;
        if( isalpha( *string ) ) {
            return( *string | CTRL_EVENT );
        } else {
            return( 0 );
        }
    } else if( isprint( *string ) ) {
        /* only accept printable characters in this position */
        return( *string );
    } else {
        return( 0 );
    }
}

static void CheckAccelFlags( uint_16 * flags, unsigned long idval )
/********************************************************************/
{
    /* unused parameters */ (void)idval;

    /* CHAR is the default */
    if( !( *flags & OS2_ACCEL_VIRTUALKEY ) && !( *flags & OS2_ACCEL_CHAR ) )
        *flags |= OS2_ACCEL_CHAR;
#if 0
    if( !( *flags & OS2_ACCEL_VIRTUALKEY ) ) {
        if( *flags & OS2_ACCEL_SHIFT ) {
            *flags &= ~OS2_ACCEL_SHIFT;
            RcWarning( ERR_ACCEL_KEYWORD_IGNORED, "SHIFT", idval );
        }
        if( *flags & OS2_ACCEL_CONTROL ) {
            *flags &= ~OS2_ACCEL_CONTROL;
            RcWarning( ERR_ACCEL_KEYWORD_IGNORED, "CONTROL", idval );
        }
    }
#endif
}

FullAccelEntryOS2 SemOS2MakeAccItem( AccelEvent event, unsigned long idval,
                    FullAccelFlagsOS2 flags )
/*************************************************************************/
{
    FullAccelEntryOS2      entry;

    memset( &entry, 0, sizeof( entry ) );
//    if( event.strevent || flags.typegiven ) {
        CheckAccelFlags( &flags.flags, idval );
        entry.entry.Ascii = event.event;
        entry.entry.Flags = flags.flags;
        entry.entry.Id = idval;
        if( event.event & CTRL_EVENT ) {
            entry.entry.Ascii  = event.event & ~CTRL_EVENT;
            entry.entry.Flags |= OS2_ACCEL_CTRL;
        }

//    } else {
//        RcError( ERR_ACCEL_NO_TYPE, idval );
//        ErrorHasOccured = true;
//        entry.entry.Ascii = 0;
//        entry.entry.Flags = 0;
//        entry.entry.Id = 0;
//    }

    return( entry );
}

FullAccelTableOS2 *SemOS2NewAccelTable( FullAccelEntryOS2 firstentry )
/***************************************************************/
{
    FullAccelTableOS2   *newtable;
    FullAccelEntryOS2   *newentry;

    newtable = RESALLOC( sizeof( FullAccelTableOS2 ) );
    newentry = RESALLOC( sizeof( FullAccelEntryOS2 ) );

    if( newtable == NULL || newentry == NULL ) {
        RcError( ERR_OUT_OF_MEMORY );
        ErrorHasOccured = true;
        return( NULL );
    }

    *newentry = firstentry;
    newtable->head = NULL;
    newtable->tail = NULL;

    ResAddLLItemAtEnd( (void **)&(newtable->head), (void **)&(newtable->tail), newentry );

    return( newtable );
}

FullAccelTableOS2 *SemOS2AddAccelEntry( FullAccelEntryOS2 currentry, FullAccelTableOS2 * currtable )
/**************************************************************************************************/
{
    FullAccelEntryOS2     *newentry;

    newentry = RESALLOC( sizeof( FullAccelEntryOS2 ) );

    if( newentry == NULL ) {
        RcError( ERR_OUT_OF_MEMORY );
        ErrorHasOccured = true;
        return( NULL );
    }

    *newentry = currentry;

    ResAddLLItemAtEnd( (void **)&(currtable->head), (void **)&(currtable->tail), newentry );

    return( currtable );
}

static void SemOS2FreeAccelTable( FullAccelTableOS2 * acctable )
/**************************************************************/
{
    FullAccelEntryOS2   *currentry;
    FullAccelEntryOS2   *nextentry;

    for( currentry = acctable->head; currentry != NULL; currentry = nextentry ) {
        nextentry = currentry->next;
        RESFREE( currentry );
    }
    RESFREE( acctable );
}

static int SemOS2CountAccelTableEntries( FullAccelTableOS2 *acctable )
/********************************************************************/
{
    FullAccelEntryOS2   *currentry;
    int                 count;

    count = 0;
    for( currentry = acctable->head; currentry != NULL; currentry = currentry->next ) {
        count++;
    }
    return( count );
}

static bool writeAccelTableEntries( FullAccelTableOS2 *acctable,
                                   FILE *fp, uint_32 codepage )
/**************************************************************/
{
    FullAccelEntryOS2   *currentry;
    bool                error;

    error = ResWriteUint16( SemOS2CountAccelTableEntries( acctable ), fp );
    if( !error ) {
        error = ResWriteUint16( codepage, fp );
    }
    for( currentry = acctable->head; currentry != NULL && !error; currentry = currentry->next ) {
        error = ResOS2WriteAccelEntry( &currentry->entry, fp );
    }
    return( error );
}

void SemOS2WriteAccelTable( WResID *name, ResMemFlags flags, uint_32 codepage,
                                                FullAccelTableOS2 *acctable )
/****************************************************************************/
{
    ResLocation     loc;
    bool            error;
    int             err_code;

    if( !ErrorHasOccured ) {
        loc.start = SemStartResource();
        error = writeAccelTableEntries( acctable, CurrResFile.fp, codepage );
        if( error ) {
            err_code = LastWresErr();
            RcError( ERR_WRITTING_RES_FILE, CurrResFile.filename, strerror( err_code ) );
            ErrorHasOccured = true;
        } else {
            loc.len = SemEndResource( loc.start );
            SemAddResourceFree( name, WResIDFromNum( OS2_RT_ACCELTABLE ), flags, loc );
        }
    } else {
        RESFREE( name );
    }
    SemOS2FreeAccelTable( acctable );
}
