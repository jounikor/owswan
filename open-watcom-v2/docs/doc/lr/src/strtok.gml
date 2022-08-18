.func begin strtok strtok_r _fstrtok _fstrtok_r
.func2 wcstok ISO C95
.func2 _mbstok
.func2 _mbstok_r
.func2 _fmbstok
.func2 _fmbstok_r
.func end
.synop begin
#include <string.h>
char *strtok( char *s1, const char *s2 );
char *strtok_r( char *s1, const char *s2, char **p1 );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Search' &funcb
.if &farfnc ne 0 .do begin
char __far *_fstrtok( char __far *s1,
                      const char __far *s2 );
char __far *_fstrtok_r( char __far *s1,
                        const char __far *s2,
                        char __far **p1 );
.ixfunc2 '&String' &ffunc
.ixfunc2 '&Search' &ffunc
.do end
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *wcstok( wchar_t *s1, const wchar_t *s2,
                 wchar_t **ptr );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Search' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.if &'length(&mfunc.) ne 0 .do begin
#include <mbstring.h>
unsigned char *_mbstok( unsigned char *s1,
                  const unsigned char *s2 );
unsigned char *_mbstok_r( unsigned char *s1,
                  const unsigned char *s2,
                  unsigned char **p1 );
.ixfunc2 '&String' &mfunc
.ixfunc2 '&Search' &mfunc
.ixfunc2 '&Wide' &mfunc
.do end
.if &'length(&fmfunc.) ne 0 .do begin
unsigned char __far *_fmbstok( unsigned char __far *s1,
                         const unsigned char __far *s2 );
unsigned char __far *_fmbstok_r( unsigned char __far *s1,
                         const unsigned char __far *s2,
                         unsigned char __far **p1 );
.ixfunc2 '&String' &fmfunc
.ixfunc2 '&Search' &fmfunc
.ixfunc2 '&Wide' &fmfunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function is used to break the string pointed to by
.arg s1
into a sequence of tokens, each of which is delimited by a character
from the string pointed to by
.arg s2
.period
The first call to
.id &funcb.
will return a pointer to the first token in
the string pointed to by
.arg s1
.period
Subsequent calls to
.id &funcb.
must pass a NULL pointer as the first
argument, in order to get the next token in the string.
The set of delimiters used in each of these calls to
.id &funcb.
can be
different from one call to the next.
.np
The first call in the sequence searches
.arg s1
for the first character that is not contained in the current delimiter
string
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
function then searches from there for a character that is
contained in the current delimiter string.
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
.np
The reentrant form of this function,
.reffunc strtok_r
.ct , also requires a pointer to a string pointer to be passed.
This pointer is used internally by the function for subsequent
calls to perform tokenizing without relying on internal state
within the function.
.farfunc &ffunc. &funcb.
.widefunc &wfunc. &funcb.
.if &'length(&wfunc.) ne 0 .do begin
The third argument
.arg ptr
points to a caller-provided
.kw wchar_t
pointer into which the
.id &wfunc.
function stores information necessary
for it to continue scanning the same wide string.
.np
On the first call in the sequence of calls to &wfunc,
.arg s1
points to a wide string.
In subsequent calls for the same string,
.arg s1
must be NULL.
If
.arg s1
is NULL, the value pointed to by
.arg ptr
matches that set by the previous call to
.id &wfunc.
for the same wide
string.
Otherwise, the value of
.arg ptr
is ignored.
The list of delimiters pointed to by
.arg s2
may be different from one call to the next.
The tokenization of
.arg s1
is similar to that for the
.id &funcb.
function.
.do end
.mbcsfunc &mfunc. &funcb.
.farfunc &fmfunc. &mfunc.
.desc end
.return begin
The
.id &funcb.
function returns a pointer to the first character of a token
or
.mono NULL
if there is no token found.
.return end
.see begin
.seelist strtok strcspn strpbrk strtok_s
.see end
.exmp begin
#include <stdio.h>
#include <string.h>
.exmp break
void main()
{
    char *p;
    char *buffer;
    char *delims = { " .," };
.exmp break
    buffer = strdup( "Find words, all of them." );
    printf( "%s\n", buffer );
    p = strtok( buffer, delims );
    while( p != NULL ) {
      printf( "word: %s\n", p );
      p = strtok( NULL, delims );
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
.class ISO C
.system
