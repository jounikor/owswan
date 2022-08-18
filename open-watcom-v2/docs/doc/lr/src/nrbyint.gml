.func nearbyint
.synop begin
#include <math.h>
double nearbyint( double x );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function rounds the argument
.arg x
to a nearby integer without the possibility of throwing
an exception.  The direction of the rounding is determined by
the current value of
.kw fegetround
.period
.desc end
.return begin
The rounded value of
.arg x
.period
.return end
.see begin
.seelist fegetround fesetround rint round trunc
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    fesetround(FE_TONEAREST);
    printf( "%f\n", nearbyint( 1.2 ) );
  }
.exmp output
1.000000
.exmp end
.class ISO C99
.system
