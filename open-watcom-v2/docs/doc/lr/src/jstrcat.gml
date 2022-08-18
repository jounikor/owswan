.func jstrcat _fjstrcat
.synop begin
#include <jstring.h>
JSTRING jstrcat( JCHAR *dst, const JCHAR *src );
.if &farfnc ne 0 .do begin
FJSTRING _fjstrcat( JCHAR __far *dst, const JCHAR __far *src );
.do end
.ixfunc2 '&Jstring' &funcb
.ixfunc2 '&Jconcat' &funcb
.ixfunc2 '&Jstring' &ffunc
.ixfunc2 '&Jconcat' &ffunc
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function appends
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions append
.do end
a copy of the Kanji string pointed to by
.arg src
(including the terminating null character)
to the end of the Kanji string pointed to by
.arg dst
.period
The first character of
.arg src
overwrites the null character at the end of
.arg dst
.period
.farfunc &ffunc. &funcb.
.desc end
.return begin
The value of
.arg dst
is returned.
.return end
.see begin
.seelist jstrcat jstrncat strcat strncat
.see end
.exmp begin
#include <stdio.h>
#include <string.h>
#include <jstring.h>

void main()
  {
    JCHAR buffer[80];
.exmp break
    strcpy( buffer, "Hello " );
    jstrcat( buffer, "world" );
    printf( "%s\n", buffer );
  }
.exmp output
Hello world
.exmp end
.class WATCOM
.system
