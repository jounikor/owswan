/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  File I/O error handling routines
*
****************************************************************************/


#include "ftnstd.h"
#include <errno.h>
#include "fileio.h"
#include "fileerr.h"


static io_status    Stat = { FILEIO_OK };

static  char    * const ErrMsgs[] = {
    #define pick(id,text)  text,
    #include "_fileio.h"
    #undef pick
};

bool    IOOk( b_file *io ) {
// Check for i/o error condition.

    io_status   err;

    if( io == NULL ) {
        err = Stat;
    } else {
        err = io->stat;
    }
    if( err == FILEIO_EOR ) {
        err = FILEIO_OK;
    }
    return( err == FILEIO_OK );
}


bool    EOFile( b_file *io ) {
// Check if end-of-file.

    if( io == NULL ) {
        return( Stat == FILEIO_EOF );
    } else {
        return( io->stat == FILEIO_EOF );
    }
}


void ErrorMsg( b_file *io, char *buf, size_t max_len )
{
// Get i/o error message.

    io_status   err;
    char        *msg;

    if( io == NULL ) {
        err = Stat;
    } else {
        err = io->stat;
    }
    if( err == FILEIO_SYS_ERROR ) {
        msg = strerror( errno );
    } else {
        msg = ErrMsgs[err];
    }
    strncpy( buf, msg, max_len - 1 );
    buf[max_len - 1] = NULLCHAR;
}


void    FSetTrunc( b_file *io ) {
// Set "truncated" condition.

    Stat = FILEIO_EOR;
    io->stat = FILEIO_EOR;
}


void    FSetSysErr( b_file *io ) {
// Set system i/o error condition.

    if( io != NULL ) {
        io->stat = FILEIO_SYS_ERROR;
    }
    Stat = FILEIO_SYS_ERROR;
}


void    FSetErr( io_status error, b_file *io ) {
// Set i/o error condition.

    if( io != NULL ) {
        io->stat = error;
    }
    Stat = error;
}

void    FSetEof( b_file *io ) {
// Set end-of-file condition.

    if( io != NULL ) {
        io->stat = FILEIO_EOF;
    }
    Stat = FILEIO_EOF;
}


void    FSetIOOk( b_file *io ) {
// Clear i/o error conditions.

    if( io != NULL ) {
        io->stat = FILEIO_OK;
    }
    Stat = FILEIO_OK;
}
