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


#include "variety.h"
#include <stdio.h>
#if defined( __OS2__ )
    #define INCL_WIN
    #include <wos2.h>
#else
    #define INCLUDE_COMMDLG_H
    #include <wwindows.h>
#endif
#include "win.h"

#if defined( __WINDOWS__ ) && defined( _M_I86 )
#pragma library("commdlg");
#endif


/*
 * createNewEntry - build a new line number entry
 */
static LPLDATA createNewEntry( LPWDATA w )
{
    LPLDATA             ld;
#ifdef _MBCS
    LPBYTE              p;

    p = FAR_mbsninc( (LPBYTE)w->tmpbuff->data, w->buffoff );
    *p = '\0';
    ld = FARmalloc( sizeof( line_data ) + FAR_mbsnbcnt( (LPBYTE)w->tmpbuff->data, w->buffoff ) );
#else
    w->tmpbuff->data[w->buffoff] = 0;
    ld = FARmalloc( sizeof( line_data ) + w->buffoff );
#endif
    if( ld == NULL )
        _OutOfMemoryExit();
    ld->next = NULL;
    w->buffoff = 0;
    FARstrcpy( ld->data, w->tmpbuff->data );
    if( w->LineTail == NULL ) {
        w->LineHead = w->LineTail = ld;
        ld->prev = NULL;
    } else {
        w->LineTail->next = ld;
        ld->prev = w->LineTail;
        w->LineTail = ld;
    }
    return( ld );

} /* createNewEntry */

/*
 * incrementLastLineNumber - guess!
 */
static void incrementLastLineNumber( LPWDATA w )
{
    int diff;

    w->LastLineNumber++;
    if( w->LastLineNumber > _AutoClearLines ) {
        _FreeAllLines( w );
        return;
    }
    diff = w->LastLineNumber - w->TopLineNumber;
    if( diff < w->height ) {
        w->CurrentLineNumber++;
        if( diff == w->height - 1 ) {
            w->TopLineNumber++;
            _ShiftWindow( w, 1 );
            /* We must clear the bottom line which NT cannot do when it
             * Scrolls up near the bottom, in this case this is the
             * simplest thing to do.
             */
            _DisplayLineInWindow( w, w->height - 1, (LPSTR)" " );
        }
    }
    _PositionScrollThumb( w );

} /* incrementLastLineNumber */

/*
 * replaceTail - replace tail line data
 */
static void replaceTail( LPWDATA w )
{
    LPLDATA             prev;

#ifdef _MBCS
    *(FAR_mbsninc( (LPBYTE)w->tmpbuff->data, w->buffoff )) = '\0';
#else
    w->tmpbuff->data[w->buffoff] = 0;
#endif

    prev = w->LineTail->prev;
    FARfree( w->LineTail );

#ifdef _MBCS
    w->LineTail = FARmalloc( sizeof( line_data ) + w->buffoff * MB_CUR_MAX );
#else
    w->LineTail = FARmalloc( sizeof( line_data ) + w->buffoff );
#endif
    if( w->LineTail == NULL )
        _OutOfMemoryExit();

    FARstrcpy( w->LineTail->data, w->tmpbuff->data );
    w->LineTail->next = NULL;
    w->LineTail->prev = prev;
    if( prev != NULL ) {
        prev->next = w->LineTail;
    } else {
        w->LineHead = w->LineTail;
    }
    _DisplayLineInWindow( w, w->LastLineNumber - w->TopLineNumber + 1, w->LineTail->data );

} /* replaceTail */

/*
 * addBuff - add current working buffer to line data structures
 */
static void addBuff( LPWDATA w )
{
    LPLDATA     ld;

    ld = createNewEntry( w );
    ld->has_cr = TRUE;
    _DisplayLineInWindow( w, w->LastLineNumber - w->TopLineNumber + 1, ld->data );

} /* addBuff */

/*
 * updateBuff - update current line with tmpbuff
 */
static void updateBuff( LPWDATA w )
{
    int oldbuff;

    if( w->LineHead == NULL || w->LineTail->has_cr ) {
        oldbuff = w->buffoff;
        addBuff( w );
        w->buffoff = oldbuff;
    } else {
        replaceTail( w );
    }
    w->LineTail->has_cr = FALSE;

} /* updateBuff */

/*
 * newLine - add a new line to the bottom
 */
static void newLine( LPWDATA w )
{
    if( w->LineTail != NULL && !w->LineTail->has_cr ) {
        replaceTail( w );
        w->buffoff = 0;
        w->LineTail->has_cr = TRUE;
    } else {
        addBuff( w );
    }
    w->lineinprogress = FALSE;
    incrementLastLineNumber( w );

} /* newLine */

