.func feof
.synop begin
#include <stdio.h>
int feof( FILE *fp );
.ixfunc2 '&StrIo' &funcb
.ixfunc2 '&Errs' &funcb
.synop end
.desc begin
The
.id &funcb.
function tests the end-of-file indicator for the stream
pointed to by
.arg fp
.period
Because this indicator is set when an input operation attempts to read
past the end of the file the &funcb
function will detect the end of the file only after an attempt
is made to read beyond the end of the file.
Thus, if a file contains 10 lines, the
.id &funcb.
will not detect
end of file after the tenth line is read; it will detect end of file
once the program attempts to read more data.
.desc end
.return begin
The
.id &funcb.
function returns non-zero
if the end-of-file indicator is set for
.arg fp
.period
.return end
.see begin
.seelist clearerr feof ferror fopen freopen perror read strerror
.see end
.exmp begin
#include <stdio.h>

void process_record( char *buf )
  {
    printf( "%s\n", buf );
  }
.exmp break
void main()
  {
    FILE *fp;
    char buffer[100];
.exmp break
    fp = fopen( "file", "r" );
    fgets( buffer, sizeof( buffer ), fp );
    while( ! feof( fp ) ) {
      process_record( buffer );
      fgets( buffer, sizeof( buffer ), fp );
    }
    fclose( fp );
  }
.exmp end
.class ISO C
.system
