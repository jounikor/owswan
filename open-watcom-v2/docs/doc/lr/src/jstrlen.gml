.func jstrlen _fjstrlen
.synop begin
#include <jstring.h>
size_t jstrlen( const JCHAR *s );
.ixfunc2 '&Jstring' &funcb
.if &farfnc ne 0 .do begin
size_t _fjstrlen( const JCHAR __far *s );
.ixfunc2 '&Jstring' &ffunc
.do end
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function computes
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions compute
.do end
the number of characters in the Kanji string pointed to by
.arg s
.period
A double-byte character is treated as one character.
.farfuncp &ffunc. &funcb.
.desc end
.return begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function returns
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions return
.do end
the number of characters that precede the terminating null character.
.return end
.see begin
.seelist jstrlen strlen
.see end
.exmp begin
#include <stdio.h>
#include <jstring.h>

void main()
  {
    printf( "%d\n", jstrlen( "Howdy" ) );
    printf( "%d\n", jstrlen( "Hello world\n" ) );
    printf( "%d\n", jstrlen( "" ) );
  }
.exmp output
5
12
0
.exmp end
.class WATCOM
.system
