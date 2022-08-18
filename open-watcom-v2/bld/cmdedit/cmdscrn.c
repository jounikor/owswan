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


#include "cmdedit.h"

void FlipScreenCursor( void )
/***************************/
{
    char        buffer[2];
    USHORT      length;

    length = 2;
    VioReadCellStr( buffer, &length, Row+RowOffset, StartCol+ColOffset, 0 );
    buffer[1] ^= 0x77;
    VioWrtCellStr( buffer, length, Row+RowOffset, StartCol+ColOffset, 0 );
}


void ReadScreen( int next_line )
/******************************/
{
    USHORT len;

    len = SCREEN_WIDTH - ( StartCol + ColOffset );
    if( next_line ) {
        len += SCREEN_WIDTH;
        if( len > LINE_WIDTH ) {
            len = LINE_WIDTH;
        }
    }
    VioReadCharStr( Line, &len, Row + RowOffset, StartCol + ColOffset, 0 );
    for( MaxCursor = len; len-- > 0; MaxCursor = len ) {
        if( Line[len] != ' ' ) {
            break;
        }
    }
    Cursor = next_line ? MaxCursor : 0;
    Draw = TRUE;
    Base = 0;
    Edited = TRUE;
    if( !next_line && MaxCursor == SCREEN_WIDTH ) {
        ReadScreen( 1 );
    }
}


void RightScreen( void )
/**********************/
{
    if( RowOffset == 0 ) return;
    if( StartCol+ColOffset == (SCREEN_WIDTH-1) ) return;
    FlipScreenCursor();
    ColOffset++;
    FlipScreenCursor();
    ReadScreen( 0 );
}


void LeftScreen( void )
/*********************/
{
    if( RowOffset == 0 ) return;
    if( StartCol+ColOffset == 0 ) return;
    FlipScreenCursor();
    ColOffset--;
    FlipScreenCursor();
    ReadScreen( 0 );
}


void UpScreen( void )
/*******************/
{
    if( RowOffset != 0 ) {
        FlipScreenCursor();
    }
    if( Row+RowOffset == 0 ) {
        RowOffset = 0;
    } else {
        RowOffset--;
    }
    if( RowOffset == 0 ) {
        MaxCursor = 0;
        Cursor = MaxCursor;
        Draw = TRUE;
        Base = 0;
        Edited = FALSE;
        return;
    }
    FlipScreenCursor();
    ReadScreen( 0 );
}


void DownScreen( void )
/*********************/
{
    if( RowOffset != 0 ) {
        FlipScreenCursor();
        RowOffset++;
    } else {
        RowOffset = -Row;
    }
    if( RowOffset == 0 ) {
        MaxCursor = 0;
        Cursor = MaxCursor;
        Draw = TRUE;
        Base = 0;
        Edited = FALSE;
        return;
    }
    FlipScreenCursor();
    ReadScreen( 0 );
}
