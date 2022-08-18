/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2016 The Open Watcom Contributors. All Rights Reserved.
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


#ifndef PILLCLI_H_INCLUDED
#define PILLCLI_H_INCLUDED

#include "piltypes.h"
#include "digcli.h"

#define LinkCli(n)      LinkCli ## n
#define _LinkCli(n)     _LinkCli ## n n

#define LINKCLIENTRY(n) DIGCLIENT LinkCli(n)

#define pick(r,n,p) typedef r (DIGCLIENT *_LinkCli ## n) p;
#include "_pillcli.h"
#undef

#define pick(r,n,p) extern r LINKCLIENTRY( n ) p;
#include "_pillcli.h"
#undef

#endif
