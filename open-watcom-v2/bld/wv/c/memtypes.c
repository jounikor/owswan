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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "mad.h"
#include "memtypes.h"
#include "dbgmem.h"
#include "dbgitem.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgmad.h"
#include "madcli.h"


static walk_result MadMemTypeWalk( mad_type_handle mth, void *d )
{
    mem_type_walk_data  *data = d;
    mad_type_info       mti;
    int                 ipl;
    int                 i;

    if( data->labels != NULL ) {
        i = data->num_types;
        MADTypeInfo( mth, &mti );
        MADCli( String )( MADTypeName( mth ), TxtBuff, TXT_LEN );
        data->labels[i] = DupStr( TxtBuff );
        data->info[i].mth = mth;
        data->info[i].item_width = GetMADMaxFormatWidth( mth );
        data->info[i].item_size = BITS2BYTES( mti.b.bits );
        data->info[i].piece_radix = MADTypePreferredRadix( mth );
        ipl = 80 / ( data->info[i].item_width + 1 ); // kludge
        if( ipl > 16 ) {
            ipl = 16;
        } else if( ipl > 8 ) {
            ipl = 8;
        } else if( ipl > 4 ) {
            ipl = 4;
        } else if( ipl > 2 ) {
            ipl = 2;
        } else {
            ipl = 1;
        }
        data->info[i].items_per_line = ipl;
    }
    data->num_types++;
    return( WR_CONTINUE );
}

void MemInitTypes( mad_type_kind mas, mem_type_walk_data *data )
{
    data->num_types = 0;
    data->labels = NULL;
    data->info = NULL;
    MADTypeWalk( mas, MadMemTypeWalk, data );
    if( data->num_types == 0 )
        return;
    data->labels = DbgAlloc( data->num_types * sizeof( *data->labels ) );
    data->info = DbgAlloc( data->num_types * sizeof( *data->info ) );
    data->num_types = 0;
    MADTypeWalk( mas, MadMemTypeWalk, data );
}

void MemFiniTypes( mem_type_walk_data *data )
{
    int         i;

    for( i = 0; i < data->num_types; ++i ) {
        DbgFree( data->labels[i] );
    }
    DbgFree( data->labels );
    DbgFree( data->info );
    data->num_types = 0;
    data->labels = NULL;
    data->info = NULL;
}

unsigned GetMADMaxFormatWidth( mad_type_handle mth )
{
    mad_radix           old_radix, new_radix;
    item_mach           tmp;
    size_t              max;
    mad_type_info       mti;
    int                 sign = 0;
    unsigned long       *plong;

    MADTypeInfo( mth, &mti );
    switch( mti.b.kind ) {
    case MTK_ADDRESS:
    case MTK_INTEGER:
        memset( &tmp, -1, sizeof( tmp ) );
        if( mti.i.nr != MNR_UNSIGNED ) {
            plong = (unsigned long *)tmp.ar + mti.i.sign_pos / 32;
            *plong &= ( 1L << ( mti.i.sign_pos % 32 ) ) - 1;
            ++sign;
        }
        break;
    case MTK_FLOAT:
        memset( &tmp, 0, sizeof( tmp ) );
        break;
    }
    new_radix = MADTypePreferredRadix( mth );
    old_radix = NewCurrRadix( new_radix );
    max = 0;
    MADTypeToString( new_radix, &mti, &tmp, TxtBuff, &max );
    NewCurrRadix( old_radix );
    return( max + sign );
}
