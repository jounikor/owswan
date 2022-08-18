.func vsnprintf_s vsnwprintf_s
.synop begin
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdarg.h>
#include <stdio.h>
int vsnprintf_s( char * restrict s, rsize_t n
           const char * restrict format, va_list arg );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdarg.h>
#include <wchar.h>
int vsnwprintf_s( char * restrict s, rsize_t n,
         const wchar_t * restrict format, va_list arg );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.*
.rtconst begin
Neither
.arg s
nor
.arg format
shall be a null pointer. The
.arg n
argument shall neither equal zero nor be greater than
.mono RSIZE_MAX.
The number of characters (including the trailing null) required for the
result to be written to the array pointed to by
.arg s
shall not be greater than
.arg n
.period
The
.mono %n
specifier (modified or not by flags, field width, or precision) shall not
appear in the string pointed to by
.arg format
.period
Any argument to
.id &funcb.
corresponding to a
.mono %s
specifier shall not be a null pointer. No encoding error shall occur.
.np
If there is a runtime-constraint violation, then if
.arg s
is not a null pointer and
.arg n
is greater than zero and less than
.mono RSIZE_MAX,
then the
.id &funcb.
function sets
.arg s[0]
to the null character.
.rtconst end
.*
.desc begin
The
.id &funcb.
function is equivalent to the
.reffunc vsnprintf
function except for the explicit runtime-constraints listed above.
.np
The
.id &funcb.
function, unlike
.reffunc vsprintf_s
.ct , will truncate the result to fit within the array pointed to by
.arg s
.period
.widefunc &wfunc. &funcb. <form>
.desc end
.*
.return begin
The
.id &funcb.
function returns the number of characters that would have been
written had
.arg n
been sufficiently large, not counting the terminating null character, or a
negative value if a runtime-constraint violation occurred. Thus, the
null-terminated output has been completely written if and only if the
returned value is nonnegative and less than
.arg n
.period
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &funcb.
function returns the number of wide characters that would have been
written had
.arg n
been sufficiently large, not counting the terminating wide null character, or
a negative value if a runtime-constraint violation occurred. Thus, the
null-terminated output has been completely written if and only if the
returned value is nonnegative and less than
.arg n
.period
.return end
.*
.see begin
.im seeprtf
.see end
.*
.exmp begin
.blktext begin
The following shows the use of
.id &funcb.
in a general error message routine.
.blktext end
.blkcode begin
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

char *fmtmsg( char *format, ... )
{
    char    *msgbuf;
    int     len;
    va_list arglist;

    va_start( arglist, format );
    len = vsnprintf( NULL, 0, format, arglist );
    va_end( arglist );
    len = len + 1 + 7;
    msgbuf = malloc( len );
    strcpy( msgbuf, "Error: " );
    va_start( arglist, format );
    vsnprintf_s( &msgbuf[7], len, format, arglist );
    va_end( arglist );
    return( msgbuf );
}

void main( void )
{
    char *msg;

    msg = fmtmsg( "%s %d %s", "Failed", 100, "times" );
    printf_s( "%s\n", msg );
    free( msg );
}
.blkcode end
.exmp end
.*
.class TR 24731
.system
