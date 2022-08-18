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
* Description:  Help file searching routines.
*
****************************************************************************/


#include <stdio.h>
#include <string.h>
#include "bool.h"
#include "help.h"
#include "helpmem.h"

#include "clibext.h"


#define DEFAULTTOPIC "TABLE OF CONTENTS"

FILE                    *curFile = NULL;
char                    curPage[HLP_PAGE_SIZE];
HelpPageHeader          *pageHeader;
char                    *stringBlock;
void                    *pageIndex;

static void loadPage( HelpHdl hdl, unsigned long pagenum )
{
    unsigned long       offset;

    if( curFile == hdl->fp && pageHeader->page_num == pagenum )
        return;
    offset = hdl->header.str_size + pagenum * HLP_PAGE_SIZE + hdl->header.datapagecnt * sizeof( uint_16 );
    if( hdl->header.ver_maj == 1 ) {
        offset += HELP_HEADER_V1_SIZE;
    } else {
        offset += HELP_HEADER_SIZE;
    }
    HelpSeek( hdl->fp, offset, HELP_SEEK_SET );
    HelpRead( hdl->fp, curPage, HLP_PAGE_SIZE );
    curFile = hdl->fp;
    pageHeader = (HelpPageHeader *)curPage;
    pageIndex = curPage + sizeof( HelpPageHeader );
    if( pageHeader->type == PAGE_DATA ) {
        stringBlock = curPage + sizeof( HelpPageHeader )
                    + pageHeader->num_entries * sizeof( PageIndexEntry );
    }
}

static void loadNextPage( HelpHdl hdl, const char *name )
{
    unsigned            i;
    HelpIndexEntry      *entry;

    entry = pageIndex;
    for( i = 0; i < pageHeader->num_entries; i++ ) {
        if( strnicmp( entry->start, name, INDEX_LEN - 1 ) >= 0 ) {
            entry++;
            break;
        }
        entry++;
    }
    entry--;    /* if we've read through the entire list load the last page */
    loadPage( hdl, entry->nextpage );
}

/*
 * doFindEntry - find the closest match that is alphabetically before
 *                      name in the current page. If no entry on the current
 *                      page comes before name return the first entry on the
 *                      page.
 */
static char *doFindEntry( const char *name, unsigned *entry_num )
{
    unsigned            i;
    size_t              len;
    PageIndexEntry      *entry;
    int                 cmpret;

    entry = pageIndex;
    len = strlen( name );
    for( i = 0; i < pageHeader->num_entries; i++ ) {
        cmpret = strnicmp( stringBlock + entry[i].name_offset, name, len );
        if( cmpret == 0 ) {
            break;
        }
        if( cmpret > 0 ) {
            if( i > 0 ) {
                i--;
            }
            break;
        }
    }
    if( i == pageHeader->num_entries ) {
        i--;
    }
    if( entry_num != NULL ) {
        *entry_num = i;
    }
    return( stringBlock + entry[i].name_offset );
}

static char *findEntry( HelpHdl hdl, const char *name, unsigned *entry_num )
{
    char        *ret;
    unsigned    pagecnt;
    unsigned    basepage;
    int         cmpret;

    pagecnt = hdl->header.datapagecnt + hdl->header.indexpagecnt;
    basepage = pageHeader->page_num;
    ret = doFindEntry( name, entry_num );
    cmpret = stricmp( ret, name );
    while( cmpret < 0 && pageHeader->page_num < pagecnt - 1 ) {
        loadPage( hdl, pageHeader->page_num + 1 );
        ret = doFindEntry( name, entry_num );
        cmpret = stricmp( ret, name );
    }
    if( cmpret > 0 && pageHeader->page_num > basepage ) {
        loadPage( hdl, pageHeader->page_num - 1 );
        ret = doFindEntry( name, entry_num );
    }
    return( ret );
}

#if 0
char *HelpFindPrev( HelpSrchInfo *info )
{
    PageIndexEntry      *entry;
    char                *ret;

    if( info->page == 0 && info->entry == 0 ) return( NULL );
    if( info->entry == 0 ) {
        info->page--;
        loadPage( info->hdl, info->page );
        info->entry = pageHeader->num_entries - 1;
    } else {
        info->entry--;
        loadPage( info->hdl, info->page );
    }
    entry = pageIndex;
    info->offset = entry[ info->entry ].entry_offset;
    ret = stringBlock + entry[ info->entry ].name_offset;
    return( ret );
}

