.func pow
.synop begin
#include <math.h>
double pow( double x, double y );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes
.arg x
raised to the power
.arg y
.period
A domain error occurs if
.arg x
is zero and
.arg y
is less than or equal to 0, or if
.arg x
is negative and
.arg y
is not an integer.
A range error may occur.
.desc end
.return begin
The
.id &funcb.
function returns the value of
.arg x
raised to the power
.arg y
.period
.im errnodom
.return end
.see begin
.seelist pow exp log sqrt
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", pow( 1.5, 2.5 ) );
  }
.exmp output
2.755676
.exmp end
.class ISO C
.system
