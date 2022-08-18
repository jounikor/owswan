.func abort
.synop begin
#include <stdlib.h>
void abort( void );
.ixfunc2 '&Process' &funcb
.synop end
.desc begin
The
.id &funcb.
function raises the signal SIGABRT.
The default action for SIGABRT is to terminate program execution,
returning control
to the process that started the calling program (usually the operating
system).
The status
.us unsuccessful termination
is returned to the invoking process by means of the function call
.mono raise(SIGABRT).
.if '&machsys' eq 'QNX' .do begin
Under QNX, the status value is 12.
.do end
.el .do begin
The exit code returned to the invoking process is
.kw EXIT_FAILURE
which is defined in the
.hdrfile stdlib.h
header file.
.do end
.desc end
.return begin
The
.id &funcb.
function does not return to its caller.
.return end
.see begin
.im seeproc
.see end
.exmp begin
#include <stdlib.h>

void main()
  {
    int major_error = 1;
.exmp break
    if( major_error )
      abort();
  }
.exmp end
.class ISO C
.system
