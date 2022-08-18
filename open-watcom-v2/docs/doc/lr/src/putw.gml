.func _putw
.synop begin
#include <stdio.h>
int _putw( int binint, FILE *fp );
.ixfunc2 '&StrIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function writes a binary value of type
.us int
to the current position of the stream
.arg fp
.period
.id &funcb.
does not affect the alignment of items in the stream, nor does
it assume any special alignment.
.np
.id &funcb.
is provided primarily for compatibility with previous libraries.
Portability problems may occur with
.id &funcb.
because the size of an
.us int
and the ordering of bytes within an
.us int
differ across systems.
.desc end
.return begin
The
.id &funcb.
function returns the value written or, if a write error
occurs, the error indicator is set and
.id &funcb.
returns
.kw EOF
.period
Since
.kw EOF
is a legitimate value to write to
.arg fp
.ct , use
.reffunc ferror
to verify that an error has occurred.
.return end
.see begin
.seelist ferror fopen fputc fputchar fputs
.seelist putc putchar puts _putw
.see end
.exmp begin
#include <stdio.h>

void main()
  {
    FILE *fp;
    int c;
.exmp break
    fp = fopen( "file", "r" );
    if( fp != NULL ) {
      while( (c = _getw( fp )) != EOF )
          _putw( c, stdout );
      fclose( fp );
    }
  }
.exmp end
.class WATCOM
.system
