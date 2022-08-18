.func rint
.synop begin
#include <math.h>
double rint( double x );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function rounds the argument
.arg x
to a nearby integer.  The direction of the rounding
is determined by the current value of
.kw fegetround
.period If supported, this function will throw a
floating point error if an overflow occurs due to the
current rounding mode.
.desc end
.return begin
The rounded value of
.arg x
.period
.return end
.see begin
.seelist fegetround fesetround nearbyint round trunc
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    fesetround(FE_TONEAREST);
    printf( "%f\n", rint( 1.2 ) );
  }
.exmp output
1.000000
.exmp end
.class ISO C99
.system
