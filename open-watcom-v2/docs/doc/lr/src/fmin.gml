.func fmin
.synop begin
#include <math.h>
double fmin( double x, double y );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function returns the smaller of
.arg x
and
.arg y
.period
.desc end
.return begin
The routine will return the smaller of
.arg x
or
.arg y
.period
.return end
.see begin
.seelist fdim fmax
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", fmin( 3.0, 2.0 ) );
  }
.exmp output
2.000000
.exmp end
.class ISO C99
.system
