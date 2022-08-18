.func _dos_creatnew
.synop begin
#include <&doshdr>
unsigned _dos_creatnew( const char *path,
                        unsigned attribute,
                        int *&fd );
.ixfunc2 '&DosFunc' &funcb
.ixfunc2 '&OsIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function uses system call 0x5B to create a new file named
.arg path
.ct , with the access attributes specified by
.arg attribute
.period
The &handle for the new file is returned in the word pointed to by
.arg &fd.
.period
If the file already exists, the create will fail.
.im creatatt
.desc end
.return begin
The
.id &funcb.
function returns zero if successful.
Otherwise, it returns an OS error code and sets
.kw errno
.period
Possible values and their interpretations:
.begterm 10
.termhd1 Constant
.termhd2 Meaning
.term EACCES
Access denied because the directory is full, or the file exists and
cannot be overwritten.
.term EEXIST
File already exists
.term EMFILE
No more &handle.s available (i.e., too many open files)
.term ENOENT
Path or file not found
.endterm
.return end
.see begin
.seelist creat _dos_creat _dos_creatnew _dos_open _dos_close
.seelist open fdopen fopen freopen _fsopen _grow_handles
.seelist _hdopen open _open_osfhandle _popen _sopen
.see end
.exmp begin
#include <stdio.h>
#include <&doshdr>
.exmp break
void main()
  {
    int &fd.1, &fd.2;
    if( _dos_creat( "file", _A_NORMAL, &amp.&fd.1 ) ){
      printf( "Unable to create file\n" );
    } else {
      printf( "Create succeeded\n" );
      if( _dos_creatnew( "file", _A_NORMAL, &amp.&fd.2 ) ){
        printf( "Unable to create new file\n" );
      }
      _dos_close( &fd.1 );
    }
  }
.exmp end
.class DOS
.system
