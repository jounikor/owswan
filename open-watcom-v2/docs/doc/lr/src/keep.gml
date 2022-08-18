.func _dos_keep
.synop begin
#include <&doshdr>
void _dos_keep( unsigned retcode, unsigned memsize );
.ixfunc2 '&TSR' &funcb
.synop end
.desc begin
The
.id &funcb.
function is used to install terminate-and-stay-resident
programs ("TSR's") in memory.
The amount of memory kept for the program is
.arg memsize
paragraphs (a paragraph is 16 bytes) from the Program Segment Prefix
which is stored in the variable
.kw _psp
.period
The value of
.arg retcode
is returned to the parent process.
.desc end
.return begin
The
.id &funcb.
function does not return to its caller.
.return end
.see begin
.seelist _dos_keep _chain_intr _dos_getvect _dos_setvect
.see end
.exmp begin
#include <&doshdr>

void permanent()
  {
    /* . */
    /* . */
    /* . */
  }

void transient()
  {
    /* . */
    /* . */
    /* . */
  }

void main()
  {
    /* initialize our TSR */
    transient();
    /*
        now terminate and keep resident
        the non-transient portion
        Note: following calculation only works in .COM files
    */
    _dos_keep( 0, (FP_OFF( transient ) + 15) >> 4 );
  }
.exmp end
.class DOS
.system
