#include <fstream.h>

int main( void ) {

    char    s[20];

    ofstream    output;
    output.open( "temp.txt" );
    output << "Hello, my world." << endl;

    ifstream    input ( "temp.txt" );
    cout << "The content of \"temp.txt\" :" << endl;
    while( (input >> s).good() ) {
        cout << s << " " << flush;
    }
}
