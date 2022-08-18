.func _ismbccntrl
.synop begin
#include <mbstring.h>
int _ismbccntrl( unsigned int ch );
.synop end
.desc begin
The
.id &funcb.
function tests for any multibyte control character.
.desc end
.return begin
The
.id &funcb.
function returns a non-zero value when the argument is a
member of this set of characters; otherwise, zero is returned.
.return end
.see begin
.im seeismbc
.see end
.exmp begin
#include <stdio.h>
#include <mbctype.h>
#include <mbstring.h>
.exmp break
unsigned int chars[] = {
    0x0D,
    '.',
    ' ',
    '1',
    'A',
    0x8140, /* double-byte space */
    0x8143, /* double-byte , */
    0x8254, /* double-byte 5 */
    0x8260, /* double-byte A */
    0x8279, /* double-byte Z */
    0x8281, /* double-byte a */
    0x829A, /* double-byte z */
    0x989F, /* double-byte L2 character */
    0xA6
};
.exmp break
#define SIZE sizeof( chars ) / sizeof( unsigned int )
.exmp break
void main()
  {
    int   i;
.exmp break
    _setmbcp( 932 );
    for( i = 0; i < SIZE; i++ ) {
      printf( "%#6.4x is %sa valid "
            "multibyte control character\n",
            chars[i],
            ( _ismbccntrl( chars[i] ) ) ? "" : "not " );
    }
  }
.exmp output
0x000d is a valid multibyte control character
0x002e is not a valid multibyte control character
0x0020 is not a valid multibyte control character
0x0031 is not a valid multibyte control character
0x0041 is not a valid multibyte control character
0x8140 is a valid multibyte control character
0x8143 is a valid multibyte control character
0x8254 is not a valid multibyte control character
0x8260 is not a valid multibyte control character
0x8279 is not a valid multibyte control character
0x8281 is not a valid multibyte control character
0x829a is not a valid multibyte control character
0x989f is not a valid multibyte control character
0x00a6 is not a valid multibyte control character
.exmp end
.class WATCOM
.system
