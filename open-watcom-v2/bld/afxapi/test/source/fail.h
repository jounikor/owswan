#ifndef _FAIL_H
#define _FAIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned errors;
extern unsigned main_terminated;

extern void fail( unsigned line );
#define _fail   (fail( __LINE__ ))

unsigned errors;
unsigned main_terminated;

void fail( unsigned line )
{
    ++errors;
    printf( "failure on line %u\n", line );
    if( main_terminated || errors > 5 ) {
        abort();
    }
}

#define _PASS \
    if( !errors ) { \
        puts( "PASS " __FILE__ ); \
        fflush( stdout ); \
    } \
    main_terminated = 1; \
    return( errors != 0 );

#define _PASS_EXIT \
    if( !errors ) { \
        puts( "PASS " __FILE__ ); \
        fflush( stdout ); \
    } \
    main_terminated = 1; \
    exit( errors != 0 );

#endif
