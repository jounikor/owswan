.func _chsize chsize
.ansiname _chsize
.synop begin
#include <&iohdr>
int _chsize( int &fd, long size );
.ixfunc2 '&DosFunc' _chsize

.deprec
int chsize( int &fd, long size );
.ixfunc2 '&DosFunc' chsize
.synop end
.desc begin
The
.id &funcb.
function changes the size of the file associated with
.arg &fd
by extending or truncating the file to the length specified by
.arg size
.period
If the file needs to be extended, the file is padded with NULL ('\0')
characters.
.if '&machsys' eq 'QNX' .do begin
.np
Note that the
.id &funcb.
function call ignores advisory locks which may
have been set by the
.reffunc fcntl
.ct ,
.reffunc lock
.ct , or
.reffunc locking
functions.
.do end
.np
.deprfunc chsize _chsize
.desc end
.return begin
The
.id &funcb.
function returns zero if successful.
A return value of -1 indicates an error, and
.kw errno
is set to indicate the error.
.return end
.error begin
.begterm 12 $compact
.termhd1 Constant
.termhd2 Meaning
.term EACCES
The specified file is locked against access.
.term EBADF
.if '&machsys' eq 'QNX' .do begin
Invalid file &handle.. or file not opened for write.
.do end
.el .do begin
Invalid file &handle..
.do end
.term ENOSPC
Not enough space left on the device to extend the file.
.endterm
.error end
.see begin
.seelist close creat open
.see end
.exmp begin
#include <stdio.h>
#include <&iohdr>
#include <fcntl.h>
#include <sys/stat.h>

void main()
  {
    int  &fd;
.exmp break
    &fd = open( "file", O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
    if( &fd != -1 ) {
      if( _chsize( &fd, 32 * 1024L ) != 0 ) {
          printf( "Error extending file\n" );
      }
      close( &fd );
    }
  }
.exmp end
.class WATCOM
.system
