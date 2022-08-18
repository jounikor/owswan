.func begin fprintf
.func2 fwprintf ISO C95
.func end
.synop begin
#include <stdio.h>
int fprintf( FILE *fp, const char *format, ... );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdio.h>
#include <wchar.h>
int fwprintf( FILE *fp, const wchar_t *format, ... );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function writes output to the file pointed to by
.arg fp
under control of the argument
.arg format
.period
The
.arg format
string is described under the description of the
.reffunc printf
function.
.widefunc &wfunc. &funcb. <form>
.desc end
.*
.return begin
The
.id &funcb.
function returns the number of characters written, or a
negative value if an output error occurred.
.if &'length(&wfunc.) ne 0 .do begin
The
.id &wfunc.
function returns the number of wide characters written, or a
negative value if an output error occurred.
.do end
.im errnoref
.return end
.*
.see begin
.im seeprtf
.see end
.*
.exmp begin
#include <stdio.h>

char *weekday = { "Saturday" };
char *month = { "April" };

void main( void )
{
    fprintf( stdout, "%s, %s %d, %d\n",
          weekday, month, 18, 1987 );
}
.exmp output
Saturday, April 18, 1987
.exmp end
.class ISO C
.system
