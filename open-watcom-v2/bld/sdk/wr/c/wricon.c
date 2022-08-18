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
* Description:  Resource editor icon/cursor manipulation.
*
****************************************************************************/


#include "wrglbl.h"
#include <limits.h>
#include "wresdefn.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define DEF_MEMFLAGS (MEMFLAG_MOVEABLE | MEMFLAG_PURE)

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

uint_32 WRAPI WRSizeOfImage( BITMAPINFOHEADER *bih )
{
    uint_32     size;

    size = (uint_32)(DIB_INFO_SIZE( bih->biBitCount ) + bih->biSizeImage);

    return( size );
}

// This function assumes that the data represents icon data WITHOUT
// an icon directory
WORD WRAPI WRCountIconImages( BYTE *data, uint_32 size )
{
    BITMAPINFOHEADER    *bih;
    WORD                count;
    uint_32             pos;

    pos = 0;
    count = 0;
    while( pos < size ) {
        bih = (BITMAPINFOHEADER *)( data + pos );
        count++;
        pos += WRSizeOfImage( bih );
        // if we overrun do not count this block
        if( pos > size ) {
            count--;
        }
    }

    return( count );
}

bool WRAPI WRCreateIconHeader( BYTE *data, size_t size, WORD type, ICONHEADER **ih, size_t *ihsize )
{
    BITMAPINFOHEADER    *bih;
    WORD                count;
    size_t              pos;
    int                 i;

    if( data == NULL || size == 0 || ih == NULL || ihsize == NULL ) {
        return( false );
    }

    count = WRCountIconImages( data, size );
    if( count == 0 ) {
        return( false );
    }

    *ihsize = sizeof( ICONHEADER ) + sizeof( ICONDIRENTRY ) * (count - 1);
    *ih = MemAlloc( *ihsize );
    if( *ih == NULL ) {
        return( false );
    }

    (*ih)->idReserved = 0;
    (*ih)->idType = type;
    (*ih)->idCount = count;

    for( i = 0, pos = 0; i < count; i++ ) {
        bih = (BITMAPINFOHEADER *)( data + pos );
        (*ih)->idEntries[i].bWidth = bih->biWidth;
        (*ih)->idEntries[i].bHeight = bih->biHeight / 2;
        if( type == 1 ) {
            (*ih)->idEntries[i].bColorCount = 1 << bih->biBitCount;
        } else {
            (*ih)->idEntries[i].bColorCount = 0;
        }
        (*ih)->idEntries[i].bReserved = 0;
        (*ih)->idEntries[i].wPlanes = bih->biPlanes;
        (*ih)->idEntries[i].wBitCount = bih->biBitCount;
        (*ih)->idEntries[i].dwBytesInRes = WRSizeOfImage( bih );
        if( i == 0 ) {
            (*ih)->idEntries[i].dwImageOffset = *ihsize;
        } else {
            (*ih)->idEntries[i].dwImageOffset = (*ih)->idEntries[i - 1].dwImageOffset +
                                                (*ih)->idEntries[i - 1].dwBytesInRes;
        }
        pos += (*ih)->idEntries[i].dwBytesInRes;
    }

    return( true );
}

