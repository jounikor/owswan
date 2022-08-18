.func int86
.synop begin
#include <i86.h>
int int86( int inter_no,
           const union REGS *in_regs,
           union REGS *out_regs );
.ixfunc2 '&CpuInt' &funcb
.synop end
.desc begin
The
.id &funcb.
function causes the computer's central processor (CPU) to be
interrupted with an interrupt whose number is given by
.arg inter_no
.period
Before the interrupt, the CPU registers are loaded from the structure
located by
.arg in_regs
.period
Following the interrupt, the structure located by
.arg out_regs
is filled with the contents of the CPU registers.
These structures may be located at the same location in memory.
.np
You should consult the technical documentation for the computer that
you are using to determine the expected register contents before and
after the interrupt in question.
.desc end
.return begin
The
.id &funcb.
function returns the value of the CPU AX register after the
interrupt.
.return end
.see begin
.im seeint
.see end
.exmp begin
/*
 * This example clears the screen on DOS
 */
#include <i86.h>

void main()
  {
    union REGS  regs;
.exmp break
    regs.w.cx = 0;
    regs.w.dx = 0x1850;
    regs.h.bh = 7;
    regs.w.ax = 0x0600;
#if defined(__386__) && defined(__DOS__)
    int386( 0x10, &regs, &regs );
#else
    int86( 0x10, &regs, &regs );
#endif
  }
.exmp end
.class Intel
.system
