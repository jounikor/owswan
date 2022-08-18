.func _m_psrlq
.synop begin
#include <mmintrin.h>
__m64 _m_psrlq(__m64 *m, __m64 *count);
.synop end
.desc begin
The 64-bit quad-word in
.arg m
is shifted to the right by the scalar shift count in
.arg count
.period
The high-order bits are filled with zeros.
The shift count is interpreted as unsigned.
Shift counts greater than 63 yield all zeros.
.desc end
.return begin
Shift right the 64-bit quad-word in
.arg m
by an amount specified in
.arg count
while shifting in zeros.
.return end
.see begin
.im seemmsrl
.see end
.exmp begin
#include <stdio.h>
#include <mmintrin.h>

#define AS_QWORD "%16.16Lx"
.exmp break
__m64   a;
__m64   b = { 0x3f04800300020001 };
__m64   c = { 0x0000000000000002 };

void main()
  {
    a = _m_psrlq( b, c );
    printf( "m1="AS_QWORD"\n"
            "m2="AS_QWORD"\n"
            "mm="AS_QWORD"\n",
            b, c, a );
  }
.exmp output
m1=3f04800300020001
m2=0000000000000002
mm=0fc12000c0008000
.exmp end
.class Intel
.system
