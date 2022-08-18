.func jstradv _fjstradv
.synop begin
#include <jstring.h>
JSTRING jstradv( const JCHAR *src, size_t n );
.ixfunc2 '&Jstring' &funcb
.ixfunc2 '&Jconcat' &funcb
.if &farfnc ne 0 .do begin
FJSTRING _fjstradv( const JCHAR __far *src, size_t n );
.ixfunc2 '&Jstring' &ffunc
.ixfunc2 '&Jconcat' &ffunc
.do end
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function skips
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions skip
.do end
over
.arg n
single-byte or double-byte characters in the Kanji string
.arg src
.period
A pointer to the next character is returned.
.farfunc &ffunc. &funcb.
.desc end
.return begin
A pointer to the next character is returned unless the number of
characters in the Kanji string is less than
.arg n
in which case a pointer to the end of the string
.arg src
is returned.
If
.arg n
is 0 then a pointer to
.arg src
is returned.
.return end
.see begin
.seelist btom jgetmoji jstradv mtob
.see end
.exmp begin
#include <stdio.h>
#include <string.h>
#include <jstring.h>

void main()
  {
    JCHAR buffer[80];
    JSTRING p;
.exmp break
    strcpy( buffer, "Hello world" );
    p = jstradv( buffer, 6 );
    printf( "%s\n", p );
  }
.exmp output
Hello world
.exmp end
.class WATCOM
.system
