#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "edit.h"
#include "win1632.h"

#define FONT_DATA       1L
#define FONT_SIZES      2L

static LPFINFO FontHead,FontTail;
static LPFINFO CurrFont;

static HFONT CFont;

/*
 * MakeFont - make the current font
 */
static void MakeFont( HWND hwnd, int index, int size )
{
LPFINFO tmp;

    if( CFont != NULL ) DeleteObject( CFont );

    tmp = FontHead;
    while( tmp->index != index ) tmp = tmp->next;

    CFont = CreateFont(
        tmp->sizes[ size ],
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        tmp->charset,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        tmp->pitch_family,
        tmp->name );

    if( hwnd != NULL ) {
        SendDlgItemMessage( hwnd, FONT_SAMPLE, WM_SETFONT, (UINT)CFont, TRUE );
    }

} /* MakeFont */


/*
 * GetFont - get a new font
 */
BOOL _EXPORT FAR PASCAL GetFont( HWND hwnd, unsigned msg, UINT wparam,
                                LONG lparam )
{

    int         i;
    int         sel,sel2;
    char        str[128];
    LPFINFO     tmp;
    WORD        cmd;

    lparam = lparam;    //shut up the compiler for the Win32 version
    switch( msg ) {
    case WM_INITDIALOG:
        tmp = FontHead;
        while( tmp != NULL ) {
            SendDlgItemMessage( hwnd, FONT_NAME, LB_ADDSTRING, 0,
                                (LONG) (LPSTR) tmp->name );
            SendDlgItemMessage( hwnd, FONT_NAME, LB_SETCURSEL, 0, 0L );
            tmp = tmp->next;
        }
        for( i=0; i < FontHead->size_count; i++ ) {
            sprintf( str, "%d", FontHead->sizes[i]);
            SendDlgItemMessage( hwnd, FONT_SIZE, LB_ADDSTRING,
                0, (LONG) (LPSTR) str );
            SendDlgItemMessage( hwnd, FONT_SIZE, LB_SETCURSEL, 0, 0L);
        }
        return( TRUE );
        break;

    case WM_COMMAND:
        cmd = LOWORD( wparam );
        if( ( cmd == FONT_NAME || cmd == FONT_SIZE ) &&
            GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_DBLCLK ) {
        }
        switch( cmd ) {
        case IDOK:
            sel = SendDlgItemMessage( hwnd, FONT_NAME, LB_GETCURSEL, 0, 0L );
            sel2 = SendDlgItemMessage( hwnd, FONT_SIZE, LB_GETCURSEL, 0, 0L );
            if( sel == LB_ERR  || sel2 == LB_ERR ) {
                EndDialog( hwnd, 0 );
            } else {
                MakeFont( NULL, sel, sel2 );
                EndDialog( hwnd, 1 );
            }
            break;

        case IDCANCEL:
            EndDialog( hwnd, 0 );
            break;

        case FONT_SIZE:
            if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCHANGE ) {
                sel = SendDlgItemMessage( hwnd, FONT_NAME, LB_GETCURSEL, 0, 0L );
                if( sel == LB_ERR) break;
                sel2 = SendDlgItemMessage( hwnd, FONT_SIZE, LB_GETCURSEL, 0, 0L );
                if( sel2 == LB_ERR ) break;
                MakeFont( hwnd, sel, sel2 );
            }
            break;
        case FONT_NAME:
            if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_SELCHANGE ) {
                sel = SendDlgItemMessage( hwnd, FONT_NAME, LB_GETCURSEL, 0, 0L );
                if( sel == LB_ERR ) break;
                tmp = FontHead;
                while( tmp->index != sel ) tmp = tmp->next;
                SendDlgItemMessage( hwnd, FONT_SIZE, LB_RESETCONTENT, 0, 0L );
                for( i=0; i< tmp->size_count; i++ ) {
                    sprintf( str, "%d", tmp->sizes[i] );
                    SendDlgItemMessage( hwnd, FONT_SIZE, LB_ADDSTRING,
                                0, (LONG) (LPSTR) str );
                    SendDlgItemMessage( hwnd, FONT_SIZE, LB_SETCURSEL, 0, 0L );
                }
                MakeFont( hwnd, sel, 0 );
            }
            break;
        }

    }
    return (FALSE);

} /* GetFont */

