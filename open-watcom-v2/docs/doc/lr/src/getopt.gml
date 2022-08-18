.func getopt
.synop begin
#include <unistd.h>
int getopt( int argc, char * const argv[],
            const char *optstring );

char   *optarg;
int    optind, opterr, optopt;
.synop end
.desc begin
The
.id &funcb.
function is a command-line parser that can be used by applications
that follow Utility Syntax Guidelines 3, 4, 5, 6, 7, 9 and 10 in the Base
Definitions volume of IEEE Std 1003.1-2001, Section 12.2, Utility Syntax
Guidelines.
.np
The parameters
.arg argc
and
.arg argv
are the argument count and argument array as passed to
.reffunc main
.period
The argument
.arg optstring
is a string of recognised option characters; if a character is followed by a
colon, the option takes an argument. All option characters allowed by Utility
Syntax Guideline 3 are allowed in
.arg optstring
.period
.np
The global variable
.kw optind
is the index of the next element of the
.arg argv[]
vector to be processed. It is initialised to 1 by the system, and &funcb
updates it when it finishes with each element of
.arg argv[]
.period
When an element of
.arg argv[]
contains multiple option characters,
.id &funcb.
uses a static variable to determine
which options have already been processed.
.np
The
.id &funcb.
function returns the next option character (if one is found) from
.arg argv
that matches a character in
.arg optstring
.ct , if there is one that matches. If the option takes an argument,
.id &funcb.
sets
the variable
.kw optarg
to point to the option-argument as follows:
.np
If the option was the last character in the string pointed to by an element of
.arg argv
.ct , then
.kw optarg
contains the next element of
.arg argv
.ct , and
.kw optind
is incremented by 2. If the resulting value of
.kw optind
is not less than
.arg argc
.ct , this indicates a missing option-argument, and
.id &funcb.
returns an error
indication.
.np
Otherwise,
.kw optarg
points to the string following the option character in that element of
.arg argv
.ct , and
.kw optind
is incremented by 1.
.np
If, when
.id &funcb.
is called:
.*
.begbull
.bull
.arg argv[optind]
is a null pointer
.bull
.arg *argv[optind]
is not the character '-'
.bull
.arg argv[optind]
points to the string "-"
.endbull
.*
.id &funcb.
returns -1 without changing
.kw optind
.period
If
.arg argv[optind]
points to the string "--",
.id &funcb.
returns -1 after incrementing
.kw optind
.period
.np
If
.id &funcb.
encounters an option character that is not contained in
.arg optstring
.ct , it returns the question-mark (?) character. If it detects a missing
option-argument, it returns the colon character (:) if the first character of
.arg optstring
was a colon, or a question-mark character (?) otherwise. In either case, &funcb
will set the global variable
.kw optopt
to the option character that caused the error. If the application has not set
the global variable
.kw opterr
to 0 and the first character of
.arg optstring
is not a colon,
.id &funcb.
also prints a diagnostic message to
.kw stderr
.period
.np
The
.id &funcb.
function is not re-entrant and hence not thread-safe.
.desc end
.return begin
The
.id &funcb.
function returns the next option character specified on the command
line.
.np
A colon (:) is returned if
.id &funcb.
detects a missing argument and the
first character of
.arg optstring
was a colon (:).
.np
A question mark (?) is returned if
.id &funcb.
encounters an option character not in
.arg optstring
or detects a missing argument and the first character of
.arg optstring
was not a colon (:).
.np
Otherwise,
.id &funcb.
returns -1 when all command line options are parsed.
.return end
.see begin
.im seeproc
.see end
.exmp begin
#include <stdio.h>
#include <unistd.h>

int main( int argc, char **argv )
{
    int     c;
    char    *ifile;
    char    *ofile;
.exmp break
    while( (c = getopt( argc, argv, ":abf:o:" )) != -1 ) {
        switch( c ) {
        case 'a':
            printf( "option a is set\n" );
            break;
        case 'b':
            printf( "option b is set\n" );
            break;
        case 'f':
            ifile = optarg;
            printf( "input filename is '%s'\n", ifile );
            break;
        case 'o':
            ofile = optarg;
            printf( "output filename is '%s'\n", ofile );
            break;
        case ':':
            printf( "-%c without filename\n", optopt );
            break;
        case '?':
            printf( "usage: %s -ab -f <filename> -o <filename>\n", argv[0] );
            break;
        }
    }
    return( 0 );
}
.exmp output
option a is set
input filename is 'in'
output filename is 'out'
.blktext begin
when the program is executed with the command
.blktext end
.blkcode begin
<program name> -afin -o out
.blkcode end
.exmp end
.class POSIX 1003.1
.system
