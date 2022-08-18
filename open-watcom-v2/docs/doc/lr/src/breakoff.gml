.func begin break&grpsfx Functions
.func2 break_off
.func2 break_on
.func end
.synop begin
#include <stdlib.h>
void break_off( void );
void break_on( void );
.synop end
.desc begin
The
.reffunc break_off
function can be used with DOS to restrict break checking (Ctrl+C,
Ctrl+Break) to screen output and keyboard input.
The
.reffunc break_on
function can be used with DOS to add break checking (Ctrl+C,
Ctrl+Break) to other activities such as disk file input/output.
.desc end
.return begin
The
.reffunc break_off
and
.reffunc break_on
functions to not return anything.
.return end
.see begin
.seelist signal
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void main()
  {
    long i;
    FILE *tmpf;
.exmp break
    tmpf = tmpfile();
    if( tmpf != NULL ) {
      printf( "Start\n" );
      break_off();
      for( i = 1; i < 100000; i++ )
        fprintf( tmpf, "%ld\n", i );
      break_on();
      printf( "Finish\n" );
    }
  }
.exmp end
.class DOS
.system
