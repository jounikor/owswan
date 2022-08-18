#include "dump.h"


int __near a;
int __far b;
int __huge c;

void f1( int __near * ) GOOD;
void f1( int __far * ) GOOD;
#ifdef _M_I86
void f1( int __huge * ) GOOD;
#endif

void g1( void )
{
    f1( &a );
    f1( &b );
    f1( &c );
}

void f2( int __near & ) GOOD;
void f2( int __huge & ) GOOD;
#ifdef _M_I86
void f2( int __far & ) GOOD;
#endif

void g2( void )
{
    f2( a );
    f2( b );
    f2( c );
}

int main( void ) {
    g1();
    g2();
#ifdef _M_I86
    CHECK_GOOD( 95 );
#else
    CHECK_GOOD( 91 );
#endif
    return errors != 0;
}
