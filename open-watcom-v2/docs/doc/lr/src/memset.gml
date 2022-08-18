.func begin memset _fmemset
.func2 wmemset ISO C95
.func end
.synop begin
#include <string.h>
void *memset( void *dst, int c, size_t length );
.ixfunc2 '&String' &funcb
.if &farfnc ne 0 .do begin
void __far *_fmemset( void __far *dst, int c,
                      size_t length );
.ixfunc2 '&String' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
wchar_t *wmemset( wchar_t *dst,
                  wchar_t c,
                  size_t length );
.ixfunc2 '&String' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function fills the first
.arg length
characters of the object pointed to by
.arg dst
with the value
.arg c
.period
.farfunc &ffunc. &funcb.
.widefunc &wfunc. &funcb.
.if &'length(&wfunc.) ne 0 .do begin
The argument
.arg length
is interpreted to mean the number of wide characters.
.do end
.desc end
.return begin
The
.id &funcb.
function returns the pointer
.arg dst
.period
.return end
.see begin
.seelist memchr memcmp memcpy _memicmp memmove memset
.see end
.exmp begin
#include <string.h>

void main( void )
{
    char buffer[80];
.exmp break
    memset( buffer, '=', 80 );
}
.exmp end
.class ISO C
.system
