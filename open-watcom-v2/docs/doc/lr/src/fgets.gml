.func fgets fgetws
.synop begin
#include <stdio.h>
char *fgets( char *buf, int n, FILE *fp );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdio.h>
#include <wchar.h>
wchar_t *fgetws( wchar_t *buf, int n, FILE *fp );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function gets a string of characters from the file
designated by
.arg fp
and stores them in the array pointed to by
.arg buf
.period
The
.id &funcb.
function stops reading characters when end-of-file is
reached, or when a newline character is read, or when
.arg n-1
characters have been read, whichever comes first.
The new-line character is not discarded.
A null character is placed immediately after the last character
read into the array.
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it gets a string
of multibyte characters (if present) from the input stream pointed to
by
.arg fp
.ct , converts them to wide characters, and stores them in the
wide character array pointed to by
.arg buf
.period
In this case,
.arg n
specifies the number of wide characters, less one, to be read.
.do end
.np
A common programming error is to assume the presence of a new-line
character in every string that is read into the array.
A new-line character will not be present when more than
.arg n-1
characters occur before the new-line.
Also, a new-line character may not appear as the last character in a
file, just before end-of-file.
.np
The
.reffunc gets
function is similar to
.id &funcb.
except that it operates with
.filename stdin
.ct , it has no size argument, and it replaces a newline character
with the null character.
.desc end
.return begin
The
.id &funcb.
function returns
.arg buf
if successful.
.mono NULL
is returned if end-of-file is encountered, or a read error occurs.
.im errnoref
.return end
.see begin
.seelist fgetc fgetchar fgets fopen getc getchar gets ungetc
.see end
.exmp begin
#include <stdio.h>

void main()
  {
    FILE *fp;
    char buffer[80];
.exmp break
    fp = fopen( "file", "r" );
    if( fp != NULL ) {
      while( fgets( buffer, 80, fp ) != NULL )
        fputs( buffer, stdout );
      fclose( fp );
    }
  }
.exmp end
.class ISO C
.system
