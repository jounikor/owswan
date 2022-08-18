.func FILENAME_MAX
.synop begin
#include <stdio.h>
#define FILENAME_MAX 123
.synop end
.desc begin
The
.id &funcb.
macro is the size of an array of char big enough to hold a string
naming any file that the implementation expects to open;
If there is no practical file name length limit,
.id &funcb.
is the recommended
size of such an array.  As file name string contents must meet other
system-specific constraints, some strings of length
.id &funcb.
may not work.
.np
.id &funcb.
typically sizes an array to hold a file name.
.desc end
.return begin
The
.id &funcb.
macro returns a positive integer value.
.return end
.exmp begin
#include <stdio.h>
#include <string.h>

.exmp break
int main( int argc, char *argv[] )
{
    if( argc ) {
        char fname[FILENAME_MAX];

        strcpy( fname, argv[0] );
        puts( fname );
    }
    return( 0 );
}
.exmp end
.class ISO C
.system
