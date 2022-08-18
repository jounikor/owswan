.func begin islower
.func2 iswlower ISO C95
.func end
.synop begin
#include <ctype.h>
int islower( int c );
.ixfunc2 '&CharTest' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wctype.h>
int iswlower( wint_t c );
.ixfunc2 '&CharTest' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function tests for any lowercase letter 'a' through 'z'.
.widefunc &wfunc. &funcb. <char>
.desc end
.return begin
The
.id &funcb.
function returns a non-zero value when argument is a lowercase
letter.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns a non-zero value when the argument is a
wide character that corresponds to a lowercase letter, or if it is one
of an implementation-defined set of wide characters for which none of
.reffunc iswcntrl
.ct ,
.reffunc iswdigit
.ct ,
.reffunc iswpunct
.ct , or
.reffunc iswspace
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
    'a',
    'z',
    'Z'
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( char )
.exmp break
void main()
{
    int   i;
.exmp break
    for( i = 0; i < SIZE; i++ ) {
        printf( "Char %c is %sa lowercase character\n",
                chars[i],
                ( islower( chars[i] ) ) ? "" : "not " );
    }
}
.exmp output
Char A is not a lowercase character
Char a is a lowercase character
Char z is a lowercase character
Char Z is not a lowercase character
.exmp end
.class ISO C
.system
