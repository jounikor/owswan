.func getservbyname
.synop begin
#include <netdb.h>
struct servent *getservbyname( const char *name, const char *protocol );
.synop end
.desc begin
The
.id &funcb.
function searches the service database for a service with
matching
.arg name
and
.arg protocol
.ct , returning a pointer the a
.kw struct servent
if found.  The function will search for matching aliases as
well.  If
.arg protocol
is NULL, the first encountered service matching the specified
name will be returned regardless of protocol.  The routine will
query the local database only.
.np
The structure returned is defined as:
.blkcode begin
.im strucsrv
.blkcode end
.np
The pointer returned by
.id &funcb.
points to a private location, and the user should free neither
the pointer itself nor any of its constituent structure members.
Subsequent calls to this function may result in the values
changing.
.np
This function is not thread-safe.  Other calls to this function
or to other functions accessing the hostname database may affect
the return value from this function.
.desc end
.return begin
If a match is found, the return value will be non-NULL. The
returned pointer should not be freed by the calling
routine.
.np
The alias names of said network are contained in the
.kw s_aliases
member as a NULL-terminated list, and this structure entry will
never be NULL.
.np
If no additional network is found or an error occurs, the return
value will be NULL.
.return end
.see begin
.seelist getservent setservent endservent getservbyport
.see end
.class POSIX 1003.1
.system