/*
 * _AddLine - add a line to the lines data structures
 */
void _AddLine( LPWDATA w, const void *in_data, unsigned len )
{
    int                     i;
    BOOL                    hadbreak;
    HWND                    hwnd;
    int                     tabcnt = 0;
    int                     nlcnt = 0;
    int                     curbufoff = 0;
#ifdef _MBCS
    const unsigned char     *data;
    static unsigned char    leadByte;
    static int              leadByteWaiting;
    unsigned char           ch[MB_CUR_MAX + 1];
    LPBYTE                  p;
#else
    const char              *data;
    char                    ch;
#endif

#ifdef _MBCS
    data = (const unsigned char *)in_data;
#else
    data = (const char *)in_data;
#endif
    hwnd = w->hwnd;

    _AccessWinLines();
    if( w->LineTail != NULL && !w->LineTail->has_cr ) {
        FARstrcpy( w->tmpbuff->data, w->LineTail->data );
#ifdef _MBCS
        curbufoff = FAR_mbslen( (LPBYTE)w->tmpbuff->data );
#else
        curbufoff = FARstrlen( w->tmpbuff->data );
#endif
        if( w->buffoff < curbufoff ) {
            w->buffoff = curbufoff;
        }
    }
    if( w->no_advance ) {
        curbufoff = 0;
    }
    for( i = 0; i < len; i++ ) {
        w->no_advance = FALSE;
        do {
            hadbreak = FALSE;
#ifdef _MBCS                        /* MBCS */
            if( tabcnt ) {
                _mbccpy( ch, (unsigned char *)" " );     /* copy the character */
                ch[_mbclen( ch )] = '\0';           /* terminate char with NULL */
                tabcnt--;
            } else if( nlcnt ) {
                _mbccpy( ch, (unsigned char *)"\n" );    /* copy the character */
                ch[_mbclen( ch )] = '\0';           /* terminate char with NULL */
                nlcnt--;
            } else {
                if( !leadByteWaiting ) {
                    if( _ismbblead( *data ) ) {
                        leadByteWaiting = 1;
                        leadByte = *data;
                        ch[0] = '\0';
                    } else {
                        ch[0] = *data;
                        ch[1] = '\0';
                    }
                } else {
                    leadByteWaiting = 0;
                    ch[0] = leadByte;
                    ch[1] = *data;
                    ch[2] = '\0';
                }
                data++;
            }

            if( !_mbccmp( ch, (unsigned char *)"\t" ) ) {
                tabcnt = TAB( curbufoff + 1 );
                continue;
            } else if( !_mbccmp( ch, (unsigned char *)"\f" ) ) {
                nlcnt = w->height;
                continue;
            } else if( !_mbccmp( ch, (unsigned char *)"\r" ) ) {
                curbufoff = 0;
                w->no_advance = TRUE;
                w->tmpbuff->has_cr = TRUE;
                continue;
            } else if( !_mbccmp( ch, (unsigned char *)"\n" ) ) {
                hadbreak = TRUE;
                newLine( w );
                curbufoff = w->buffoff;
            } else if( !_mbccmp( ch, (unsigned char *)"\b" ) ) {
                if( curbufoff > 0 ) {
                    p = FAR_mbsninc( (LPBYTE)w->tmpbuff->data, curbufoff - 1 );
                    if( _ismbblead( *p ) ) {
                        *p = ' ';           /* stomp lead byte */
                        /* char split into 2; don't change curbufoff */
                    } else {
                        curbufoff--;        /* back up one character */
                    }
                }
            } else if( ch[0] != '\0' ) {
                FAR_mbccpy( FAR_mbsninc( (LPBYTE)w->tmpbuff->data, curbufoff ), ch );
                curbufoff++;
                if( w->buffoff < curbufoff ) {
                    w->buffoff = curbufoff;
                }
                if( TOOWIDE( w->buffoff, w ) ) {
                    hadbreak = TRUE;
                    newLine( w );
                    curbufoff = w->buffoff;
                }
            }
#else                               /* SBCS */
            if( tabcnt ) {
                ch = ' ';
                tabcnt--;
            } else if( nlcnt ) {
                ch = '\n';
                nlcnt--;
            } else {
                ch = data[i];
            }

            if( ch == '\t' ) {
                tabcnt = TAB( curbufoff + 1 );
                continue;
            } else if( ch == '\f' ) {
                nlcnt = w->height;
                continue;
            } else if( ch == '\r' ) {
                curbufoff = 0;
                w->no_advance = TRUE;
                w->tmpbuff->has_cr = TRUE;
                continue;
            } else if( ch == '\n' ) {
                hadbreak = TRUE;
                newLine( w );
                curbufoff = w->buffoff;
            } else if( ch == '\b' ) {
                if( curbufoff > 0 ) {
                    curbufoff--;
                }
            } else {
                w->tmpbuff->data[curbufoff++] = ch;
                if( w->buffoff < curbufoff ) {
                    w->buffoff = curbufoff;
                }
                if( TOOWIDE( w->buffoff, w ) ) {
                    hadbreak = TRUE;
                    newLine( w );
                    curbufoff = w->buffoff;
                }
            }
#endif
        } while( tabcnt || nlcnt );
    }
    if( !hadbreak ) {
        updateBuff( w );
    }
    _ReleaseWinLines();

} /* _AddLine */

