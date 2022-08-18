.func fegetround
.synop begin
#include <fenv.h>
int fegetround( void );
.ixfunc2 'Floating Point Environment' &funcb
.synop end
.*
.desc begin
The
.id &funcb.
function returns a value that indicates the rounding direction mode, as
specified in the current floating point environment.
.desc end
.*
.return begin
The
.id &funcb.
function returns the value of the rounding direction macro
representing the current rounding direction or a negative value if there is no such
rounding direction macro or the current rounding direction is not determinable.
.np
For valid rounding modes see
.reffunc fesetround
.period
.return end
.*
.see begin
.seelist fesetround
.see end
.*
.exmp begin
#include <stdio.h>
#include <fenv.h>
.exmp break
void main( void )
{
    int mode;
    mode = fegetround();
    if ( mode == FE_TONEAREST )
        printf( "Nearest\n" );
    else if ( mode == FE_DOWNWARD )
        printf( "Down\n" );
    else if ( mode == FE_TOWARDZERO )
        printf( "To Zero\n" );
    else if ( mode == FE_UPWARD )
        printf( "Up\n" );
}
.exmp end
.class ISO C99
.system
