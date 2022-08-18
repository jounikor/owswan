.func vscanf_s vwscanf_s
.synop begin
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdarg.h>
#include <stdio.h>
int vscanf_s( const char * restrict format, va_list arg );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdarg.h>
#include <wchar.h>
int vwscanf_s( const wchar_t * restrict format, va_list arg );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.*
.rtconst begin
The argument
.arg format
shall not be a null pointer.
Any argument indirected through in order to store converted input shall
not be a null pointer.
.np
If there is a runtime-constraint violation, the
.id &funcb.
function does not
attempt to perform further input, and it is unspecified to what extent
.id &funcb.
performed input before discovering the runtime-constraint violation.
.rtconst end
.*
.desc begin
The
.id &funcb.
function is equivalent to
.reffunc scanf_s
.ct , with the variable argument list replaced by
.arg arg
.ct , which shall have been initialized by the
.reffunc va_start
macro (and possibly subsequent
.reffunc va_arg
calls). The
.id &funcb.
function does not invoke the
.reffunc va_end
macro.
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it accepts a
wide character string argument for
.arg format
.period
.do end
.desc end
.*
.return begin
The
.id &funcb.
function returns
.kw EOF
if an input failure occurred before any conversion or if there was
a runtime-constraint violation.
Otherwise, the
.id &funcb.
function returns the number of input items
successfully assigned, which can be fewer than provided for, or even zero.
.np
When a file input error occurs, the
.kw errno
global variable may be set.
.return end
.*
.see begin
.im seevscnf
.see end
.*
.exmp begin
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdarg.h>

void find( char *format, ... )
{
    va_list arglist;
.exmp break
    va_start( arglist, format );
    vscanf_s( format, arglist );
    va_end( arglist );
}
.exmp break
void main( void )
{
    int day, year;
    char weekday[10], month[10];
.exmp break
    find( "%s %s %d %d",
            weekday, sizeof( weekday ),
            month, sizeof( month ),
            &day, &year );
    printf_s( "\n%s, %s %d, %d\n",
            weekday, month, day, year );
}
.exmp end
.*
.class TR 24731
.system
