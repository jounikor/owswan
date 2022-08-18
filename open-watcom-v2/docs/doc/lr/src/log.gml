.func log
.synop begin
#include <math.h>
double log( double x );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the natural logarithm (base e) of
.arg x
.period
A domain error occurs if the argument is negative.
A range error occurs if the argument is zero.
.desc end
.return begin
The
.id &funcb.
function returns the natural logarithm of the argument.
.im errnodom
.return end
.see begin
.seelist log exp log10 log2 pow matherr
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", log(.5) );
  }
.exmp output
-0.693147
.exmp end
.class ISO C
.system
