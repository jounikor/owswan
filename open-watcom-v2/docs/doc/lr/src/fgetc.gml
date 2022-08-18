.func fgetc fgetwc
.synop begin
#include <stdio.h>
int fgetc( FILE *fp );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdio.h>
#include <wchar.h>
wint_t fgetwc( FILE *fp );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function gets the next character from the file designated by
.arg fp
.period
The character is
.id signed.
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it gets the next
multibyte character (if present) from the input stream pointed to by
.arg fp
and converts it to a wide character.
.do end
.desc end
.return begin
The
.id &funcb.
function returns the next character from the input stream
pointed to by
.arg fp
.period
If the stream is at end-of-file, the end-of-file indicator is set and
.id &funcb.
returns
.kw EOF
.period
If a read error occurs, the error indicator is set and
.id &funcb.
returns
.kw EOF
.period
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function returns the next wide character from the input
stream pointed to by
.arg fp
.period
If the stream is at end-of-file, the end-of-file indicator is set and
.id &wfunc.
returns
.kw WEOF
.period
If a read error occurs, the error indicator is set and
.id &wfunc.
returns
.kw WEOF
.period
If an encoding error occurs,
.kw errno
is set to
.kw EILSEQ
and
.id &wfunc.
returns
.kw WEOF
.period
.do end
.np
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
    int c;
.exmp break
    fp = fopen( "file", "r" );
    if( fp != NULL ) {
      while( (c = fgetc( fp )) != EOF )
        fputc( c, stdout );
      fclose( fp );
    }
  }
.exmp end
.class ISO C
.system
