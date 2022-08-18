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
* Description:  RDOS waitpid() implementation.
*
****************************************************************************/


#include "variety.h"
#include <sys/wait.h>
#include "rdos.h"
#include "_rdos.h"

_WCRTLINK pid_t waitpid( pid_t __pid, int *__stat_loc, int __options )
{
    int wait;

    /* unused parameters */ (void)__stat_loc; (void)__options;

    wait = RdosCreateWait();
    RdosAddWaitForProcessEnd( wait, __pid, (void *)__pid );
    while (RdosIsProcessRunning( __pid )) {
        RdosWaitForever( wait );
    }
    RdosCloseWait( wait );

    return( __pid );
}