bool WRAPI WRCreateCursorResHeader( RESCURSORHEADER **rch, size_t *rchsize, BYTE *data, size_t data_size )
{
    CURSORHEADER        *ch;
    size_t              chsize;
    ICONHEADER          *ih;
    size_t              ihsize;
    int                 i;
    bool                ok;

    ih = NULL;

    ok = (rch != NULL && rchsize != NULL && data != NULL && data_size != 0);

    if( ok ) {
        *rch = NULL;
        *rchsize = 0;
        ch = (CURSORHEADER *)data;
        chsize = sizeof( CURSORHEADER );
        chsize += sizeof( CURSORDIRENTRY ) * (ch->cdCount - 1);
        ok = WRCreateIconHeader( data + chsize, data_size - chsize, 2, &ih, &ihsize );
    }

    if( ok ) {
        *rchsize = sizeof( RESCURSORHEADER );
        *rchsize += sizeof( RESCURSORDIRENTRY ) * (ih->idCount - 1);
        *rch = (RESCURSORHEADER *)MemAlloc( *rchsize );
        ok = (*rch != NULL);
    }

    if( ok ) {
        memcpy( *rch, ch, sizeof( WORD ) * 3 );
        for( i = 0; i < ih->idCount; i++ ) {
            (*rch)->cdEntries[i].bWidth = ih->idEntries[i].bWidth;
            (*rch)->cdEntries[i].bHeight = ih->idEntries[i].bHeight * 2;
            (*rch)->cdEntries[i].wPlanes = ih->idEntries[i].wPlanes;
            (*rch)->cdEntries[i].wBitCount = ih->idEntries[i].wBitCount;
            (*rch)->cdEntries[i].lBytesInRes = ih->idEntries[i].dwBytesInRes;
            (*rch)->cdEntries[i].wNameOrdinal = i + 1;
        }
    }

    if( ih != NULL ) {
        MemFree( ih );
    }

    return( ok );
}

bool WRAPI WRCreateIconResHeader( RESICONHEADER **rih, size_t *rihsize, BYTE *data, size_t data_size )
{
    ICONHEADER          *pih;
    size_t              pihsize;
    ICONHEADER          *ih;
    size_t              ihsize;
    int                 i;
    bool                ok;

    ih = NULL;

    ok = (rih != NULL && rihsize != NULL && data != NULL && data_size != 0);

    if( ok ) {
        pih = (ICONHEADER *)data;
        pihsize = sizeof( ICONHEADER );
        pihsize += sizeof( ICONDIRENTRY ) * (pih->idCount - 1);
        ok = WRCreateIconHeader( data + pihsize, data_size - pihsize, 1, &ih, &ihsize );
    }

    if( ok ) {
        *rihsize = sizeof( RESICONHEADER );
        *rihsize += sizeof( RESICONDIRENTRY ) * (ih->idCount - 1);
        *rih = (RESICONHEADER *)MemAlloc( *rihsize );
        ok = (*rih != NULL);
    }

    if( ok ) {
        memcpy( *rih, pih, sizeof( WORD ) * 3 );
        for( i = 0; i < ih->idCount; i++ ) {
            (*rih)->idEntries[i].bWidth = ih->idEntries[i].bWidth;
            (*rih)->idEntries[i].bHeight = ih->idEntries[i].bHeight;
            (*rih)->idEntries[i].bColorCount = ih->idEntries[i].bColorCount;
            (*rih)->idEntries[i].bReserved = 0;
            (*rih)->idEntries[i].wPlanes = ih->idEntries[i].wPlanes;
            (*rih)->idEntries[i].wBitCount = ih->idEntries[i].wBitCount;
            (*rih)->idEntries[i].lBytesInRes = ih->idEntries[i].dwBytesInRes;
            (*rih)->idEntries[i].wNameOrdinal = i + 1;
        }
    }

    if( ih != NULL ) {
        MemFree( ih );
    }

    return( ok );
}

bool WRAPI WRAddCursorHotspot( BYTE **cursor, size_t *size, CURSORHOTSPOT *hs )
{
    int hs_size;

    hs_size = sizeof( CURSORHOTSPOT );

    if( cursor == NULL || size == NULL ) {
        return( false );
    }

    *cursor = MemRealloc( *cursor, *size + hs_size );
    if( *cursor == NULL ) {
        return( false );
    }
    memmove( *cursor + hs_size, *cursor, *size );
    memcpy( *cursor, hs, hs_size );
    *size += hs_size;

    return( true );
}

