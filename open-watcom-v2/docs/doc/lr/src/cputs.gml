.func cputs
.synop begin
#include <conio.h>
int cputs( const char *buf );
.ixfunc2 '&KbIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function writes the character string pointed to by
.arg buf
directly to the console using the
.reffunc putch
function.
Unlike the
.reffunc puts
function, the carriage-return and line-feed characters are not appended
to the string.
The terminating null character is not written.
.desc end
.return begin
The &funcb
function returns a non-zero value if an error occurs; otherwise, it
returns zero.
.im errnoref
.return end
.see begin
.seelist fputs putch puts
.see end
.exmp begin
#include <conio.h>

void main()
  {
    char buffer[82];
.exmp break
    buffer[0] = 80;
    cgets( buffer );
    cputs( &buffer[2] );
    putch( '\r' );
    putch( '\n' );
  }
.exmp end
.class WATCOM
.system
