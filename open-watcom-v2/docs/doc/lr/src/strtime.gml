.func _strtime _wstrtime
.synop begin
#include <time.h>
char *_strtime( char *timestr )
.ixfunc2 '&Conversion' &funcb
.if &'length(&wfunc.) ne 0 .do begin
wchar_t _wstrtime( wchar_t *timestr );
.ixfunc2 '&Conversion' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function copies the current time to the buffer pointed to by
.arg timestr
.period
The time is formatted as "HH:MM:SS"
where "HH" is two digits representing the hour in 24-hour notation,
where "MM" is two digits representing the minutes past the hour, and
where "SS" is two digits representing seconds.
The buffer must be at least 9 bytes long.
.widefunc &wfunc. &funcb.
.desc end
.return begin
The
.id &funcb.
function returns a pointer to the resulting text string
.arg timestr
.period
.return end
.see begin
.seelist asctime Functions ctime Functions gmtime localtime mktime
.seelist _strdate _strtime time tzset
.see end
.exmp begin
#include <stdio.h>
#include <time.h>

void main()
  {
    char timebuff[9];
.exmp break
    printf( "%s\n", _strtime( timebuff ) );
  }
.exmp end
.class WATCOM
.system
