/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Retrieve or set the scheduling parameter structure.
*
****************************************************************************/


#include "variety.h"
#include <sched.h>
#include <sys/types.h>
#include "linuxsys.h"


_WCRTLINK int sched_getparam( pid_t pid, struct sched_param *sp )
{
    syscall_res res = sys_call2( SYS_sched_getparam, pid, (u_long)sp );
    __syscall_return( int, res );
}

_WCRTLINK int sched_setparam( pid_t pid, const struct sched_param *sp )
{
    syscall_res res = sys_call2( SYS_sched_setparam, pid, (u_long)sp );
    __syscall_return( int, res );
}
