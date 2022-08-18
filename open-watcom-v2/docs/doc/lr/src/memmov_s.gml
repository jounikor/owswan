.func memmove_s wmemmove_s
.synop begin
#define __STDC_WANT_LIB_EXT1__  1
#include <string.h>
errno_t memmove_s( void * restrict s1,
                   rsize_t s1max,
                   const void * restrict s2,
                   rsize_t n );
.ixfunc2 '&Copy' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
errno_t wmemmove_s( wchar_t * restrict s1,
                    rsize_t s1max,
                    const wchar_t * restrict s2,
                    size_t n );
.ixfunc2 '&Copy' &wfunc
.do end
.synop end
.*
.rtconst begin
Neither
.arg s1
nor
.arg s2
shall be a null pointer. Neither
.arg s1max
nor
.arg n
shall be greater than
.arg RSIZE_MAX
.period
.arg n
shall not be greater than
.arg s1max
.period
.np
If there is a runtime-constraint violation, the
.id &funcb.
function stores zeros in the first
.arg s1max
characters of the object pointed to by
.arg s1
if
.arg s1
is not a null pointer and
.arg s1max
is
not greater than
.arg RSIZE_MAX
.period
.rtconst end
.*
.desc begin
The
.id &funcb.
function copies
.arg n
characters from the buffer pointed to by
.arg s2
into the buffer pointed to by
.arg s1
.period
This copying takes place as if the
.arg n
characters from the buffer
pointed to by
.arg s2
are first copied into a temporary array of
.arg n
characters that does not overlap the objects pointed to by
.arg s1
or
.arg s2
.ct , and then the
.arg n
characters from the temporary array are copied into the object pointed to by
.arg s1
.period
.np
See the
.arg memcpy_s
function if you wish to copy objects that do not overlap.
.widefunc &wfunc. &funcb.
.if &'length(&wfunc.) ne 0 .do begin
The arguments
.arg s1max
and
.arg n
are interpreted to mean the number of wide characters.
.do end
.desc end
.*
.return begin
.saferet
.return end
.*
.see begin
.seelist memchr memcmp memcpy _memicmp memmove memset memcpy_s
.see end
.exmp begin
#define __STDC_WANT_LIB_EXT1__  1
#include <string.h>
void main( void )
{
    char buffer[80] = "0123456789";
.exmp break
    memmove_s( buffer + 1, sizeof( buffer ), buffer, 79 );
    buffer[0] = '*';
    printf( buffer );
}
.exmp output
*0123456789
.exmp end
.class TR 24731
.system
