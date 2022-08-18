.func wctype
.synop begin
#include <wctype.h>
wctype_t wctype( const char *property );
.ixfunc2 '&CharTest' &funcb
.ixfunc2 '&Wide' &funcb
.synop end
.*
.desc begin
The
.id &funcb.
function constructs a value with type
.kw wctype_t
that describes a class of wide characters identified by the string
argument,
.arg property
.period
The constructed value is affected by the
.kw LC_CTYPE
category of the current locale; the constructed value becomes
indeterminate if the category's setting is changed.
.np
The twelve strings listed below are valid in all locales as
.arg property
arguments to the
.id &funcb.
function.
.begterm 10
.termhd1 Constant
.termhd2 Meaning
.*
.termnx alnum
any wide character for which one of
.reffunc iswalpha
or
.reffunc iswdigit
is true
.*
.termnx alpha
any wide character for which
.reffunc iswupper
 or
.reffunc iswlower
is true, that is, for any wide character that is one of an
implementation-defined set for which none of
.reffunc iswcntrl
.ct ,
.reffunc iswdigit
.ct ,
.reffunc iswpunct
.ct , or
.reffunc iswspace
is true
.*
.termnx blank
any wide character corresponding to a standard blank character
(space or horizontal tab) or is one of an implementation-defined set of wide
characters for which
.reffunc iswblank
is true
.*
.termnx cntrl
any control wide character
.*
.termnx digit
any wide character corresponding to a decimal-digit character
.*
.termnx graph
any printable wide character except a space wide character
.*
.termnx lower
any wide character corresponding to a lowercase letter, or one of an
implementation-defined set of wide characters for which none of
.reffunc iswcntrl
.ct ,
.reffunc iswdigit
.ct ,
.reffunc iswpunct
.ct , or
.reffunc iswspace
is true
.*
.termnx print
any printable wide character including a space wide character
.*
.termnx punct
any printable wide character that is not a space wide character or a
wide character for which
.reffunc iswalnum
is true
.*
.termnx space
any wide character corresponding to a standard white-space character
or is one of an implementation-defined set of wide
characters for which
.reffunc iswalnum
is false
.*
.termnx upper
any wide character corresponding to a uppercase letter, or if c is one
of an implementation-defined set of wide characters for which none of
.reffunc iswcntrl
.ct ,
.reffunc iswdigit
.ct ,
.reffunc iswpunct
.ct , or
.reffunc iswspace
is true
.*
.termnx xdigit
any wide character corresponding to a hexadecimal digit character
.endterm
.desc end
.*
.return begin
If
.arg property
identifies a valid class of wide characters according to the
.kw LC_CTYPE
category of the current locale, the
.id &funcb.
function returns a non-zero
value that is valid as the second argument to the
.reffunc iswctype
function; otherwise, it returns zero.
.return end
.*
.see begin
.im seeis
.see end
.*
.exmp begin
#include <stdio.h>
#include <wchar.h>

char *types[] = {
    "alnum",
    "blank",
    "alpha",
    "cntrl",
    "digit",
    "graph",
    "lower",
    "print",
    "punct",
    "space",
    "upper",
    "xdigit"
};
.exmp break
void main( void )
{
    int     i;
    wint_t  wc = 'A';
.exmp break
    for( i = 0; i < 12; i++ )
        if( iswctype( wc, wctype( types[i] ) ) )
            printf( "%s\n", types[i] );
}
.exmp output
alnum
alpha
graph
print
upper
xdigit
.exmp end
.*
.class ISO C95
.system
