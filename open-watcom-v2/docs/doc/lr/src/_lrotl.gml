.func _lrotl
.synop begin
#include <stdlib.h>
unsigned long _lrotl( unsigned long value,
                      unsigned int shift );
.ixfunc2 '&Rotate' &funcb
.synop end
.desc begin
The
.id &funcb.
function rotates the unsigned long integer, determined by
.arg value
.ct , to the left by the number of bits specified in
.arg shift
.period
.desc end
.return begin
The rotated value is returned.
.return end
.see begin
.seelist _lrotr _rotl _rotr
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>

unsigned long mask = 0x12345678;

void main()
  {
    mask = _lrotl( mask, 4 );
    printf( "%08lX\n", mask );
  }
.exmp output
23456781
.exmp end
.class WATCOM
.system
