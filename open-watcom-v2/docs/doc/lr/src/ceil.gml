.func ceil
.synop begin
#include <math.h>
double ceil( double x );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function (ceiling function)
computes the smallest integer not less than
.arg x
.period
.desc end
.return begin
The
.id &funcb.
function returns the smallest integer not less than
.arg x
.ct , expressed as a
.id double
.period
.return end
.see begin
.seelist floor
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f %f %f %f %f\n", ceil( -2.1 ), ceil( -2. ),
        ceil( 0.0 ), ceil( 2. ), ceil( 2.1 ) );
  }
.exmp output
-2.000000 -2.000000 0.000000 2.000000 3.000000
.exmp end
.class ISO C
.system
