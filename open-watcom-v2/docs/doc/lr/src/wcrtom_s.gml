.func begin wcrtomb_s
.func2  _fwcrtomb_s WATCOM
.func end
.synop begin
#define __STDC_WANT_LIB_EXT1__  1
#include <wchar.h>
errno_t wcrtomb_s( size_t * restrict retval,
                   char * restrict s, rsize_t smax,
                   wchar_t wc, mbstate_t * restrict ps);
.ixfunc2 '&Wide' wcrtomb_s
.ixfunc2 '&Multibyte' wcrtomb_s
.if &farfnc ne 0 .do begin
errno_t _fwcrtomb_s( size_t __far * restrict retval,
                   char __far * restrict s, rsize_t smax,
                   wchar_t wc, mbstate_t __far * restrict ps);
.ixfunc2 '&Wide' _fwcrtomb_s
.ixfunc2 '&Multibyte' _fwcrtomb_s
.do end
.synop end
.*
.rtconst begin
Neither
.arg retval
nor
.arg ps
shall be a null pointer. If
.arg s
is not a null pointer, then
.arg smax
shall not equal zero and shall not be greater than
.kw RSIZE_MAX
.period
If
.arg s
is not a null pointer,
then
.arg smax
shall be not be less than the number of bytes to be stored in the array pointed
to by
.arg s
.period
If
.arg s
is a null pointer, then
.arg smax
shall equal zero.
.np
If there is a runtime-constraint violation, then
.id &funcb.
does the following. If
.arg s
is not a null pointer and
.arg smax
is greater than zero and not greater than
.kw RSIZE_MAX
.ct , then
.id &funcb.
sets
.arg s[0]
to the null character.
.im _mbsret6
.rtconst end
.*
.desc begin
If
.arg s
is a null pointer, the
.id &funcb.
function is equivalent to the call
.br
wcrtomb_s(&retval, buf, sizeof buf, L'\0', ps)
.br
where
.arg retval
and
.arg buf
are internal variables of the appropriate types, and the size of
.arg buf
is greater than MB_CUR_MAX.
.np
If
.arg s
is not a null pointer, the
.id &funcb.
function determines the number of bytes
needed to represent the multibyte character that corresponds to the wide character given
by
.arg wc
(including any shift sequences), and stores the multibyte character representation
in the array whose first element is pointed to by
.arg s
.period
At most
.kw MB_CUR_MAX
bytes are
stored. If
.arg wc
is a null wide character, a null byte is stored, preceded by any shift
sequence needed to restore the initial shift state; the resulting state
described is the initial conversion state.
.np
If
.arg wc
does not correspond to
.im _mbsret5
Otherwise, the
.id &funcb.
function stores into
.arg *retval
the number of bytes (including any shift sequences) stored in the array pointed
to by
.arg s
.period
.farfuncp &ffunc. &funcb.
.desc end
.*
.return begin
.saferet
.return end
.*
.see begin
.im seembc
.see end
.*
.exmp begin
#define __STDC_WANT_LIB_EXT1__  1
#include <stdio.h>
#include <wchar.h>
#include <mbctype.h>
#include <errno.h>

const wchar_t wc[] = {
    0x0020,
    0x002e,
    0x0031,
    0x0041,
    0x3000,     /* double-byte space */
    0xff21,     /* double-byte A */
    0x3048,     /* double-byte Hiragana */
    0x30a3,     /* double-byte Katakana */
    0xff61,     /* single-byte Katakana punctuation */
    0xff66,     /* single-byte Katakana alphabetic */
    0xff9f,     /* single-byte Katakana alphabetic */
    0x720d,     /* double-byte Kanji */
    0x0000
};
.exmp break
#define SIZE sizeof( wc ) / sizeof( wchar_t )
.exmp break
int main()
{
    int         i, j, k;
    char        s[2];
    errno_t     rc;
    size_t      retval;
    mbstate_t   state;

    _setmbcp( 932 );
    j = 1;
    for( i = 0; i < SIZE; i++ ) {
        rc = wcrtomb_s( &retval, s, 2, wc[i], &state );
        if( rc != 0 ) {
          printf( " - illegal wide character\n" );
        } else {
          printf( "%d bytes in character ", retval );
          if ( retval == 0 ) {
              k = 0;
          } else if ( retval == 1 ) {
              k = s[0];
          } else if( retval == 2 ) {
              k = s[0]<<8 | s[1];
          }
          printf( "(%#6.4x->%#6.4x)\n", wc[i], k );
        }
    }
    return( 0 );
}
.exmp output
1 bytes in character (0x0020->0x0020)
1 bytes in character (0x002e->0x002e)
1 bytes in character (0x0031->0x0031)
1 bytes in character (0x0041->0x0041)
2 bytes in character (0x3000->0x8140)
2 bytes in character (0xff21->0x8260)
2 bytes in character (0x3048->0x82a6)
2 bytes in character (0x30a3->0x8342)
1 bytes in character (0xff61->0x00a1)
1 bytes in character (0xff66->0x00a6)
1 bytes in character (0xff9f->0x00df)
2 bytes in character (0x720d->0xe0a1)
1 bytes in character (  0000->0x0069)
.exmp end
.class TR 24731
.system
