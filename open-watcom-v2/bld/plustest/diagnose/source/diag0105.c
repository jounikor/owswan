#if !defined( ARCH ) || ( ARCH != 386 )
    #error system dependent test for 386
#else

#error <assert.h>
#include <assert.h>
#error <bios.h>
#if !defined( __UNIX__ )
  #include <bios.h>
#endif
#error <complex.h>
#include <complex.h>
#error <conio.h>
#include <conio.h>
#error <ctype.h>
#include <ctype.h>
#error <direct.h>
#if !defined( __UNIX__ )
  #include <direct.h>
#endif
#error <dos.h>
#if !defined( __UNIX__ )
  #include <dos.h>
#endif
#error <dosfunc.h>
#if !defined( __UNIX__ )
  #include <dosfunc.h>
#endif
#error <env.h>
#include <env.h>
#error <errno.h>
#include <errno.h>
#error <except.h>
#include <except.h>
#error <fcntl.h>
#include <fcntl.h>
#error <float.h>
#include <float.h>
#error <fstream.h>
#include <fstream.h>
#error <generic.h>
#include <generic.h>
#error <graph.h>
#if !defined( __LINUX__ )
  #include <graph.h>
#endif
#error <i86.h>
#include <i86.h>
#error <io.h>
#if !defined( __UNIX__ )
  #include <io.h>
#endif
#error <iomanip.h>
#include <iomanip.h>
#error <iostream.h>
#include <iostream.h>
#error <limits.h>
#include <limits.h>
#error <locale.h>
#include <locale.h>
#error <malloc.h>
#include <malloc.h>
#error <math.h>
#include <math.h>
#error <new.h>
#include <new.h>
#error <pgchart.h>
#if !defined( __LINUX__ )
  #include <pgchart.h>
#endif
#error <process.h>
#include <process.h>
#error <search.h>
#include <search.h>
#error <setjmp.h>
#include <setjmp.h>
#error <share.h>
#include <share.h>
#error <signal.h>
#include <signal.h>
#error <stdarg.h>
#include <stdarg.h>
#error <stddef.h>
#include <stddef.h>
#error <stdio.h>
#include <stdio.h>
#error <stdiobuf.h>
#include <stdiobuf.h>
#error <stdlib.h>
#include <stdlib.h>
#error <streambu.h>
#include <streambu.h>
#error <string.h>
#include <string.h>
#error <strstrea.h>
#include <strstrea.h>
#error <time.h>
#include <time.h>
#error <unistd.h>
#include <unistd.h>
#error <utime.h>
#include <utime.h>
#error <windows.h>
#if !defined( __UNIX__ )
  #include <windows.h>
#endif
#error <wsample.h>
#include <wsample.h>
#error <sys/locking.h>
#include <sys/locking.h>
#error <sys/stat.h>
#include <sys/stat.h>
#error <sys/timeb.h>
#include <sys/timeb.h>
#error <sys/types.h>
#include <sys/types.h>
#error <sys/utime.h>
#if !defined( __UNIX__ )
  #include <sys/utime.h>
#endif
#error <custcntl.h>

#error <dde.h>


struct x53 {};
void x54()
{
    catch( int &x )
    {
    }
}
void x60( int (*pf)( int ), int x )
{
    delete x;
    delete pf;
}
struct x65 {
    friend void x54();
    friend void x54();
    friend struct x53;
    friend struct x53;
};
void x69()
{
    if( b )
        break;
    case 1:
        if( b ) {
            continue;
        }
    default:
        if( b ) {
        }
}
typedef void x81( static int a )
{
}
auto int x84;
void *operator new();
struct x88 {
    static x88();
};
int x91( int x, int y, int z )
{
    return x > y > z;
}
unsigned x95()
{
    return UINT_MAX + UINT_MAX;
}
void x99( int b, int a )
{
    if( b = 0 ) {
        if( b = a ) {
            switch( 3 ) {
                case 1:
                case 2:
                default:
                default:
            }
        }
    }
    label:
}
void x113()
{
    template <class T> void foo( T );
    template <class T> class S;
}
static extern int a;
struct x119 {
    void operator ++( char );
};
#if 0
#endif text
void x124()
{
    unsigned i;
    for( i = 10; 0; --i ) {
    }
    if( i < 0 ) {
    }
}

#endif
