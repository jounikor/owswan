.func write _write writev
.synop begin
#include <&iohdr>
int write( int &fd, void *buffer, unsigned len );
.ixfunc2 '&OsIo' write
.if &'length(&_func.) ne 0 .do begin
int _write( int &fd, void *buffer, unsigned len );
.ixfunc2 '&OsIo' _write
.do end
int writev( int fildes,
            const struct iovec *iov,
            int iovcnt );
.ixfunc2 '&OsIo' writev
.synop end
.desc begin
The
.id &funcb.
function writes data at the operating system level.
The number of bytes transmitted is given by
.arg len
and the data to be transmitted is located at the address specified by
.arg buffer
.period
.np
The 
.id writev
function performs the same action as
.id write
.ct , but gathers the output data from the iovcnt buffers specified by 
the members of the iov array: iov[0], iov[1], ..., iov[iovcnt-1].
.np
.im ansiconf
.np
The
.arg &fd
value is returned by the
.reffunc open
function.
The access mode must have included either
.kw O_WRONLY
or
.kw O_RDWR
when the
.reffunc open
function was invoked.
.np
The data is written to the file at the end when the file was opened with
.kw O_APPEND
included as part of the access mode; otherwise, it is written at the
current file position for the file in question.
This file position can be determined with the
.reffunc _tell
function and can be set with the
.reffunc lseek
function.
.np
When
.kw O_BINARY
is included in the access mode, the data is transmitted unchanged.
When
.kw O_TEXT
is included in the access mode, the data is transmitted with extra
carriage return characters inserted before each linefeed character
encountered in the original data.
.np
A file can be truncated under DOS and OS/2 2.0 by specifying 0 as the
.arg len
argument.
.bd Note,
however, that this doesn't work under OS/2 2.1, Windows NT/2000, and
other operating systems.
To truncate a file in a portable manner, use the
.reffunc _chsize
function.
.desc end
.return begin
The
.id &funcb.
function returns the number of bytes
(does not include any extra carriage-return characters transmitted)
of data transmitted to the file.
When there is no error, this is the number given by the
.arg len
argument.
In the case of an error, such as there being no space available to
contain the file data, the return value will be less than the number
of bytes transmitted.
A value of &minus.1 may be returned in the case of some output errors.
.im errnoref
.return end
.see begin
.im seeioos
.see end
.exmp begin
#include <stdio.h>
#include <&iohdr>
#include <fcntl.h>
.exmp break
char buffer[]
        = { "A text record to be written" };
.exmp break
void main( void )
{
    int &fd;
    int size_written;
.exmp break
    /* open a file for output             */
    /* replace existing file if it exists */
    &fd = open( "file",
.if '&machsys' eq 'QNX' .do begin
                O_WRONLY | O_CREAT | O_TRUNC,
.do end
.el .do begin
                O_WRONLY | O_CREAT | O_TRUNC | O_TEXT,
.do end
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
    if( &fd != -1 ) {
.exmp break
        /* write the text                     */
        size_written = write( &fd, buffer,
                              sizeof( buffer ) );
.exmp break
        /* test for error                     */
        if( size_written != sizeof( buffer ) ) {
            printf( "Error writing file\n" );
        }
.exmp break
        /* close the file                     */
        close( &fd );
    }
}
.exmp end
.class POSIX 1003.1
.system
