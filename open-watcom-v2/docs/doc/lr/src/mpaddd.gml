.func _m_paddd
.synop begin
#include <mmintrin.h>
__m64 _m_paddd(__m64 *m1, __m64 *m2);
.synop end
.desc begin
The signed or unsigned 32-bit double-words of
.arg m2
are added to the respective signed or unsigned 32-bit double-words of
.arg m1
and the result is stored in memory.
If any result element does not fit into 32 bits (overflow), the lower
32-bits of the result elements are stored (i.e., truncation takes
place).
.desc end
.return begin
The result of adding the packed double-words of two 64-bit multimedia
values is returned.
.return end
.see begin
.im seemmadd
.see end
.exmp begin
#include <stdio.h>
#include <mmintrin.h>

#define AS_DWORDS "%8.8lx %8.8lx"
.exmp break
__m64   a;
__m64   b = { 0x0123456789abcdef };
__m64   c = { 0xfedcba9876543210 };

void main()
  {
    a = _m_paddd( b, c );
    printf( "m1="AS_DWORDS"\n"
            "m2="AS_DWORDS"\n"
            "mm="AS_DWORDS"\n",
        b._32[1], b._32[0],
        c._32[1], c._32[0],
        a._32[1], a._32[0] );
  }
.exmp output
m1=01234567 89abcdef
m2=fedcba98 76543210
mm=ffffffff ffffffff
.exmp end
.class Intel
.system
