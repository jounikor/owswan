#include <complex.h>

int main( void ) {

    Complex    a (24, 27);

    cout << "The hyperbolic sine of " << a << " = "
         << sinh( a ) << endl;
}
