.func strcpy _fstrcpy wcscpy _mbscpy _fmbscpy
.synop begin
#include <string.h>
char *strcpy( char *dst, const char *src );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Copy' &funcb
.if &farfnc ne 0 .do begin
char __far *_fstrcpy( char __far *dst,
                      const char __far *src );
.ixfunc2 '&String' &ffunc
.ixfunc2 '&Copy' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wcscpy( wchar_t *dst, const wchar_t *src );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Copy' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.if &'length(&mfunc.) ne 0 .do begin
#include <mbstring.h>
int _mbscpy( unsigned char *dst,
             const unsigned char *src );
.ixfunc2 '&String' &mfunc
.ixfunc2 '&Copy' &mfunc
.ixfunc2 '&Multibyte' &mfunc
.do end
.if &'length(&fmfunc.) ne 0 .do begin
int _fmbscpy( unsigned char __far *dst,
              const unsigned char __far *src );
.ixfunc2 '&String' &fmfunc
.ixfunc2 '&Copy' &fmfunc
.ixfunc2 '&Multibyte' &fmfunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function copies the string pointed to by
.arg src
(including the terminating null character) into the array pointed to by
.arg dst
.period
Copying of overlapping objects is not guaranteed to work properly.
See the description for the
.reffunc memmove
function to copy objects that overlap.
.farfunc &ffunc. &funcb.
.widefunc &wfunc. &funcb.
.mbcsfunc &mfunc. &funcb.
.farfuncp &fmfunc. &mfunc.
.desc end
.return begin
The value of
.arg dst
is returned.
.return end
.see begin
.seelist strcpy strdup strncpy strcpy_s strncpy_s
.see end
.exmp begin
#include <stdio.h>
#include <string.h>

void main()
{
    auto char buffer[80];
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
