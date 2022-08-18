.func _popen _wpopen
.synop begin
#include <stdio.h>
FILE *_popen( const char *command, const char *mode );
.ixfunc2 '&Direct' &funcb
.if &'length(&wfunc.) ne 0 .do begin
FILE *_wpopen( const wchar_t *command, const wchar_t *mode );
.ixfunc2 '&Direct' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function executes the command specified by
.arg command
and creates a pipe between the calling process and the executed
command.
.np
Depending on the
.arg mode
argument, the stream pointer returned may be used to read from or
write to the pipe.
.np
The executed command has an environment the same as its parents.
The command will be started as follows:
.if '&machsys' eq 'QNX' .do begin
.millust begin
spawnl(<shell_path>, "sh", "-c", command, (char *)NULL);
.millust end
.do end
.el .do begin
spawnl(<shell_path>, <shell>, "-c", command, (char *)NULL);
.do end
.pc
where
.mono <shell_path>
is an unspecified path for the
.if '&machsys' eq 'QNX' .do begin
.kw sh
utility.
.do end
.el .do begin
shell utility and
.mono <shell>
is one of "command.com" (DOS, Windows 95) or "cmd.exe"
(Windows NT/2000, OS/2).
.do end
.np
The
.arg mode
argument to
.id &funcb.
is a string that specifies an I/O mode for the pipe.
.begnote
.notehd1 Mode
.notehd2 Meaning
.note "r"
The calling process will read from the standard output of the child
process using the stream pointer returned by
.id &funcb.
.
.note "w"
The calling process will write to the standard input of the child
process using the stream pointer returned by
.id &funcb.
.
.endnote
.np
The letter "t" may be added to any of the above modes to indicate that
the file is (or must be) a text file (i.e., CR/LF pairs are converted
to newline characters).
.np
The letter "b" may be added to any of the above modes to indicate
that the file is (or must be) a binary file (an ISO C requirement for
portability to systems that make a distinction between text and binary
files).
.np
When default file translation is specified (i.e., no "t" or "b" is
specified), the value of the global variable
.kw _fmode
establishes whether the file is to treated as a binary or a text file.
Unless this value is changed by the program, the default will be text
mode.
.np
A stream opened by
.id &funcb.
should be closed by the
.reffunc _pclose
function.
.desc end
.return begin
The
.id &funcb.
function returns a non-NULL stream pointer upon successful
completion.
If
.id &funcb.
is unable to create either the pipe or the subprocess, a
.mono NULL
stream pointer is returned and
.kw errno
is set appropriately.
.return end
.error begin
.begterm 12
.termhd1 Constant
.termhd2 Meaning
.term EINVAL
The
.arg mode
argument is invalid.
.endterm
.np
.id &funcb.
may also set
.kw errno
values as described by the
.reffunc _pipe
and
.reffunc spawnl
functions.
.error end
.see begin
.seelist _grow_handles _pclose perror _pipe
.see end
.exmp begin
/*
 * Executes a given program, converting all
 * output to upper case.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char   buffer[256];
.exmp break
void main( int argc, char **argv )
  {
    int  i;
    int  c;
    FILE *f;

    for( i = 1; i < argc; i++ ) {
      strcat( buffer, argv[i] );
      strcat( buffer, " " );
    }
.exmp break
    if( ( f = _popen( buffer, "r" ) ) == NULL ) {
      perror( "_popen" );
      exit( 1 );
    }
    while( ( c = getc(f) ) != EOF ) {
      if( islower( c ) )
          c = toupper( c );
      putchar( c );
    }
    _pclose( f );
  }
.exmp end
.class WATCOM
.system