unsigned long HelpGetOffset( HelpSrchInfo cursor )
{
    return( cursor.offset );
}


char *HelpFindNext( HelpSrchInfo *info )
{
    char                *ret;
    PageIndexEntry      *entry;
    unsigned            pagecnt;

    pagecnt = info->hdl->header.datapagecnt + info->hdl->header.indexpagecnt;
    if( info->page == pagecnt ) return( NULL );
    loadPage( info->hdl, info->page );
    info->entry++;
    if( info->entry == pageHeader->num_entries ) {
        info->page++;
        if( info->page == pagecnt ) return( NULL );
        loadPage( info->hdl, info->page );
        info->entry = 0;
    }
    entry = pageIndex;
    info->offset = entry[ info->entry ].entry_offset;
    ret = stringBlock + entry[ info->entry ].name_offset;
    return( ret );
}
#endif

unsigned HelpFindFirst( HelpHdl hdl, const char *name, HelpSrchInfo *info )
{
    unsigned            ret;
    PageIndexEntry      *entry;

    loadPage( hdl, 0 );
    while( pageHeader->type != PAGE_DATA ) {
        loadNextPage( hdl, name );
    }
    findEntry( hdl, name, &ret );
    if( info != NULL ) {
        info->hdl = hdl;
        info->entry = ret;
        info->page = pageHeader->page_num;
        entry = pageIndex;
        info->offset = entry[ info->entry ].entry_offset;
    }
    ret += hdl->itemindex[ pageHeader->page_num - hdl->header.indexpagecnt ];
    return( ret );
}

char *HelpGetIndexedTopic( HelpHdl hdl, unsigned index )
{
    unsigned            i;
    PageIndexEntry      *entry;

    if( hdl == NULL || index >= hdl->header.topiccnt )
        return( NULL );
    for( i = 0; i < hdl->header.datapagecnt - 1; i++ ) {
        if( hdl->itemindex[i + 1] > index ) {
            break;
        }
    }
    loadPage( hdl, i + hdl->header.indexpagecnt );
    index -= hdl->itemindex[i];
    entry = pageIndex;
    entry += index;
    return( stringBlock + entry->name_offset );
}

unsigned long HelpFindTopicOffset( HelpHdl hdl, const char *topic )
{
    unsigned            entry_num;
    PageIndexEntry      *entry;
    char                *foundtopic;

    if( hdl != NULL ) {
        loadPage( hdl, 0 );
        while( pageHeader->type != PAGE_DATA ) {
            loadNextPage( hdl, topic );
        }
        foundtopic = findEntry( hdl, topic, &entry_num );
        if( stricmp( foundtopic, topic ) == 0 ) {
            entry = pageIndex;
            return( entry[ entry_num ].entry_offset );
        }
    }
    return( (unsigned long)-1 );
}

