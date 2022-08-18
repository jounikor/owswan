.func tanh
.synop begin
#include <math.h>
double tanh( double x );
.ixfunc2 '&Math' &funcb
.ixfunc2 '&Trig' &funcb
.ixfunc2 '&Hyper' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the hyperbolic tangent of
.arg x
.period
.np
When the
.arg x
argument is large, partial or total loss of significance may occur.
The
.reffunc matherr
function will be invoked in this case.
.desc end
.return begin
The
.id &funcb.
function returns the hyperbolic tangent value.
.im errnoref
.return end
.see begin
.seelist tanh cosh sinh matherr
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", tanh(.5) );
  }
.exmp output
0.462117
.exmp end
.class ISO C
.system
