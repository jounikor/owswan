.func intrf
.synop begin
#include <i86.h>
void intrf( int inter_no, union REGPACK *regs );
.ixfunc2 '&CpuInt' &funcb
.synop end
.desc begin
The
.id &funcb.
functions cause the computer's central processor (CPU) to
be interrupted with an interrupt whose number is given by
.arg inter_no
.period
Before the interrupt, the CPU registers are loaded from the structure
located by
.arg regs
.period
Low 8-bit of the CPU flags is set to the flags member of the structure
.arg regs
.period
.np
All of the segment registers must contain valid values.
Failure to do so will cause a segment violation when running
in protect mode.
If you don't care about a particular segment register, then it
can be set to 0 which will not cause a segment violation.
Following the interrupt, the structure located by
.arg regs
is filled with the contents of the CPU registers.
.np
.id &funcb.
function is similar to the
.reffunc int86x
function. Exception is that only one structure is used for the register
values and that the BP (EBP in 386 library) register is included in
the set of registers that are passed and saved and the CPU flags are
set to flags member of the structure
.arg regs
.
.np
You should consult the technical documentation for the computer that
you are using to determine the expected register contents before and
after the interrupt in question.
.desc end
.return begin
The
.id &funcb.
function do not return a value.
.return end
.see begin
.im seeint
.see end
.if '&machsys' ne 'QNX' .do begin
.exmp begin
#include <stdio.h>
#include <string.h>
#include <i86.h>

void main() /* Print location of Break Key Vector */
  {
    union REGPACK regs;
.exmp break
    memset( &regs, 0, sizeof(union REGPACK) );
    regs.w.ax = 0x3523;
    regs.w.flags = 0;
    intrf( 0x21, &regs );
    printf( "Break Key vector is "
#if defined(__386__)
            "%x:%lx\n", regs.w.es, regs.x.ebx );
#else
            "%x:%x\n", regs.w.es, regs.x.bx );
#endif
  }
.exmp output
Break Key vector is eef:13c
.exmp end
.do end
.class Intel
.system
