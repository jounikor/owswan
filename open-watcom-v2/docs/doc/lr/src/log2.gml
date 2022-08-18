.func log2
.synop begin
#include <math.h>
double log2( double x );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the logarithm (base 2) of
.arg x
.period
A domain error occurs if the argument is negative.
A range error occurs if the argument is zero.
.desc end
.return begin
The
.id &funcb.
function returns the logarithm (base 2) of the argument.
.im errnodom
.return end
.see begin
.seelist log2 exp log log10 pow matherr
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", log2(.25) );
  }
.exmp output
-2.000000
.exmp end
.class WATCOM
.system
