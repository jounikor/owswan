.func inp
.synop begin
#include <conio.h>
unsigned int inp( int port );
.ixfunc2 '&PortIo' &funcb
.synop end
.desc begin
The
.id &funcb.
function reads one byte from the 80x86 hardware port whose
number is given by
.arg port
.period
.im portdesc
.im privity
.desc end
.return begin
The value returned is the byte that was read.
.return end
.see begin
.seelist inp inpd inpw outp outpd outpw
.see end
.exmp begin
#include <conio.h>

void main()
  {
    /* turn off speaker */
    outp( 0x61, inp( 0x61 ) & 0xFC );
  }
.exmp end
.class Intel
.system
