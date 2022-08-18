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
* Description:  Export DIP Loading/unloading of symbolic information.
*
****************************************************************************/


#include "exp.h"
#include "walloca.h"
#include "exedos.h"
#include "exeos2.h"
#include "exeflat.h"
#include "exepe.h"
#include "exenov.h"
#include "exeelf.h"


#if defined( __WATCOMC__ ) && defined( __386__ )

/* WD looks for this symbol to determine module bitness */
int __nullarea;
#pragma aux __nullarea "*";

#endif

typedef union {
        dos_exe_header  mz;
        os2_exe_header  ne;
        os2_flat_header lx;
        pe_header       pe;
} any_header;


static struct {
    unsigned long       fpos;
    unsigned            len;
    unsigned            off;
    unsigned_8          data[256];
} Buff;

static unsigned long BSeek( FILE *fp, unsigned long p, dig_seek w )
{
    unsigned long       bpos;
    unsigned long       npos = 0;

    bpos = Buff.fpos - Buff.len;
    switch( w ) {
    case DIG_END:
        return( DIG_SEEK_ERROR ); /* unsupported */
    case DIG_CUR:
        npos = bpos + p + Buff.off;
        break;
    case DIG_ORG:
        npos = p;
        break;
    }
    if( npos >= bpos && npos < (bpos+Buff.len) ) {
        Buff.off = npos - bpos;
        return( npos );
    }
    DCSeek( fp, npos, DIG_ORG );
    Buff.fpos = DCTell( fp );
    Buff.off = 0;
    Buff.len = 0;
    return( Buff.fpos );
}

static size_t BRead( FILE *fp, void *b, size_t s )
{
    size_t      got;
    size_t      want;

    if( s > sizeof( Buff.data ) ) {
        DCSeek( fp, Buff.fpos + Buff.off - Buff.len, DIG_ORG );
        Buff.fpos = DCTell( fp );
        Buff.len = 0;
        Buff.off = 0;
        if( Buff.fpos == DIG_SEEK_ERROR )
            return( 0 );
        got = DCRead( fp, b, s );
        Buff.fpos += got;
        return( got );
    }
    want = s;
    got = Buff.len - Buff.off;
    if( got > want )
        got = want;
    memcpy( b, &Buff.data[Buff.off], got );
    Buff.off += got;
    want -= got;
    if( want > 0 ) {
        size_t len;

        len = DCRead( fp, &Buff.data[0], sizeof( Buff.data ) );
        if( len == DIG_RW_ERROR ) {
            Buff.fpos = DIG_SEEK_ERROR;
            Buff.off = 0;
            Buff.len = 0;
            return( DIG_RW_ERROR );
        }
        Buff.len = len;
        Buff.fpos += Buff.len;
        b = (unsigned_8 *)b + got;
        memcpy( b, &Buff.data[0], want );
        Buff.off = want;
    }
    return( s );
}

#define ROUND_UP( d, r ) (((d)+(r)-1) & ~((r)-1))

static void *HunkAlloc( imp_image_handle *iih, size_t size )
{
    exp_hunk    *hunk;
    size_t      alloc;

    size = ROUND_UP( size, sizeof( void * ) );
    hunk = iih->hunks;
    if( hunk == NULL || size > hunk->left ) {
        alloc = HUNK_SIZE;
        if( alloc < size )
            alloc = size;
        hunk = DCAlloc( (sizeof( *hunk ) - HUNK_SIZE) + alloc );
        if( hunk == NULL )
            return( NULL );
        hunk->next = iih->hunks;
        iih->hunks = hunk;
        hunk->left = alloc;
    }
    hunk->left -= size;
    return( &hunk->data[hunk->left] );
}

static void ImpUnloadInfo( imp_image_handle *iih )
{
    exp_hunk    *curr;
    exp_hunk    *next;

    for( curr = iih->hunks; curr != NULL; curr = next ) {
        next = curr->next;
        DCFree( curr );
    }
    iih->hunks = NULL;
}

exp_block       *FindAddrBlock( imp_image_handle *iih, addr_ptr addr )
{
    exp_block   *b;

    for( b = iih->addr; b != NULL; b = b->next ) {
        if( SameAddrSpace( b->start, addr )
          && b->start.offset <= addr.offset
          && (b->start.offset+b->len) > addr.offset ) {
            return( b );
        }
    }
    return( NULL );
}

static dip_status AddName( imp_image_handle *iih, unsigned len, char *name )
{
    char        *start;
    char        *end;

    end = NULL;
    start = name;
    for( ; len > 0; --len ) {
        switch( *name ) {
        case ':':
        case '\\':
        case '/':
            start = name + 1;
            end = NULL;
            break;
        case '.':
            end = name;
            break;
        }
        ++name;
    }
    if( end == NULL )
        end = name;
    iih->len = end - start;
    iih->name = HunkAlloc( iih, iih->len );
    if( iih->name == NULL )
        return( DS_ERR | DS_NO_MEM );
    memcpy( iih->name, start, iih->len );
    return( DS_OK );
}

