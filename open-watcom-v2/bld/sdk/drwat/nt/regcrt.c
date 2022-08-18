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


#include "drwatcom.h"
#include <ctype.h>
#include "mad.h"
#include "regcrt.h"
#include "bitman.h"


static unsigned getMADMaxFormatWidth( mad_type_handle mth )
{
    mad_radix           radix;
    void                *tmp;
    unsigned            max;
    mad_type_info       mti;
    int                 sign;
    unsigned long       mask;
    char                TxtBuff[100];

    sign = 0;
    MADTypeInfo( mth, &mti );
    tmp=alloca( mti.b.bits );
    switch( mti.b.kind ) {
    case MTK_ADDRESS:
    case MTK_INTEGER:
        memset( tmp, -1, mti.b.bits );
        if( mti.i.nr != MNR_UNSIGNED ) {
            mask = 1L << mti.i.sign_pos;
            (*(unsigned_32 *)(tmp)) &= ~mask;
            ++sign;
        }
        break;
    case MTK_FLOAT:
        memset( tmp, 0, mti.b.bits );
        break;
    }
    radix = MADTypePreferredRadix( mth );
    max = 0;
    MADTypeToString( radix, &mti, tmp, TxtBuff, &max );
    return( max + sign );
}

static void getRegSetInfo( mad_reg_set_data *data, mad_registers *regs, int *max_len,
        int *mv, int *num_reg)
{
    unsigned            i;
    unsigned            maxd;
    unsigned            maxv;
    const char          *descript;
    unsigned            max_descript;
    const mad_reg_info  *rinfo;
    unsigned            temp_maxv;
    mad_type_handle     mth;

    maxd = 0;
    maxv = 0;
    for( i = 0;; i++ ) {
        if( MADRegSetDisplayGetPiece( data, regs, i, &descript,
            &max_descript, &rinfo, &mth, &temp_maxv ) != MS_OK ) {
            break;
        }
        if( temp_maxv == 0 && rinfo != NULL ) {
            temp_maxv = getMADMaxFormatWidth( mth );
        }
        if( max_descript == 0 && descript != NULL ) {
            max_descript = strlen( descript );
        }
        if( maxd < max_descript )
            maxd = max_descript;
        if( maxv < temp_maxv ) {
            maxv = temp_maxv;
        }
    }
    *max_len = maxd + maxv + 2;
    *num_reg = i;
    *mv = maxv + 1;
}

static void getMadString( mad_reg_set_data *data, mad_registers *regs, int i,
    RegStringCreateData *create )
{
    mad_type_handle     mth;
    mad_type_info       mti;
    mad_radix           radix;
    const mad_reg_info  *rinfo;
    void                *value;
    unsigned            max_len;
    const char          *descript;

    MADRegSetDisplayGetPiece( data, regs, i, &descript, &( create[i].maxd ), &rinfo, &mth, &( create[i].length ) );

    if ( create[i].maxd == 0 && descript != NULL ){
        create[i].maxd = strlen( descript );
    }
    if ( descript != NULL && !IsEmptyString( descript ) ) {
        strcpy( create[i].buffer, descript );
        create[i].maxd ++;
    } else {
        create[i].buffer[0] = '\0';
        create[i].maxd = 0;
    }

    if( create[i].length == 0 && rinfo != NULL ) {
       create[i].length = getMADMaxFormatWidth( mth );
    }

    if( rinfo != NULL ) {
        MADTypeInfo( rinfo->mth, &mti );
        value = alloca( BITS2BYTES( mti.b.bits ) );
        BitGet( value, (unsigned char *)regs, rinfo->bit_start, rinfo->bit_size );
        radix = MADTypePreferredRadix( mth );
        max_len = create[i].length + 1;
        MADTypeHandleToString( radix, mth, value, create[i].value, &max_len );
    } else {
        create[i].value[0] = '\0';
    }
}

void FreeRegStringCreate( RegStringCreateData *reg_create, int num_regs )
{
    int i;
    for ( i = 0; i < num_regs; i++ ){
        MemFree( reg_create[i].buffer );
        MemFree( reg_create[i].value );
    }
    MemFree( reg_create );
}

bool IsEmptyString( const char *s )
{
    while( *s ) {
        if( isalnum( *s ) ) {
            return( false );
        }
        s++;
    }
    return( true );
}

void GetRegStringCreate( mad_registers *regs, mad_reg_set_data *reg_set,
    int width, RegStringCreateData **rc, int *nregs, int *ncols )
{
    RegStringCreateData *reg_create;
    int                 num_columns;
    int                 num_regs;
    int                 max_len;
    int                 i;
    int                 j;
    char                *buffer;
    int                 maxv;

    getRegSetInfo( reg_set, regs, &max_len, &maxv, &num_regs );
    reg_create = MemAlloc( sizeof(RegStringCreateData) * num_regs );
    for( i = 0; i < num_regs; i++ ){
        reg_create[i].buffer = MemAlloc( max_len + 1 );
        reg_create[i].value = MemAlloc( maxv + 1 );
        getMadString( reg_set, regs, i, reg_create );
    }

    num_columns = MADRegSetDisplayGrouping(reg_set);
    if( num_columns == 0 ){
        num_columns = width / max_len;
    }
    num_columns = num_columns;
    if( num_columns > num_regs )
        num_columns = num_regs;
    for( i = 0; i < num_columns; i++ ){
        for( j = i; j < num_regs; j += num_columns ) {
            if( reg_create[i].maxd < reg_create[j].maxd )
                reg_create[i].maxd = reg_create[j].maxd;
            if( reg_create[i].length < reg_create[j].length ) {
                reg_create[i].length = reg_create[j].length;
            }
        }
        reg_create[i].length += reg_create[i].maxd;
    }
    buffer = alloca( max_len + 1 );
    j = 0;
    for( i = 0; i < num_regs; i++ ){
        sprintf( buffer, "%*s%s", -reg_create[j].maxd, reg_create[i].buffer, reg_create[i].value );
        strcpy( reg_create[i].buffer, buffer );
        j++;
        if( j == num_columns ) {
            j = 0;
        }
    }
    *rc = reg_create;
    *nregs = num_regs;
    *ncols = num_columns;
}
