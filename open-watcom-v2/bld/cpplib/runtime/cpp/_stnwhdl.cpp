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


// OPNEW.CPP -- operator ::new default definition
//           -- set operator new handler, this routine is used to set the
//              handler to deal with allocation failure during operator new
//           -- the default new handler is NULL
//
// 94/02/27  -- G.R.Bentz        -- defined for MFC
// 94/10/14  -- J.W.Welch        -- use PgmThread
// 94/11/18  -- J.W.Welch        -- include rtexcept

#include "cpplib.h"
#include "rtexcept.h"


_WPRTLINK
PFU _set_new_handler(           // SET HANDLER FOR NEW FAILURE
    PFU handler )               // - new handler to be used
{
    THREAD_CTL *thr;            // - thread control
    PFU previous_handler;

    thr = PgmThread();
    previous_handler = thr->_new_handler;
    thr->_new_handler = handler;
    return( previous_handler );
}
