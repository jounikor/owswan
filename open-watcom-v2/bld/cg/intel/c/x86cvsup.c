/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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


#include "_cgstd.h"
#include "coderep.h"
#include "procdef.h"
#include "model.h"
#include "x86objd.h"
#include "zoiks.h"
#include "cgaux.h"
#include "cvdbg.h"
#include "dbsyms.h"
#include "cvsyms.h"
#include "objout.h"
#include "x86obj.h"


void    CVDefSegs( void )
/***********************/
{
    if( _IsModel( DBG_LOCALS ) ) {
        CVSyms = DbgSegDef( "$$SYMBOLS", "DEBSYM", SEG_COMB_PRIVATE+SEG_USE_32 );
    }
    if( _IsModel( DBG_TYPES ) ) {
        CVTypes = DbgSegDef( "$$TYPES", "DEBTYP", SEG_COMB_PRIVATE+SEG_USE_32 );
    }
}

