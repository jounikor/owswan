.func feclearexcept
.synop begin
#include <fenv.h>
int feclearexcept( int excepts );
.ixfunc2 'Floating Point Environment' &funcb
.synop end
.*
.desc begin
The
.id &funcb.
function attempts to clear the floating-point exceptions
specified by the
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
argument is zero or if all
the specified exceptions were successfully cleared. Otherwise, it returns
a nonzero value.
.return end
.*
.see begin
.seelist fegetexceptflag feraiseexcept fesetexceptflag fetestexcept
.see end
.*
.exmp begin
#include <fenv.h>
.exmp break
void main( void )
{
    feclearexcept( FE_OVERFLOW|FE_UNDERFLOW );
}
.exmp end
.class ISO C99
.system
