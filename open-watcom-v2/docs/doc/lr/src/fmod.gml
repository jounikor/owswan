.func fmod
.synop begin
#include <math.h>
double fmod( double x, double y );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the floating-point remainder of
.arg x/y
.ct , even if the quotient
.arg x/y
is not representable.
.desc end
.return begin
The
.id &funcb.
function returns the value
.arg x &minus. (i * y)
.ct , for some integer
.arg i
such that, if
.arg y
is non-zero, the result has the same sign as
.arg x
and magnitude less than the magnitude of
.arg y
.period
If the value of
.arg y
is zero, then the value returned is zero.
.return end
.see begin
.seelist fmod ceil fabs floor
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", fmod(  4.5,  2.0 ) );
    printf( "%f\n", fmod( -4.5,  2.0 ) );
    printf( "%f\n", fmod(  4.5, -2.0 ) );
    printf( "%f\n", fmod( -4.5, -2.0 ) );
  }
.exmp output
0.500000
-0.500000
0.500000
-0.500000
.exmp end
.class ISO C
.system
