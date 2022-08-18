.func ftell
.synop begin
#include <stdio.h>
long int ftell( FILE *fp );
.ixfunc2 '&StrIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function returns the current read/write position
of the file specified by
.arg fp
.period
This position defines the character that will be read or written by the
next I/O operation on the file.
The value returned by
.id &funcb.
can be used in a subsequent call to
.reffunc fseek
to set the file to the same position.
.desc end
.return begin
The
.id &funcb.
function returns the current read/write position
of the file specified by
.arg fp
.period
When an error is detected,
.mono -1L
is returned.
.im errnoref
.return end
.see begin
.seelist ftell fgetpos fopen fsetpos fseek
.see end
.exmp begin
#include <stdio.h>
.exmp break
long int filesize( FILE *fp )
  {
    long int save_pos, size_of_file;
.exmp break
    save_pos = ftell( fp );
    fseek( fp, 0L, SEEK_END );
    size_of_file = ftell( fp );
    fseek( fp, save_pos, SEEK_SET );
    return( size_of_file );
  }
.exmp break
void main()
  {
    FILE *fp;

    fp = fopen( "file", "r" );
    if( fp != NULL ) {
      printf( "File size=%ld\n", filesize( fp ) );
      fclose( fp );
    }
  }
.exmp end
.class ISO C
.system
