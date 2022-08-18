/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "owlpriv.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


#define BUFFER_SIZE     1024

static int owl_write( owl_client_file f, const char *buff, size_t size )
{
    return( fwrite( buff, 1, size, f ) != size );
}

static long owl_tell( owl_client_file f )
{
    return( ftell( f ) );
}

static int owl_seek( owl_client_file f, long offset, int where )
{
    return( fseek( f, offset, where ) );
}

void main( int argc, char *argv[] ) {

    owl_handle          owl;
    owl_file_handle     file;
    owl_client_funcs    funcs = { owl_write, owl_tell, owl_seek, malloc, free };
    owl_buffer          *buffer;
    unsigned            i, test_size;

    owl = OWLInit( &funcs, OWL_CPU_PPC );
    file = OWLFileInit( owl, "test", stdout, OWL_FORMAT_ELF, OWL_FILE_OBJECT );
    buffer = OWLBufferInit( file );
    test_size = ( ( argc > 1 ) ? atoi( argv[ 1 ] ) : 8192 ) / sizeof( i );
    for( i = 0; i < test_size; i++ ) {
        OWLBufferWrite( buffer, &i, sizeof( i ) );
    }
    OWLBufferEmit( buffer );
    fwrite( "PASS2", 1, strlen( "PASS2" ), stdout );
    for( i = 0; i < test_size; i++ ) {
        OWLBufferPatch( buffer, i * 4, i );
    }
    OWLBufferEmit( buffer );
    OWLBufferFini( buffer );
    // OWLFileFini( file );
    // OWLFini( owl );
}
