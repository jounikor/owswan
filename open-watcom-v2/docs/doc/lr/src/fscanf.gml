.func begin fscanf
.func2 fwscanf ISO C95
.func end
.synop begin
#include <stdio.h>
int fscanf( FILE *fp, const char *format, ... );
.ixfunc2 '&StrIo' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <stdio.h>
#include <wchar.h>
int fwscanf( FILE *fp, const wchar_t *format, ... );
.ixfunc2 '&StrIo' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.*
.safealt
.*
.desc begin
The
.id &funcb.
function scans input from the file designated by
.arg fp
under control of the argument
.arg format
.period
Following the format string is a list of addresses to receive values.
The
.arg format
string is described under the description of the
.reffunc scanf
function.
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it accepts a
wide character string argument for
.arg format
.period
.do end
.desc end
.*
.return begin
The
.id &funcb.
function returns
.kw EOF
if an input failure occurred before any conversion.
Otherwise, the number of input arguments for which
values were successfully scanned and stored is returned.
When a file input error occurs, the
.kw errno
global variable may be set.
.return end
.*
.see begin
.im seevscnf
.see end
.*
.exmp begin
.blktext begin
To scan a date in the form "Saturday April 18 1987":
.blktext end
.blkcode begin
#include <stdio.h>

void main( void )
{
    int day, year;
    char weekday[10], month[10];
    FILE *in_data;
.exmp break
    in_data = fopen( "file", "r" );
    if( in_data != NULL ) {
        fscanf( in_data, "%s %s %d %d",
                weekday, month, &day, &year );
        printf( "Weekday=%s Month=%s Day=%d Year=%d\n",
                weekday, month, day, year );
        fclose( in_data );
    }
}
.blkcode end
.exmp end
.*
.class ISO C90
.system
