.func trunc
.synop begin
#include <math.h>
double trunc( double x );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function truncates the argument
.arg x
to the appropriate integer.  The function is equivalent to
.reffunc floor
for positive numbers and
.reffunc ceil
for negative numbers.
.desc end
.return begin
The value of
.arg x
without any fractional values.
.return end
.see begin
.seelist nearbyint rint round floor ceil
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", round( 1.5 ) );
  }
.exmp output
1.000000
.exmp end
.class ISO C99
.system
