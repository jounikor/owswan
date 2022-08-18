.func _localtime
.synop begin
#include <time.h>
struct tm *_localtime( const time_t *timer,
                       struct tm *tmbuf );
.ixfunc2 '&TimeFunc' &funcb
.im structtm
.synop end
.desc begin
The
.id &funcb.
function converts the calendar time pointed to by
.arg timer
into a structure of type
.kw tm
.ct , of time information, expressed as local time.
The information is stored in the structure pointed to by
.arg tmbuf
.period
Whenever
.id &funcb.
is called, the
.reffunc tzset
function is also called.
.pp
The calendar time is usually obtained by using the
.reffunc time
function.
That time is Coordinated Universal Time (UTC) (formerly known as
Greenwich Mean Time (GMT)).
.im tzref
.desc end
.return begin
The
.id &funcb.
function returns a pointer to a
.kw tm
structure containing the time information.
.return end
.see begin
.im seetime
.see end
.exmp begin
#include <stdio.h>
#include <time.h>
.exmp break
void main()
  {
    time_t time_of_day;
    auto char buf[26];
    auto struct tm tmbuf;
.exmp break
    time_of_day = time( NULL );
    _localtime( &time_of_day, &tmbuf );
    printf( "It is now: %s", _asctime( &tmbuf, buf ) );
  }
.exmp output
It is now: Sat Nov 30 15:58:27 1991
.exmp end
.class WATCOM
.system
