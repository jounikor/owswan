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


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgmem.h"
#include "dui.h"
#include "banner.h"
#include "litdef.h"

#ifdef JAPANESE
  #define pick(c,e,j) LITSTR( c, j )
#else
  #define pick(c,e,j) LITSTR( c, e )
#endif

#define LITSTR( x, y ) char *LIT_DUI( x );
#include "wddui.str"
#undef LITSTR

void DUIInitLiterals( void )
{
    #define LITSTR( x, y ) LIT_DUI( x ) = DUILoadString( DBG_DUI_LITERAL_##x );
    #include "wddui.str"
    #undef LITSTR
}

void DUIFiniLiterals( void )
{
    #define LITSTR( x, y ) DUIFreeString( LIT_DUI( x ) );
    #include "wddui.str"
    #undef LITSTR
}
#undef pick