bool WRAPI WRGetAndAddCursorImage( BYTE *data, WResDir dir, CURSORDIRENTRY *cd, uint_16 ord )
{
    BYTE                *cursor;
    bool                dup;
    size_t              size;
    WResID              *tname;
    WResID              *rname;
    WResLangType        lang;
    CURSORHOTSPOT       hotspot;
    bool                ok;

    dup = false;
    lang.lang = DEF_LANG;
    lang.sublang = DEF_SUBLANG;
    tname = NULL;
    rname = NULL;

    ok = (data != NULL && dir != NULL && cd != NULL && cd->dwBytesInRes != 0);

    if( ok ) {
        cursor = (BYTE *)MemAlloc( cd->dwBytesInRes );
        ok = (cursor != NULL);
    }

    if( ok ) {
        memcpy( cursor, data + cd->dwImageOffset, cd->dwBytesInRes );
        hotspot.xHotspot = cd->wXHotspot;
        hotspot.yHotspot = cd->wYHotspot;
        size = cd->dwBytesInRes;
        ok = WRAddCursorHotspot( &cursor, &size, &hotspot );
    }

    if( ok ) {
        tname = WResIDFromNum( RESOURCE2INT( RT_CURSOR ) );
        ok = (tname != NULL);
    }

    if( ok ) {
        rname = WResIDFromNum( ord );
        ok = (rname != NULL);
    }

    if( ok ) {
        ok = !WResAddResource( tname, rname, DEF_MEMFLAGS, 0, size, dir, &lang, &dup );
    }

    if( ok ) {
        ok = WRFindAndSetData( dir, tname, rname, &lang, cursor );
    }

    if( !ok ) {
        if( cursor != NULL ) {
            MemFree( cursor );
        }
    }

    if( tname != NULL ) {
        MemFree( tname );
    }

    if( rname != NULL ) {
        MemFree( rname );
    }

    return( ok );
}

bool WRAPI WRGetAndAddIconImage( BYTE *data, WResDir dir, ICONDIRENTRY *id, uint_16 ord )
{
    BYTE                *icon;
    bool                dup;
    WResID              *tname;
    WResID              *rname;
    WResLangType        lang;
    bool                ok;

    dup = false;
    lang.lang = DEF_LANG;
    lang.sublang = DEF_SUBLANG;
    tname = NULL;
    rname = NULL;

    ok = (data != NULL && dir != NULL && id != NULL && id->dwBytesInRes != 0);

    if( ok ) {
        icon = (BYTE *)MemAlloc( id->dwBytesInRes );
        ok = (icon != NULL);
    }

    if( ok ) {
        memcpy( icon, data + id->dwImageOffset, id->dwBytesInRes );
        tname = WResIDFromNum( RESOURCE2INT( RT_ICON ) );
        ok = (tname != NULL);
    }

    if( ok ) {
        rname = WResIDFromNum( ord );
        ok = (rname != NULL);
    }

    if( ok ) {
        ok = !WResAddResource( tname, rname, DEF_MEMFLAGS, 0,
                               id->dwBytesInRes, dir, &lang, &dup );
    }

    if( ok ) {
        ok = WRFindAndSetData( dir, tname, rname, &lang, icon );
    }

    if( !ok ) {
        if( icon != NULL ) {
            MemFree( icon );
        }
    }

    if( tname != NULL ) {
        MemFree( tname );
    }

    if( rname != NULL ) {
        MemFree( rname );
    }

    return( ok );
}

bool WRAPI WRFindImageId( WRInfo *info, WResTypeNode **otnode,
                             WResResNode **ornode, WResLangNode **lnode,
                             uint_16 type, uint_16 id, WResLangType *ltype )
{
    WResTypeNode        *tnode;
    WResResNode         *rnode;
    WResLangType        lang;
    bool                ok;

    ok = ( info != NULL && lnode != NULL &&
          (type == RESOURCE2INT( RT_ICON ) || type == RESOURCE2INT( RT_CURSOR )) );

    if( ok ) {
        tnode = WRFindTypeNode( info->dir, type, NULL );
        ok = (tnode != NULL);
    }

    if( ok ) {
        if( otnode != NULL ) {
            *otnode = tnode;
        }
        rnode = WRFindResNode( tnode, id, NULL );
        ok = (rnode != NULL);
    }

    if( ok ) {
        if( ornode != NULL ) {
            *ornode = rnode;
        }
        if( ltype != NULL ) {
            lang = *ltype;
        } else {
            lang.lang = DEF_LANG;
            lang.sublang = DEF_SUBLANG;
        }
        *lnode = WRFindLangNodeFromLangType( rnode, &lang );
        ok = (*lnode != NULL);
    }

    if( !ok ) {
        *lnode = NULL;
    }

    return( ok );
}

