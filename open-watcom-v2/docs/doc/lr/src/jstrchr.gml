.func jstrchr _fjstrchr
.synop begin
#include <jstring.h>
JSTRING jstrchr( const JCHAR *s, JMOJI c );
.ixfunc2 '&Jstring' &funcb
.ixfunc2 '&Jsearch' &funcb
.if &farfnc ne 0 .do begin
FJSTRING _fjstrchr( const JCHAR __far *s, JMOJI c );
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
the first occurrence of the single-byte or double-byte character
.arg c
in the Kanji string pointed to by
.arg s
.period
The terminating null character is considered to be part of the string.
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
a pointer to the located character, or
.mono NULL
if the character does not occur in the Kanji string.
.return end
.see begin
.seelist jstrchr jstrcspn jstrrchr jstrspn jstrstr jstrtok
.seelist memchr strcspn strrchr strspn strstr strtok
.see end
.exmp begin
#include <stdio.h>
#include <string.h>
#include <jstring.h>

void main()
  {
    JCHAR buffer[80];
    JSTRING where;
.exmp break
    strcpy( buffer, "video x-rays" );
    where = jstrchr( buffer, 'x' );
    if( where == NULL ) {
        printf( "'x' not found\n" );
    }
  }
.exmp end
.class WATCOM
.system
