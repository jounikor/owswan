.func _getdcwd _wgetdcwd
.synop begin
#include <direct.h>
char *_getdcwd( int drive, char *buffer, size_t maxlen );
.ixfunc2 '&Direct' &funcb
.if &'length(&wfunc.) ne 0 .do begin
wchar_t *_wgetdcwd( int drive, wchar_t *buffer, size_t maxlen );
.ixfunc2 '&Direct' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function gets the full path of the current working directory
on the specified drive.
The
.arg drive
argument specifies the drive (0 = default drive, 1 = A, 2 = B, etc.).
The
.arg buffer
address is either
.mono NULL
or is the location at which a string containing the name of the
current working directory is placed.
In the latter case, the value of
.arg maxlen
is the length in characters (including the terminating null character)
which can be be used to store this name.
An error occurs if the length of the path (including the terminating
null character) exceeds
.arg maxlen
.period
.np
The maximum size that might be required for
.arg buffer
is
.kw PATH_MAX
+ 1 bytes.
.np
When
.arg buffer
has a value of
.mono NULL,
a string is allocated using
.reffunc malloc
to contain the name of the current working directory.
This string may be freed using the
.reffunc free
function.
.widefunc &wfunc. &funcb.
.if &'length(&wfunc.) ne 0 .do begin
The
.arg maxlen
is the length in wide characters (wchar_t).
.do end
.desc end
.return begin
The
.id &funcb.
function returns the address of the string containing the
name of the current working directory on the specified drive, unless
an error occurs, in which case
.mono NULL
is returned.
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term ENODEV
The drive cannot be accessed.
.term ENOMEM
Not enough memory to allocate a buffer.
.term ERANGE
The buffer is too small (specified by
.arg size
.ct ) to contain the name of the current working directory.
.endterm
.error end
.see begin
.seelist chdir chmod getcwd _getdcwd mkdir mknod rmdir
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

void main()
  {
    char *cwd;
.exmp break
    cwd = _getdcwd( 3, NULL, 0 );
    if( cwd != NULL ) {
      printf( "The current directory on drive C is %s\n",
              cwd );
      free( cwd );
    }
  }
.exmp output
The current directory on drive C is C:\PROJECT\C
.exmp end
.class WATCOM
.system
