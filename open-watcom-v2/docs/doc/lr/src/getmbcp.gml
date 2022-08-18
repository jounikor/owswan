.func _getmbcp
.synop begin
#include <mbctype.h>
int _getmbcp( void );
.synop end
.desc begin
The
.id &funcb.
function returns the current multibyte code page number.
.desc end
.return begin
The
.id &funcb.
function returns the current multibyte code page.
A return value of zero indicates that a single-byte character code page
is in use.
.return end
.see begin
.im seeismbb
.see end
.exmp begin
#include <stdio.h>
#include <mbctype.h>

void main()
  {
    printf( "%d\n", _setmbcp( 932 ) );
    printf( "%d\n", _getmbcp() );
  }
.exmp output
0
932
.exmp end
.class WATCOM
.system
