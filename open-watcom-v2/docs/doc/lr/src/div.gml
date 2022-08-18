.func div
.synop begin
#include <stdlib.h>
div_t div( int numer, int denom );

typedef struct {
    int quot;     /* quotient  */
    int rem;      /* remainder */
} div_t;
.synop end
.*
.desc begin
The
.id &funcb.
function calculates the quotient
and remainder of the division of the numerator
.arg numer
by the denominator
.arg denom
.period
.desc end
.*
.return begin
The
.id &funcb.
function returns a structure of type
.kw div_t
which contains the fields
.kw quot
and
.kw rem
.period
.return end
.*
.see begin
.seelist div ldiv lldiv imaxdiv
.see end
.*
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void print_time( int seconds )
{
     div_t  min_sec;

     min_sec = div( seconds, 60 );
     printf( "It took %d minutes and %d seconds\n",
             min_sec.quot, min_sec.rem );
}
.exmp break
void main( void )
{
    print_time( 130 );
}
.exmp output
It took 2 minutes and 10 seconds
.exmp end
.class ISO C90
.system
