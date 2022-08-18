.func _dwYield
.synop begin
#include <wdefwin.h>
int _dwYield( void );
.synop end
.desc begin
The
.id &funcb.
function yields control back to the operating system,
thereby giving other processes a chance to run.
.np
The
.id &funcb.
function is one of the support functions that can be called
from an application using &company's default windowing support.
.desc end
.return begin
The
.id &funcb.
function returns 1 if it was successful and 0 if not.
.return end
.see begin
.seelist _dwDeleteOnClose _dwSetAboutDlg _dwSetAppTitle _dwSetConTitle _dwShutDown _dwYield
.see end
.exmp begin
#include <wdefwin.h>
#include <stdio.h>

void main()
  {
    int i;
.exmp break
    for( i = 0; i < 1000; i++ ) {
      /* give other processes a chance to run */
      _dwYield();
      /* do CPU-intensive calculation */
      /*  .  */
      /*  .  */
      /*  .  */
    }
  }
.exmp end
.class WATCOM
.system
