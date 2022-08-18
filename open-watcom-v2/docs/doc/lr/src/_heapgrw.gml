.func begin _heapgrow Functions
.func2 _heapgrow
.func2 _fheapgrow
.func2 _nheapgrow
.func end
.synop begin
.ixfunc2 '&Heap' _heapgrow
.ixfunc2 '&Heap' _fheapgrow
.ixfunc2 '&Heap' _nheapgrow
.ixfunc2 '&Memory' _heapgrow
.ixfunc2 '&Memory' _fheapgrow
.ixfunc2 '&Memory' _nheapgrow
#include <malloc.h>
void  _heapgrow( void );
void _nheapgrow( void );
void _fheapgrow( void );
.synop end
.desc begin
The
.reffunc _nheapgrow
function attempts to grow the near heap to the maximum size of 64K.
You will want to do this in the small data models if you are using both
.reffunc malloc
and
.reffunc _fmalloc
or
.reffunc halloc
.period
Once a call to
.reffunc _fmalloc
or
.reffunc halloc
has been made, you may not be able to allocate any memory with
.reffunc malloc
unless space has been reserved for the near heap using either
.reffunc malloc
.ct ,
.reffunc sbrk
or
.reffunc _nheapgrow
.period
.np
The
.reffunc _fheapgrow
function doesn't do anything to the heap because the far heap will be
extended automatically when needed.
If the current far heap cannot be extended, then another far heap will be
started.
.np
In a small data memory model, the
.id &funcb.
function is equivalent to the
.reffunc _nheapgrow
function; in a large data memory model, the
.id &funcb.
function is
equivalent to the
.reffunc _fheapgrow
function.
.desc end
.return begin
These functions do not return a value.
.return end
.see begin
.seelist _heapchk _heapenable _heapgrow _heapmin _heapset _heapshrink _heapwalk
.see end
.exmp begin
#include <stdio.h>
#include <malloc.h>

void main()
  {
    char *p, *fmt_string;
    fmt_string = "Amount of memory available is %u\n";
    printf( fmt_string, _memavl() );
    _nheapgrow();
    printf( fmt_string, _memavl() );
    p = (char *) malloc( 2000 );
    printf( fmt_string, _memavl() );
  }
.exmp output
Amount of memory available is 0
Amount of memory available is 62732
Amount of memory available is 60730
.exmp end
.class WATCOM
.system
