.func strcat_s wcscat_s
.synop begin
#define __STDC_WANT_LIB_EXT1__  1
#include <string.h>
errno_t strcat_s( char * restrict s1,
                  rsize_t s1max,
                  const char * restrict s2 );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Concats' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
errno_t wcscat_s( wchar_t * restrict s1,
                  rsize_t s1max,
                  const wchar_t * restrict s2 );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Concats' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.*
.rtconst begin
Let
.arg m
denote the value
.arg s1max - strnlen_s(s1, s1max)
upon entry to
.reffunc strcat_s
.period
Neither
.arg s1
nor
.arg s2
shall be a null pointer.
.arg s1max
shall not be greater than
.kw RSIZE_MAX
.period
.arg s1max
shall not equal zero.
.arg m
shall not equal zero.
.arg m
shall be greater than
.arg strnlen_s(s2, m)
.period
Copying shall not take place between objects that overlap.
.np
If there is a runtime-constraint violation, then if
.arg s1
is not a null pointer and
.arg s1max
is greater than zero and not greater than
.kw RSIZE_MAX
.ct , then
.reffunc strcat_s
sets
.arg s1[0]
to the null character.
.rtconst end
.*
.desc begin
The
.id &funcb.
function appends a copy of the string pointed to by
.arg s2
(including the terminating null character) to the end of the string pointed to by
.arg s1
.period
The initial character from
.arg s2
overwrites the null character at the end of
.arg s1
.period
All elements following the terminating null character (if any) written by
.id &funcb.
in the array of
.arg s1max
characters pointed to by
.arg s1
take unspecified values when
.id &funcb.
returns.
.widefunc &wfunc. &funcb.
.desc end
.*
.return begin
.saferet
.return end
.*
.see begin
.seelist strcat strncat strncat_s
.see end
.*
.exmp begin
#define __STDC_WANT_LIB_EXT1__  1
#include <stdio.h>
#include <string.h>

void main( void )
{
    char buffer[80];
.exmp break
    strcpy_s( buffer, sizeof( buffer ), "Hello " );
    strcat_s( buffer, sizeof( buffer ), "world" );
    printf( "%s\n", buffer );
}
.exmp output
Hello world
.exmp end
.*
.class TR 24731
.system
