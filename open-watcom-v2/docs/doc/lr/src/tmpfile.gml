.func tmpfile
.synop begin
#include <stdio.h>
FILE *tmpfile( void );
.ixfunc2 '&StrIo' &funcb
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function creates a temporary binary file that will
automatically be removed when it is closed or at program termination.
The file is opened for update.
.if '&machsys' ne 'QNX' .do begin
For all systems except NetWare, the temporary file is located in the
path specified by one of the following environment variables, if one
is defined.
Otherwise, the current working directory is used.
.ix 'environment variable' 'tmpfile'
They are listed in the order examined:
.kw TMP
.ct ,
.kw TEMP
.ct ,
.kw TMPDIR
.ct , and
.kw TEMPDIR
.period
.do end
.desc end
.return begin
The
.id &funcb.
function returns a pointer to the stream of the file that it
created.
If the file cannot be created, the
.id &funcb.
function returns
.mono NULL.
.im errnoref
.return end
.see begin
.seelist fopen fopen_s freopen freopen_s mkstemp _mktemp _tempnam tmpfile tmpfile_s tmpnam tmpnam_s
.see end
.exmp begin
#include <stdio.h>

static FILE *TempFile;

void main()
{
    TempFile = tmpfile();
    /* . */
    /* . */
    /* . */
    fclose( TempFile );
}
.exmp end
.class ISO C
.system
