.func atan
.synop begin
#include <math.h>
double atan( double x );
.ixfunc2 '&Math' &funcb
.ixfunc2 '&Trig' &funcb
.ixfunc2 '&Hyper' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the principal value of the
arctangent of
.arg x
.period
.desc end
.return begin
The
.id &funcb.
function returns the arctangent in the range (&minus.&pi./2,&pi./2).
.return end
.see begin
.seelist acos asin atan2
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    printf( "%f\n", atan(.5) );
  }
.exmp output
0.463648
.exmp end
.class ISO C
.system
