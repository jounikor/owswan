.if &farfnc eq 0 .do begin
.func free
.synop begin
#include <stdlib.h>
void free( void *ptr );
.ixfunc2 '&Memory' free
.synop end
.desc begin
When the value of the argument
.arg ptr
is
.mono NULL,
the
.id &funcb.
function does nothing; otherwise, the
.id &funcb.
function
deallocates the memory block located by the argument
.arg ptr
which points to a memory block previously allocated through a call to
.reffunc calloc
.ct ,
.reffunc malloc
or
.reffunc realloc
.period
After the call, the freed block is available for allocation.
.desc end
.return begin
The
.id &funcb.
function returns no value.
.return end
.see begin
.im seealloc
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void main()
  {
    char *buffer;
.exmp break
    buffer = (char *)malloc( 80 );
    if( buffer == NULL ) {
      printf( "Unable to allocate memory\n" );
    } else {

      /* rest of code goes here */

      free( buffer );  /* deallocate buffer */
    }
  }
.exmp end
.class ISO C
.do end
.************************
.el .do begin
.func begin free Functions
.func2 free
.func2 _bfree
.func2 _ffree
.func2 _nfree
.func end
.synop begin
#include <stdlib.h>  For ISO C compatibility (free only)
#include <malloc.h>  Required for other function prototypes
void free( void *ptr );
void _bfree( __segment seg, void __based(void) *ptr );
void _ffree( void __far  *ptr );
void _nfree( void __near *ptr );
.ixfunc2 '&Memory' free
.ixfunc2 '&Memory' _bfree
.ixfunc2 '&Memory' _ffree
.ixfunc2 '&Memory' _nfree
.synop end
.desc begin
When the value of the argument
.arg ptr
is
.mono NULL,
the
.id &funcb.
function does nothing; otherwise, the
.id &funcb.
function
deallocates the memory block located by the argument
.arg ptr
which points to a memory block previously allocated through a call to
the appropriate version of
.reffunc calloc
.ct ,
.reffunc malloc
or
.reffunc realloc
.period
After the call, the freed block is available for allocation.
.np
Each function deallocates memory from a particular heap, as listed below:
.begterm 8
.termhd1 Function
.termhd2 Heap
.term free
Depends on data model of the program
.term _bfree
Based heap specified by
.arg seg
value
.term _ffree
Far heap (outside the default data segment)
.term _nfree
Near heap (inside the default data segment)
.endterm
.np
In a large data memory model, the
.id &funcb.
function is equivalent to the
.reffunc _ffree
function; in a small data memory model, the
.id &funcb.
function is
equivalent to the
.reffunc _nfree
function.
.desc end
.return begin
The
.id &funcb.
functions return no value.
.return end
.see begin
.im seealloc
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>

void main()
  {
    char *buffer;
.exmp break
    buffer = (char *)malloc( 80 );
    if( buffer == NULL ) {
      printf( "Unable to allocate memory\n" );
    } else {

      /* rest of code goes here */

      free( buffer );  /* deallocate buffer */
    }
  }
.exmp end
.class ISO C
.do end
.system
