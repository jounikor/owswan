.func closedir _wclosedir
.synop begin
#include <&dirhdr>
int closedir( DIR *dirp );
.ixfunc2 '&Direct' &funcb
.if &'length(&wfunc.) ne 0 .do begin
int _wclosedir( WDIR *dirp );
.ixfunc2 '&Direct' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function closes the directory specified by
.arg dirp
and frees the memory allocated by
.reffunc opendir
.period
.if '&machsys' eq 'QNX' .do begin
.np
The result of using a directory stream after one of the
.reffunc exec&grpsfx
or
.reffunc spawn&grpsfx
family of functions is undefined.
After a call to the
.reffunc fork
function, either the parent or the child (but not both) may continue
processing the directory stream using
.reffunc readdir
or
.reffunc rewinddir
or both.
If both the parent and child processes use these functions, the result
is undefined.
Either or both processes may use the
.id &funcb.
function.
.do end
.if &'length(&wfunc.) ne 0 .do begin
.np
The
.id &wfunc.
function is identical to
.id &funcb.
except that it closes a
directory of wide character filenames opened by
.reffunc _wopendir
.period
.do end
.desc end
.return begin
.if '&machsys' eq 'QNX' .do begin
If successful, the
.id &funcb.
function returns zero.
Otherwise &minus.1 is returned and
.kw errno
is set to indicate the error.
.do end
.el .do begin
The
.id &funcb.
function returns zero if successful, non-zero otherwise.
.do end
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term EBADF
The argument
.arg dirp
does not refer to an open directory stream.
.if '&machsys' eq 'QNX' .do begin
.term EINTR
The
.id &funcb.
function was interrupted by a signal.
.do end
.endterm
.error end
.see begin
.im seeiodir
.see end
.exmp begin
.blktext begin
To get a list of files contained in the directory
.if '&machsys' eq 'QNX' .do begin
.filename /home/fred
of your node:
.do end
.el .do begin
.filename \watcom\h
on your default disk:
.do end
.blktext end
.blkcode begin
#include <stdio.h>
#include <&dirhdr>
.if '&machsys' ne 'QNX' .do begin

typedef struct {
    unsigned short  twosecs : 5;    /* seconds / 2 */
    unsigned short  minutes : 6;
    unsigned short  hours   : 5;
} ftime_t;

typedef struct {
    unsigned short  day     : 5;
    unsigned short  month   : 4;
    unsigned short  year    : 7;
} fdate_t;
.do end

void main()
  {
    DIR *dirp;
    struct dirent *direntp;
.if '&machsys' ne 'QNX' .do begin
    ftime_t *f_time;
    fdate_t *f_date;
.do end
.exmp break
.if '&machsys' eq 'QNX' .do begin
    dirp = opendir( "/home/fred" );
.do end
.el .do begin
    dirp = opendir( "\\watcom\\h" );
.do end
    if( dirp != NULL ) {
      for(;;) {
        direntp = readdir( dirp );
        if( direntp == NULL ) break;
.if '&machsys' eq 'QNX' .do begin
        printf( "%s\n", direntp->d_name );
.do end
.el .do begin
        f_time = (ftime_t *)&direntp->d_time;
        f_date = (fdate_t *)&direntp->d_date;
        printf( "%-12s %d/%2.2d/%2.2d "
                "%2.2d:%2.2d:%2.2d \n",
            direntp->d_name,
            f_date->year + 1980,
            f_date->month,
            f_date->day,
            f_time->hours,
            f_time->minutes,
            f_time->twosecs * 2 );
.do end
      }
      closedir( dirp );
    }
  }
.blkcode end
.im dblslash
.exmp end
.class POSIX 1003.1
.system
