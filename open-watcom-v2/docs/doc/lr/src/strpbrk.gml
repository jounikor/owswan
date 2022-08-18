.func strpbrk _fstrpbrk wcspbrk _mbspbrk _fmbspbrk
.synop begin
#include <string.h>
char *strpbrk( const char *s, const char *charset );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Search' &funcb
.if &farfnc ne 0 .do begin
char __far *_fstrpbrk( const char __far *s,
                       const char __far *charset );
.ixfunc2 '&String' &ffunc
.ixfunc2 '&Search' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wcspbrk( const wchar_t *s,
                  const wchar_t *charset );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Search' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.if &'length(&mfunc.) ne 0 .do begin
#include <mbstring.h>
unsigned char *_mbspbrk( const unsigned char *s,
                         const unsigned char *charset );
.ixfunc2 '&String' &mfunc
.ixfunc2 '&Search' &mfunc
.ixfunc2 '&Wide' &mfunc
.do end
.if &'length(&fmfunc.) ne 0 .do begin
unsigned char __far *_fmbspbrk(
                    const unsigned char __far *s,
                    const unsigned char __far *charset );
.ixfunc2 '&String' &fmfunc
.ixfunc2 '&Search' &fmfunc
.ixfunc2 '&Wide' &fmfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function locates the first occurrence in the string pointed
to by
.arg s
of any character from the string pointed to by
.arg charset
.period
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
if no character from
.arg charset
occurs in
.arg s
.period
.return end
.see begin
.seelist strchr strpbrk strrchr strtok
.see end
.exmp begin
#include <stdio.h>
#include <string.h>

void main()
  {
    char *p = "Find all vowels";
.exmp break
    while( p != NULL ) {
      printf( "%s\n", p );
      p = strpbrk( p+1, "aeiouAEIOU" );
    }
  }
.exmp output
Find all vowels
ind all vowels
all vowels
owels
els
.exmp end
.class ISO C
.system
