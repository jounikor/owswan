/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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


//
// LOGFILE      : file logging support
//

#include "ftnstd.h"
#include "rundat.h"
#include "rtflags.h"
#include "units.h"
#include "hexcnv.h"
#include "wrutils.h"


extern  void                    SwapIOCBs(void);


void    LogFile( ftnfile *fcb ) {
//===============================

    uint        len;
    char        *ptr;
    pointer     fp;
    char        buff[3];

    if( fcb->flags & FTN_LOG_IO ) {
        SwapIOCBs();
        fp = IOCB->fileinfo->fileptr;
        if( ( RTFlags & DB_USED ) == 0 ) {
            IOCB->fileinfo->fileptr = NULL;
        }
        if( (fcb->unitid == STANDARD_OUTPUT) || (fcb->unitid == STANDARD_INPUT) ) {
            Drop( '*' );
        } else {
            SendInt( fcb->unitid );
        }
        if( AltIOCB->flags & IOF_OUTPT ) {
            Drop( '>' );
            len = fcb->col;
        } else {
            Drop( '<' );
            len = fcb->len;
        }
        if( fcb->formatted == UNFORMATTED_IO ) {
            ptr = fcb->buffer;
            while( len != 0 ) {
                BToHS( ptr, 2, &buff );
                SendWSLStr( &buff );
                ++ptr;
                --len;
            }
        } else {
            SendStr( fcb->buffer, len );
        }
        SendEOR();
        IOCB->fileinfo->fileptr = fp;
        SwapIOCBs();
    }
}
