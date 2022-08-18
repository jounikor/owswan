.func _memavl
.synop begin
#include <malloc.h>
size_t _memavl( void );
.ixfunc2 '&Memory' &funcb
.synop end
.desc begin
The
.id &funcb.
function returns the number of bytes of memory available for
dynamic memory allocation in the near heap (the default data segment).
In the tiny, small and medium memory models, the default data segment
is only extended as needed to satisfy requests for memory allocation.
Therefore, you will need to call
.reffunc _nheapgrow
in these memory models before calling
.id &funcb.
in order to get a
meaningful result.
.pp
The number returned by
.id &funcb.
may not represent a single contiguous
block of memory.
Use the
.reffunc _memmax
function to find the largest contiguous block of memory that can be
allocated.
.desc end
.return begin
The
.id &funcb.
function returns the number of bytes of memory available
for dynamic memory allocation in the near heap (the default data segment).
.return end
.see begin
.seelist calloc Functions _freect _memmax
.seelist _heapgrow Functions malloc Functions realloc Functions
.see end
.exmp begin
#include <stdio.h>
#include <malloc.h>

void main()
  {
    char *p;
    char *fmt = "Memory available = %u\n";
.exmp break
    printf( fmt, _memavl() );
    _nheapgrow();
    printf( fmt, _memavl() );
    p = (char *) malloc( 2000 );
    printf( fmt, _memavl() );
  }
.exmp output
Memory available = 0
Memory available = 62732
Memory available = 60730
.exmp end
.class WATCOM
.system
