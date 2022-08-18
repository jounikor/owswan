.func begin toupper _toupper
.func2 towupper ISO C95
.func end
.synop begin
#include <ctype.h>
int toupper( int c );
.ixfunc2 '&CharTest' &funcb
.ixfunc2 '&Conversion' &funcb
int _toupper( int c );
.ixfunc2 '&CharTest' &_func
.ixfunc2 '&Conversion' &_func
.if &'length(&wfunc.) ne 0 .do begin
#include <wctype.h>
wint_t towupper( wint_t c );
.ixfunc2 '&CharTest' &wfunc
.ixfunc2 '&Conversion' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function converts
.arg c
to a uppercase letter if
.arg c
represents a lowercase letter.
.np
The
.id &_func.
function is a version of
.id &funcb.
to be used only when
.arg c
is known to be lowercase.
.widefunc &wfunc. &funcb.
.desc end
.return begin
The
.id &funcb.
function returns the corresponding uppercase letter when the
argument is a lowercase letter; otherwise, the original character is
returned.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns the corresponding wide character uppercase
letter when the argument is a wide character lowercase letter;
otherwise, the original wide character is returned.
.do end
.np
The result of
.id &_func.
is undefined if
.arg c
is not a lowercase letter.
.return end
.see begin
.im seeis
.seelist _strlwr _strupr tolower toupper
.see end
.exmp begin
#include <stdio.h>
#include <ctype.h>

char chars[] = {
    'a',
    '5',
    '$',
    'z'
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( char )
.exmp break
void main()
{
    int   i;
.exmp break
    for( i = 0; i < SIZE; i++ ) {
        printf( "%c ", toupper( chars[ i ] ) );
    }
    printf( "\n" );
}
.exmp output
A 5 $ Z
.exmp end
.class ISO C
.system
