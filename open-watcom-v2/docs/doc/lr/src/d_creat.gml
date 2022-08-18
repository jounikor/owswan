.func _dos_creat
.synop begin
#include <&doshdr>
unsigned _dos_creat( const char *path,
                     unsigned attribute,
                     int *&fd );
.ixfunc2 '&DosFunc' &funcb
.ixfunc2 '&OsIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function uses system call 0x3C to create a new file named
.arg path
.ct , with the access attributes specified by
.arg attribute
.period
The &handle for the new file is returned in the word pointed to by
.arg &fd
.period
If the file already exists, the contents will be erased, and the
attributes of the file will remain unchanged.
.im creatatt
.desc end
.return begin
The
.id &funcb.
function returns zero if successful.
Otherwise, it returns an OS error code and sets
.kw errno
accordingly.
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
    int &fd;
.exmp break
    if( _dos_creat( "file", _A_NORMAL, &amp.&fd ) != 0 ){
      printf( "Unable to create file\n" );
    } else {
      printf( "Create succeeded\n" );
      _dos_close( &fd );
    }
  }
.exmp end
.class DOS
.system
