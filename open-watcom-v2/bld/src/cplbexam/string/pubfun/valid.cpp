#include <string.hpp>

int main( void ) {

    String    s ("Open Watcom C++");

    cout << "String \"" << s << "\" is "
         << ( s.valid() ? "valid." : "invalid!" ) << endl;
}
