#include <complex.h>

int main( void ) {

    Complex    a (24, 27);

    cout << "The value of e to the power of " << a << " = "
         << exp( a ) << endl;
}