/*
 * _UpdateInputLine - add data to current line; return number of chars
 *                   on next line if line break was forced
 */
int _UpdateInputLine( LPWDATA w, char *line, unsigned len, BOOL force_add )
{
    int         i,j;
    BOOL        justnew=FALSE;
    BOOL        wassplit=FALSE;

    _AccessWinLines();
    w->lineinprogress = TRUE;
    j = w->buffoff;
    for( i = 0; i < len; i++ ) {
        justnew = FALSE;
        if( TOOWIDE( j, w ) ) {
#ifdef _MBCS
            FAR_mbccpy( FAR_mbsninc( (LPBYTE)w->tmpbuff->data, j ), (LPBYTE)"" );
#else
            w->tmpbuff->data[j] = 0;
#endif
            w->buffoff = j;
            newLine( w );
            j = 0;
            justnew = TRUE;
            wassplit = TRUE;
        }
#ifdef _MBCS
        FAR_mbccpy( FAR_mbsninc( (LPBYTE)w->tmpbuff->data, j ), FAR_mbsninc( (LPBYTE)line, i ) );
#else
        w->tmpbuff->data[j] = line[i];
#endif
        j++;
    }
#ifdef _MBCS
    FAR_mbccpy( FAR_mbsninc( (LPBYTE)w->tmpbuff->data, j ), (LPBYTE)"" );
#else
    w->tmpbuff->data[j] = 0;
#endif

    if( force_add && !justnew ) {
        w->buffoff = j;
        newLine( w );
    } else {
        _DisplayLineInWindow( w, w->LastLineNumber - w->TopLineNumber + 1, w->tmpbuff->data );
        if( !wassplit ) {
            j = -1;
        }
    }
    _ReleaseWinLines();

    return( j );
} /* _UpdateInputLine */


#define MAXLNE 32768L
/*
 * _PositionScrollThumb
*/
void _PositionScrollThumb( LPWDATA w )
{
    DWORD       c;
    WORD        curr, end;
    DWORD       ll;

    c = w->TopLineNumber;
    ll = _GetLastLineNumber( w );
    if( ll > MAXLNE ) {
        end = MAXLNE;
        curr = (WORD)(((DWORD) c * MAXLNE)/ll);
    } else {
        end = ll;
        curr = c;
    }
#if defined( __OS2__ )
    if( end > w->height ) {
        end -= w->height;
    }
    WinSendMsg( WinWindowFromID( WinQueryWindow( w->hwnd, QW_PARENT ),
                                FID_VERTSCROLL ),
                SBM_SETSCROLLBAR, MPFROMSHORT( curr ),
                MPFROM2SHORT( 1, end ) );
#else
    SetScrollRange( w->hwnd, SB_VERT, 1, end, FALSE );
    SetScrollPos( w->hwnd, SB_VERT, curr, TRUE );
#endif
} /* _PositionScrollThumb */


/*
 * _GetLineFromThumbPosition - given a thumb position, find line in file
 */
DWORD _GetLineFromThumbPosition( LPWDATA w, WORD n )
{
    DWORD       new;
    DWORD       ll;

    ll = _GetLastLineNumber( w );

    if( ll > MAXLNE ) {
        new = ((DWORD)n*ll)/MAXLNE;
    } else {
        new = (DWORD) n;
    }
    return( new );

} /* _GetLineFromThumbPosition */


/*
 * _GetLineDataPointer - get line data pointer
 */
LPLDATA _GetLineDataPointer( LPWDATA w, DWORD line )
{
    DWORD       cnt;
    LPLDATA     ld;

    _AccessWinLines();
    if( line == w->LastLineNumber ) {
        if( w->lineinprogress ) {
            _ReleaseWinLines();
            return( w->tmpbuff );
        }
    }

    ld = w->LineHead;
    for( cnt = 1; cnt < line && ld != NULL; ++cnt ) {
        ld = ld->next;
    }
    _ReleaseWinLines();
    return( ld );

} /* _GetLineDataPointer */

