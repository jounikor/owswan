.func cabs
.synop begin
#include <math.h>
double cabs( struct complex value );

struct _complex {
    double  x;  /* real part      */
    double  y;  /* imaginary part */
};
.ixfunc2 '&Math' &funcb
.synop end
.desc begin
The
.id &funcb.
function computes the absolute value of the complex number
.arg value
by a calculation which is equivalent to
.blkcode begin
sqrt( (value.x*value.x) + (value.y*value.y) )
.blkcode end
.blktext begin
In certain cases, overflow errors may occur which will cause the
.reffunc matherr
routine to be invoked.
.blktext end
.desc end
.return begin
The absolute value is returned.
.return end
.exmp begin
#include <stdio.h>
#include <math.h>

struct _complex c = { -3.0, 4.0 };
.exmp break
void main()
  {
    printf( "%f\n", cabs( c ) );
  }
.exmp output
5.000000
.exmp end
.class WATCOM
.system
