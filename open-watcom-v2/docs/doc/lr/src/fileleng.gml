.func _filelength _filelengthi64 filelength
.ansiname _filelength
.synop begin
#include <&iohdr>
long _filelength( int &fd );
.ixfunc2 '&OsIo' _filelength
__int64 _filelengthi64( int &fd );
.ixfunc2 '&OsIo' _filelengthi64

.deprec
long filelength( int &fd );
.ixfunc2 '&OsIo' filelength
.synop end
.desc begin
The
.id &funcb.
function returns, as a 32-bit long integer, the number of
bytes in the opened file indicated by the file &handle
.arg &fd
.period
.np
The &func64 function returns, as a 64-bit integer, the number of
bytes in the opened file indicated by the file &handle
.arg &fd
.period
.np
.deprfunc filelength _filelength
.desc end
.return begin
If an error occurs in
.id &funcb.
(&minus.1L) is returned.
.np
If an error occurs in &func64, (&minus.1I64) is returned.
.np
.im errnoref
.np
Otherwise, the number of bytes written to the file is returned.
.return end
.see begin
.seelist fstat lseek _tell
.see end
.exmp begin
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <&iohdr>

void main( void )
{
    int &fd;
.exmp break
    /* open a file for input              */
.if '&machsys' eq 'QNX' .do begin
    &fd = open( "file", O_RDONLY );
.do end
.el .do begin
    &fd = open( "file", O_RDONLY | O_TEXT );
.do end
    if( &fd != -1 ) {
        printf( "Size of file is %ld bytes\n",
              _filelength( &fd ) );
        close( &fd );
    }
}
.exmp output
Size of file is 461 bytes
.exmp end
.class WATCOM
.system