/*
 * _FreeAllLines - as it sounds!
 */
void _FreeAllLines( LPWDATA w )
{
    LPLDATA     ld;
    LPLDATA     ld_next;

    _AccessWinLines();
    for( ld = w->LineHead; ld != NULL; ld = ld_next ) {
        ld_next = ld->next;
        FARfree( ld );
    }
    w->LineHead = w->LineTail = NULL;
    w->buffoff = 0;
    w->CurrentLineNumber = w->LastLineNumber = w->TopLineNumber = 1;
    _PositionScrollThumb( w );
    _ReleaseWinLines();

} /* _FreeAllLines */

#if !defined( __OS2__ )

static char filterFiles[] = "Result Files (*.TXT)" \
                            "\0" \
                            "*.TXT" \
                            "\0\0";

/*
 * _SaveAllLines - save all lines to a file
 */
void _SaveAllLines( LPWDATA w )
{
    char                fname[_MAX_PATH];
    OPENFILENAME        of;
    BOOL                rc;
    FILE                *f;
    LPLDATA             ld;

    fname[0] = 0;
    memset( &of, 0, sizeof( OPENFILENAME ) );
    of.lStructSize = sizeof( OPENFILENAME );
    of.hwndOwner = _MainWindow;
    of.lpstrFilter = filterFiles;
    of.nFilterIndex = 1L;
    of.lpstrFile = fname;
    of.nMaxFile = _MAX_PATH;
    of.lpstrTitle = "Save File Name Selection";
    of.Flags = OFN_HIDEREADONLY;
    rc = GetSaveFileName( &of );

    if( !rc ) {
        return;
    }

    /*
     * save lines
     */
    _AccessWinLines();
    f = fopen( fname, "w" );
    if( f == NULL ) {
        MessageBox( (HWND)NULL, fname,"Error opening file", MB_OK );
        return;
    }
    for( ld = w->LineHead; ld != NULL; ld = ld->next ) {
#if defined( _M_I86 )
        fprintf( f,"%Fs\n", ld->data );
#else
        fprintf( f,"%s\n", ld->data );
#endif
    }
    fclose( f );
    _ReleaseWinLines();
    MessageBox( (HWND)NULL, fname, "Data saved to file", MB_OK );

} /* _SaveAllLines */

#define MAX_BYTES       0xfffeL
/*
 * _CopyAllLines - copy lines to clipboard
 */
void _CopyAllLines( LPWDATA w )
{
    LPLDATA     ld;
    DWORD       total;
    unsigned    len;
    HANDLE      data;
    unsigned    slen;
#if defined(__NT__)
    char        *ptr;
#else
    char        _WCFAR *ptr;
#endif

    /*
     * get number of bytes
     */
    _AccessWinLines();
    total = 0;
    for( ld = w->LineHead; ld != NULL; ld = ld->next ) {
        total += FARstrlen( ld->data ) + 2;
    }
    if( total > MAX_BYTES ) {
        len = (unsigned)MAX_BYTES;
    } else {
        len = total;
    }

    /*
     * get memory block
     */
    data = GlobalAlloc( GMEM_MOVEABLE, len + 1 );
    if( data == NULL ) {
        MessageBox( (HWND)NULL, "Out of Memory","Copy to Clipboard", MB_OK );
        _ReleaseWinLines();
        return;
    }
    ptr = GetPtrGlobalLock( data );

    /*
     * copy data into block
     */
    total = 0;
    for( ld = w->LineHead; ld != NULL; ld = ld->next ) {
        slen = FARstrlen( ld->data ) + 2;
        if( total + slen > MAX_BYTES )
            break;
#if defined(__NT__)
        memcpy( &ptr[total], ld->data, slen - 2 );
#else
        _fmemcpy( &ptr[total], ld->data, slen - 2 );
#endif
        ptr[total + slen - 2] = 0x0d;
        ptr[total + slen - 1] = 0x0a;
        total += slen;
    }
    ptr[total] = 0;
    GlobalUnlock( data );

    /*
     * dump data to the clipboard
     */
    if( OpenClipboard( w->hwnd ) ) {
        EmptyClipboard();
        SetClipboardData( CF_TEXT, data );
        CloseClipboard();
    }
    _ReleaseWinLines();

} /* _CopyAllLines */
#endif

/*
 * _GetLastLineNumber
 */
DWORD _GetLastLineNumber( LPWDATA w )
{
    DWORD       ll;

    ll = w->LastLineNumber;
    if( w->lineinprogress )
        ll++;
    return( ll );

} /* _GetLastLineNumber */
