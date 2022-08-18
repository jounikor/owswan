.func ispnkana
.synop begin
#include <jctype.h>
int ispnkana( int c );
.ixfunc2 '&CharTest' &funcb
.synop end
.desc begin
The
.id &funcb.
function tests if the argument
.arg c
is a single-byte punctuation character such as a comma (,) or a period
(.) or single-byte Katakana punctuation character.
These are any characters for which the following expression is true:
.millust begin
ispunct(c) || iskpun(c)
.millust end
.desc end
.return begin
The
.id &funcb.
function returns zero if the argument is not a single-byte
punctuation character or single-byte Katakana punctuation character;
otherwise, a non-zero value is returned.
.return end
.see begin
.im seejis
.see end
.exmp begin
#include <stdio.h>
#include <jstring.h>
#include <jctype.h>

JMOJI chars[] = {
    '.',
    0x9941,
    0xA4,
    0xA6
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( JMOJI )
.exmp break
void main()
  {
    int   i;
.exmp break
    for( i = 0; i < SIZE; i++ ) {
      printf( "Char is %sa single-byte "
              "punctuation character\n",
            ( ispnkana( chars[i] ) ) ? "" : "not " );
    }
  }
.exmp output
Char is a single-byte punctuation character
Char is not a single-byte punctuation character
Char is a single-byte punctuation character
Char is not a single-byte punctuation character
.exmp end
.class WATCOM
.system
