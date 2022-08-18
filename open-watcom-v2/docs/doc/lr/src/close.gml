.func close _close
.synop begin
#include <&iohdr>
int close( int &fd );
.ixfunc2 '&OsIo' &funcb
.if &'length(&_func.) ne 0 .do begin
int _close( int &fd );
.ixfunc2 '&OsIo' &_func
.do end
.synop end
.desc begin
The
.id &funcb.
function closes a file at the operating system level.
The
.arg &fd
value is the file &handle returned by a successful execution of one of
the
.if '&machsys' eq 'QNX' .do begin
.reffunc creat
.ct ,
.reffunc dup
.ct ,
.reffunc dup2
.ct ,
.reffunc fcntl
.ct ,
.reffunc open
or
.reffunc _sopen
.do end
.el .do begin
.reffunc creat
.ct ,
.reffunc dup
.ct ,
.reffunc dup2
.ct ,
.reffunc open
or
.reffunc _sopen
.do end
functions.
.im ansiconf
.desc end
.return begin
The
.id &funcb.
function returns zero if successful.
Otherwise, it returns &minus.1 and
.kw errno
is set to indicate the error.
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term EBADF
The
.arg &fd
argument is not a valid file &handle..
.if '&machsys' eq 'QNX' .do begin
.term EINTR
The
.id &funcb.
function was interrupted by a signal.
.term EIO
An i/o error occurred while updating the directory information.
.term ENOSPC
A previous buffered write call has failed.
.do end
.endterm
.error end
.see begin
.if '&machsys' eq 'QNX' .do begin
.seelist creat dup dup2 fcntl open _sopen
.do end
.el .do begin
.seelist creat dup dup2 open _sopen
.do end
.see end
.exmp begin
#include <fcntl.h>
#include <&iohdr>

void main()
  {
    int &fd;
.exmp break
    &fd = open( "file", O_RDONLY );
    if( &fd != -1 ) {
      /* process file */
      close( &fd );
    }
  }
.exmp end
.class POSIX 1003.1
.system
