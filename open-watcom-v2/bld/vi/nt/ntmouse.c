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


#include "vi.h"
#include "mouse.h"

int     CurrMouseStatus;
windim  CurrMouseCol;
windim  CurrMouseRow;

/*
 * SetMouseSpeed - set mouse movement speed
 */
void SetMouseSpeed( int speed )
{
    speed = speed;

} /* SetMouseSpeed */

/*
 * SetMousePosition - set the mouse position
 */
void SetMousePosition( windim row, windim col )
{
    MouseRow = row;
    MouseCol = col;

} /* SetMousePosition */

/*
 * PollMouse - poll the mouse for it's state
 */
void PollMouse( int *status, windim *row, windim *col )
{
    *status = CurrMouseStatus;
    *col = CurrMouseCol;
    *row = CurrMouseRow;

} /* PollMouse */

/*
 * InitMouse - initialize the mouse
 */
void InitMouse( void )
{
    if( !EditFlags.UseMouse ) {
        return;
    }

    SetMousePosition( EditVars.WindMaxWidth / 2 - 1, EditVars.WindMaxHeight / 2 - 1 );
    SetMouseSpeed( EditVars.MouseSpeed );
    PollMouse( &MouseStatus, &MouseRow, &MouseCol );

} /* InitMouse */

/*
 * FiniMouse - done with the mouse
 */
void FiniMouse( void )
{
} /* FiniMouse */
