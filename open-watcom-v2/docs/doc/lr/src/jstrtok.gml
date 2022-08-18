.func jstrtok _fjstrtok
.synop begin
#include <jstring.h>
JSTRING jstrtok( JCHAR *s1, const JCHAR *s2 );
.ixfunc2 '&Jstring' &funcb
.ixfunc2 '&Jsearch' &funcb
.if &farfnc ne 0 .do begin
FJSTRING _fjstrtok( JCHAR __far *s1, const JCHAR __far *s2 );
.ixfunc2 '&Jstring' &ffunc
.ixfunc2 '&Jsearch' &ffunc
.do end
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function is
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions are
.do end
used to break the Kanji string pointed to by
.arg s1
into a sequence of tokens, each of which is delimited by
a single-byte or double-byte character from the string
pointed to by
.arg s2
.period
The first call to
.id &funcb.
will return a pointer to the first token in
the Kanji string pointed to by
.arg s1
.period
Subsequent calls to
.id &funcb.
must pass a NULL pointer as the first
argument, in order to get the next token in the Kanji string.
The set of delimiters used in each of these calls to
.id &funcb.
can be
different from one call to the next.
.np
The first call in the sequence searches
.arg s1
for the first single-byte or double-byte character that is
not contained in the current delimiter string
.arg s2
.period
If no such character is found, then there are no tokens in
.arg s1
and the
.id &funcb.
function returns a NULL pointer.
If such a character is found, it is the start of the first token.
.np
The
.id &funcb.
function then searches from there for a single-byte or
double-byte character that is contained in the current delimiter
If no such character is found, the current token extends to the end of
the string pointed to by
.arg s1
.period
If such a character is found, it is overwritten by a null character,
which terminates the current token.
The
.id &funcb.
function saves a pointer to the following character, from
which the next search for a token will start when the first argument
is a NULL pointer.
.np
Because
.id &funcb.
may modify the original string, that string should be
duplicated if the string is to be re-used.
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
a pointer to the first character of a token or
.mono NULL
if there is no token found.
.return end
.see begin
.seelist jstrtok jstrcspn jstrpbrk strtok strcspn strpbrk
.see end
.exmp begin
#include <stdio.h>
#include <string.h>
#include <jstring.h>

void main()
  {
    JSTRING p;
    JSTRING buffer;
    JSTRING delims = { " .," };

    buffer = strdup( "Find words, all of them." );
    printf( "%s\n", buffer );
    p = jstrtok( buffer, delims );
    while( p != NULL ) {
      printf( "word: %s\n", p );
      p = jstrtok( NULL, delims );
    }
    printf( "%s\n", buffer );
  }
.exmp output
Find words, all of them.
word: Find
word: words
word: all
word: of
word: them
Find
.exmp end
.class WATCOM
.system
