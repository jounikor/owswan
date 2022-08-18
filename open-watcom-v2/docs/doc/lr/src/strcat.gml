.func strcat _fstrcat wcscat _mbscat _fmbscat
.synop begin
#include <string.h>
char *strcat( char *dst, const char *src );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Concats' &funcb
.if &farfnc ne 0 .do begin
char __far *_fstrcat( char __far *dst,
                      const char __far *src );
.ixfunc2 '&String' &ffunc
.ixfunc2 '&Concats' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wcscat( wchar_t *dst, const wchar_t *src );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Concats' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.if &'length(&mfunc.) ne 0 .do begin
#include <mbstring.h>
unsigned char *_mbscat( unsigned char *dst,
                  const unsigned char *src );
.ixfunc2 '&String' &mfunc
.ixfunc2 '&Concats' &mfunc
.do end
.if &'length(&fmfunc.) ne 0 .do begin
.ixfunc2 '&Multibyte' &mfunc
unsigned char __far *_fmbscat( unsigned char __far *dst,
                         const unsigned char __far *src );
.ixfunc2 '&String' &fmfunc
.ixfunc2 '&Concats' &fmfunc
.ixfunc2 '&Multibyte' &fmfunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function appends a copy of the string pointed to
by
.arg src
(including the terminating null character)
to the end of the string pointed to by
.arg dst
.period
The first character of
.arg src
overwrites the null character at the end of
.arg dst
.period
.farfunc &ffunc. &funcb.
.widefunc &wfunc. &funcb.
.mbcsfunc &mfunc. &funcb.
.farfunc &fmfunc. &mfunc.
.desc end
.return begin
The value of
.arg dst
is returned.
.return end
.see begin
.seelist strcat strncat strcat_s strncat_s
.see end
.exmp begin
#include <stdio.h>
#include <string.h>

void main()
{
    char buffer[80];
.exmp break
    strcpy( buffer, "Hello " );
    strcat( buffer, "world" );
    printf( "%s\n", buffer );
}
.exmp output
Hello world
.exmp end
.class ISO C
.system
