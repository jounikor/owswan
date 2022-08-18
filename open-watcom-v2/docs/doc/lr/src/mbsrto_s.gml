.func begin mbsrtowcs_s
.func2 _fmbsrtowcs_s WATCOM
.func end
.synop begin
#define __STDC_WANT_LIB_EXT1__  1
#include <wchar.h>
errno_t mbsrtowcs_s( size_t * restrict retval,
                     wchar_t * restrict dst, rsize_t dstmax,
                     const char ** restrict src, rsize_t len,
                     mbstate_t * restrict ps);
.ixfunc2 '&Multibyte' mbsrtowcs_s
.if &farfnc ne 0 .do begin
errno_t _fmbsrtowcs_s( size_t __far * restrict retval,
                       wchar_t __far * restrict dst, rsize_t dstmax,
                       const char __far * __far * restrict src, rsize_t len,
                       mbstate_t __far * restrict ps);
.ixfunc2 '&Multibyte' _fmbsrtowcs_s
.do end
.synop end
.*
.rtconst begin
None of
.arg retval
.ct ,
.arg src
.ct ,
.arg *src
.ct , or
.arg ps
shall be null pointers. If
.arg dst
is not a null pointer, then neither
.arg len
nor
.arg dstmax
shall be greater than
.kw RSIZE_MAX
.period
If
.arg dst
is a null
pointer, then
.arg dstmax
shall equal zero. If
.arg dst
is not a null pointer, then
.arg dstmax
shall not equal zero. If
.arg dst
is not a null pointer and
.arg len
is not less than
.arg dstmax
.ct , then a null character shall occur within the first
.arg dstmax
multibyte characters of the array pointed to by
.arg *src
.period
.np
If there is a runtime-constraint violation, then
.id &funcb.
does the following.
.im _mbsret6
If
.arg dst
is not a null pointer and
.arg dstmax
is greater than zero and less than
.kw RSIZE_MAX
.ct , then
.id &funcb.
sets
.arg dst[0]
to the null wide character.
.rtconst end
.*
.desc begin
The
.id &funcb.
function converts a sequence of multibyte characters that begins
in the conversion state described by the object pointed to by
.arg ps
.ct , from the array indirectly pointed to by
.arg src
into a sequence of corresponding wide characters. If
.arg dst
is not a null pointer, the converted characters are stored into the array
pointed to by
.arg dst
.period
Conversion continues up to and including a terminating null character,
which is also stored.
.np
Conversion stops earlier in two cases: when a sequence of bytes is encountered
that does not form a valid multibyte character, or (if
.arg dst
is not a null pointer) when
.arg len
wide characters have been stored into the array pointed to by
.arg dst
.period
If
.arg dst
is not a null pointer and no null wide character was stored
into the array pointed to by
.arg dst
.ct , then
.arg dst[len]
is set to the null wide character. Each conversion takes place as if by a call
to the
.reffunc mbrtowc
function.
.np
If
.arg dst
is not a null pointer, the pointer object pointed to by
.arg src
is assigned either a null pointer (if conversion stopped due to reaching a
terminating null character) or the address
just past the last multibyte character converted (if any). If conversion
stopped due to reaching a terminating null character and if
.arg dst
is not a null pointer, the resulting state described is the initial
conversion state.
.np
Regardless of whether
.arg dst
is or is not a null pointer, if the input conversion encounters a
sequence of bytes that do not form
.im _mbsret5
Otherwise, the
.id &funcb.
function stores into
.arg *retval
the number of multibyte characters successfully converted, not including the
terminating null character (if any).
.np
All elements following the terminating null wide character (if any) written by
.id &funcb.
in the array of
.arg dstmax
wide characters pointed to by
.arg dst
take unspecified values when
.id &funcb.
returns.
.np
If copying takes place between objects that overlap, the objects take on unspecified
values.
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
.exmp begin
#define __STDC_WANT_LIB_EXT1__  1
#include <stdio.h>
#include <wchar.h>
#include <mbctype.h>
#include <errno.h>

const char chars[] = {
    ' ',
    '.',
    '1',
    'A',
    0x81,0x40, /* double-byte space */
    0x82,0x60, /* double-byte A */
    0x82,0xA6, /* double-byte Hiragana */
    0x83,0x42, /* double-byte Katakana */
    0xA1,      /* single-byte Katakana punctuation */
    0xA6,      /* single-byte Katakana alphabetic */
    0xDF,      /* single-byte Katakana alphabetic */
    0xE0,0xA1, /* double-byte Kanji */
    0x00
};
.exmp break
int main()
{
    int         i;
    size_t      retval;
    const char  *src;
    wchar_t     wc[50];
    mbstate_t   pstate;
    errno_t     rc;

    _setmbcp( 932 );
    src = chars;
    rc = mbsrtowcs( &retval, wc, 50, &src, sizeof(chars), &pstate );
    if( rc != 0 ) {
        printf( "Error in multibyte character string\n" );
    } else {
        for( i = 0; i < retval; i++ ) {
            printf( "%#6.4x\n", wc[i] );
        }
    }
    return( 0 );
}
.exmp end
.class TR 24731
.system
