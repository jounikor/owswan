.func getche
.synop begin
#include <conio.h>
int getche( void );
.ixfunc2 '&KbIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function obtains the next available keystroke from the console.
The function will wait until a keystroke is available.
That character is echoed on the screen at the position of the cursor
(use
.reffunc getch
when it is not desired to echo the keystroke).
.pp
The
.reffunc kbhit
function can be used to determine if a keystroke is available.
.desc end
.return begin
A value of
.kw EOF
is returned when an error is detected; otherwise,
the
.id &funcb.
function returns the value of the keystroke (or character).
.pp
When the keystroke represents an extended function key (for example, a
function key, a cursor-movement key or the ALT key with a letter or a
digit),
.if '&machsys' eq 'QNX' .do begin
0xff
.do end
.el .do begin
zero
.do end
is returned and the next call to
.id &funcb.
returns a value for the
extended function.
.return end
.see begin
.im seeiocon
.see end
.exmp begin
#include <stdio.h>
#include <conio.h>

void main()
  {
    int c;
.exmp break
    printf( "Press any key\n" );
    c = getche();
    printf( "You pressed %c(%d)\n", c, c );
  }
.exmp end
.class WATCOM
.system
