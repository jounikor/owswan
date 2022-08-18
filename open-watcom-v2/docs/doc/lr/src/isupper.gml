.func begin isupper
.func2 iswupper ISO C95
.func end
.synop begin
#include <ctype.h>
int isupper( int c );
.ixfunc2 '&CharTest' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wctype.h>
int iswupper( wint_t c );
.ixfunc2 '&CharTest' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function tests for any uppercase letter 'A' through 'Z'.
.widefunc &wfunc. &funcb. <char>
.desc end
.return begin
The
.id &funcb.
function returns a non-zero value when the argument is an
uppercase letter.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns a non-zero value when the argument is a
wide character that corresponds to an uppercase letter, or if it is one
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
        printf( "Char %c is %san uppercase character\n",
                chars[i],
                ( isupper( chars[i] ) ) ? "" : "not " );
    }
}
.exmp output
Char A is an uppercase character
Char a is not an uppercase character
Char z is not an uppercase character
Char Z is an uppercase character
.exmp end
.class ISO C
.system
