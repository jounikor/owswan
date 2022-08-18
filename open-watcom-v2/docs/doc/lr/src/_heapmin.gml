.func begin _heapmin Functions
.func2 _heapmin
.func2 _bheapmin
.func2 _fheapmin
.func2 _nheapmin
.func end
.synop begin
.ixfunc2 '&Heap' _heapmin
.ixfunc2 '&Heap' _bheapmin
.ixfunc2 '&Heap' _fheapmin
.ixfunc2 '&Heap' _nheapmin
.ixfunc2 '&Memory' _heapmin
.ixfunc2 '&Memory' _bheapmin
.ixfunc2 '&Memory' _fheapmin
.ixfunc2 '&Memory' _nheapmin
#include <malloc.h>
int  _heapmin( void );
int _bheapmin( __segment seg );
int _fheapmin( void );
int _nheapmin( void );
.synop end
.desc begin
The
.id &funcb.
functions attempt to shrink the specified heap to its
smallest possible size by returning all free entries at the end of the
heap back to the system.
This can be used to free up as much memory as possible before using the
.reffunc system
function or one of the
.reffunc spawn&grpsfx
functions.
.np
The various
.id &funcb.
functions shrink the following heaps:
.begterm 12
.termhd1 Function
.termhd2 Heap Minimized
.term _heapmin
Depends on data model of the program
.term _bheapmin
Based heap specified by
.arg seg
value;
.mono _NULLSEG
specifies all based heaps
.term _fheapmin
Far heap (outside the default data segment)
.term _nheapmin
Near heap (inside the default data segment)
.endterm
.np
In a small data memory model, the
.id &funcb.
function is equivalent to the
.reffunc _nheapmin
function; in a large data memory model, the
.id &funcb.
function is
equivalent to the
.reffunc _fheapmin
function.
It is identical to the
.reffunc _heapshrink
function.
.desc end
.return begin
These functions return zero if successful, and non-zero if some error
occurred.
.return end
.see begin
.seelist _heapchk _heapenable _heapgrow _heapmin _heapset _heapshrink _heapwalk
.see end
.exmp begin
#include <stdlib.h>
#include <malloc.h>

void main()
  {
    _heapmin();
.if '&machsys' eq 'QNX' .do begin
    system( "cd /home/fred" );
.do end
.el .do begin
    system( "chdir c:\\watcomc" );
.do end
  }
.im dblslash
.exmp end
.class WATCOM
.system
