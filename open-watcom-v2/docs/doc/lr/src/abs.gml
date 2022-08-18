.func abs
.synop begin
#include <stdlib.h>
int abs( int j );
.synop end
.*
.desc begin
The
.id &funcb.
function returns the absolute value of its integer argument
.arg j
.period
.desc end
.*
.return begin
The
.id &funcb.
function returns the absolute value of its argument.
.return end
.*
.see begin
.seelist labs llabs imaxabs fabs
.see end
.*
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void main( void )
{
    printf( "%d %d %d\n", abs( -5 ), abs( 0 ), abs( 5 ) );
}
.exmp output
5 0 5
.exmp end
.*
.class ISO C90
.system
