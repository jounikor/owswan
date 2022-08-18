.func begin isblank
.func2 iswblank
.func end
.synop begin
#include <ctype.h>
int isblank( int c );
.ixfunc2 '&CharTest' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wctype.h>
int iswblank( wint_t c );
.ixfunc2 '&CharTest' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function tests for the following blank characters:
.begpoint $compact $setptnt 12
.pointhd1 Constant
.pointhd2 Character
.point ' '
space
.point '\t'
horizontal tab
.endpoint
.widefunc &wfunc. &funcb. <char>
.desc end
.return begin
The
.id &funcb.
function returns a non-zero character when the argument is
one of the indicated blank characters.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns a non-zero value when the argument is a
wide character that corresponds to a standard blank character or
is one of an implementation-defined set of wide characters for which
.reffunc iswalnum
is false.
.do end
Otherwise, zero is returned.
.return end
.see begin
.im seeis
.see end
.exmp begin
#include <stdio.h>
#include <ctype.h>

char chars[] = {
    'A',
    0x09,
    ' ',
    0x7d
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( char )
.exmp break
void main()
{
    int   i;
.exmp break
    for( i = 0; i < SIZE; i++ ) {
      printf( "Char %c is %sa blank character\n",
            chars[i],
            ( isblank( chars[i] ) ) ? "" : "not " );
    }
}
.exmp output
Char A is not a blank character
Char     is a blank character
Char   is a blank character
Char } is not a blank character
.exmp end
.class ISO C99
.system
