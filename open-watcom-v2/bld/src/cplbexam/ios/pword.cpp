#include <iostream.h>
#include <strstream.h>

int main( void ) {

    int      index1 , index2;
    char     s[10];

    istrstream     greet ( "Hello," );
    index1 = greet.xalloc();
    greet.pword( index1 ) = " Open Watcom C compiler users";
    index2 = greet.xalloc();
    greet.pword( index2 ) = " Open Watcom C++ compiler users";
    greet >> s;
    cout << s << (char *)greet.pword( index1 )  << endl;
    cout << s << (char *)greet.pword( index2 )  << endl;
}
