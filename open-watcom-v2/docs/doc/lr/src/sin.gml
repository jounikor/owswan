.func sin
.synop begin
#include <math.h>
double sin( double x );
.ixfunc2 '&Math' &funcb
.ixfunc2 '&Trig' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the sine of
.arg x
(measured in radians).
A large magnitude argument may yield a result with little or no significance.
.desc end
.return begin
The
.id &funcb.
function returns the sine value.
.return end
.see begin
.seelist sin acos asin atan atan2 cos tan
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", sin(.5) );
  }
.exmp output
0.479426
.exmp end
.class ISO C
.system
