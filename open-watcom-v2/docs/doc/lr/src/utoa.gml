.func utoa _utoa _utow
.synop begin
#include <stdlib.h>
char *utoa( unsigned int value,
            char *buffer,
            int radix );
.ixfunc2 '&Conversion' &funcb
.if &'length(&_func.) ne 0 .do begin
char *_utoa( unsigned int value,
             char *buffer,
             int radix );
.ixfunc2 '&Conversion' &_func
.do end
.if &'length(&wfunc.) ne 0 .do begin
wchar_t *_utow( unsigned int value,
                wchar_t *buffer,
                int radix );
.ixfunc2 '&Conversion' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function converts the unsigned binary integer
.arg value
into the equivalent string in base
.arg radix
notation storing the result in the character array pointed to by
.arg buffer
.period
A null character is appended to the result.
The size of
.arg buffer
must be at least (8 * sizeof(int) + 1)
bytes when converting values in base 2.
That makes the size 17 bytes on 16-bit machines, and 33 bytes on
32-bit machines.
The value of
.arg radix
must satisfy the condition:
.millust begin
2 <= radix <= 36
.millust end
.im ansiconf
.widefunc &wfunc. &funcb. <ret>
.desc end
.return begin
The
.id &funcb.
function returns the pointer to the result.
.return end
.see begin
.im seestoi
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void main()
{
    int base;
    char buffer[18];
.exmp break
    for( base = 2; base <= 16; base = base + 2 )
        printf( "%2d %s\n", base,
                utoa( (unsigned) 12765, buffer, base ) );
}
.exmp output
 2 11000111011101
 4 3013131
 6 135033
 8 30735
10 12765
12 7479
14 491b
16 31dd
.exmp end
.class WATCOM
.system
