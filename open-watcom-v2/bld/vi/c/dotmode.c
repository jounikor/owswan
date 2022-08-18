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

/*
 * SaveDotCmd - save the current dot buffer
 */
void SaveDotCmd( void )
{
    memcpy( DotCmd, DotBuffer, DotDigits * sizeof( vi_key ) );
    DotCmdCount = DotDigits;

} /* SaveDotCmd */

/*
 * DoDotMode - process the pressing of '.'
 */
vi_rc DoDotMode( void )
{
    vi_rc   rc = ERR_NO_ERR;
    long    repcnt;

    /*
     * check if memorizing; '.' causes memorizing to end
     */
    if( EditFlags.MemorizeMode ) {
        EditFlags.MemorizeMode = false;
        Message1( "%sended", MEMORIZE_MODE );
        DotDigits--;
        SaveDotCmd();
        DotDigits = 0;
        return( ERR_NO_ERR );
    }

    /*
     * check if we can enter dot mode
     */
    if( EditFlags.DotMode ) {
        return( ERR_ALREADY_IN_DOT_MODE );
    }
    if( EditFlags.InputKeyMapMode ) {
        return( ERR_INPUT_KEYMAP_RUNNING );
    }
    if( DotCmdCount <= 0 ) {
        return( ERR_NO_ERR );
    }

    /*
     * yes, run until done
     */
    repcnt = GetRepeatCount();
    StartUndoGroup( UndoStack );
    while( repcnt-- > 0 ) {
        EditFlags.DotMode = true;
        DotCount = 0;
        LastError = ERR_NO_ERR;
        for( ;; ) {
            LastEvent = GetNextEvent( false );
            if( DotCount > DotCmdCount ) {
                break;
            }
            rc = DoLastEvent();
            if( rc > ERR_NO_ERR || LastError != ERR_NO_ERR ) {
                break;
            }
            DoneLastEvent( rc, true );
        }
    }
    EndUndoGroup( UndoStack );
    DCDisplayAllLines();
    return( rc );

} /* DoDotMode */

/*
 * DoAltDotMode - do alternate dot mode ('=')
 */
vi_rc DoAltDotMode( void )
{
    vi_rc   rc = ERR_NO_ERR;
    long    repcnt;

    if( EditFlags.AltMemorizeMode ) {
        EditFlags.AltMemorizeMode = false;
        Message1( "Alternate %sended", MEMORIZE_MODE );
        AltDotDigits--;
        return( ERR_NO_ERR );
    }

    /*
     * check if we can enter dot mode
     */
    if( EditFlags.AltDotMode || EditFlags.DotMode ) {
        return( ERR_ALREADY_IN_DOT_MODE );
    }
    if( EditFlags.InputKeyMapMode ) {
        return( ERR_INPUT_KEYMAP_RUNNING );
    }
    if( AltDotDigits <= 0 ) {
        return( ERR_NO_ERR );
    }

    /*
     * yes, run until done
     */
    DotDigits = 0;
    repcnt = GetRepeatCount();
    StartUndoGroup( UndoStack );
    while( repcnt-- > 0 ) {
        EditFlags.AltDotMode = true;
        AltDotCount = 0;
        LastError = ERR_NO_ERR;
        for( ;; ) {
            LastEvent = GetNextEvent( false );
            if( AltDotCount > AltDotDigits ) {
                break;
            }
            rc = DoLastEvent();
            if( rc > ERR_NO_ERR || LastError != ERR_NO_ERR ) {
                break;
            }
            DoneLastEvent( rc, false );
        }
        EditFlags.AltDotMode = false;
    }
    EndUndoGroup( UndoStack );
    DCDisplayAllLines();
    return( rc );

} /* DoAltDotMode */
