.func _dos_commit
.synop begin
#include <dos.h>
unsigned _dos_commit( int handle );
.synop end
.desc begin
The
.id &funcb.
function uses system call 0x68 to flush to disk the DOS
buffers associated with the file indicated by
.arg handle
.period
It also forces an update on the corresponding disk directory and the
file allocation table.
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
.seelist _dos_commit _dos_close _dos_creat _dos_open _dos_write fflush
.see end
.exmp begin
#include <stdio.h>
#include <dos.h>
#include <fcntl.h>

void main()
  {
    int handle;
.exmp break
    if( _dos_open( "file", O_RDONLY, &handle ) != 0 ) {
        printf( "Unable to open file\n" );
    } else {
        if( _dos_commit( handle ) == 0 ) {
            printf( "Commit succeeded.\n" );
        }
        _dos_close( handle );
    }
  }
.exmp output
Commit succeeded.
.exmp end
.class DOS
.system
