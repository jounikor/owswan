#include <string.hpp>

int main( void ) {

    String    s1 ("Open Watcom C++");
    String    s2 ("Open Watcom C++ compiler");
    String    s3 ("Open Watcom C");
    char     *pch1, *pch2;

    pch1 = "Open Watcom C++";
    pch2 = "Open Watcom C";
    cout << "String \"" << s1 << "\" "
         << ( operator <=( s1, s2 ) ? "is shorter than or equals to" :
                                      "is longer than" )
         << " string \"" << s2 << "\"" << endl;
    cout << "String \"" << s1 << "\" "
         << ( operator <=( s1, s3 ) ? "is shorter than or equals to" :
                                      "is longer than" )
         << " string \"" << s3 << "\"" << endl;
    cout << "String \"" << s1 << "\" "
         << ( operator <=( s1, pch1 ) ? "is shorter than or equals to" :
                                        "is longer than" )
         << " string \"" << pch1 << "\"" << endl;
    cout << "String \"" << pch2 << "\" "
         << ( operator <=( pch2, s1 ) ? "is shorter than or equals to" :
                                        "is longer than" )
         << " string \"" << s1 << "\"" << endl;
}
