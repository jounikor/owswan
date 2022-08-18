.func setpwent
.synop begin
#include <pwd.h>
.if '&machsys' eq 'QNX' .do begin
int setpwent( void );
.do end
.el .do begin
void setpwent( void );
.do end
.synop end
.desc begin
The
.id &funcb.
function returns pointer for iterating over the system's password database
to the first entry.  It is normally called prior to using any of the POSIX
functions that access the password database to ensure starting at the first
entry.
.desc end
.see begin
.seelist getpwent endpwent getpwnam getpwuid
.see end
.exmp begin
.blktext begin
The following program will print out each user and their user ID in
the system's password database
.blktext end
.blkcode begin
#include <stdio.h>
#include <pwd.h>

void main()
  {
    struct passwd *pw;

    setpwent();

    while((pw = getpwent()) != NULL) {
        printf("User id %d is %s\n", (int)pw->pw_uid, pw->pw_name);
    }

    endpwent();
  }
.blkcode end
.exmp end
.class POSIX 1003.1
.system
