#include <string.hpp>

int main( void ) {

    String    s ("Open Watcom C++ compiler");

    cout << "String \"" << s << "\" in upper-case: "
         << "\"" <<  s.upper() << "\"" << endl;
}

