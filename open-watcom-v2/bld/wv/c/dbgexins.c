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


#include <stdlib.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgerr.h"
#include "dbgscan.h"
#include "dbgparse.h"
#include "dbgexins.h"
#include "dbgdot.h"


extern bool             HasLinInfo(address );
extern void             *WndAsmInspect(address);
extern void             *WndSrcInspect(address);


/*
 * DoAsmExam -- examine assembly on input from prompt window
 */

/*
 * AsmExam -- process examine/assembly command
 */

void AsmExam( void )
{
    address     addr;
//    bool        prompt;

    addr = GetCodeDot();
    OptMemAddr( EXPR_CODE, &addr );
//    prompt = true;
    ReqEOC();
    WndAsmInspect( addr );
}


/*
 * DoSrcExam -- examine source on input from prompt window
 */

/*
 * SrcExam -- process examine/source command
 */

void SrcExam( void )
{
    address     addr;
//    bool        prompt;

    addr = GetCodeDot();
    OptMemAddr( EXPR_CODE, &addr );
//    prompt = true;
    ReqEOC();
    WndSrcInspect( addr );
}
