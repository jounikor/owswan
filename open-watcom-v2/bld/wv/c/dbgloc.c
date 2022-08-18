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


#include <stdlib.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "mad.h"
#include "dui.h"
#include "remcore.h"
#include "dbgreg.h"
#include "addarith.h"
#include "dbgupdt.h"
#include "dbgloc.h"


void LocationCreate( location_list *ll, location_type lt, void *d )
{
    address     addr;

    if( lt == LT_ADDR ) {
        /* have to do this first with temp in case of overlap between 'll' and 'd' */
        addr = *(address *)d;
        ll->e[0].u.addr = addr;
    } else {
        ll->e[0].u.p = d;
    }
    ll->num = 1;
    ll->flags = 0;
    ll->e[0].bit_start = 0;
    ll->e[0].bit_length = 0;
    ll->e[0].type = lt;
}

void LocationAppend( location_list *ll, location_list *new )
{
    memcpy( ll->e + ll->num, new->e, new->num * sizeof( new->e[0] ) );
    ll->num += new->num;
    ll->flags |= new->flags;
}

void LocationSet( location_list *ll, unsigned start, unsigned bits )
{
    ll->e[0].bit_start = start;
    ll->e[0].bit_length = bits;
}

void LocationAdd( location_list *ll, long sbits )
{
    location_entry      *le;
    unsigned long       add;
    unsigned            num;
    unsigned long       bits;

    bits = sbits;
    if( sbits < 0 ) {
        bits = -bits;
        add = (bits + 7) / 8;
        if( ll->e[0].type == LT_ADDR ) {
            ll->e[0].u.addr = AddrAdd( ll->e[0].u.addr, -add );
        } else {
            ll->e[0].u.p = (byte *)ll->e[0].u.p - add;
        }
        bits = 8 - (bits % 8);
        bits %= 8;
    }
    num = 0;
    le = ll->e;
    for( ;; ) {
        if( le->bit_length == 0 )
            break;
        if( le->bit_length > bits )
            break;
        bits -= le->bit_length;
        ++num;
    }
    if( num != 0 ) {
        ll->num -= num;
        memcpy( ll->e, le, ll->num * sizeof( ll->e[0] ) );
    }
    add = bits / 8;
    bits = bits % 8;
    ll->e[0].bit_start += bits;
    if( ll->e[0].bit_length != 0 )
        ll->e[0].bit_length -= bits;
    if( ll->e[0].type == LT_ADDR ) {
        ll->e[0].u.addr = AddrAdd( ll->e[0].u.addr, add );
    } else {
        ll->e[0].u.p = (byte *)ll->e[0].u.p + add;
    }
}

#ifdef DEADCODE
void LocationTrunc( location_list *ll, unsigned bits )
{
    unsigned    i;

    if( bits == 0 )
        return;
    i = 0;
    for( ;; ) {
        if( i >= ll->num )
            return;
        if( ll->e[i].bit_length == 0 )
            break;
        if( ll->e[i].bit_length > bits )
            break;
        bits -= ll->e[i].bit_length;
        ++i;
    }
    ll->e[i].bit_length = bits;
}
#endif

#define TEMP_SIZE       128
#define MAX_BIT_SIZE    0x1000
#define BUMP_INTERNAL( i, b )   (i).u.p = (byte *)(i).u.p + (b)
#define NORMALIZE_BITSTART( i )                             \
    {                                                       \
        if( (i).type == LT_ADDR ) {                         \
            (i).u.addr.mach.offset += BYTEIDX( (i).bit_start ); \
        } else {                                            \
            BUMP_INTERNAL( i, (i).bit_start / 8 );          \
        }                                                   \
        (i).bit_start = BITIDX( (i).bit_start );            \
    }

