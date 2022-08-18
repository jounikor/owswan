#include <wcqueue.h>
#include <iostream.h>


int main( void ) {
    WCQueue<int,WCValSList<int> >       queue;

    queue.insert( 7 );
    queue.insert( 8 );
    queue.insert( 9 );
    queue.insert( 10 );

    cout << "\nNumber of queue entries: " << queue.entries() << "\n";
    cout << "First entry = [" << queue.first() << "]\n";
    cout << "Last entry = [" << queue.last() << "]\n";
    while( !queue.isEmpty() ) {
        cout << queue.get() << "\n";
    };
    cout.flush();
}
