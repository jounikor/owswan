.func _mktemp _wmktemp
.synop begin
#include <&iohdr>
char *_mktemp( char *template );
.ixfunc2 '&FileOp' &funcb
.if &'length(&wfunc.) ne 0 .do begin
#include <wchar.h>
wchar_t *_wmktemp( wchar_t *template );
.ixfunc2 '&FileOp' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function creates a unique filename by modifying the
.arg template
argument.
.id &funcb.
automatically handles multi-byte character string arguments as
appropriate, recognizing multi-byte character sequences according to
the multibyte code page currently in use by the run-time system.
.widefunc &wfunc. &funcb.
.np
The string
.arg template
has the form
.mono baseXXXXXX
where
.mono base
is the fixed part of the generated filename and
.mono XXXXXX
is the variable part of the generated filename.
Each of the 6 X's is a placeholder for a character supplied by &funcb..
Each placeholder character in
.arg template
must be an uppercase "X".
.id &funcb.
preserves
.mono base
and replaces the first of the 6 trailing X's with a lowercase
alphabetic character (a-z).
.id &funcb.
replaces the following 5 trailing X's with a five-digit value;
this value is a unique number identifying the calling process or
thread.
.np
.id &funcb.
checks to see if a file with the generated name already exists
and if so selects another letter, in succession, from "a" to "z" until
it finds a file that doesn't exist.
If it is unsuccessful at finding a name for a file that does not
already exist,
.id &funcb.
returns NULL.
At most, 26 unique file names can be returned to the calling process
or thread.
.desc end
.return begin
The
.id &funcb.
function returns a pointer to the modified
.arg template
.period
The
.id &funcb.
function returns NULL if
.arg template
is badly formed or no more unique names can be created from the given
template.
.return end
.error begin
.error end
.see begin
.seelist fopen freopen mkstemp _mktemp _tempnam tmpfile tmpnam
.see end
.exmp begin
#include <stdio.h>
#include <string.h>
#include <io.h>

#define TMPLTE "_tXXXXXX"

void main()
  {
    char name[sizeof(TMPLTE)];
    char *mknm;
    int i;
    FILE *fp;

    for( i = 0; i < 30; i++ ) {
      strcpy( name, TMPLTE );
      mknm = _mktemp( name );
      if( mknm == NULL )
        printf( "Name is badly formed\n" );
      else {
        printf( "Name is %s\n", mknm );
        fp = fopen( mknm, "w" );
        if( fp != NULL ) {
          fprintf( fp, "Name is %s\n", mknm );
          fclose( fp );
        }
      }
    }
  }
.exmp end
.class WATCOM
.system
