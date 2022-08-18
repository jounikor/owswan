.func ungetc ungetwc
.synop begin
#include <stdio.h>
int ungetc( int c, FILE *fp );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdio.h>
#include <wchar.h>
wint_t ungetwc( wint_t c, FILE *fp );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function pushes the character specified by
.arg c
back onto the input stream pointed to by
.arg fp
.period
This character will be returned by the next read on the stream.
The pushed-back character will be discarded if a call is made to the
.reffunc fflush
function or to a file positioning function (
.ct .reffunc fseek
.ct ,
.reffunc fsetpos
or
.reffunc rewind
.ct ) before the next read operation is performed.
.np
Only one character (the most recent one) of pushback is remembered.
.np
The
.id &funcb.
function clears the end-of-file indicator, unless the value
of
.arg c
is
.kw EOF
.period
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it pushes the
wide character specified by
.arg c
back onto the input stream pointed to by
.arg fp
.period
.np
The
.id &wfunc.
function clears the end-of-file indicator, unless the value
of
.arg c
is
.kw WEOF
.period
.do end
.desc end
.return begin
The
.id &funcb.
function returns the character pushed back.
.return end
.see begin
.seelist fgetc fgetchar fgets fopen getc getchar gets ungetc
.see end
.exmp begin
#include <stdio.h>
#include <ctype.h>

void main()
  {
    FILE *fp;
    int c;
    long value;
.exmp break
    fp = fopen( "file", "r" );
    value = 0;
    c = fgetc( fp );
    while( isdigit(c) ) {
        value = value*10 + c - '0';
        c = fgetc( fp );
    }
    ungetc( c, fp ); /* put last character back */
    printf( "Value=%ld\n", value );
    fclose( fp );
  }
.exmp end
.class ISO C
.system
