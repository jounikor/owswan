#include <iostream.h>

int main( void ) {

    char    *bp;
    int     len = 20;

    bp = new char [len];
    cout << "Enter a string:" << endl;
    cin.get( bp, len );
    cout << "The number of characters extracted = " << cin.gcount() << endl;
    delete[] bp;
}