bool WRAPI WRAppendDataToData( BYTE **d1, size_t *d1size, BYTE *d2, size_t d2size )
{
    if( d1 == NULL || d1size == NULL || d2 == NULL || d2size == 0 ) {
        return( false );
    }

    if( *d1size + d2size > INT_MAX ) {
        return( false );
    }

    *d1 = MemRealloc( *d1, *d1size + d2size );
    if( *d1 == NULL ) {
        return( false );
    }

    memcpy( *d1 + *d1size, d2, d2size );
    *d1size += d2size;

    return( true );
}

bool WRAPI WRAddCursorImageToData( WRInfo *info, WResLangNode *lnode,
                                      BYTE **data, size_t *size, CURSORHOTSPOT *hotspot )
{
    BYTE        *ldata;
    size_t      hs_size; // size of hotspot info
    bool        ok;

    ldata = NULL;

    ok = (info != NULL && lnode != NULL && data != NULL && size != NULL && hotspot != NULL);

    if( ok ) {
        ldata = WRCopyResData( info, lnode );
        ok = (ldata != NULL);
    }

    if( ok ) {
        hs_size = sizeof( CURSORHOTSPOT );
        memcpy( hotspot, ldata, hs_size );
        ok = WRAppendDataToData( data, size, ldata + hs_size, lnode->Info.Length - hs_size );
    }

    if( ldata != NULL ) {
        MemFree( ldata );
    }

    return( ok );
}

bool WRAPI WRAddIconImageToData( WRInfo *info, WResLangNode *lnode, BYTE **data, size_t *size )
{
    BYTE        *ldata;
    bool        ok;

    ldata = NULL;

    ok = (info != NULL && lnode != NULL && data != NULL && size != NULL);

    if( ok ) {
        ldata = WRCopyResData( info, lnode );
        ok = (ldata != NULL);
    }

    if( ok ) {
        ok = WRAppendDataToData( data, size, ldata, lnode->Info.Length );
    }

    if( ldata != NULL ) {
        MemFree( ldata );
    }

    return( ok );
}

bool WRAPI WRCreateCursorData( WRInfo *info, WResLangNode *lnode, BYTE **data, size_t *size )
{
    WResLangNode        *ilnode;
    BYTE                *ldata;
    RESCURSORHEADER     *rch;
    CURSORHEADER        *ch;
    CURSORHOTSPOT       hotspot;
    WResLangType        lt;
    uint_16             ord;
    size_t              osize;
    int                 i;
    bool                ok;

    ok = (info != NULL && lnode != NULL && data != NULL && size != NULL);

    if( ok ) {
        ldata = WRCopyResData( info, lnode );
        ok = (ldata != NULL);
    }

    if( ok ) {
        rch = (RESCURSORHEADER *)ldata;
        *size = sizeof( CURSORHEADER );
        *size += sizeof( CURSORDIRENTRY ) * (rch->cwCount - 1);
        *data = (BYTE *)MemAlloc( *size );
        ch = (CURSORHEADER *)*data;
        ok = (*data != NULL);
    }

    if( ok ) {
        memcpy( ch, rch, sizeof( WORD ) * 3 );
    }

    if( ok ) {
        for( i = 0; ok && i < rch->cwCount; i++ ) {
            ord = (uint_16)rch->cdEntries[i].wNameOrdinal;
            lt = lnode->Info.lang;
            ok = WRFindImageId( info, NULL, NULL, &ilnode, RESOURCE2INT( RT_CURSOR ), ord, &lt );
            if( ok ) {
                osize = *size;
                ok = WRAddCursorImageToData( info, ilnode, data, size, &hotspot );
                if( ok ) {
                    ch = (CURSORHEADER *)*data;
                    ch->cdEntries[i].bWidth = rch->cdEntries[i].bWidth;
                    ch->cdEntries[i].bHeight = rch->cdEntries[i].bHeight / 2;
                    ch->cdEntries[i].bColorCount = 0;
                    ch->cdEntries[i].bReserved = 0;
                    ch->cdEntries[i].wXHotspot = hotspot.xHotspot;
                    ch->cdEntries[i].wYHotspot = hotspot.yHotspot;
                    ch->cdEntries[i].dwBytesInRes = *size - osize;
                    ch->cdEntries[i].dwImageOffset = osize;
                }
            }
        }
    }

    return( ok );
}

