.func _dos_setfileattr
.synop begin
#include <&doshdr>
unsigned _dos_setfileattr( const char *path,
                           unsigned attributes );
.ixfunc2 '&DosFunc' &funcb
.synop end
.desc begin
The
.id &funcb.
function uses system call 0x43 to set the attributes
of the file or directory that
.arg path
points to.
The possible attributes are:
.im dosattr
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
.seelist _dos_setfileattr _dos_getfileattr
.see end
.exmp begin
#include <stdio.h>
#include <&doshdr>
.exmp break
print_attribute()
  {
    unsigned attribute;
.exmp break
    _dos_getfileattr( "file", &attribute );
    printf( "File attribute is %x\n", attribute );
    if( attribute & _A_RDONLY ) {
        printf( "This is a read-only file\n" );
    } else {
        printf( "This is not a read-only file\n" );
    }
  }
.exmp break
void main()
  {
    int      &fd;
.exmp break
    if( _dos_creat( "file", _A_RDONLY, &amp.&fd ) != 0 ){
      printf( "Error creating file\n" );
    }
    print_attribute();
    _dos_setfileattr( "file", _A_NORMAL );
    print_attribute();
    _dos_close( &fd );
  }
.exmp end
.class DOS
.system
