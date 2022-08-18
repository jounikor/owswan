.func strncasecmp
.synop begin
#include <strings.h>
int strncasecmp( const char *s1,
                 const char *s2,
                 size_t len );
.ixfunc2 '&String' &funcb
.ixfunc2 '&Compare' &funcb
.synop end
.desc begin
The function compares, without case sensitivity,
the string pointed to by
.arg s1
to the string pointed to by
.arg s2
.ct , for at most
.arg len
characters.
All uppercase characters from
.arg s1
and
.arg s2
are mapped to lowercase for the purposes of doing the comparison.
.np
The
.id &funcb.
function is identical to the
.reffunc _strnicmp
function.
.desc end
.return begin
The function returns an integer less than, equal to,
or greater than zero, indicating that the string pointed to by
.arg s1
is, ignoring case, less than, equal to, or greater than the string pointed
to by
.arg s2
.period
.return end
.see begin
.seelist strcmp _stricmp strncmp _strnicmp strcasecmp strncasecmp
.see end
.exmp begin
#include <stdio.h>
#include <strings.h>

int main( void )
{
    printf( "%d\n", strncasecmp( "abcdef", "ABCXXX", 10 ) );
    printf( "%d\n", strncasecmp( "abcdef", "ABCXXX",  6 ) );
    printf( "%d\n", strncasecmp( "abcdef", "ABCXXX",  3 ) );
    printf( "%d\n", strncasecmp( "abcdef", "ABCXXX",  0 ) );
    return( 0 );
}
.exmp output
-20
-20
0
0
.exmp end
.class POSIX 1003.1
.system