HelpHdl InitHelpSearch( FILE *fp )
{
    HelpHdl     hdl;
    size_t      len;
    uint_16     *buffer;
    int         str_cnt;
    uint_16     *str_len;
    char        *ptr;
    uint_32     u32;
    uint_16     u16;

    HelpSeek( fp, 0, HELP_SEEK_SET );
    hdl = HelpMemAlloc( sizeof( struct HelpHdl ) );
    hdl->fp = fp;
    HelpRead( fp, &u32, sizeof( u32 ) );
    hdl->header.sig1 = u32;
    HelpRead( fp, &u32, sizeof( u32 ) );
    hdl->header.sig2 = u32;
    HelpRead( fp, &u16, sizeof( u16 ) );
    hdl->header.ver_maj = u16;
    HelpRead( fp, &u16, sizeof( u16 ) );
    hdl->header.ver_min = u16;
    if( hdl->header.sig1 != HELP_SIG_1
      || hdl->header.sig2 != HELP_SIG_2
      || hdl->header.ver_min != HELP_MIN_VER ) {
        HelpMemFree( hdl );
        hdl = NULL;
    } else if( hdl->header.ver_maj == HELP_MAJ_V1
      || hdl->header.ver_maj == HELP_MAJ_VER ) {
        HelpRead( fp, &u16, sizeof( u16 ) );
        hdl->header.indexpagecnt = u16;
        HelpRead( fp, &u16, sizeof( u16 ) );
        hdl->header.datapagecnt = u16;
        HelpRead( fp, &u32, sizeof( u32 ) );
        hdl->header.topiccnt = u32;
        if( hdl->header.ver_maj == HELP_MAJ_V1 ) {
            u16 = 0;                        // no str_size in V1 header format
        } else {
            HelpRead( fp, &u16, sizeof( u16 ) );
        }
        hdl->header.str_size = u16;
        HelpSeek( fp, 6 * sizeof( uint_16 ), HELP_SEEK_CUR );
        if( hdl->header.str_size ) {
            buffer = HelpMemAlloc( hdl->header.str_size );
            HelpRead( fp, buffer, hdl->header.str_size );
            str_cnt = buffer[0];
            str_len = buffer + 1;
            ptr = (char *)( str_len + str_cnt );
            if( str_len[0] != 0 ) {
                hdl->def_topic = HelpDupStr( ptr );
            } else {
                hdl->def_topic = HelpDupStr( DEFAULTTOPIC );
            }
            if( str_len[1] != 0 ) {
                hdl->desc_str = HelpDupStr( ptr + str_len[0] );
            } else {
                hdl->desc_str = NULL;
            }
            HelpMemFree( buffer );
        } else {
            hdl->def_topic = HelpDupStr( DEFAULTTOPIC );
            hdl->desc_str = NULL;
        }
        len = hdl->header.datapagecnt * ( sizeof( uint_16 ) );
        hdl->itemindex = HelpMemAlloc( len );
        HelpRead( fp, hdl->itemindex, len );
    } else {
        HelpMemFree( hdl );
        hdl = NULL;
    }

    return( hdl );
}

char *GetDefTopic( HelpHdl hdl )
{
    char        *topic;

    if( hdl == NULL ) {
        topic = DEFAULTTOPIC;
    } else {
        topic = hdl->def_topic;
    }
    return( topic );
}

char *GetDescrip( HelpHdl hdl )
{
    char        *description;

    if( hdl == NULL ) {
        description = NULL;
    } else {
        description = hdl->desc_str;
    }
    return( description );
}

void FiniHelpSearch( HelpHdl hdl )
{
    if( hdl != NULL ) {
        if( hdl->itemindex != NULL ) {
            HelpMemFree( hdl->itemindex );
        }
        if( hdl->def_topic != NULL ) {
            HelpMemFree( hdl->def_topic );
        }
        if( hdl->desc_str != NULL ) {
            HelpMemFree( hdl->desc_str );
        }
        HelpMemFree( hdl );
    }
}

#ifdef TEST_SEARCH
#include "trmemcvr.h"

void main( int argc, char *argv[] )
{
    FILE                *fp;
    HelpHdl             hdl;
    char                name[_MAX_PATH];
    char                *cur;
    HelpSrchInfo        cursor;
    unsigned            i;

    if( argc != 2 ) {
        printf( "USAGE:\n" );
        printf( "exename <help file>\n" );
        return;
    }
    fp = HelpOpen( argv[1] );
    if( fp == NULL ) {
        printf( "Unable to open %s\n", argv[1] );
        return;
    }
    TRMemOpen();
    hdl = InitHelpSearch( fp );
    for( ;; ) {
        gets( name );
        if( !strcmp( name, "bob" ) ) break;
        cur = HelpFindFirst( hdl, name, &cursor );
        for( i = 0; i < 5; i++ ) {
            if( cur == NULL ) break;
            printf( "     %s\n", cur );
            HelpMemFree( cur );
            cur = HelpFindNext( &cursor );
        }
        if( cur != NULL ) {
            HelpMemFree( cur );
        }
    }
    FiniHelpSearch( hdl );
    HelpClose( fp );
    TRMemClose();
}
#endif
