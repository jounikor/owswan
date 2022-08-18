.func mbstowcs _fmbstowcs
.synop begin
#include <stdlib.h>
size_t mbstowcs( wchar_t *pwcs, const char *s, size_t n );
.ixfunc2 '&Multibyte' &funcb
.if &farfnc ne 0 .do begin
#include <mbstring.h>
size_t _fmbstowcs( const wchar_t __far *pwcs,
                   char __far *s,
                   size_t n );
.ixfunc2 '&Multibyte' &ffunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function converts a sequence of multibyte characters pointed
to by
.arg s
into their corresponding wide character codes and stores not more than
.arg n
codes into the array pointed to by
.arg pwcs
.period
The
.id &funcb.
function does not convert any multibyte characters beyond
the null character.
At most
.arg n
elements of the array pointed to by
.arg pwcs
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
.seelist mbstowcs mbstowcs_s mblen mbtowc wctomb wctomb_s wcstombs wcstombs_s
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void main()
  {
    char    *wc = "string";
    wchar_t wbuffer[50];
    int     i, len;
.exmp break
    len = mbstowcs( wbuffer, wc, 50 );
    if( len != -1 ) {
      wbuffer[len] = '\0';
      printf( "%s(%d)\n", wc, len );
      for( i = 0; i < len; i++ )
        printf( "/%4.4x", wbuffer[i] );
      printf( "\n" );
    }
  }
.exmp output
string(6)
/0073/0074/0072/0069/006e/0067
.exmp end
.class ISO C95
.system
