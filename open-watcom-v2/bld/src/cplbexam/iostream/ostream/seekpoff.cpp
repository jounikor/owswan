#include <iostream.h>
#include <fstream.h>

int main( void ) {

    char    ch[20];
    int     pos = 7;

    fstream    test ( "temp.txt", ios::in|ios::out );
    test << "Hello, my world." << endl;
    test.seekg( 0, ios::beg );
    while( (test >> ch).good() ) {
        cout << ch << " " << flush;
    }
    cout << endl;
    test.clear();
    test.seekp( pos, ios::beg );
    test << "Open Watcom C++ users." << endl;
    test.seekg( 0, ios::beg );
    while( (test >> ch).good() ) {
        cout << ch << " " << flush;
    }
    cout << endl;
}
