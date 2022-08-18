.func _dwShutDown
.synop begin
#include <wdefwin.h>
int _dwShutDown( void );
.synop end
.desc begin
The
.id &funcb.
function shuts down the default windowing I/O system.
The application will continue to execute but no windows will be
available for output.
Care should be exercised when using this function since any subsequent
output may cause unpredictable results.
.np
When the application terminates, it will not be necessary to manually
close the main window.
.np
The
.id &funcb.
function is one of the support functions that can be called
from an application using &company's default windowing support.
.desc end
.return begin
The
.id &funcb.
function returns 1 if it was successful and 0 if not.
.return end
.see begin
.seelist _dwDeleteOnClose _dwSetAboutDlg _dwSetAppTitle _dwSetConTitle _dwShutDown _dwYield
.see end
.exmp begin
#include <wdefwin.h>
#include <stdio.h>

void main()
  {
    FILE *sec;
.exmp break
    _dwSetAboutDlg( "Hello World About Dialog",
                    "About Hello World\n"
                    "Copyright 1994 by WATCOM\n" );
    _dwSetAppTitle( "Hello World Application Title" );
    _dwSetConTitle( 0, "Hello World Console Title" );
    printf( "Hello World\n" );
.exmp break
    sec = fopen( "CON", "r+" );
    _dwSetConTitle( fileno( sec ),
                    "Hello World Second Console Title" );
    _dwDeleteOnClose( fileno( sec ) );
    fprintf( sec, "Hello to second console\n" );
    fprintf( sec, "Press Enter to close this console\n" );
    fflush( sec );
    fgetc( sec );
    fclose( sec );
    _dwShutDown();
    /*
      do more computing that does not involve
      console input/output
    */
  }
.exmp end
.class WATCOM
.system
