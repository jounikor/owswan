#include "fail.h"

#if __WATCOM_REVISION__ >= 7
#define INLINE inline
#else
#define INLINE
#pragma inline_depth(0)
#endif

#if defined(__386__) || defined(__I86__)

inline int sub( int a, int b ) {
    return a - b;
}
#pragma aux (__stdcall) sub parm reverse;

int inline burst( int, int );
#ifdef __386__
#pragma aux burst = \
	"xor eax,edx" \
	"and eax,edx" \
	"or eax,edx" \
	parm caller [eax] [edx] \
	value [eax] \
	modify exact [eax];
#else
#pragma aux burst = \
	"xor ax,dx" \
	"and ax,dx" \
	"or ax,dx" \
	parm caller [ax] [dx] \
	value [ax] \
	modify exact [ax];
#endif

int inline uses_burst( int a, int b ) {
    ++a;
    b -= 2;
    return burst( a, b );
}

int inline my_xor( short a, short b ) {
    short r;
#if __WATCOM_REVISION__ >= 8
    __asm {
	mov ax,word ptr a
	xor ax,word ptr b
	mov word ptr r,ax
    };
#else
    r = a ^ b;
#endif
    return r;
}


int INLINE my_and( short a, short b ) {
    short r;
#pragma aux _do_and = \
    "mov ax,word ptr a" \
    "and ax,word ptr b" \
    "mov word ptr r,ax" \
    ;
    _do_and();
    return r;
}

inline int foo( int x, int y ) {
    int r;
    r = sub( x, y );
    if( r != (x-y) ) _fail;
    r = burst( x, y );
    if( r != ((( x^y ) & y ) | y ) ) _fail;
    r = uses_burst( x, y );
    x++;
    y-=2;
    if( r != ((( x^y ) & y ) | y ) ) _fail;
    return( r );
}

int five = 5;
int two = 2;
int twelve = 12;
int three = 3;

int main() {
    if( foo( five, three ) != 1 ) _fail;
    if( my_xor( two, two ) != 0 ) _fail;
    if( my_xor( three, twelve ) != 15 ) _fail;
    if( my_and( two, two ) != 2 ) _fail;
    if( my_and( three, twelve ) != 0 ) _fail;
    _PASS;
}
#else
ALWAYS_PASS
#endif
