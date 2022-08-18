.func tan
.synop begin
#include <math.h>
double tan( double x );
.ixfunc2 '&Math' &funcb
.ixfunc2 '&Trig' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the tangent of
.arg x
(measured in radians).
A large magnitude argument may yield a result with little or no significance.
.desc end
.return begin
The
.id &funcb.
function returns the tangent value.
.im errnoref
.return end
.see begin
.seelist tan atan atan2 cos sin tanh
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", tan(.5) );
  }
.exmp output
0.546302
.exmp end
.class ISO C
.system
