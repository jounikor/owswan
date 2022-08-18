.func strrchr _fstrrchr wcsrchr _mbsrchr _fmbsrchr
.synop begin
#include <string.h>
char *strrchr( const char *s, int c );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Search' &funcb
.if &farfnc ne 0 .do begin
char __far *_fstrrchr( const char __far *s, int c );
.ixfunc2 '&String' &ffunc
.ixfunc2 '&Search' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wcsrchr( const wchar_t *s, wint_t c );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Search' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.if &'length(&mfunc.) ne 0 .do begin
#include <mbstring.h>
unsigned char *_mbsrchr( const unsigned char *s,
                         unsigned int c );
.ixfunc2 '&String' &mfunc
.ixfunc2 '&Search' &mfunc
.ixfunc2 '&Multibyte' &mfunc
.do end
.if &'length(&fmfunc.) ne 0 .do begin
unsigned char __far *_fmbsrchr(
                        const unsigned char __far *s,
                        unsigned int c );
.ixfunc2 '&String' &fmfunc
.ixfunc2 '&Search' &fmfunc
.ixfunc2 '&Multibyte' &fmfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function locates the last occurrence of
.arg c
(converted to a char) in the string pointed to by
.arg s
.period
The terminating null character is considered to be part of the string.
.farfunc &ffunc. &funcb.
.widefunc &wfunc. &funcb.
.mbcsfunc &mfunc. &funcb.
.farfunc &fmfunc. &mfunc.
.desc end
.return begin
The
.id &funcb.
function returns a pointer to the located character,
or a NULL pointer if the character does not occur in the string.
.return end
.see begin
.seelist strrchr strchr strpbrk
.see end
.exmp begin
#include <stdio.h>
#include <string.h>

void main()
{
    printf( "%s\n", strrchr( "abcdeaaklmn", 'a' ) );
    if( strrchr( "abcdeaaklmn", 'x' ) == NULL )
        printf( "NULL\n" );
}
.exmp output
aklmn
NULL
.exmp end
.class ISO C
.system
