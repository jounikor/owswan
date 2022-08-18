.func pclose
.synop begin
#include <stdio.h>
int pclose( FILE *fp );
.synop end
.desc begin
The
.id &funcb.
function closes the pipe associated with
.arg fp
and waits for the subprocess created by
.reffunc popen
to terminate.
.desc end
.return begin
The
.id &funcb.
function returns the termination status of the command
language interpreter.
If an error occured,
.id &funcb.
returns (-1) with
.kw errno
set appropriately.
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term EINTR
The
.id &funcb.
function was interrupted by a signal while waiting for the
child process to terminate.
.term ECHILD
The
.id &funcb.
function was unable to obtain the termination status of the
child process.
.endterm
.error end
.see begin
.seelist perror pipe popen
.see end
.seexmp popen
.class POSIX 1003.1
.system
