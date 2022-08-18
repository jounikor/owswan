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
* Description:  BIOS-style functions for *nix
*
****************************************************************************/


#include "vi.h"
#include "win.h"

uint_32 BIOSGetColorRegister( unsigned short a )
{
    /* unused parameters */ (void)a;

    return( 0 );
}

void    BIOSSetBlinkAttr( unsigned char on )
{
    /* unused parameters */ (void)on;
}

void    BIOSSetColorRegister( unsigned short reg, unsigned char r, unsigned char g, unsigned char b )
{
    /* unused parameters */ (void)reg; (void)r; (void)g; (void)b;
}

void    BIOSSetCursorPos( unsigned char page, unsigned char row, unsigned char col )
{
    /* unused parameters */ (void)page; (void)row; (void)col;
}

unsigned short  BIOSGetCursorPos( unsigned char page )
{
    /* unused parameters */ (void)page;

    return( 0 );
}

int     KeyboardInit( void )
{
    return( 0 );
}

unsigned    BIOSGetKeyboard( unsigned *scan )
{
    if( scan != NULL )
        *scan = 0;
    return( VI_KEY( NULL ) );
}

bool    BIOSKeyboardHit( void )
{
    return( 0 );
}

void    MyVioShowBuf( size_t offset, unsigned nchars )
{
    /* unused parameters */ (void)offset; (void)nchars;
}
