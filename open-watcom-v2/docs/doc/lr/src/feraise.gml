.func feraiseexcept
.synop begin
#include <fenv.h>
int feraiseexcept( int excepts );
.ixfunc2 'Floating Point Environment' &funcb
.synop end
.*
.desc begin
The
.id &funcb.
function attempts to raise the floating-point exceptions specified by the
.arg excepts
argument.
.np
For valid exception values see
.reffunc fegetexceptflag
.period
.desc end
.*
.return begin
The
.id &funcb.
function returns zero if the
.arg excepts
argument is zero or if all the specified exceptions were successfully
raised. Otherwise, it returns a nonzero value.
.return end
.*
.see begin
.seelist feclearexcept fegetexceptflag fetestexcept
.see end
.*
.exmp begin
#include <fenv.h>
.exmp break
void main( void )
{
    feraiseexcept( FE_DIVBYZERO );
}
.exmp end
.class ISO C99
.system
