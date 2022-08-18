.func remainder
.synop begin
#include <math.h>
double remainder( double x, double y );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes remainder of the division of
.arg x
by
.arg y
.period
.desc end
.return begin
The remainder of the division of
.arg x
by
.arg y
.period
.return end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", remainder( 7.0, 2.0 ) );
  }
.exmp output
1.00000
.exmp end
.class ISO C99
.system
