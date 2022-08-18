.func strerror_s _wcserror_s
.synop begin
#define __STDC_WANT_LIB_EXT1__  1
#include <string.h>
errno_t strerror_s( char * s,
                    rsize_t maxsize,
                    errno_t errnum );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Errs' &funcb
.if &'length(&wfunc.) ne 0 .do begin
errno_t _wcserror_s( wchar_t * s,
                    rsize_t maxsize,
                    errno_t errnum );
.ixfunc2 '&String' &wfunc
.ixfunc2 '&Errs' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.*
.rtconst begin
.arg s
shall not be a null pointer.
.arg maxsize
shall not be greater than
.kw RSIZE_MAX
.period
.arg maxsize
shall not equal zero.
.np
If there is a runtime-constraint violation, then the array (if any) pointed to by
.arg s
is not modified.
.rtconst end
.*
.desc begin
The
.id &funcb.
function maps the number in
.arg errnum
to a locale-specific message string. Typically,the values for
.arg errnum
come from errno, but
.id &funcb.
shall map any value of type int to a message.
If the length of the desired string is less than
.arg maxsize
.ct , then the string is copied to the array pointed to by
.arg s
.period
Otherwise, if
.arg maxsize
is greater than zero, then
.arg maxsize-1
characters are copied from the string to the array pointed to by
.arg s
and then
.arg s[maxsize-1]
is set to the null character. Then, if
.arg maxsize
is greater than 3, then
.arg s[maxsize-2], s[maxsize-3]
.ct , and
.arg s[maxsize-4]
are set to the character period (.).
.widefunc &wfunc. &funcb.
.desc end
.*
.return begin
.saferet
.return end
.*
.see begin
.seelist clearerr feof ferror perror strerror strerrorlen_s
.see end
.*
.exmp begin
#define __STDC_WANT_LIB_EXT1__  1
#include <stdio.h>
#include <string.h>
#include <errno.h>

void main( void )
{
    FILE *fp;
    char emsg[ 100 ];
.exmp break
    fp = fopen( "file.nam", "r" );
    if( fp == NULL ) {
        strerror_s( emsg, sizeof( emsg ), errno );
        printf( "Unable to open file: %s\n", emsg );
    }
}
.exmp end
.*
.class TR 24731
.system
