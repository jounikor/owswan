.func _strdate _wstrdate
.synop begin
#include <time.h>
char *_strdate( char *datestr )
.ixfunc2 '&Conversion' &funcb
.if &'length(&wfunc.) ne 0 .do begin
wchar_t _wstrdate( wchar_t *datestr );
.ixfunc2 '&Conversion' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function copies the current date to the buffer pointed to by
.arg datestr
.period
The date is formatted as "MM/DD/YY"
where "MM" is two digits representing the month,
where "DD" is two digits representing the day, and
where "YY" is two digits representing the year.
The buffer must be at least 9 bytes long.
.widefunc &wfunc. &funcb.
.desc end
.return begin
The
.id &funcb.
function returns a pointer to the resulting text string
.arg datestr
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
    char datebuff[9];
.exmp break
    printf( "%s\n", _strdate( datebuff ) );
  }
.exmp end
.class WATCOM
.system