bool WRAPI WRCreateIconData( WRInfo *info, WResLangNode *lnode, BYTE **data, size_t *size )
{
    WResLangNode        *ilnode;
    BYTE                *ldata;
    RESICONHEADER       *rih;
    ICONHEADER          *ih;
    WResLangType        lt;
    uint_16             ord;
    size_t              osize;
    int                 i;
    bool                ok;

    ok = (info != NULL && lnode != NULL && data != NULL && size != NULL);

    if( ok ) {
        ldata = WRCopyResData( info, lnode );
        ok = (ldata != NULL);
    }

    if( ok ) {
        rih = (RESICONHEADER *)ldata;
        *size = sizeof( ICONHEADER );
        *size += sizeof( ICONDIRENTRY ) * (rih->cwCount - 1);
        *data = (BYTE *)MemAlloc( *size );
        ih = (ICONHEADER *)*data;
        ok = (*data != NULL);
    }

    if( ok ) {
        memcpy( ih, rih, sizeof( WORD ) * 3 );
    }

    if( ok ) {
        for( i = 0; ok && i < rih->cwCount; i++ ) {
            ord = (uint_16)rih->idEntries[i].wNameOrdinal;
            lt = lnode->Info.lang;
            ok = WRFindImageId( info, NULL, NULL, &ilnode, RESOURCE2INT( RT_ICON ), ord, &lt );
            if( ok ) {
                osize = *size;
                ok = WRAddIconImageToData( info, ilnode, data, size );
                if( ok ) {
                    ih = (ICONHEADER *)*data;
                    ih->idEntries[i].bWidth = rih->idEntries[i].bWidth;
                    ih->idEntries[i].bHeight = rih->idEntries[i].bHeight;
                    ih->idEntries[i].bColorCount = rih->idEntries[i].bColorCount;
                    //ih->idEntries[i].wPlanes = rih->idEntries[i].wPlanes;
                    //ih->idEntries[i].wBitCount = rih->idEntries[i].wBitCount;
                    ih->idEntries[i].wPlanes = 0;
                    ih->idEntries[i].wBitCount = 0;
                    ih->idEntries[i].bReserved = 0;
                    ih->idEntries[i].dwBytesInRes = *size - osize;
                    ih->idEntries[i].dwImageOffset = osize;
                }
            }
        }
    }

    return( ok );
}

uint_16 WRAPI WRFindUnusedImageId( WRInfo *info, uint_16 start )
{
    WResLangNode        *lnode;
    bool                found;
    bool                rollover;

    found = false;
    rollover = false;
    if( start == 0 ) {
        start = 1;
    }
    if( start == 1 ) {
        rollover = true;
    }

    for( ;; ) {
        if( start > 0x7fff ) {
            if( !rollover ) {
                rollover = true;
                start = 1;
            } else {
                break;
            }
        }
        if( !WRFindImageId( info, NULL, NULL, &lnode, RESOURCE2INT( RT_ICON ), start, NULL ) ) {
            if( !WRFindImageId( info, NULL, NULL, &lnode, RESOURCE2INT( RT_CURSOR ), start, NULL ) ) {
                found = true;
                break;
            }
        }
        start++;
    }

    if( !found ) {
        start = 0;
    }

    return( start );
}

