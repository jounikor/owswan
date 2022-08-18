.func begin isspace
.func2 iswspace ISO C95
.func end
.synop begin
#include <ctype.h>
int isspace( int c );
.ixfunc2 '&CharTest' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wctype.h>
int iswspace( wint_t c );
.ixfunc2 '&CharTest' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function tests for the following white-space characters:
.begpoint $compact $setptnt 12
.pointhd1 Constant
.pointhd2 Character
.point ' '
space
.point '\f'
form feed
.point '\n'
new-line or linefeed
.point '\r'
carriage return
.point '\t'
horizontal tab
.point '\v'
vertical tab
.endpoint
.widefunc &wfunc. &funcb. <char>
.desc end
.return begin
The
.id &funcb.
function returns a non-zero character when the argument is
one of the indicated white-space characters.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns a non-zero value when the argument is a
wide character that corresponds to a standard white-space character or
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
        printf( "Char %c is %sa space character\n",
                chars[i],
                ( isspace( chars[i] ) ) ? "" : "not " );
    }
}
.exmp output
Char A is not a space character
Char     is a space character
Char   is a space character
Char } is not a space character
.exmp end
.class ISO C
.system
