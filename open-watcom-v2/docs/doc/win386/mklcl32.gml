.func MK_LOCAL32
.synop begin
#include <windows.h>
void far *MK_LOCAL32( void * fp16 );
.synop end
.desc begin
The &funcb function converts a 16-bit near pointer to a 32-bit far
pointer.
This is needed whenever Windows returns a 16-bit near pointer that is
to be accessed by the 32-bit program.
.desc end
.return begin
The &funcb returns a 32-bit far pointer.
.return end
.see begin
.seelist MK_FP32 MK_FP16
.see end
.exmp begin
#include <windows.h>

  WORD ich,cch;
  char *pch;
  char far *fpch;
  HANDLE hT;

  /*
   * Request the data from an edit window; copy it
   * into a local buffer so that it can be passed
   * to TextOut
   */
  ich = (WORD) SendMessage( hwndEdit,
                            EM_LINEINDEX,
                            iLine,
                            0L );
  cch = (WORD) SendMessage( hwndEdit,
                            EM_LINELENGTH,
                            ich,
                            0L );
  fpch = MK_LOCAL32( LocalLock( hT ) ) ;
  pch = alloca( cch );
  _fmemcpy( pch, fpch + ich, cch );

  TextOut( hdc, 0, yExtSoFar, (LPSTR) pch, cch );
  LocalUnlock( hT );
.exmp end
.class WIN386