static dip_status DoLocAssign( location_list *dst, const location_list *src,
                        unsigned long len, bool sign_extend )
{
    byte                tmp[TEMP_SIZE + 1];
    byte                tmp2;
    byte                tmp3;
    byte                mask;
    size_t              num_bytes;
    unsigned            dest_num_bytes;
    size_t              size;
    unsigned            sidx;
    unsigned            didx;
    location_entry      sitem;
    location_entry      ditem;
    size_t              i;
    int                 shift;
    bool                padding;
    unsigned_32         pad_bytes = 0;
    bool                mem_mod;
    size_t              (*modify)( address, const void *, size_t );
    unsigned long       bits;

    modify = _IsOn( SW_RECORD_LOCATION_ASSIGN ) ? ChangeMem : ProgPoke;
    mem_mod = false;
    bits = BYTES2BITS( len ); /* turn byte length into bit length */
    padding = false;
    sidx = 0;
    didx = 0;
    sitem.bit_start = 0;
    sitem.bit_length = 0;
    sitem.type = 0;
    ditem.bit_start = 0;
    ditem.bit_length = 0;
    ditem.type = 0;
    for( ; bits > 0; bits -= size ) {
        while( sitem.bit_length == 0 ) {
            if( padding ) {
                sitem.bit_start = 0;
                sitem.bit_length = TYPE2BITS( pad_bytes );
                sitem.type = LT_INTERNAL;
                sitem.u.p = &pad_bytes;
            } else if( sidx < src->num ) {
                sitem = src->e[sidx++];
                if( sidx == src->num && sitem.bit_length != 0 ) {
                    padding = true;
                    if( sign_extend ) {
                        size = sitem.bit_start + sitem.bit_length - 1;
                        num_bytes = BYTEIDX( size );
                        if( sitem.type == LT_ADDR ) {
                            sitem.u.addr.mach.offset += num_bytes;
                            if( ProgPeek( sitem.u.addr, &tmp2, 1 ) != 1 ) {
                                return( DS_ERR|DS_NO_READ_MEM );
                            }
                            sitem.u.addr.mach.offset -= num_bytes;
                        } else {
                            tmp2 = *( (byte *)sitem.u.p + num_bytes );
                        }
                        if( tmp2 & (1 << BITIDX( size )) ) {
                            /* sign bit is on - flip padding bytes */
                            pad_bytes = ~pad_bytes;
                        }
                    }
                }
            } else if( bits > MAX_BIT_SIZE ) {
                sitem.bit_length = MAX_BIT_SIZE;
            } else {
                sitem.bit_length = bits;
            }
        }
        while( ditem.bit_length == 0 ) {
            if( didx < dst->num ) {
                ditem = dst->e[didx++];
                if( didx == dst->num && ditem.bit_length != 0 ) {
                    if( bits > ditem.bit_length ) {
                        bits = ditem.bit_length;
                    }
                }
            } else if( bits > MAX_BIT_SIZE ) {
                ditem.bit_length = MAX_BIT_SIZE;
            } else {
                ditem.bit_length = bits;
            }
        }
        if( !mem_mod && ditem.type == LT_ADDR ) {
            mem_mod = true;
            DbgUpdate( UP_MEM_CHANGE );
        }
        NORMALIZE_BITSTART( sitem );
        NORMALIZE_BITSTART( ditem );
        /* pick the smallest size to move */
        size = sitem.bit_length;
        if( ditem.bit_length < size )
            size = ditem.bit_length;
        if( sitem.bit_start == 0
          && ditem.bit_start == 0
          && (sitem.type != LT_ADDR || ditem.type != LT_ADDR)
          && size >= BYTES2BITS( 1 ) ) {
            /* special case - can move straight from one to the other */
            num_bytes = BYTEIDX( size );
            size -= BYTES2BITS( num_bytes );
            bits -= BYTES2BITS( num_bytes );
            if( sitem.type == LT_ADDR ) {
                if( ProgPeek( sitem.u.addr, ditem.u.p, num_bytes ) != num_bytes )
                    return( DS_ERR|DS_NO_READ_MEM );
                sitem.u.addr.mach.offset += num_bytes;
                BUMP_INTERNAL( ditem, num_bytes );
            } else if( ditem.type == LT_ADDR ) {
                if( modify( ditem.u.addr, sitem.u.p, num_bytes ) != num_bytes )
                    return( DS_ERR|DS_NO_WRITE_MEM );
                BUMP_INTERNAL( sitem, num_bytes );
                ditem.u.addr.mach.offset += num_bytes;
            } else {
                memmove( ditem.u.p, sitem.u.p, num_bytes );
                BUMP_INTERNAL( sitem, num_bytes );
                BUMP_INTERNAL( ditem, num_bytes );
            }
            sitem.bit_length -= BYTES2BITS( num_bytes );
            ditem.bit_length -= BYTES2BITS( num_bytes );
        }
        if( size != 0 ) {
            /* generalized case, have to bit-blit */
            if( size > BYTES2BITS( TEMP_SIZE ) )
                size = BYTES2BITS( TEMP_SIZE );
            num_bytes = UNALGN_BITS2BYTES( sitem.bit_start + size );
            if( sitem.type == LT_ADDR ) {
                if( ProgPeek( sitem.u.addr, tmp, num_bytes ) != num_bytes ) {
                    return( DS_ERR|DS_NO_READ_MEM );
                }
            } else {
                memcpy( tmp, sitem.u.p, num_bytes );
            }
            mask = (1 << BITIDX( sitem.bit_start + size )) - 1;
            if( mask != 0 ) {
                tmp[num_bytes - 1] &= mask;
            }
            shift = sitem.bit_start - ditem.bit_start;
            tmp2 = 0;
            if( shift < 0 ) {
                shift = -shift;
                for( i = 0; i < num_bytes; ++i ) {
                    tmp3 = tmp[i];
                    tmp[i] = (tmp3 << shift) | (tmp2 >> (BYTES2BITS( 1 ) - shift));
                    tmp2 = tmp3;
                }
                tmp[i] = tmp2 >> (BYTES2BITS( 1 ) - shift);
            } else if( shift > 0 ) {
                for( i = num_bytes; i-- > 0; ) {
                    tmp3 = tmp[i];
                    tmp[i] = (tmp3 >> shift) | (tmp2 << (BYTES2BITS( 1 ) - shift));
                    tmp2 = tmp3;
                }
            }
            if( ditem.bit_start != 0 ) {
                if( ditem.type == LT_ADDR ) {
                    if( ProgPeek( ditem.u.addr, &tmp2, 1 ) != 1 ) {
                        return( DS_ERR|DS_NO_WRITE_MEM );
                    }
                } else {
                    tmp2 = *(byte *)ditem.u.p;
                }
                mask = (1 << ditem.bit_start) - 1;
                tmp2 &= mask;
                tmp[0] &= ~mask;
                tmp[0] |= tmp2;
            }
            num_bytes = BYTEIDX( ditem.bit_start + size );
            dest_num_bytes = UNALGN_BITS2BYTES( ditem.bit_start + size );
            if( dest_num_bytes > num_bytes ) {
                if( ditem.type == LT_ADDR ) {
                    ditem.u.addr.mach.offset += num_bytes;
                    if( ProgPeek( ditem.u.addr, &tmp2, 1 ) != 1 ) {
                        return( DS_ERR|DS_NO_READ_MEM );
                    }
                    ditem.u.addr.mach.offset -= num_bytes;
                } else {
                    tmp2 = *( (byte *)ditem.u.p + num_bytes );
                }
                mask = (1 << BITIDX( ditem.bit_start + size )) - 1;
                tmp2 &= ~mask;
                tmp[num_bytes] &= mask;
                tmp[num_bytes] |= tmp2;
                num_bytes = dest_num_bytes;
            }
            if( ditem.type == LT_ADDR ) {
                if( modify( ditem.u.addr, tmp, num_bytes ) != num_bytes ) {
                    return( DS_ERR|DS_NO_WRITE_MEM );
                }
            } else {
                memcpy( ditem.u.p, tmp, num_bytes );
            }
        }
        sitem.bit_start += size;
        ditem.bit_start += size;
        sitem.bit_length -= size;
        ditem.bit_length -= size;
    }
    return( DS_OK );
}

dip_status LocationAssign( location_list *dst, const location_list *src,
                        unsigned long len, bool sign_extend )
{
    dip_status  ds;
    unsigned    flags;
    unsigned    i;

    flags = dst->flags >> LLF_REG_FLAGS_SHIFT;
    if( flags != 0 ) {
        for( i = 0; i < dst->num; ++i ) {
            if( dst->e[i].type == LT_INTERNAL ) {
                MADRegUpdateStart( dst->e[i].u.p, flags,
                     dst->e[i].bit_start, dst->e[i].bit_length );
            }
        }
    }
    ds = DoLocAssign( dst, src, len, sign_extend );
    if( flags != 0 ) {
        for( i = 0; i < dst->num; ++i ) {
            if( dst->e[i].type == LT_INTERNAL ) {
                MADRegUpdateEnd( dst->e[i].u.p, flags,
                     dst->e[i].bit_start, dst->e[i].bit_length );
            }
        }
    }
    return( ds );
}
