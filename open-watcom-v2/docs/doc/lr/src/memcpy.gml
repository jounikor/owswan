.func begin memcpy _fmemcpy
.func2 wmemcpy ISO C95
.func end
.synop begin
#include <string.h>
void *memcpy( void *dst,
              const void *src,
              size_t length );
.ixfunc2 '&Copy' &funcb
.if &farfnc ne 0 .do begin
void __far *_fmemcpy( void __far *dst,
                      const void __far *src,
                      size_t length );
.ixfunc2 '&Copy' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wmemcpy( wchar_t *dst,
                  const wchar_t *src,
                  size_t length );
.ixfunc2 '&Copy' &wfunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function copies
.arg length
characters from the buffer pointed to by
.arg src
into the buffer pointed to by
.arg dst
.period
Copying of overlapping objects is not guaranteed to work properly.
See the
.reffunc memmove
function if you wish to copy objects that overlap.
.farfunc &ffunc. &funcb.
.widefunc &wfunc. &funcb.
.if &'length(&wfunc.) ne 0 .do begin
The argument
.arg length
is interpreted to mean the number of wide characters.
.do end
.desc end
.*
.return begin
The original value of
.arg dst
is returned.
.return end
.*
.see begin
.seelist memchr memcmp memcpy _memicmp memmove memset memcpy_s memmove_s
.see end
.*
.exmp begin
#include <stdio.h>
#include <string.h>

void main( void )
{
    auto char buffer[80];
.exmp break
    memcpy( buffer, "Hello", 5 );
    buffer[5] = '\0';
    printf( "%s\n", buffer );
}
.exmp end
.*
.class ISO C
.system