/*
 * FontSelect - select a new font
 */
void FontSelect( LPEDATA ed )
{
FARPROC fp;

    if( CFont != NULL ) DeleteObject( CFont );
    CFont = NULL;

    fp = MakeProcInstance( (FARPROC)GetFont, ed->inst );
    if( DialogBox( ed->inst, "GetFont", ed->hwnd, (DLGPROC)fp ) ) {
        if( ed->font != NULL ) DeleteObject( ed->font );
        ed->font = CFont;
        SendMessage( ed->editwnd, WM_SETFONT, (UINT)ed->font, TRUE );
    }
    FreeProcInstance( fp );

} /* FontSelect */

/*
 * EnumFontsProc - get all fonts
 */
int _EXPORT FAR PASCAL EnumFontsProc( LPLOGFONT logfont,
                        LPTEXTMETRIC textmetric, UINT fonttype, LPSTR data )
{
LOGFONT         far *farfont;
LPFINFO         curr;
short           _FAR *newsize;

    fonttype = fonttype;        /* shut compiler up */
    textmetric = textmetric;

    switch( (WORD) data ) {
    case FONT_DATA:
        curr = MemAlloc( sizeof( font_info ) );
        if( curr == NULL ) return( FALSE );
        farfont = MK_FP32( logfont );
        curr->charset = farfont->lfCharSet;
        curr->pitch_family = farfont->lfPitchAndFamily;
    #ifdef __NT__
        strcpy( curr->name, farfont->lfFaceName );
    #else
        _fstrcpy( curr->name, farfont->lfFaceName );
    #endif
        if( FontTail == NULL ) {
            FontHead = FontTail = curr;
            curr->index = 0;
        } else {
            FontTail->next = curr;
            curr->index = FontTail->index + 1;
            FontTail = curr;
        }
        return( curr->index + 1 );

    case FONT_SIZES:
        newsize = MemRealloc( CurrFont->sizes,
                sizeof( short ) * (CurrFont->size_count+1 ) );
        if( newsize == NULL ) return( 0 );
        CurrFont->sizes = newsize;
        farfont = MK_FP32( logfont );
        CurrFont->sizes[ CurrFont->size_count ] = farfont->lfHeight;
        CurrFont->size_count++;
        return( CurrFont->size_count );
    default:
        return( 0 );
        break;
    }

} /* EnumFontsProc */

/*
 * GetAllFonts - get all fonts available
 */
void GetAllFonts( LPEDATA ed )
{
    HDC                 hdc;
    FARPROC             fp;
    LPFINFO             tmp,next;

    /*
     * erase old list
     */
    tmp = FontHead;
    while( tmp != NULL ) {
        next = tmp->next;
        MemFree( tmp );
        tmp = next;
    }

    fp = MakeProcInstance( (FARPROC)EnumFontsProc, ed->inst );
    hdc = GetDC( ed->hwnd );
    EnumFontFamilies( hdc, (LPSTR) 0, (FONTENUMPROC)fp,
                      (LPARAM) FONT_DATA );

    /*
     * get all sizes
     */
    CurrFont = FontHead;
    while( CurrFont != NULL ) {
        CurrFont->size_count = 0;
        EnumFontFamilies( hdc, CurrFont->name, (FONTENUMPROC)fp,
                          (LPARAM) FONT_SIZES );
        CurrFont = CurrFont->next;
    }

    ReleaseDC( ed->hwnd, hdc );
    FreeProcInstance( fp );

} /* GetAllFonts */

