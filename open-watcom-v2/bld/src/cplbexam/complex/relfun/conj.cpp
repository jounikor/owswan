#include <complex.h>

int main( void ) {

    Complex    a (24, 27);

    cout << "The conjugate of " << a << " = "
         << conj( a ) << endl;
}
