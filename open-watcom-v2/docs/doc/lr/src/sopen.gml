.func _sopen _wsopen sopen
.synop begin
#include <&iohdr>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <share.h>
int _sopen( const char *filename,
           int access, int share, ... );
.ixfunc2 '&OsIo' _sopen
.if &'length(&wfunc.) ne 0 .do begin
int _wsopen( const wchar_t *filename,
           int access, int share, ... );
.ixfunc2 '&OsIo' _wsopen

.deprec
int sopen( const char *filename,
           int access, int share, ... );
.ixfunc2 '&OsIo' sopen
.do end
.synop end
.desc begin
The
.id &funcb.
function opens a file at the operating system level for
shared access.
The name of the file to be opened is given by
.arg filename
.period
The file will be accessed according to the access mode specified by
.arg access
.period
When the file is to be created, the optional argument must be given
which establishes the future access permissions for the file.
Additionally, the sharing mode of the file is given by the
.arg share
argument.
The optional argument is the file permissions to be used when
.kw O_CREAT
flag is on in the
.arg access
mode.
.im ansiconf
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it accepts a
wide character string argument.
.do end
.im openacc
.im openper
.np
The shared access for the file,
.arg share
.ct , is established by a combination of bits defined in the
.hdrfile share.h
header file.
The following values may be set:
.begterm 12 $compact
.termhd1 Value
.termhd2 Meaning
.term SH_COMPAT
Set compatibility mode.
.term SH_DENYRW
Prevent read or write access to the file.
.term SH_DENYWR
Prevent write access of the file.
.term SH_DENYRD
Prevent read access to the file.
.term SH_DENYNO
Permit both read and write access to the file.
.endterm
.if '&machsys' eq 'QNX' .do begin
.np
Note that
.millust begin
open( path, oflag, ... );
.millust end
is the same as:
.millust begin
_sopen( path, oflag, SH_COMPAT, ... );
.millust end
.np
Note that the
.id &funcb.
function call ignores advisory locks which may
have been set by the
.reffunc fcntl
.ct ,
.reffunc lock
.ct , or
.reffunc locking
functions.
.do end
.el .do begin
.np
You should consult the technical documentation for the DOS system that
you are using for more detailed information about these sharing modes.
.do end
.np
.deprfunc sopen _sopen
.desc end
.return begin
If successful,
.id &funcb.
returns a &handle for the file.
When an error occurs while opening the file, &minus.1 is returned.
.im errnoref
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term EACCES
Access denied because
.arg path
specifies a directory or a volume ID,
or sharing mode denied due to a conflicting open.
.term EMFILE
No more &handle.s available (too many open files)
.term ENOENT
Path or file not found
.endterm
.error end
.see begin
.im seeioos
.see end
.exmp begin
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <share.h>

void main( void )
{
    int &fd;
.exmp break
    /* open a file for output                  */
    /* replace existing file if it exists      */

    &fd = _sopen( "file",
                O_WRONLY | O_CREAT | O_TRUNC,
                SH_DENYWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

    /* read a file which is assumed to exist   */

    &fd = _sopen( "file", O_RDONLY, SH_DENYWR );

    /* append to the end of an existing file   */
    /* write a new file if file does not exist */

    &fd = _sopen( "file",
                O_WRONLY | O_CREAT | O_APPEND,
                SH_DENYWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
}
.exmp end
.class WATCOM
.system
