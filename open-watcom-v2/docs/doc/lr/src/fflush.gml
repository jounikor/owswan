.func fflush
.synop begin
#include <stdio.h>
int fflush( FILE *fp );
.ixfunc2 '&StrIo' &funcb
.synop end
.desc begin
If the file
.arg fp
is open for output or update, the &funcb
function causes any unwritten data to be written to the file.
.ix '&StrIo' 'ungetc'
If the file
.arg fp
is open for input or update, the
.id &funcb.
function undoes the effect of
any preceding
.reffunc ungetc
operation on the stream.
If the value of
.arg fp
is
.mono NULL,
then all files that are open will be flushed.
.desc end
.return begin
The
.id &funcb.
function returns
.kw EOF
if a write error occurs and zero otherwise.
.im errnoref
.return end
.see begin
.seelist fflush fgetc fgets flushall fopen getc gets setbuf setvbuf ungetc
.see end
.exmp begin
#include <stdio.h>
#include <conio.h>

void main()
  {
    printf( "Press any key to continue..." );
    fflush( stdout );
    getch();
  }
.exmp end
.class ISO C
.system
