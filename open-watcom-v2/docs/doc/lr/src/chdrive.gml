.func _chdrive
.synop begin
#include <direct.h>
int _chdrive( int drive );
.ixfunc2 '&DosFunc' &funcb
.synop end
.desc begin
The
.id &funcb.
function changes the current working drive to the one specified by
.arg drive
.period
A value of 1 is drive A, 2 is drive B, 3 is drive C, etc.
.desc end
.return begin
The
.id &funcb.
function returns zero if drive is successfully changed.
Otherwise, -1 is returned.
.return end
.see begin
.seelist _dos_getdrive _dos_setdrive _getdrive
.see end
.exmp begin
#include <stdio.h>
#include <direct.h>
.exmp break
void main( void )
{
    int drive = 3;
.exmp break
    if( _chdrive( drive  ) == 0 )
        printf( "Changed the current drive to %c\n",
                'A' + drive - 1 );
}
.exmp output
Changed the current drive to C
.exmp end
.class DOS
.system
