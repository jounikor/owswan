#include <complex.h>

int main( void ) {

    Complex    a (-23.4, 28);

    cout << a << " with opposite sign = "
         << a.operator -() << endl;
}
