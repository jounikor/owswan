.func begin ispunct
.func2 iswpunct ISO C95
.func end
.synop begin
#include <ctype.h>
int ispunct( int c );
.ixfunc2 '&CharTest' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wctype.h>
int iswpunct( wint_t c );
.ixfunc2 '&CharTest' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function tests for any punctuation character such as a comma
(,) or a period (.).
.widefunc &wfunc. &funcb. <char>
.desc end
.return begin
The
.id &funcb.
function returns a non-zero value when the argument is a
punctuation character.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns a non-zero value when the argument is a
printable wide character that is neither the space wide character nor
a wide character for which
.reffunc iswalnum
is true.
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
    '!',
    '.',
    ',',
    ':',
    ';'
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( char )
.exmp break
void main()
{
    int   i;
.exmp break
    for( i = 0; i < SIZE; i++ ) {
        printf( "Char %c is %sa punctuation character\n",
                chars[i],
                ( ispunct( chars[i] ) ) ? "" : "not " );
    }
}
.exmp output
Char A is not a punctuation character
Char ! is a punctuation character
Char . is a punctuation character
Char , is a punctuation character
Char : is a punctuation character
Char ; is a punctuation character
.exmp end
.class ISO C
.system