static dip_status AddBlock( imp_image_handle *iih, addr_seg seg, addr_off off,
                        unsigned_32 len, unsigned_8 code )
{
    exp_block   *new;

    new = HunkAlloc( iih, sizeof( *new ) );
    if( new == NULL )
        return( DS_ERR | DS_NO_MEM );
    new->start.segment = seg;
    new->start.offset = off;
    new->len = len;
    new->code = code;
    new->next = iih->addr;
    iih->addr = new;
    return( DS_OK );
}

static dip_status AddSymbol( imp_image_handle *iih, addr_seg seg, addr_off off,
                        unsigned len, char *name )
{
    exp_sym     *new;

    new = HunkAlloc( iih, (sizeof( *new ) - 1) + len );
    if( new == NULL )
        return( DS_ERR | DS_NO_MEM );
    new->addr.segment = seg;
    new->addr.offset = off;
    new->len = len;
    memcpy( new->name, name, len );
    new->next = iih->gbl;
    iih->gbl = new;
    return( DS_OK );
}

static dip_status ProcTable( FILE *fp, imp_image_handle *iih, unsigned resident )
{
    unsigned_8  len;
    char        buff[256 + sizeof(unsigned_16)];
    unsigned    first;
    unsigned    ord;
    dip_status  ds;

    first = 1;
    for( ;; ) {
        if( BRead( fp, &len, sizeof( len ) ) != sizeof( len ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( len == 0 )
            break;
        if( BRead( fp, buff, len + 2 ) != ( len + 2 ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        ord = *(unsigned_16 *)&buff[len];
        if( resident && first ) {
            first = 0;
            ds = AddName( iih, len, buff );
            if( ds != DS_OK ) {
                return( ds );
            }
        } else if( ord != 0 ) {
            /* this is putting in entry number, we'll get real addr later */
            ds = AddSymbol( iih, 0, ord, len, buff );
            if( ds != DS_OK ) {
                return( ds );
            }
        }
    }
    return( DS_OK );
}

static dip_status TryNE( FILE *fp, imp_image_handle *iih, any_header *head, unsigned_32 off )
{
    segment_record      seg;
    dip_status          ds;
    unsigned            ord;
    bundle_prefix       pref;
    union {
        movable_record  mov;
        fixed_record    fix;
    }                   entry;
    unsigned            i;
    exp_sym             *s;

    if( BSeek( fp, off + head->ne.segment_off, DIG_ORG ) != off + head->ne.segment_off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    for( i = 0; i < head->ne.segments; ++i ) {
        if( BRead( fp, &seg, sizeof( seg ) ) != sizeof( seg ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        ds = AddBlock( iih, i + 1, 0, seg.size == 0 ? 0x10000 : seg.size, (seg.info & SEG_DATA) == 0 );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    if( BSeek( fp, off + head->ne.resident_off, DIG_ORG ) != off + head->ne.resident_off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    ds = ProcTable( fp, iih, 1 );
    if( ds != DS_OK )
        return( ds );
    if( head->ne.nonres_size != 0 ) {
        if( BSeek( fp, head->ne.nonres_off, DIG_ORG ) != head->ne.nonres_off ) {
            return( DS_ERR | DS_FSEEK_FAILED );
        }
        ds = ProcTable( fp, iih, 0 );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    /* change all the symbol addresses from entry numbers to seg/offsets */
    if( BSeek( fp, off + head->ne.entry_off, DIG_ORG ) != off + head->ne.entry_off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    ord = 1;
    for( ;; ) {
        if( BRead( fp, &pref, sizeof( pref ) ) != sizeof( pref ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( pref.number == 0 )
            break;
        if( pref.type == 0 ) {
            ord += pref.number;
        } else {
            for( i = 0; i < pref.number; ++i ) {
                if( pref.type == MOVABLE_ENTRY_PNT ) {
                    if( BRead( fp, &entry.mov, sizeof( entry.mov ) ) != sizeof( entry.mov ) ) {
                        return( DS_ERR | DS_FREAD_FAILED );
                    }
                } else {
                    if( BRead( fp, &entry.fix, sizeof( entry.fix ) ) != sizeof( entry.fix ) ) {
                        return( DS_ERR | DS_FREAD_FAILED );
                    }
                    entry.mov.entry = entry.fix.entry;
                    entry.mov.entrynum = pref.type;
                }
                if( entry.mov.info & ENTRY_EXPORTED ) {
                    for( s = iih->gbl; s != NULL; s = s->next ) {
                        if( s->addr.segment == 0 && s->addr.offset == ord ) {
                            s->addr.segment = entry.mov.entrynum;
                            s->addr.offset = entry.mov.entry;
                            break;
                        }
                    }
                }
                ++ord;
            }
        }
    }
    return( DS_OK );
}

static dip_status TryLX( FILE *fp, imp_image_handle *iih, any_header *head, unsigned_32 off )
{
    object_record       seg;
    dip_status          ds;
    unsigned            ord;
    flat_bundle_prefix  pref;
    union {
        flat_bundle_entry32     e32;
        flat_bundle_entry16     e16;
        flat_bundle_gate16      g16;
        flat_bundle_entryfwd    fwd;
    }                   entry;
    unsigned            i;
    exp_sym             *s;

    if( BSeek( fp, off + head->lx.objtab_off, DIG_ORG ) != off + head->lx.objtab_off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    for( i = 0; i < head->lx.num_objects; ++i ) {
        if( BRead( fp, &seg, sizeof( seg ) ) != sizeof( seg ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( (seg.flags & OBJ_RESOURCE) == 0 ) {
            ds = AddBlock( iih, i + 1, 0, seg.size, (seg.flags&OBJ_EXECUTABLE)!=0 );
            if( ds != DS_OK ) {
                return( ds );
            }
        }
    }
    if( BSeek( fp, off + head->lx.resname_off, DIG_ORG ) != off + head->lx.resname_off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    ds = ProcTable( fp, iih, 1 );
    if( ds != DS_OK )
        return( ds );
    if( head->lx.nonres_size != 0 ) {
        if( BSeek( fp, head->lx.nonres_off, DIG_ORG ) != head->lx.nonres_off ) {
            return( DS_ERR | DS_FSEEK_FAILED );
        }
        ds = ProcTable( fp, iih, 0 );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    /* change all the symbol addresses from entry numbers to seg/offsets */
    if( BSeek( fp, off + head->lx.entry_off, DIG_ORG ) != off + head->lx.entry_off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    ord = 1;
    memset( &entry, 0, sizeof( entry ) );
    for( ;; ) {
        if( BRead( fp, &pref, sizeof( pref ) ) != sizeof( pref ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( pref.b32_cnt == 0 )
            break;
        if( pref.b32_type == FLT_BNDL_EMPTY ) {
            ord += pref.b32_cnt;
            BSeek( fp, DIG_SEEK_POSBACK( 2 ), DIG_CUR );   /* backup 2 bytes */
        } else {
            for( i = 0; i < pref.b32_cnt; ++i ) {
                switch( pref.b32_type ) {
                case FLT_BNDL_ENTRY16:
                    if( BRead( fp, &entry.e16, sizeof( entry.e16 ) ) != sizeof( entry.e16 ) ) {
                        return( DS_ERR | DS_FREAD_FAILED );
                    }
                    entry.e32.e32_offset = entry.e16.e32_offset;
                    break;
                case FLT_BNDL_GATE16:
                    if( BRead( fp, &entry.g16, sizeof( entry.g16 ) ) != sizeof( entry.g16 ) ) {
                        return( DS_ERR | DS_FREAD_FAILED );
                    }
                    entry.e32.e32_offset = entry.g16.offset;
                    break;
                case FLT_BNDL_ENTRY32:
                    if( BRead( fp, &entry.e32, sizeof( entry.e32 ) ) != sizeof( entry.e32 ) ) {
                        return( DS_ERR | DS_FREAD_FAILED );
                    }
                    break;
                case FLT_BNDL_ENTRYFWD:
                    if( BRead( fp, &entry.fwd, sizeof( entry.fwd ) ) != sizeof( entry.fwd ) ) {
                        return( DS_ERR | DS_FREAD_FAILED );
                    }
                    /* don't know how to handle these */
                    entry.fwd.e32_flags &= ~ENTRY_EXPORTED;
                    break;
                }
                if( entry.e32.e32_flags & ENTRY_EXPORTED ) {
                    for( s = iih->gbl; s != NULL; s = s->next ) {
                        if( s->addr.segment == 0 && s->addr.offset == ord ) {
                            s->addr.segment = pref.b32_obj;
                            s->addr.offset = entry.e32.e32_offset;
                            break;
                        }
                    }
                }
                ++ord;
            }
        }
    }
    return( DS_OK );
}

#define MAX_EXPORTS_PER         1024
#define NAME_CACHE_SIZE         (8*1024)
typedef struct {
    any_header                  *head;
    pe_object                   *obj;
    unsigned_32                 *eat;
    unsigned long               name_ptr_base;
    unsigned long               ord_base;
    unsigned_32                 name_ptrs[MAX_EXPORTS_PER];
    unsigned_16                 ords[MAX_EXPORTS_PER];
    unsigned long               cache_name_rva;
    unsigned                    cache_name_len;
    FILE                        *fp;
    unsigned char               name_cache[NAME_CACHE_SIZE];
} pe_export_info;

static unsigned long ObjSize( pe_object *obj )
{
    unsigned long       size;

    size = obj->virtual_size;
    if( size == 0 )
        size = obj->physical_size;
    return( size );
}

static unsigned long RVAToPos( unsigned long rva, any_header *head, pe_object *obj )
{
    unsigned    i;
    for( i = 0; i < head->pe.num_objects; ++i ) {
        if( rva >= obj[i].rva && rva < (obj[i].rva + ObjSize(&obj[i])) ) {
            return( rva - obj[i].rva + obj[i].physical_offset );
        }
    }
    return( 0 );
}

static char *CacheName( pe_export_info *exp, unsigned long rva )
{
    unsigned            off;
    unsigned long       pos;

    off = rva - exp->cache_name_rva;
    if( rva < exp->cache_name_rva
      || rva >= exp->cache_name_rva + exp->cache_name_len
      || memchr( &exp->name_cache[off], '\0', exp->cache_name_len - off ) == NULL ) {
        /* Name is not fully in the cache. Have to read it in. */
        exp->cache_name_rva = rva;
        exp->cache_name_len = exp->head->pe.table[PE_TBL_EXPORT].rva +
                              exp->head->pe.table[PE_TBL_EXPORT].size - rva;
        if( exp->cache_name_len > NAME_CACHE_SIZE ) {
            exp->cache_name_len = NAME_CACHE_SIZE;
        }
        pos = RVAToPos( rva, exp->head, exp->obj );
        if( BSeek( exp->fp, pos, DIG_ORG ) != pos ) {
            DCStatus( DS_ERR | DS_FSEEK_FAILED );
            return( NULL );
        }
        if( BRead( exp->fp, exp->name_cache, exp->cache_name_len ) != exp->cache_name_len ) {
            DCStatus( DS_ERR | DS_FREAD_FAILED );
            return( NULL );
        }
        /* Assuming a single name is < NAME_CACHE_SIZE */
        off = 0;
    }
    return( (char *)&exp->name_cache[off] );
}

static dip_status PEExportBlock( imp_image_handle *iih, pe_export_info *exp, unsigned num )
{
    unsigned            i;
    unsigned            j;
    unsigned long       exp_rva;
    char                *name;
    dip_status          ds;

    if( BSeek( exp->fp, exp->name_ptr_base, DIG_ORG ) != exp->name_ptr_base ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    if( BRead( exp->fp, exp->name_ptrs, num * sizeof( unsigned_32 ) ) != num * sizeof( unsigned_32 ) ) {
        return( DS_ERR | DS_FREAD_FAILED );
    }
    if( BSeek( exp->fp, exp->ord_base, DIG_ORG ) != exp->ord_base ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    if( BRead( exp->fp, exp->ords, num * sizeof( unsigned_16 ) ) != num * sizeof( unsigned_16 ) ) {
        return( DS_ERR | DS_FREAD_FAILED );
    }
    for( i = 0; i < num; ++i ) {
        /* MS document lies about bias in export ordinal table! */
        exp_rva = exp->eat[exp->ords[i] /*- dir->ordinal_base*/];
        if( exp_rva < exp->head->pe.table[PE_TBL_EXPORT].rva
          || exp_rva >= (exp->head->pe.table[PE_TBL_EXPORT].rva+exp->head->pe.table[PE_TBL_EXPORT].size) ) {
            /* not a forwarder entry */
            for( j = 0; j < exp->head->pe.num_objects; ++j ) {
                if( exp_rva >= exp->obj[j].rva
                  && exp_rva < (exp->obj[j].rva+ObjSize( &exp->obj[j] )) ) {
                    /* found the object */
                    name = CacheName( exp, exp->name_ptrs[i] );
                    if( name == NULL ) {
                        return( DS_ERR | DS_FAIL );
                    }
                    ds = AddSymbol( iih, j + 1, exp_rva - exp->obj[j].rva, strlen( name ), name );
                    if( ds != DS_OK ) {
                        return( ds );
                    }
                }
            }
        }
    }
    return( DS_OK );
}

static dip_status TryPE( FILE *fp, imp_image_handle *iih, any_header *head, unsigned_32 off )
{
    unsigned            i;
    pe_object           *obj;
    pe_export_directory dir;
    size_t              obj_size;
    dip_status          ds;
    unsigned long       pos;
    pe_export_info      *exp;
    char                *name;
    unsigned            chunk;

#define EXPORT_SIZE     head->pe.table[PE_TBL_EXPORT].size
#define EXPORT_RVA      head->pe.table[PE_TBL_EXPORT].rva

    if( EXPORT_SIZE == 0 )
        return( DS_FAIL );

    pos = head->pe.nt_hdr_size + offsetof( pe_header, magic ) + off;
    if( BSeek( fp, pos, DIG_ORG ) != pos ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    obj_size = head->pe.num_objects * sizeof( *obj );
    obj = walloca( obj_size );
    if( BRead( fp, obj, obj_size ) != obj_size ) {
        return( DS_ERR | DS_FREAD_FAILED );
    }
    for( i = 0; i < head->pe.num_objects; ++i ) {
        ds = AddBlock( iih, i + 1, 0, ObjSize( &obj[i] ), (obj[i].flags & PE_OBJ_EXECUTABLE) != 0 );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    pos = RVAToPos( EXPORT_RVA, head, obj );
    if( BSeek( fp, pos, DIG_ORG ) != pos ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    if( BRead( fp, &dir, sizeof( dir ) ) != sizeof( dir ) ) {
        return( DS_ERR | DS_FREAD_FAILED );
    }
    exp = DCAlloc( sizeof( *exp ) + dir.num_eat_entries * sizeof( unsigned_32 ) );
    if( exp == NULL ) {
        return( DS_ERR | DS_NO_MEM );
    }
    exp->eat  = (unsigned_32 *)&exp[1];
    exp->fp  = fp;
    exp->head = head;
    exp->obj  = obj;
    exp->cache_name_rva = 0;
    exp->cache_name_len = 0;

    pos = RVAToPos( dir.address_table_rva, head, obj );
    ds = DS_ERR | DS_FSEEK_FAILED;
    if( BSeek( fp, pos, DIG_ORG ) == pos ) {
        ds = DS_ERR | DS_FREAD_FAILED;
        if( BRead( fp, exp->eat, dir.num_eat_entries * sizeof( unsigned_32 ) ) == dir.num_eat_entries * sizeof( unsigned_32 ) ) {
            ds = DS_ERR | DS_FAIL;
            name = CacheName( exp, dir.name_rva );
            if( name != NULL ) {
                ds = AddName( iih, strlen( name ), name );
                if( ds == DS_OK ) {
                    exp->name_ptr_base = RVAToPos( dir.name_ptr_table_rva, head, obj );
                    exp->ord_base      = RVAToPos( dir.ordinal_table_rva, head, obj );
                    for( ; dir.num_name_ptrs != 0; ) {
                        chunk = dir.num_name_ptrs;
                        if( chunk > MAX_EXPORTS_PER )
                            chunk = MAX_EXPORTS_PER;
                        ds = PEExportBlock( iih, exp, chunk );
                        if( ds != DS_OK ) {
                            break;
                        }
                        exp->name_ptr_base += chunk * sizeof( unsigned_32 );
                        exp->ord_base      += chunk * sizeof( unsigned_16 );
                        dir.num_name_ptrs  -= chunk;
                    }
                }
            }
        }
    }
    if( ds != DS_OK ) {
        DCFree( exp );
    }
    return( ds );
}

static dip_status TryStub( FILE *fp, imp_image_handle *iih )
{
    unsigned_32         off;
    any_header          head;

    switch( BRead( fp, &head.mz, sizeof( head.mz ) ) ) {
    case DIG_RW_ERROR:
        return( DS_ERR | DS_FREAD_FAILED );
    case sizeof( head.mz ):
        break;
    default:
        return( DS_FAIL );
    }
    if( head.mz.signature != DOS_SIGNATURE )
        return( DS_FAIL );
    if( head.mz.reloc_offset < (OS2_NE_OFFSET + sizeof( off )) )
        return( DS_FAIL );
    if( BSeek( fp, OS2_NE_OFFSET, DIG_ORG ) != OS2_NE_OFFSET ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    if( BRead( fp, &off, sizeof( off ) ) != sizeof( off ) ) {
        return( DS_ERR | DS_FREAD_FAILED );
    }
    if( BSeek( fp, off, DIG_ORG ) != off ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    if( BRead( fp, &head, sizeof( head ) ) != sizeof( head ) ) {
        return( DS_ERR | DS_FREAD_FAILED );
    }
    switch( head.ne.signature ) {
    case OS2_SIGNATURE_WORD:
        /* Hey, it's an NE executable */
        return( TryNE( fp, iih, &head, off ) );
    case OSF_FLAT_SIGNATURE:
    case OSF_FLAT_LX_SIGNATURE:
        /* Hey, it's an LX/LE executable */
        return( TryLX( fp, iih, &head, off ) );
    case PE_SIGNATURE:
    case PL_SIGNATURE:
        /* Hey, it's a PE executable (or Pharlap's variant of it) */
        return( TryPE( fp, iih, &head, off ) );
    }
    return( DS_FAIL );
}

#define CODE_SEGMENT    1
#define DATA_SEGMENT    2
static dip_status TryNLM( FILE *fp, imp_image_handle *iih )
{
    nlm_header          head;
    dip_status          ds;
    unsigned long       data_size;
    unsigned            count;
    unsigned            seg;
    nov_dbg_info        dbg;
    char                name[256];

    switch( BRead( fp, &head, sizeof( head ) ) ) {
    case DIG_RW_ERROR:
        return( DS_ERR | DS_FREAD_FAILED );
    case sizeof( head ):
        break;
    default:
        return( DS_FAIL );
    }
    if( memcmp( head.signature, NLM_SIGNATURE, NLM_SIGNATURE_LENGTH ) != 0 ) {
        return( DS_FAIL );
    }
    if( head.version > NLM_VERSION ) {
        /* one of those funky packed NLM's */
        return( DS_FAIL );
    }
    ds = AddName( iih, head.moduleName[0], &head.moduleName[1] );
    if( ds != DS_OK )
        return( ds );
    if( head.codeImageSize != 0 ) {
        ds = AddBlock( iih, CODE_SEGMENT, 0, head.codeImageSize, 1 );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    data_size = head.dataImageSize + head.uninitializedDataSize;
    if( data_size != 0 ) {
        ds = AddBlock( iih, DATA_SEGMENT, 0, data_size, 0 );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    if( head.numberOfDebugRecords != 0 ) {
        /* use the Novell style debugging information */
        if( BSeek( fp, head.debugInfoOffset, DIG_ORG ) != head.debugInfoOffset ) {
            return( DS_ERR | DS_FSEEK_FAILED );
        }
        for( count = head.numberOfDebugRecords; count > 0; --count ) {
            if( BRead( fp, &dbg, sizeof( dbg ) ) != sizeof( dbg ) ) {
                return( DS_ERR | DS_FREAD_FAILED );
            }
            if( BRead( fp, name, dbg.namelen ) != dbg.namelen ) {
                return( DS_ERR | DS_FREAD_FAILED );
            }
            seg = (dbg.type == DBG_DATA) ? DATA_SEGMENT : CODE_SEGMENT;
            ds = AddSymbol( iih, seg, dbg.offset, dbg.namelen, name );
            if( ds != DS_OK ) {
                return( ds );
            }
        }
        return( DS_OK );
    }
    if( BSeek( fp, head.publicsOffset, DIG_ORG ) != head.publicsOffset ) {
        return( DS_ERR | DS_FSEEK_FAILED );
    }
    for( count = head.numberOfPublics; count > 0; --count ) {
        if( BRead( fp, &dbg.namelen, sizeof( dbg.namelen ) ) != sizeof( dbg.namelen ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( BRead( fp, name, dbg.namelen ) != dbg.namelen ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( BRead( fp, &dbg.offset, sizeof( dbg.offset ) ) != sizeof( dbg.offset ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        if( dbg.offset & NOV_EXP_ISCODE ) {
            dbg.offset &= ~NOV_EXP_ISCODE;
            seg = CODE_SEGMENT;
        } else {
            seg = DATA_SEGMENT;
        }
        ds = AddSymbol( iih, seg, dbg.offset, dbg.namelen, name );
        if( ds != DS_OK ) {
            return( ds );
        }
    }
    return( DS_OK );
}


static void ByteSwapShdr( Elf32_Shdr *elf_sec, bool byteswap )
{
    if( byteswap ) {
        SWAP_32( elf_sec->sh_name );
        SWAP_32( elf_sec->sh_type );
        SWAP_32( elf_sec->sh_flags );
        SWAP_32( elf_sec->sh_addr );
        SWAP_32( elf_sec->sh_offset );
        SWAP_32( elf_sec->sh_size );
        SWAP_32( elf_sec->sh_link );
        SWAP_32( elf_sec->sh_info );
        SWAP_32( elf_sec->sh_addralign );
        SWAP_32( elf_sec->sh_entsize );
    }
}

static void ByteSwapPhdr( Elf32_Phdr *elf_ph, bool byteswap )
{
    if( byteswap ) {
        SWAP_32( elf_ph->p_type );
        SWAP_32( elf_ph->p_offset );
        SWAP_32( elf_ph->p_vaddr );
        SWAP_32( elf_ph->p_paddr );
        SWAP_32( elf_ph->p_filesz );
        SWAP_32( elf_ph->p_memsz );
        SWAP_32( elf_ph->p_flags );
        SWAP_32( elf_ph->p_align );
    }
}

static void ByteSwapSym( Elf32_Sym *elf_sym, bool byteswap )
{
    if( byteswap ) {
        SWAP_32( elf_sym->st_name );
        SWAP_32( elf_sym->st_value );
        SWAP_32( elf_sym->st_size );
        SWAP_32( elf_sym->st_shndx );
    }
}

static dip_status TryELF( FILE *fp, imp_image_handle *iih )
{
    Elf32_Ehdr          head;
    Elf32_Phdr          phe;
    dip_status          ds;
    unsigned            i;
    unsigned            j;
    unsigned            num_syms;
    unsigned            len;
    unsigned long       off;
    int                 code;
    char                *strings;
    char                *new;
    char                *name;
    Elf32_Shdr          *sect;
    Elf32_Shdr          *strtab;
    Elf32_Sym           sym;
    unsigned            tab_type;
    bool                byte_swap;

    switch( BRead( fp, &head, sizeof( head ) ) ) {
    case DIG_RW_ERROR:
        return( DS_ERR | DS_FREAD_FAILED );
    case sizeof( head ):
        break;
    default:
        return( DS_FAIL );
    }
    if( memcmp( head.e_ident, ELF_SIGNATURE, ELF_SIGNATURE_LEN ) != 0 ) {
        return( DS_FAIL );
    }
    if( head.e_ident[EI_CLASS] == ELFCLASS64 ) {
        // no support yet
        return( DS_FAIL );
    }

    byte_swap = false;
#ifdef __BIG_ENDIAN__
    if( head.e_ident[EI_DATA] == ELFDATA2LSB ) {
#else
    if( head.e_ident[EI_DATA] == ELFDATA2MSB ) {
#endif
        byte_swap = true;
        SWAP_16( head.e_type );
        SWAP_16( head.e_machine );
        SWAP_32( head.e_version );
        SWAP_32( head.e_entry );
        SWAP_32( head.e_phoff );
        SWAP_32( head.e_shoff );
        SWAP_32( head.e_flags );
        SWAP_16( head.e_ehsize );
        SWAP_16( head.e_phentsize );
        SWAP_16( head.e_phnum );
        SWAP_16( head.e_shentsize );
        SWAP_16( head.e_shnum );
        SWAP_16( head.e_shstrndx );
    }

    if( head.e_phoff == 0 || head.e_shoff == 0 ) {
        return( DS_FAIL );
    }
    /* Add address blocks */
    off = head.e_phoff;
    i = 0;
    for( i = 0; i < head.e_phnum; ++i ) {
        if( BSeek( fp, off, DIG_ORG ) != off ) {
            return( DS_ERR | DS_FSEEK_FAILED );
        }
        if( BRead( fp, &phe, sizeof( phe ) ) != sizeof( phe ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        ByteSwapPhdr( &phe, byte_swap );
        if( phe.p_type == PT_LOAD ) {
            if( phe.p_flags & PF_X ) {
                code = 1;
            } else {
                code = 0;
            }
            ds = AddBlock( iih,
                code ? MAP_FLAT_CODE_SELECTOR : MAP_FLAT_DATA_SELECTOR,
                phe.p_vaddr, phe.p_memsz, code );
            if( ds != DS_OK ) {
                return( ds );
            }
        }
        off += head.e_phentsize;
    }
    /* Add Symbols */
    sect = walloca( head.e_shnum * sizeof( *sect ) );
    off = head.e_shoff;
    for( i = 0; i < head.e_shnum; ++i ) {
        if( BSeek( fp, off, DIG_ORG ) != off ) {
            return( DS_ERR | DS_FSEEK_FAILED );
        }
        if( BRead( fp, &sect[i], sizeof( sect[i] ) ) != sizeof( sect[i] ) ) {
            return( DS_ERR | DS_FREAD_FAILED );
        }
        ByteSwapShdr( &sect[i], byte_swap );
        off += head.e_shentsize;
    }

    /* If there is a SHT_SYMTAB section use that, otherwise SHT_DYNSYM */
    tab_type = SHT_NULL;
    for( i = 1; i < head.e_shnum; ++i ) {
        switch( sect[i].sh_type ) {
        case SHT_SYMTAB:
            tab_type = SHT_SYMTAB;
            break;
        case SHT_DYNSYM:
            if( tab_type == SHT_NULL )
                tab_type = SHT_DYNSYM;
            break;
        }
    }
    if( tab_type == SHT_NULL ) {
        /* We didn't find any symbol tables - must be stripped */
        ImpUnloadInfo( iih );
        return( DS_FAIL );
    }

    strtab = NULL;
    strings = NULL;
    for( i = 1; i < head.e_shnum; ++i ) {
        if( sect[i].sh_type == tab_type ) {
            if( strtab != &sect[sect[i].sh_link] ) {
                strtab = &sect[sect[i].sh_link];
                new = DCRealloc( strings, strtab->sh_size );
                if( new == NULL ) {
                    DCFree( strings );
                    return( DS_ERR | DS_NO_MEM );
                }
                strings = new;
                if( BSeek( fp, strtab->sh_offset, DIG_ORG ) != strtab->sh_offset ) {
                    DCFree( strings );
                    return( DS_ERR | DS_FSEEK_FAILED );
                }
                if( BRead( fp, strings, strtab->sh_size ) != strtab->sh_size ) {
                    DCFree( strings );
                    return( DS_ERR | DS_FREAD_FAILED );
                }
            }
            off = sect[i].sh_offset;
            num_syms = sect[i].sh_size / sect[i].sh_entsize;
            for( j = 0; j < num_syms; ++j ) {
                if( BSeek( fp, off, DIG_ORG ) != off ) {
                    DCFree( strings );
                    return( DS_ERR | DS_FSEEK_FAILED );
                }
                if( BRead( fp, &sym, sizeof( sym ) ) != sizeof( sym ) ) {
                    DCFree( strings );
                    return( DS_ERR | DS_FREAD_FAILED );
                }
                ByteSwapSym( &sym, byte_swap );
                name = &strings[sym.st_name];
                len = strlen( name );
                ds = DS_OK;
                switch( ELF32_ST_TYPE( sym.st_info ) ) {
                case STT_FILE:
                    /* Make first filename we see be the module name */
                    if( iih->name == NULL ) {
                        ds = AddName( iih, len, name );
                    }
                    break;
                case STT_NOTYPE:
                    if( sym.st_shndx < head.e_shnum ) {
                        addr_seg    seg;

                        /* Take a guess: If pointing to an executable section,
                         * symbol is probably code.
                         */
                        if( sect[sym.st_shndx].sh_flags & SHF_EXECINSTR )
                            seg = MAP_FLAT_CODE_SELECTOR;
                        else
                            seg = MAP_FLAT_DATA_SELECTOR;
                        ds = AddSymbol( iih, seg, sym.st_value, len, name );
                    }
                    break;
                case STT_OBJECT:
                    if( sym.st_shndx < head.e_shnum ) {
                        ds = AddSymbol( iih, MAP_FLAT_DATA_SELECTOR, sym.st_value, len, name );
                    }
                    break;
                case STT_FUNC:
                    if( sym.st_shndx < head.e_shnum ) {
                        ds = AddSymbol( iih, MAP_FLAT_CODE_SELECTOR, sym.st_value, len, name );
                    }
                    break;
                }
                if( ds != DS_OK ) {
                    DCFree( strings );
                    return( ds );
                }
                off += sect[i].sh_entsize;
            }
        }
    }
    DCFree( strings );
    if( iih->gbl == NULL ) {
        /* We didn't find any symbols - must be stripped */
        ImpUnloadInfo( iih );
        return( DS_FAIL );
    }

    /* No module name yet, make something up */
    if( iih->name == NULL ) {
        static char     unknown[] = "Unknown000";

        ds = AddName( iih, sizeof( unknown ) - 1, unknown );
        if( ds != DS_OK )
            return( ds );
        for( name = &unknown[sizeof( unknown ) - 2]; ++*name == ('9' + 1); --name ) {
            *name = '0';
        }
    }

    return( DS_OK );
}

static dip_status (*Try[])( FILE *, imp_image_handle * ) = {
    TryStub,
    TryNLM,
    TryELF,
    NULL
};

dip_status DIPIMPENTRY( LoadInfo )( FILE *fp, imp_image_handle *iih )
{
    dip_status  ds;
    int         i;

    iih->gbl = NULL;
    iih->addr = NULL;
    iih->name = NULL;
    iih->hunks = NULL;
    Buff.len = 0;
    Buff.off = 0;

    i = 0;
    for( ;; ) {
        if( Try[i] == NULL )
            return( DS_FAIL );
        switch( BSeek( fp, 0, DIG_ORG ) ) {
        case DIG_SEEK_ERROR:
            return( DS_FSEEK_FAILED );
        case 0:
            break;
        default:
            return( DS_FAIL );
        }
        ds = Try[i]( fp, iih );
        if( ds & DS_ERR ) {
            DCStatus( ds );
            /* clean up any allocations */
            ImpUnloadInfo( iih );
            return( ds );
        }
        if( ds == DS_OK )
            break;
        ++i;
    }
    return( DS_OK );
}

void DIPIMPENTRY( MapInfo )( imp_image_handle *iih, void *d )
{
    exp_block   *b;
    exp_sym     *s;

    for( s = iih->gbl; s != NULL; s = s->next ) {
        DCMapAddr( &s->addr, d );
    }
    for( b = iih->addr; b != NULL; b = b->next ) {
        DCMapAddr( &b->start, d );
    }
}

void DIPIMPENTRY( UnloadInfo )( imp_image_handle *iih )
{
    ImpUnloadInfo( iih );
}
