.func _dos_setdrive
.synop begin
#include <&doshdr>
void _dos_setdrive( unsigned drive, unsigned *total );
.ixfunc2 '&DosFunc' &funcb
.synop end
.desc begin
The
.id &funcb.
function uses system call 0x0E to set the current default
disk drive to be the drive specified by
.arg drive
.ct , where 1 = drive A, 2 = drive B, etc.
The total number of disk drives is returned in the word pointed to by
.arg total
.period
For DOS versions 3.0 or later, the minimum number of drives returned
is 5.
.desc end
.return begin
The
.id &funcb.
function has no return value.
If an invalid drive number is specified, the function fails with no
error indication.
You must use the
.reffunc _dos_getdrive
function to check that the desired drive has been set.
.return end
.see begin
.seelist _dos_getdiskfree _dos_getdrive _dos_setdrive _getdiskfree _getdrive
.see end
.exmp begin
#include <stdio.h>
#include <&doshdr>
.exmp break
void main()
  {
    unsigned drive1, drive2, total;
.exmp break
    _dos_getdrive( &drive1 );
    printf( "Current drive is %c\n", 'A' + drive1 - 1 );
    /* try to change to drive C */
    _dos_setdrive( 3, &total );
    _dos_getdrive( &drive2 );
    printf( "Current drive is %c\n", 'A' + drive2 - 1 );
    /* go back to original drive */
    _dos_setdrive( drive1, &total );
    _dos_getdrive( &drive1 );
    printf( "Current drive is %c\n", 'A' + drive1 - 1 );
    printf( "Total number of drives is %u\n", total );
  }
.exmp output
Current drive is D
Current drive is C
Total number of drives is 6
.exmp end
.class DOS
.system
