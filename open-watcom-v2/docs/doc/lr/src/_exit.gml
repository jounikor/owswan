.func begin _Exit
.func2 _exit POSIX 1003.1
.func end
.synop begin
#include <stdlib.h>
void _Exit( int status );
void _exit( int status );
.ixfunc2 '&Process' &funcb
.synop end
.desc begin
The
.id &funcb.
function causes normal program termination to occur.
.autonote
.note
The functions registered by the
.reffunc atexit
or
.reffunc _onexit
functions are not called.
.if '&machsys' eq 'QNX' .do begin
.note
All open file descriptors and directory streams in the calling process
are closed.
.note
If the parent process of the calling process is executing a
.reffunc wait
or
.reffunc waitpid
.ct , it is notified of the calling process's termination and the low order
8 bits of
.arg status
are made available to it.
.note
If the parent process of the calling process is not executing a
.reffunc wait
or
.reffunc waitpid
function, the exit
.arg status
code is saved for return to the parent process whenever the parent
process executes an appropriate subsequent
.reffunc wait
or
.reffunc waitpid
.period
.note
Termination of a process does not directly terminate its children.
The sending of a
.kw SIGHUP
signal as described below indirectly terminates children in some
circumstances.
Children of a terminated process shall be assigned a new parent
process ID, corresponding to an implementation-defined system process.
.note
If the implementation supports the
.kw SIGCHLD
signal, a
.kw SIGCHLD
signal shall be sent to the parent process.
.note
If the process is a controlling process, the
.kw SIGHUP
signal will be sent to each process in the foreground process group of
the controlling terminal belonging to the calling process.
.note
If the process is a controlling process, the controlling terminal
associated with the session is disassociated from the session,
allowing it to be acquired by a new controlling process.
.note
If the implementation supports job control, and if the exit of the
process causes a process group to become orphaned, and if any member
of the newly-orphaned process group is stopped, then a
.kw SIGHUP
signal followed by a
.kw SIGCONT
signal will be sent to each process in the newly-orphaned process
group.
.endnote
.pp
These consequences will occur on process termination for any reason.
.do end
.el .do begin
.note
Any unopened files are not closed and any buffered output is not
flushed to the associated files or devices.
.note
Any files created by
.reffunc tmpfile
are not removed.
.note
The return
.arg status
is made available to the parent process.
Only the low order byte of
.arg status
is available on DOS systems.
The
.arg status
value is typically set to 0 to indicate successful termination and
set to some other value to indicate an error.
.endnote
.do end
The
.id _exit
is functionaly equivalent to
.id &funcb.
.period
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
#include <stdio.h>
#include <stdlib.h>

void main( int argc, char *argv[] )
{
    FILE *fp;
.exmp break
    if( argc <= 1 ) {
        fprintf( stderr, "Missing argument\n" );
        exit( EXIT_FAILURE );
    }
.exmp break
    fp = fopen( argv[1], "r" );
    if( fp == NULL ) {
        fprintf( stderr, "Unable to open '%s'\n", argv[1] );
        _Exit( EXIT_FAILURE );
    }
    fclose( fp );
    _Exit( EXIT_SUCCESS );
}
.exmp end
.class ISO C99
.system
