#include <stdio.h>
#include <string.h>

char buffer[80] = "world";

void main()
  {
    _setmbcp( 932 );
    if( _strnicoll( buffer, "Hello" ) < 0 ) {
        printf( "Less than\n" );
    }
  }
