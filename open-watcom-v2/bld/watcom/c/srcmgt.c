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
* Description:  Source management routines for debugger, profiler etc.
*
****************************************************************************/


#ifdef __header1
#include __header1
#endif
#include <limits.h>
#include <string.h>
#include "watcom.h"
#include "srcmgt.h"

#include "clibext.h"


#define SM_CR           '\r'
#define SM_LF           '\n'
#define SM_TAB          '\t'

typedef struct browser {
    struct browser      *next;
    char                *line_ptr;
    char                *line_end;
    unsigned long       bias;
    unsigned long       file_off;
    unsigned long       eof_off;
    unsigned            cur_line;
    char                *cur_line_ptr;
    char                *open_name;
    sm_file_handle      fp;
    sm_mod_handle       mod;
    sm_cue_fileid       id;
    int                 use;
    char                line_buf[ SM_BUF_SIZE ];
} browser;

static browser *FileList = NULL;

static browser *FInitSource( sm_file_handle fp, sm_mod_handle mod, sm_cue_fileid id )
{
    browser     *hndl;

    _SMAlloc( hndl, sizeof( browser ) );
    if( hndl == NULL )
        return( NULL );
    hndl->next = FileList;
    FileList = hndl;
    hndl->file_off = (unsigned long)(-(long)SM_BUF_SIZE);
    hndl->cur_line_ptr = hndl->line_ptr = hndl->line_end = hndl->line_buf;
    hndl->cur_line = 1;
    hndl->fp = fp;
    SMSeekStart( fp );
    hndl->bias = SMTell( fp );
    SMSeekEnd( fp );
    hndl->eof_off = SMTell( fp );
    SMSeekOrg( fp, hndl->bias );
    hndl->open_name = NULL;
    hndl->use = 1;
    hndl->mod = mod;
    hndl->id = id;
    return( hndl );
}


void FClearOpenSourceCache( void )
{
    browser             *hndl;

    for( hndl = FileList; hndl != NULL; hndl = hndl->next ) {
        hndl->mod = SM_NO_MOD;
    }
}

browser *FOpenSource( const char *name, sm_mod_handle mod, sm_cue_fileid id )
{
    sm_file_handle      fp;
    browser             *hndl;

    if( mod != SM_NO_MOD ) {
        for( hndl = FileList; hndl != NULL; hndl = hndl->next ) {
            if( hndl->mod == mod && hndl->id == id ) {
                hndl->use++;
                return( hndl );
            }
        }
    }
    fp = SMOpenRead( name );
    if( SMNilHandle( fp ) )
        return( NULL );
    hndl = FInitSource( fp, mod, id );
    if( hndl == NULL ) {
        SMClose( fp );
    } else {
        _SMAlloc( hndl->open_name, strlen( name ) + 1 );
        strcpy( hndl->open_name, name );
    }
    return( hndl );
}


unsigned long FSize( browser *hndl )
{
    unsigned long       old;
    unsigned long       size;

    old = SMTell( hndl->fp );
    SMSeekEnd( hndl->fp );
    size = SMTell( hndl->fp );
    SMSeekOrg( hndl->fp, old );
    return( size );
}


unsigned long FLastOffset( browser *hndl )
{
    return( (unsigned long)( hndl->file_off + ( hndl->line_ptr - hndl->line_buf ) ) );
}


int FileIsRemote( browser *hndl )
{
    /* unused parameters */ (void)hndl;

    return( SMFileRemote( hndl->fp ) );
}


char *FGetName( browser *hndl )
{
    return( hndl->open_name );
}


void FDoneSource( browser *hndl )
{
    browser     **owner;

    if( hndl != NULL ) {
        hndl->use--;
        if( hndl->use == 0 ) {
            for( owner = &FileList; *owner != hndl; owner = &((*owner)->next) )
                ;
            *owner = hndl->next;
            SMClose( hndl->fp );
            _SMFree( hndl->open_name );
            _SMFree( hndl );
        }
    }
}


static int get_block( browser *hndl, unsigned long off )
{
    sm_read_len         len;
    unsigned long       loc;

    if( off >= hndl->eof_off )
        return( 0 );
    loc = hndl->bias + off;
    if( SMSeekFail( SMSeekOrg( hndl->fp, loc ) ) ) {
        hndl->eof_off = off;
        return( 0 );
    }
    len = SM_BUF_SIZE;
    if( off + len > hndl->eof_off )
        len = hndl->eof_off - off;
    len = SMReadStream( hndl->fp, hndl->line_buf, len );
    if( SMReadError( hndl->fp, len ) ) {
        hndl->eof_off = off;
        return( 0 );
    }
    hndl->file_off = off;
    hndl->line_end = hndl->line_buf + len;
    return( 1 );
}


