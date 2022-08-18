.func begin asctime Functions
.func2 asctime
.func2 _asctime
.func2 _wasctime
.func2 __wasctime
.func end
.synop begin
#include <time.h>
char * asctime( const struct tm *timeptr );
char *_asctime( const struct tm *timeptr, char *buf );
.ixfunc2 '&TimeFunc' asctime
.ixfunc2 '&TimeFunc' _asctime
.if &'length(&wfunc.) ne 0 .do begin
wchar_t *_wasctime( const struct tm *timeptr );
wchar_t *__wasctime( const struct tm *timeptr, wchar_t *buf );
.ixfunc2 '&TimeFunc' _wasctime
.ixfunc2 '&TimeFunc' __wasctime
.ixfunc2 '&Wide' _wasctime
.ixfunc2 '&Wide' __wasctime
.do end
.im structtm
.synop end
.*
.safealt
.*
.desc begin
The
.idbold &funcb.
functions convert the time information in the structure
pointed to by
.arg timeptr
into a string containing exactly 26 characters.
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
.reffunc ctime
is called.
The non-ISO C function
.reffunc _asctime
places the result string in the buffer pointed to by
.arg buf
.period
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.reffunc _wasctime
and
.reffunc __wasctime
functions are identical to their
.reffunc asctime
and
.reffunc _asctime
counterparts except that they deal with wide character strings.
.do end
.desc end
.return begin
The
.idbold &funcb.
functions return a pointer to the character string result.
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
    struct tm  time_of_day;
    time_t     ltime;
    auto char  buf[26];
.exmp break
    time( &ltime );
    _localtime( &ltime, &time_of_day );
    printf( "Date and time is: %s\n",
            _asctime( &time_of_day, buf ) );
  }
.exmp output
Date and time is: Sat Mar 21 15:58:27 1987
.exmp end
.class ISO C
.system
