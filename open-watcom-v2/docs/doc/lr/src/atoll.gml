.func atoll _wtoll
.synop begin
#include <stdlib.h>
long long int atoll( const char *ptr );
.ixfunc2 '&Conversion' &funcb
.if &'length(&wfunc.) ne 0 .do begin
long long int _wtoll( const wchar_t *ptr );
.ixfunc2 '&Conversion' &wfunc
.ixfunc2 '&Wide' &wfunc
.do end
.synop end
.desc begin
The
.id &funcb.
function converts the string pointed to by
.arg ptr
to
.id long long int
representation.
.widefunc &wfunc. &funcb.
.desc end
.return begin
The
.id &funcb.
function returns the converted value.
.return end
.see begin
.im seestoi
.see end
.exmp begin
#include <stdlib.h>

void main()
{
    long int x;
.exmp break
    x = atoll( "-289356768201" );
}
.exmp end
.class ISO C
.system
