.func begin wcstombs_s
.func2 _fwcstombs_s WATCOM
.func end
.synop begin
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdlib.h>
errno_t wcstombs_s( size_t * restrict retval,
                    char * restrict dst,
                    rsize_t dstmax,
                    const wchar_t * restrict src,
                    rsize_t len);
.ixfunc2 '&Wide' wcstombs_s
.ixfunc2 '&Multibyte' wcstombs_s
.if &farfnc ne 0 .do begin
errno_t _fwcstombs_s( size_t __far * restrict retval,
                      char __far * restrict dst,
                      rsize_t dstmax,
                      const wchar_t __far * restrict src,
                      rsize_t len);
.ixfunc2 '&Wide' _fwcstombs_s
.ixfunc2 '&Multibyte' _fwcstombs_s
.do end
.synop end
.*
.rtconst begin
Neither
.arg retval
nor
.arg src
shall be a null pointer. If
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
is a null pointer, then
.arg dstmax
shall equal zero. If
.arg dst
is not a null pointer,then
.arg dstmax
shall not equal zero. If
.arg dst
is not a null pointer and
.arg len
is not less than
.arg dstmax
.ct , then the conversion
shall have been stopped (see below) because a terminating null wide character was
reached or because an encoding error occurred.
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
to the null character.
.rtconst end
.*
.desc begin
The
.id &funcb.
function converts a sequence of wide characters from the array
pointed to by
.arg src
into a sequence of corresponding multibyte characters that begins in
the initial shift state. If
.arg dst
is not a null pointer,the converted characters are then stored
into the array pointed to by
.arg dst
.period
Conversion continues up to and including a terminating
null wide character, which is also stored.
.np
Conversion stops earlier in two cases:
.br
when a wide character is reached that does not correspond to a valid multibyte
character;
.br
(if
.arg dst
is not a null pointer) when the next multibyte character would exceed the
limit of
.arg n
total bytes to be stored into the array pointed to by
.arg dst
.period
If the wide
character being converted is the null wide character, then
.arg n
is the lesser of
.arg len
or
.arg dstmax
.period
Otherwise,
.arg n
is the lesser of
.arg len
or
.arg dstmax-1
.period
.np
If the conversion stops without converting a null wide character and
.arg dst
is not a null
pointer, then a null character is stored into the array pointed to by
.arg dst
immediately
following any multibyte characters already stored. Each conversion takes place as if by a
call to the wcrtomb function.
.np
Regardless of whether
.arg dst
is or is not a null pointer, if the input conversion encounters a
wide character that does not correspond to
.im _mbsret4
.period
Otherwise, the
.id &funcb.
function stores into
.arg *retval
the number of bytes in the
resulting multibyte character sequence, not including the terminating null character (if
any).
.np
All elements following the terminating null character (if any) written by &funcb.
in the array of
.arg dstmax
elements pointed to by
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
.seelist wcstombs_s wcstombs mblen mbtowc mbstowcs mbstowcs_s wctomb wctomb_s
.see end
.exmp begin
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
.exmp break
wchar_t wbuffer[] = {
    0x0073,
    0x0074,
    0x0072,
    0x0069,
    0x006e,
    0x0067,
    0x0073,
    0x0074,
    0x0072,
    0x0069,
    0x006e,
    0x0067,
    0x0000
  };
.exmp break
int main()
{
    char    mbsbuffer[50];
    int     i;
    size_t  retval;
    errno_t rc;
.exmp break
    rc = wcstombs_s( &retval, mbsbuffer, 50, wbuffer, sizeof( wbuffer ) );
    if( rc == 0 ) {
        for( i = 0; i < retval; i++ )
            printf( "/%4.4x", wbuffer[i] );
        printf( "\n" );
        mbsbuffer[retval] = '\0';
        printf( "%s(%d)\n", mbsbuffer, retval );
    }
    return( rc );
}
.exmp output
/0073/0074/0072/0069/006e/0067
string(6)
.exmp end
.class TR 24731
.system
