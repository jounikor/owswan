#include <iostream.h>

void *operator new( size_t, ostream *p ) { return( p ); }

int main( void ) {

    char     blob[512];

    ostream     *p = new ( blob ) ostream ( cout );
    *p << "hi" << endl;
    p->~ostream();
}
