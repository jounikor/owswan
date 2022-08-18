.func _bios_printer
.synop begin
#include <bios.h>
unsigned short _bios_printer( unsigned service,
                              unsigned port,
                              unsigned data );
.ixfunc2 '&BiosFunc' &funcb
.synop end
.desc begin
The
.id &funcb.
function uses INT 0x17 to perform printer output
services to the printer specified by
.arg port
.period
The values for service are:
.begterm 12
.termhd1 Value
.termhd2 Meaning
.term _PRINTER_WRITE
Sends the low-order byte of
.arg data
to the printer specified by
.arg port
.period
.term _PRINTER_INIT
Initializes the printer specified by
.arg port
.period
.term _PRINTER_STATUS
Get the status of the printer specified by
.arg port
.period
.endterm
.desc end
.return begin
The
.id &funcb.
function returns a printer status byte defined as follows:
.begnote $compact
.notehd1 Bit
.notehd2 Meaning
.setptnt 0 14
.sr ptntelmt = 0
.note bit 0 (0x01)
Printer timed out
.note bits 1-2
Unused
.note bit 3 (0x08)
I/O error
.note bit 4 (0x10)
Printer selected
.note bit 5 (0x20)
Out of paper
.note bit 6 (0x40)
Printer acknowledge
.note bit 7 (0x80)
Printer not busy
.endnote
.sr ptntelmt = 1
.return end
.exmp begin
#include <stdio.h>
#include <bios.h>

void main()
  {
    unsigned short status;
.exmp break
    status = _bios_printer( _PRINTER_STATUS, 1, 0 );
    printf( "Printer status: 0x%2.2X\n", status );
  }
.exmp end
.class BIOS
.system
