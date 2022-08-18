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
* Description:  Internal helpers dealing with Win32 file attributes.
*
****************************************************************************/


#include "variety.h"
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include "fileacc.h"
#include "openmode.h"
#include <windows.h>
#include "ntext.h"


void __GetNTCreateAttr( unsigned attr, LPDWORD desired_access, LPDWORD os_attr )
{
    if( attr & _A_RDONLY ) {
        *desired_access = GENERIC_READ;
        *os_attr = FILE_ATTRIBUTE_READONLY;
    } else {
        *desired_access = GENERIC_READ | GENERIC_WRITE;
        *os_attr = FILE_ATTRIBUTE_NORMAL;
    }
    if( attr & _A_HIDDEN ) {
        *os_attr |= FILE_ATTRIBUTE_HIDDEN;
    }
    if( attr & _A_SYSTEM ) {
        *os_attr |= FILE_ATTRIBUTE_SYSTEM;
    }

}

void __GetNTAccessAttr( unsigned rwmode, LPDWORD desired_access, LPDWORD os_attr )
{
    if( rwmode == O_RDWR ) {
        *desired_access = GENERIC_READ | GENERIC_WRITE;
        *os_attr = FILE_ATTRIBUTE_NORMAL;
    } else if( rwmode == O_WRONLY ) {
        *desired_access = GENERIC_WRITE;
        *os_attr = FILE_ATTRIBUTE_NORMAL;
    } else {
        *desired_access = GENERIC_READ;
        *os_attr = FILE_ATTRIBUTE_READONLY;
    }
}

void __GetNTShareAttr( unsigned mode, LPDWORD share_mode )
{
    unsigned share;
    unsigned rwmode;

    share  = mode & OPENMODE_SHARE_MASK;
    rwmode = mode & OPENMODE_ACCESS_MASK;

    switch( share ) {
    case OPENMODE_DENY_COMPAT:
        /*
         * Always allow reopening for read.  Since we don't want the same
         * file opened twice for writing, only allow the file to be opened
         * for writing hereafter if we're opening it now in read-only mode.
         *                      -- M. Hildebrand, 14-jun-96
         */
        *share_mode = FILE_SHARE_READ;  /* can always open again for read */
        if( rwmode == OPENMODE_ACCESS_RDONLY ) {
            *share_mode |= FILE_SHARE_WRITE;    /* can open again for write */
        }
        break;
    case OPENMODE_DENY_ALL:
        *share_mode = 0;
        break;
    case OPENMODE_DENY_READ:
        *share_mode = FILE_SHARE_WRITE;
        break;
    case OPENMODE_DENY_WRITE:
        *share_mode = FILE_SHARE_READ;
        break;
    case OPENMODE_DENY_NONE:
        *share_mode = FILE_SHARE_READ|FILE_SHARE_WRITE;
        break;
    };
}
