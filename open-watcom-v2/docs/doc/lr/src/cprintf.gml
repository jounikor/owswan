.func cprintf
.synop begin
#include <conio.h>
int cprintf( const char *format, ... );
.ixfunc2 '&KbIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function writes output directly to the console under control
of the argument
.arg format
.period
The
.reffunc putch
function is used to output characters to the console.
The
.arg format
string is described under the description of the
.reffunc printf
function.
.desc end
.return begin
The
.id &funcb.
function returns the number of characters written.
.return end
.see begin
.im seeprtf
.see end
.exmp begin
#include <conio.h>

void main()
  {
    char *weekday, *month;
    int day, year;
.exmp break
    weekday = "Saturday";
    month = "April";
    day = 18;
    year = 1987;
    cprintf( "%s, %s %d, %d\n",
          weekday, month, day, year );
  }
.exmp output
Saturday, April 18, 1987
.exmp end
.class WATCOM
.system
