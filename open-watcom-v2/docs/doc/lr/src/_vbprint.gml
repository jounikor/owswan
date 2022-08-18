.func _vbprintf _vbwprintf
.synop begin
#include <stdio.h>
#include <stdarg.h>
int _vbprintf( char *buf, size_t bufsize,
               const char *format, va_list arg );
.ixfunc2 '&String' &funcb
.if &'length(&wfunc.) ne 0 .do begin
int _vbwprintf( wchar_t *buf, size_t bufsize,
                const wchar_t *format, va_list arg );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function formats data under control of the
.arg format
control string and writes the result to
.arg buf
.period
The argument
.arg bufsize
specifies the size of the character array
.arg buf
into which the generated output is placed.
The
.arg format
string is described under the description of the
.reffunc printf
function.
The
.id &funcb.
function is equivalent to the
.reffunc _bprintf
function, with the variable argument list replaced with
.arg arg
.ct , which has been initialized by the
.reffunc va_start
macro.
.widefunc &wfunc. &funcb. <form>
.desc end
.return begin
The
.id &funcb.
function returns the number of characters written, or a
negative value if an output error occurred.
.return end
.see begin
.im seevprtf
.see end
.exmp begin
.blktext begin
The following shows the use of
.id &funcb.
in a general error message routine.
.blktext end
.blkcode begin
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

char msgbuf[80];
.exmp break
char *fmtmsg( char *format, ... )
  {
    va_list arglist;

    va_start( arglist, format );
    strcpy( msgbuf, "Error: " );
    _vbprintf( &msgbuf[7], 73, format, arglist );
    va_end( arglist );
    return( msgbuf );
  }
.exmp break
void main()
  {
    char *msg;

    msg = fmtmsg( "%s %d %s", "Failed", 100, "times" );
    printf( "%s\n", msg );
  }
.blkcode end
.exmp end
.class WATCOM
.system
