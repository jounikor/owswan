.func cwait _cwait
.synop begin
#include <process.h>
int cwait( int *status, int process_id, int action );
.ixfunc2 '&OS2Func' &funcb
.ixfunc2 '&NTFunc' &funcb
.if &'length(&_func.) ne 0 .do begin
int _cwait( int *status, int process_id, int action );
.ixfunc2 '&OS2Func' &_func
.ixfunc2 '&NTFunc' &_func
.do end
.synop end
.desc begin
The
.id &funcb.
function suspends the calling process until the specified
process terminates.
.im waitstat
.np
The
.arg process_id
argument specifies which process to wait for.
Under Win32, any process can wait for any other process for which the
process ID is known.
Under OS/2, a process can wait for any of its child processes.
For example, a process ID is returned by certain forms of the
.reffunc spawn&grpsfx
functions that is used to start a child process.
.np
The
.arg action
argument specifies when the parent process resumes execution.
This argument is ignored in Win32, but is accepted for compatibility
with OS/2 (although Microsoft handles the
.arg status
value differently from OS/2!).
The possible values are:
.begterm 17
.termhd1 Value
.termhd2 Meaning
.term WAIT_CHILD
Wait until the specified child process has ended.
.term WAIT_GRANDCHILD
Wait until the specified child process and all of the child processes
of that child process have ended.
.endterm
.np
Under Win32, there is no parent-child relationship.
.desc end
.return begin
The
.id &funcb.
function returns the (child's) process ID if the (child)
process terminated normally.
Otherwise,
.id &funcb.
returns &minus.1 and sets
.kw errno
to one of the following values:
.begterm 10
.termhd1 Constant
.termhd2 Meaning
.term EINVAL
Invalid action code
.term ECHILD
Invalid process ID, or the child does not exist.
.term EINTR
The child process terminated abnormally.
.endterm
.return end
.see begin
.seelist cwait exit _Exit _exit spawn&grpsfx wait
.see end
.exmp begin
#include <stdio.h>
#include <process.h>
.exmp break
void main()
  {
     int   process_id;
     int   status;
.exmp break
     process_id = spawnl( P_NOWAIT, "child.exe",
                "child", "parm", NULL );
     cwait( &status, process_id, WAIT_CHILD );
  }
.exmp end
.class WATCOM
.system
