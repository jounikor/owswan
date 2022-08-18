.func begin ctime Functions
.func2 ctime
.func2 _ctime
.func2 _wctime
.func2 __wctime
.func end
.synop begin
#include <time.h>
char *ctime( const time_t *timer );
char *_ctime( const time_t *timer, char *buf );
.ixfunc2 '&TimeFunc' ctime
.ixfunc2 '&TimeFunc' _ctime
.if &'length(&wfunc.) ne 0 .do begin
wchar_t *_wctime( const time_t *timer );
wchar_t *__wctime( const time_t *timer, wchar_t *buf );
.ixfunc2 '&TimeFunc' _wctime
.ixfunc2 '&Wide' _wctime
.ixfunc2 '&TimeFunc' __wctime
.ixfunc2 '&Wide' __wctime
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.idbold &funcb.
functions convert the calendar time pointed to by
.arg timer
to local time in the form of a string.
The
.idbold &funcb.
function is equivalent to
.millust begin
asctime( localtime( timer ) )
.millust end
.np
The
.idbold &funcb.
functions convert the time into a string containing exactly
26 characters.
This string has the form shown in the following example:
.millust begin
Sat Mar 21 15:58:27 1987\n\0
.millust end
.pc
All fields have a constant width.
The new-line character
.id '\n'
and the null character
.id '\0'
occupy the last two positions of the string.
.np
The ISO C function
.idbold &funcb.
places the result string in a static buffer
that is re-used each time
.idbold &funcb.
or
.reffunc asctime
is called.
The non-ISO C function
.reffunc _ctime
places the result string in the buffer pointed to by
.arg buf
.period
.if &'length(&wfunc.) ne 0 .do begin
.np
.widefunc _wctime &funcb. <ret>
.widefunc __wctime _ctime <ret>
.do end
.np
Whenever the
.idbold &funcb.
functions are called, the
.reffunc tzset
function is also called.
.np
The calendar time is usually obtained by using the
.reffunc time
function.
That time is Coordinated Universal Time (UTC) (formerly known as
Greenwich Mean Time (GMT)).
.im tzref
.desc end
.return begin
The
.idbold &funcb.
functions return the pointer to the string containing the
local time.
.return end
.see begin
.im seetime
.seelist ctime_s
.see end
.exmp begin
#include <stdio.h>
#include <time.h>
.exmp break
void main()
{
    time_t time_of_day;
    auto char buf[26];
.exmp break
    time_of_day = time( NULL );
    printf( "It is now: %s", _ctime( &time_of_day, buf ) );
}
.exmp output
It is now: Fri Dec 25 15:58:42 1987
.exmp end
.class ISO C
.system
