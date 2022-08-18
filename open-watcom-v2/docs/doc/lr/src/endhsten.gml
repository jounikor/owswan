.func endhostent
.synop begin
#include <netdb.h>
void endhostent( void );
.synop end
.desc begin
The
.id &funcb.
function closes the network host database at /etc/hosts. If the
network host database is not open, this call results in no actions.
.desc end
.see begin
.seelist gethostent sethostent
.see end
.class POSIX 1003.1
.system
