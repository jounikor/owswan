.func jstrnset _fjstrnset
.synop begin
#include <jstring.h>
JSTRING jstrnset( JCHAR *s1, JMOJI fill, size_t n );
.ixfunc2 '&Jstring' &funcb
.if &farfnc ne 0 .do begin
FJSTRING _fjstrnset( JCHAR __far *s1, JMOJI fill, size_t n );
.ixfunc2 '&Jstring' &ffunc
.do end
.synop end
.desc begin
.if &farfnc eq 0 .do begin
The
.id &funcb.
function fills
.do end
.el .do begin
The
.id &funcb.
and
.id &ffunc.
functions fill
.do end
the string
.arg s1
with the value of the argument
.arg fill
.period
If
.arg fill
is a single-byte character, its value must be in the low-order byte
of the 16-bit value and the high-order byte must be zero.
If
.arg fill
is a double-byte character,
the first byte of the double-byte character must be in the high-order
byte of the 16-bit value
and the second byte of the double-byte character must be in the
low-order byte of the 16-bit value.
.np
If
.arg fill
is the null character, then both high- and low-order bytes must
be null.
.np
When the value of
.arg len
is greater than the length of the string, the entire string is filled.
Otherwise, that number of characters at the start of the string are set
to the fill character.
.np
If after filling the string with as many single-byte or double-byte
characters as possible,
the next character in the string is the second byte of a double-byte
character, then that character is set to ASCII blank (hex '20').
.farfunc &ffunc. &funcb.
.desc end
.return begin
The address of the original string
.arg s1
is returned.
.return end
.see begin
.seelist jstrnset jstrset _strnset _strset
.see end
.exmp begin
#include <stdio.h>
#include <jstring.h>

JCHAR source[] = { "A sample STRING" };

void main()
  {
    printf( "%s\n", source );
    printf( "%s\n", jstrnset( source, '=', 100 ) );
    printf( "%s\n", jstrnset( source, '*', 7 ) );
  }
.exmp output
A sample STRING
===============
*******========
.exmp end
.class WATCOM
.system