static int next_src_chr( browser *hndl )
{
    int         c;

    if( hndl->line_ptr == hndl->line_end ) {
        if( !get_block( hndl, hndl->file_off + SM_BUF_SIZE ) )
            return( -1 );
        hndl->line_ptr = hndl->line_buf;
    }
    c = *hndl->line_ptr++;
    return( c );
}


static int next_src_line( browser *hndl )
{
    int         c;
    int         count = 0;

    do {
        c = next_src_chr( hndl );
        if( c == -1 ) {
            if( count )
                break;
            return( 0 );
        }
        count++;
    } while( c != SM_LF );
    hndl->cur_line++;
    hndl->cur_line_ptr = hndl->line_ptr;
    return( 1 );
}


#define BACKUP  (SM_BUF_SIZE / 4 * 3)

static int prev_src_chr( browser *hndl )
{
    unsigned    off;
    int         c;

    if( hndl->line_ptr == hndl->line_buf ) {
        off = ( hndl->file_off >= BACKUP ) ? BACKUP : hndl->file_off;
        if( !get_block( hndl, hndl->file_off - off ) )
            return( -1 );
        hndl->line_ptr = hndl->line_buf + off;
        if( hndl->line_ptr == hndl->line_buf ) {    /* at start of file */
            hndl->line_ptr--;
            return( SM_LF );
        }
    }
    c = *--(hndl->line_ptr);
    return( c );
}


static int prev_src_line( browser *hndl )
{
    int         c;

    if( prev_src_chr( hndl ) == -1 )
        return( 0 );
    do {
        c = prev_src_chr( hndl );
        if( c == -1 ) {
            return( 0 );
        }
    } while( c != SM_LF );
    hndl->cur_line--;
    hndl->line_ptr++;
    hndl->cur_line_ptr = hndl->line_ptr;
    return( 1 );
}


int FCurrLine( browser *hndl )
{
    return( hndl->cur_line );
}


size_t FReadLine( browser *hndl, int line, int off, char *buff, size_t buff_len )
{
    int         i;
    char        *ptr;
    int         tab_pos;
    int         ch;
    int         tab;

    i = hndl->cur_line - line;
    if( line - 1 < i ) { /* faster to seek to start and go forward */
        if( hndl->file_off != 0 )
            get_block( hndl, 0 );
        hndl->cur_line_ptr = hndl->line_buf;
        hndl->cur_line = 1;
        i = 1 - line;
    }
    hndl->line_ptr = hndl->cur_line_ptr;
    if( i < 0 ) {
        do {
            if( !next_src_line( hndl ) ) {
                return( FREADLINE_ERROR );
            }
        } while( ++i != 0 );
    } else if( i > 0 ) {
        do {
            if( !prev_src_line( hndl ) ) {
                return( FREADLINE_ERROR );
            }
        } while( --i != 0 );
    }
    if( buff_len == 0 )
        return( 0 );
    ptr = buff;
    i = 0;
    do {
        do {
            ch = next_src_chr( hndl );
        } while( ch == SM_CR );
        if( ch == -1 ) {
            if( i == 0 ) {
                return( FREADLINE_ERROR );
            } else {
                ch = SM_LF;
            }
        }
        if( ch == SM_LF ) {
            break;
        } else if( ch == SM_TAB ) {
            tab = SMTabIntervalGet();
            if( tab != 0 ) {
                tab_pos = ( i + tab ) - ( ( i + tab ) % tab );
            } else {
                tab_pos = i;
            }
            for( ;; ) {
                if( i >= tab_pos )
                    break;
                if( i >= off ) {
                    *ptr++ = ' ';
                    if( --buff_len == 0 ) {
                        break;
                    }
                }
                ++i;
            }
        } else {
            if( i >= off ) {
                *ptr++ = (char)ch;
                --buff_len;
            }
            ++i;
        }
    } while( buff_len > 0 );
    while( ch != SM_LF ) {
        ch = next_src_chr( hndl );
        if( ch == -1 ) {
            return( ptr - buff );
        }
    }
    hndl->cur_line++;
    hndl->cur_line_ptr = hndl->line_ptr;
    return( ptr - buff );
}
