.func jtokata
.synop begin
#include <jstring.h>
JMOJI jtokata( JMOJI c );
.ixfunc2 '&CharTest' &funcb
.synop end
.desc begin
The
.id &funcb.
converts a double-byte Hiragana character to a Katakana
character.
A double-byte Hiragana character is any character for which
the following expression is true:
.millust begin
0x829F <= c <= 0x82F1
.millust end
.np
.us Note:
The Japanese double-byte character set includes Kanji, Hiragana, and
Katakana characters - both alphabetic and numeric.
Kanji is the ideogram character set of the Japanese character set.
Hiragana and Katakana are two types of phonetic character sets of
the Japanese character set.
The Hiragana code set includes 83 characters and the Katakana code set
includes 86 characters.
.desc end
.return begin
The
.id &funcb.
function returns the argument value if the argument is not a
double-byte Hiragana character;
otherwise, the equivalent Katakana character is returned.
.return end
.see begin
.im seejto
.see end
.exmp begin
#include <stdio.h>
#include <jstring.h>

JMOJI chars[] = {
    0x829F,
    0x82B0,
    0x82F1
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( JMOJI )
.exmp break
void main()
  {
    int   i;
    JMOJI c1, c2;
.exmp break
    for( i = 0; i < SIZE; i++ ) {
        c1 = chars[ i ];
        c2 = jtokata( c1 );
        printf( "%c%c - %c%c\n", c1>>8, c1, c2>>8, c2 );
    }
  }
.exmp end
.class WATCOM
.system
