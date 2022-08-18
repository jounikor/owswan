.func jstrrev _fjstrrev
.synop begin
#include <jstring.h>
JSTRING jstrrev( JCHAR *s1 );
.ixfunc2 '&Jstring' &funcb
.if &farfnc ne 0 .do begin
FJSTRING _fjstrrev( JCHAR __far *s1 );
.ixfunc2 '&Jstring' &ffunc
.do end
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function replaces
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions replace
.do end
the Kanji string
.arg s1
with a Kanji string whose single-byte or double-byte characters are in the
reverse order.
.farfunc &ffunc. &funcb.
.desc end
.return begin
The address of the original string
.arg s1
is returned.
.return end
.see begin
.seelist jstrrev _strrev
.see end
.exmp begin
#include <stdio.h>
#include <jstring.h>

JCHAR source[] = { "A sample STRING" };

void main()
  {
    printf( "%s\n", source );
    printf( "%s\n", jstrrev( source ) );
    printf( "%s\n", jstrrev( source ) );
  }
.exmp output
A sample STRING
GNIRTS elpmas A
A sample STRING
.exmp end
.class WATCOM
.system
