.func jstrstr _fjstrstr
.synop begin
#include <jstring.h>
JSTRING jstrstr( const JCHAR *s, const JCHAR *substr );
.ixfunc2 '&Jstring' &funcb
.ixfunc2 '&Jsearch' &funcb
.if &farfnc ne 0 .do begin
FJSTRING _fjstrstr( const JCHAR __far *s, const JCHAR __far *substr );
.ixfunc2 '&Jstring' &ffunc
.ixfunc2 '&Jsearch' &ffunc
.do end
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function locates
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions locate
.do end
the first occurrence in the Kanji string pointed to by
.arg s
of the sequence of single-byte and double-byte characters (excluding
the terminating null character) in the Kanji string pointed to by
.arg substr
.period
.farfunc &ffunc. &funcb.
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
a pointer to the located string, or
.mono NULL
if the string is not found.
.return end
.see begin
.seelist jstrcspn jstrstr strcspn strstr
.see end
.exmp begin
#include <stdio.h>
#include <jstring.h>

void main()
  {
    printf( "%s\n",
            jstrstr( "This is an example", "is" )
          );
  }
.exmp output
is is an example
.exmp end
.class WATCOM
.system
