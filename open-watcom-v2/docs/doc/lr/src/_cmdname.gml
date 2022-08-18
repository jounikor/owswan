.func _cmdname
.synop begin
#include <process.h>
char *_cmdname( char *buffer );
.ixfunc2 '&String' &funcb
.synop end
.desc begin
The
.id &funcb.
function obtains a copy of the executing program's pathname
and places it in
.arg buffer
.period
.desc end
.return begin
If the pathname of the executing program cannot be determined then
.mono NULL
is returned; otherwise the address of
.arg buffer
is returned.
.return end
.see begin
.seelist getcmd
.see end
.exmp begin
#include <stdio.h>
#include <process.h>

void main()
  {
    char buffer[PATH_MAX];
.exmp break
    printf( "%s\n", _cmdname( buffer ) );
  }
.exmp end
.class WATCOM
.system
