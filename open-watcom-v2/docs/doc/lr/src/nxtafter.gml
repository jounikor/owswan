.func nextafter
.synop begin
#include <math.h>
double nextafter( double x, double y );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function returns the next machine floating point
number of
.arg x
in the direction towards
.arg y
.period
.desc end
.return begin
The next representable floating point value after or before
.arg x
in the direction of
.arg y
.period
.return end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", nextafter( 8.0, 9.0 ) );
  }
.exmp output
8.000000
.exmp end
.class ISO C99
.system
