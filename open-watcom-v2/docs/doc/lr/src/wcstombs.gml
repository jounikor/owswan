.func wcstombs _fwcstombs
.synop begin
#include <stdlib.h>
size_t wcstombs( char *s, const wchar_t *pwcs, size_t n );
.ixfunc2 '&Wide' &funcb
.ixfunc2 '&Multibyte' &funcb
.if &farfnc ne 0 .do begin
#include <mbstring.h>
size_t _fwcstombs( char __far *s,
                   const wchar_t __far *pwcs,
                   size_t n );
.ixfunc2 '&Wide' &ffunc
.ixfunc2 '&Multibyte' &ffunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function converts a sequence of wide character codes from the
array pointed to by
.arg pwcs
into a sequence of multibyte characters and stores them in the
array pointed to by
.arg s
.period
The
.id &funcb.
function stops if a multibyte character would exceed the limit of
.arg n
total bytes, or if the null character is stored.
At most
.arg n
bytes of the array pointed to by
.arg s
will be modified.
.farfuncp &ffunc. &funcb.
.desc end
.return begin
.im _mbsret3
Otherwise, the
.id &funcb.
function returns the number of array elements
modified, not including the terminating zero code if present.
.return end
.see begin
.seelist wcstombs wcstombs_s mblen mbtowc mbstowcs mbstowcs_s wctomb wctomb_s
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>
.exmp break
wchar_t wbuffer[] = {
    0x0073,
    0x0074,
    0x0072,
    0x0069,
    0x006e,
    0x0067,
    0x0000
  };
.exmp break
void main()
  {
    char    mbsbuffer[50];
    int     i, len;
.exmp break
    len = wcstombs( mbsbuffer, wbuffer, 50 );
    if( len != -1 ) {
      for( i = 0; i < len; i++ )
        printf( "/%4.4x", wbuffer[i] );
      printf( "\n" );
      mbsbuffer[len] = '\0';
      printf( "%s(%d)\n", mbsbuffer, len );
    }
  }
.exmp output
/0073/0074/0072/0069/006e/0067
string(6)
.exmp end
.class ISO C
.system
