.func _atouni
.synop begin
#include <stdlib.h>
wchar_t *_atouni( wchar_t *wcs, const char *sbcs );
.synop end
.desc begin
The
.id &funcb.
function converts the string pointed to by
.arg sbcs
to a wide character string and places it in the buffer
pointed to by
.arg wcs
.period
.np
The conversion ends at the first null character.
.desc end
.return begin
The
.id &funcb.
function returns the first argument as a result.
.return end
.see begin
.seelist atoi atol itoa ltoa
.seelist strtod strtol strtoul ultoa utoa
.see end
.exmp begin
#include <stdlib.h>

void main()
  {
    wchar_t wcs[12];
.exmp break
    _atouni( wcs, "Hello world" );
  }
.exmp end
.class WATCOM
.system
