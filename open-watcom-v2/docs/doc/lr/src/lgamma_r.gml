.func lgamma_r
.synop begin
#include <math.h>
double lgamma_r( double x, int *y );
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function returns the natural logarithm of the
absolute value of the Gamma function of
.arg x
.period The sign of the Gamma function will be returned
in the location pointed to by
.arg y
.period
.desc end
.return begin
If successful, the return value is the natural logarithm of
the absolute value of the Gamma function computed for
.arg x
.period  When the argument is not-a-number, the function
returns NAN.  For arguments of the values positive or negative
infinity, the function returns positive or negative infinity
respectively.
.return end
.see begin
.seelist lgamma tgamma
.see end
.exmp begin
#include <stdio.h>
#include <math.h>

void main()
  {
    int i;
    printf( "%f\n", lgamma_r( 2.0, &i ) );
    printf( "%d\n", i );
  }
.exmp output
0.00000
1
.exmp end
.class WATCOM
.system
