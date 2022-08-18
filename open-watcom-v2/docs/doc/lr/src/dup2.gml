.func dup2 _dup2
.synop begin
#include <&iohdr>
int dup2( int &fd, int &fd.2 );
.ixfunc2 '&OsIo' &funcb
.if &'length(&_func.) ne 0 .do begin
int _dup2( int &fd, int &fd.2 );
.ixfunc2 '&OsIo' &_func
.do end
.synop end
.desc begin
The
.id &funcb.
function duplicates the file &handle given by the argument
.arg &fd
.period
The new file &handle is identical to the original in that it references
the same file or device, it has the same open mode (read and/or write)
and it will have identical file position to the original (changing the
position with one &handle will result in a changed position in the
other).
.pp
The number of the new &handle is
.arg &fd.2
.period
If a file already is opened with this &handle, the file is closed
before the duplication is attempted.
.im ansiconf
.if '&machsys' eq 'QNX' .do begin
.pp
The call
.millust begin
    dup_&fd = dup2( &fd, &fd.2 );
.millust end
.pc
is equivalent to:
.millust begin
    close( &fd.2 );
    dup_&fd = fcntl( &fd, F_DUPFD, &fd.2 );
.millust end
.do end
.desc end
.return begin
.if '&machsys' eq 'QNX' .do begin
The
.id &funcb.
function returns the value of
.arg &fd.2
if successful.
.do end
.el .do begin
The
.id &funcb.
function returns zero if successful.
.do end
Otherwise, &minus.1 is returned and
.kw errno
is set to indicate the error.
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term EBADF
The argument
.arg &fd
is not a valid open file &handle
or
.arg &fd.2
is out of range.
.term EMFILE
The number of file &handle.s would exceed {OPEN_MAX}, or
no file &handle.s above
.arg &fd.2
are available.
.endterm
.error end
.see begin
.im seeioos
.see end
.exmp begin
#include <fcntl.h>
#include <&iohdr>

void main()
  {
    int &fd, dup_&fd;

    &fd = open( "file",
.if '&machsys' eq 'QNX' .do begin
                O_WRONLY | O_CREAT | O_TRUNC,
.do end
.el .do begin
                O_WRONLY | O_CREAT | O_TRUNC | O_TEXT,
.do end
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
    if( &fd != -1 ) {
      dup_&fd = 4;
      if( dup2( &fd, dup_&fd ) != -1 ) {

        /* process file */
.exmp break
        close( dup_&fd );
      }
      close( &fd );
    }
  }
.exmp end
.class POSIX 1003.1
.system
