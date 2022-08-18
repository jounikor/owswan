/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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


#include <conio.h>
#include "gdefn.h"
#include "gbios.h"


#if defined( _DEFAULT_WINDOWS )

static void PutPalette( short pixval, WPI_COLOUR colour )
//=================================================

{
    short               red, green, blue;

    red = ( colour & 0x000000ff );
    red = red * 4.0625f;
    green = ( colour & 0x0000ff00 ) >> 8;
    green = green * 4.0625f;
    blue = ( colour & 0x00ff0000 ) >> 16;
    blue = blue * 4.0625f;
    _Set_RGB_COLOR( pixval, _wpi_getrgb( red, green, blue ) );
}


static WPI_COLOUR GetPalette( short pixval )
//==========================================

{
    return _Get_RGB_COLOR( pixval );
}


#else

/* EGA Colour Mapping
   ==================

   In modes 13 and 14, each of the 16 colour indices (or pixel values)
   may be assigned one of the 16 colours values ( 0..7, 0x10 + 8..15 ).

   In mode 16, each of the colour indices may be assigned one of the 64
   possible colours. These colour values are of the form 00rgbRGB, where
   rgb are the secondary intensities and RGB are the high intensities.
   REMAPPALETTE uses the red, green and blue values to derive an EGA
   colour value.

   In mode 16, if there is only 64K of EGA memory (4 colours) the colour
   indices are actually 0, 1, 4, 5.
*/


#define PRIMARY         1
#define SECONDARY       8


static char             EGA_Intensity[] = {
    0,                      /*  0..15 */
    SECONDARY,              /* 16..31 */
    PRIMARY,                /* 32..47 */
    PRIMARY + SECONDARY     /* 48..63 */
};


extern long GetVGAPalette( short func, short reg );
#if defined( _M_I86 )
    #pragma aux GetVGAPalette = \
            "push bp"                   \
            "int 10h"                   \
            "pop  bp"                   \
            "mov  ah,ch" /* (green) */  \
            "mov  al,dh" /* (red)   */  \
            "mov  dl,cl" /* (blue)  */  \
            "xor  dh,dh"                \
        __parm __caller [__ax] [__bx] \
        __value         [__ax __dx] \
        __modify        [__cx]
#else
    #pragma aux GetVGAPalette = \
            "push ebp"      \
            "int 10H"       \
            "pop  ebp"      \
            "xchg cl,ch"    \
            "movzx eax,cx"  \
            "shl  eax,08H"  \
            "mov  al,dh"    \
        __parm __caller [__eax] [__ebx] \
        __value         [__eax] \
        __modify        [__ecx __edx]
#endif


static void PutPalette( short pixval, long colour )
//=================================================

{
    unsigned short      blue;
    unsigned short      green;
    unsigned short      red;
    short               cnvcol;
    short               mode;

    blue = ( (unsigned long)colour & 0x00ff0000 ) >> 16;
    green = (unsigned short)( colour & 0x0000ff00 ) >> 8;
    red = colour & 0x000000ff;
    switch( _CurrState->vc.adapter ) {
    case _MCGA :
    case _VGA :
    case _SVGA :
        VideoInt( _BIOS_SET_PALETTE + 0x10, pixval, ( green << 8 ) + blue, red << 8 );
        break;
    case _EGA :
        mode = _CurrState->vc.mode;
        if( mode == 13 || mode == 14 ) {
            cnvcol = _CnvColour( colour );
            if( cnvcol > 7 ) {
                cnvcol |= 0x10;         /* set intensity bit */
            }
        } else {
            red >>= 4;      /* map from range 0..63 to range 0..3 */
            green >>= 4;
            blue >>= 4;
            cnvcol = EGA_Intensity[blue] + ( EGA_Intensity[green] << 1 )
                                           + ( EGA_Intensity[red] << 2 );
        }
        VideoInt( _BIOS_SET_PALETTE, ( cnvcol << 8 ) + pixval, 0, 0 );
    }
}


static long GetPalette( short pixval )
//====================================

{
    long                prev;

    switch( _CurrState->vc.adapter ) {
    case _MCGA :
    case _VGA :
    case _SVGA :
        prev = GetVGAPalette( _BIOS_SET_PALETTE + 0x15, pixval );
        break;
    case _EGA :
        prev = 0;
    }
    return( prev );
}

#endif


void _RemapNum( long _WCI86FAR *colours, short num )
//=============================================

{
    short               i;

#if !defined( _DEFAULT_WINDOWS )
    if( _CurrState->vc.adapter != _EGA ) {
        if( _FastMap( colours, num ) ) {
            return;
        }
    }
#endif
    for( i = 0; i < num; ++i ) {
        PutPalette( i, colours[i] );
    }
}


_WCRTLINK long _WCI86FAR _CGRAPH _remappalette( short pixval, long colour )
/*==========================================================

   This routine sets the colour indexed by pixval to the new colour.  It
   returns the previous colour at pixval or -1 if unsuccessful. */

{
    long                prev;

    if( pixval < 0 || pixval >= _CurrState->vc.numcolors ) {
        _ErrorStatus = _GRINVALIDPARAMETER;
        return( -1 );
    }
    if( _CurrState->vc.adapter < _MCGA ) {
        _ErrorStatus = _GRERROR;
        return( -1 );
    }
    prev = GetPalette( pixval );
    PutPalette( pixval, colour );

    return( prev );
}

Entry1( _REMAPPALETTE, _remappalette ) // alternate entry-point


_WCRTLINK short _WCI86FAR _CGRAPH _remapallpalette( long _WCI86FAR *colours )
/*=======================================================

   This routine remaps the entire palette to the colours specified by
   the parameter.  It returns a success flag. */

{
    short               num;

    if( _CurrState->vc.adapter < _MCGA ||
        ( _CurrState->vc.mode == 7 || _CurrState->vc.mode == 15 ) ) {
        _ErrorStatus = _GRERROR;
        return( 0 );
    }
    if( _GrMode ) {
        num = _CurrState->vc.numcolors;
    } else {
        num = 16;       // vc.numcolors is 32
    }
    _RemapNum( colours, num );
    return( -1 );
}

Entry1( _REMAPALLPALETTE, _remapallpalette ) // alternate entry-point