bool WRAPI WRCreateCursorEntries( WRInfo *info, WResLangNode *lnode,
                                     void *data, size_t size )
{
    RESCURSORHEADER     *rch;
    CURSORHEADER        *ch;
    uint_16             ord;
    size_t              rchsize;
    int                 i;
    bool                ok;

    ok = (info != NULL && lnode != NULL && data != NULL && size != 0);

    if( ok ) {
        if( lnode->data != NULL ) {
            MemFree( lnode->data );
            lnode->data = NULL;
        }
        lnode->Info.Length = 0;
        ok = WRCreateCursorResHeader( &rch, &rchsize, data, size );
    }

    if( ok ) {
        lnode->data = (void *)rch;
        lnode->Info.Length = rchsize;
        ord = 0;
        ch = (CURSORHEADER *)data;
        for( i = 0; ok && i < rch->cwCount; i++ ) {
            ord = WRFindUnusedImageId( info, ord );
            ok = (ord != 0);
            if( ok ) {
                rch->cdEntries[i].wNameOrdinal = ord;
                ok = WRGetAndAddCursorImage( data, info->dir, &ch->cdEntries[i], ord );
            }
        }
    }

    return( ok );
}

bool WRAPI WRCreateIconEntries( WRInfo *info, WResLangNode *lnode, void *data, size_t size )
{
    RESICONHEADER       *rih;
    ICONHEADER          *ih;
    uint_16             ord;
    size_t              rihsize;
    int                 i;
    bool                ok;

    ok = (info != NULL && lnode != NULL && data != NULL && size != 0);

    if( ok ) {
        if( lnode->data != NULL ) {
            MemFree( lnode->data );
            lnode->data = NULL;
        }
        lnode->Info.Length = 0;
        ok = WRCreateIconResHeader( &rih, &rihsize, data, size );
    }

    if( ok ) {
        lnode->data = (void *)rih;
        lnode->Info.Length = rihsize;
        ord = 0;
        ih = (ICONHEADER *)data;
        for( i = 0; ok && i < rih->cwCount; i++ ) {
            ord = WRFindUnusedImageId( info, ord );
            ok = (ord != 0);
            if( ok ) {
                rih->idEntries[i].wNameOrdinal = ord;
                ok = WRGetAndAddIconImage( data, info->dir, &ih->idEntries[i], ord );
            }
        }
    }

    return( ok );
}

bool WRAPI WRDeleteGroupImages( WRInfo *info, WResLangNode *lnode, uint_16 type )
{
    WResLangType        lt;
    WResTypeNode        *itnode;
    WResResNode         *irnode;
    WResLangNode        *ilnode;
    void                *data;
    RESICONHEADER       *ih;
    RESCURSORHEADER     *ch;
    int                 i;
    uint_16             ord;
    bool                ok;

    data = NULL;
    ok = ( info != NULL && lnode != NULL
           && (type == RESOURCE2INT( RT_GROUP_ICON ) || type == RESOURCE2INT( RT_GROUP_CURSOR )) );

    if( ok ) {
        data = WRCopyResData( info, lnode );
        ok = (data != NULL);
    }

    if( ok ) {
        if( type == RESOURCE2INT( RT_GROUP_ICON ) ) {
            ih = (RESICONHEADER *)data;
            for( i = 0; ok && i < ih->cwCount; i++ ) {
                ord = (uint_16)ih->idEntries[i].wNameOrdinal;
                lt = lnode->Info.lang;
                if( WRFindImageId( info, &itnode, &irnode, &ilnode, RESOURCE2INT( RT_ICON ), ord, &lt ) ) {
                    if( ilnode->data != NULL ) {
                        MemFree( ilnode->data );
                        ilnode->data = NULL;
                    }
                    ok = WRRemoveLangNodeFromDir( info->dir, &itnode, &irnode, &ilnode );
                }
            }
        } else {
            ch = (RESCURSORHEADER *)data;
            for( i = 0; ok && i < ch->cwCount; i++ ) {
                ord = (uint_16)ch->cdEntries[i].wNameOrdinal;
                lt = lnode->Info.lang;
                if( WRFindImageId( info, &itnode, &irnode, &ilnode, RESOURCE2INT( RT_CURSOR ), ord, &lt ) ) {
                    if( ilnode->data != NULL ) {
                        MemFree( ilnode->data );
                        ilnode->data = NULL;
                    }
                    ok = WRRemoveLangNodeFromDir( info->dir, &itnode, &irnode, &ilnode );
                }
            }
        }
    }

    if( data != NULL ) {
        MemFree( data );
    }

    return( ok );
}
