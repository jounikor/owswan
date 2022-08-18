.func strchr _fstrchr wcschr _mbschr _fmbschr
.synop begin
#include <string.h>
char *strchr( const char *s, int c );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Search' &funcb
.if &farfnc ne 0 .do begin
char __far *_fstrchr( const char __far *s, int c );
.ixfunc2 '&String' &ffunc
.ixfunc2 '&Search' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wcschr( const wchar_t *s, wint_t c );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Search' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.if &'length(&mfunc.) ne 0 .do begin
#include <mbstring.h>
unsigned char *_mbschr( const unsigned char *s,
                        unsigned int c );
.ixfunc2 '&String' &mfunc
.ixfunc2 '&Search' &mfunc
.ixfunc2 '&Multibyte' &mfunc
.do end
.if &'length(&fmfunc.) ne 0 .do begin
unsigned char __far *_fmbschr(
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
function locates the first occurrence of
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
function returns a pointer to the located character, or
.mono NULL
if the character does not occur in the string.
.return end
.see begin
.seelist strchr memchr strcspn strrchr strspn strstr strtok
.see end
.exmp begin
#include <stdio.h>
#include <string.h>

void main()
  {
    char buffer[80];
    char *where;
.exmp break
    strcpy( buffer, "video x-rays" );
    where = strchr( buffer, 'x' );
    if( where == NULL ) {
        printf( "'x' not found\n" );
    }
  }
.exmp end
.class ISO C
.system
