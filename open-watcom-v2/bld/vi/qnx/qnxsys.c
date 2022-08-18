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


#include "vi.h"
#include <i86.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/fd.h>
#include <sys/kernel.h>
#include <sys/psinfo.h>
#include <sys/name.h>
#include <sys/vc.h>
#include "vibios.h"
#include "win.h"
#include "dosx.h"


extern int                      PageCnt;
extern struct _console_ctrl     *QNXCon;
extern int                      QNXConHandle;
extern unsigned int             QNXConsole;

int FileSysNeedsCR( int handle )
{
    struct _fd_entry    fd_info;
    struct _psinfo      ps_info;
    pid_t               dos_pid;

    if( qnx_fd_query( 0, 0, handle, &fd_info ) != handle )
        return( 0 );
    dos_pid = qnx_name_locate( fd_info.nid, "qnx/dosfsys", 0, NULL );
    if( dos_pid == -1 )
        return( 0 );
    if( qnx_psinfo( PROC_PID, dos_pid, &ps_info, 0, NULL ) != dos_pid )
        return( 0 );
    if( ps_info.flags & _PPF_VID ) {
        qnx_vc_detach( dos_pid );
        dos_pid =  ps_info.un.vproc.remote_pid;

    }
    return( dos_pid == fd_info.pid );
}

/*
 * NewCursor - change cursor to insert mode type
 */
void NewCursor( window_id wid, cursor_type ct )
{
    unsigned char   base,nbase;

    wid = wid;
    if( EditFlags.Monocolor ) {
        base = 14;
    } else {
        base = 16;
    }
    nbase = ( (unsigned)base * ( 100 - ct.height ) ) / 100;
    BIOSSetCursorTyp( nbase, base - 1 );

} /* NewCursor */

/*
 * MyBeep - ring beeper
 */
void MyBeep( void )
{
    if( EditFlags.BeepFlag ) {
    }

} /* MyBeep */

/*
 * ScreenInit - get screen info
 */
void ScreenInit( void )
{
    struct _osinfo              info;
    int                         rows, cols;
    unsigned                    size;
    unsigned                    seg;
    vi_rc                       rc;


    QNXCon = console_open( QNXConHandle, O_WRONLY );
    if( QNXCon == NULL ) {
        // FatalError( ERR_BAD_TERMINAL );
        ChangeDirectory( HomeDirectory );
        exit( 0 );
    }
    if( console_size( QNXCon, QNXConsole, 0, 0, &rows, &cols ) != 0 ) {
        console_close( QNXCon );
        FatalError( ERR_BAD_TERMINAL );
    }
    rc = BIOSKeyboardInit();
    if( rc != ERR_NO_ERR ) {
        console_close( QNXCon );
        FatalError( rc );
    }
    EditVars.WindMaxWidth = cols;
    EditVars.WindMaxHeight = rows;

    qnx_osinfo( 0, &info );
    switch( info.primary_monitor ) {
    case _MONITOR_PGS:
    case _MONITOR_CGA:
    case _MONITOR_PS30_COLOR:
    case _MONITOR_EGA_COLOR:
    case _MONITOR_VGA_COLOR:
        EditFlags.Color = true;
        break;
    case _MONITOR_EGA_MONO:
    case _MONITOR_VGA_MONO:
    case _MONITOR_PS30_MONO:
        EditFlags.BlackAndWhite = true;
        break;
    default:
        EditFlags.Monocolor = true;
        break;
    }
    size = cols * rows * sizeof( char_info );
    seg = qnx_segment_alloc( size );
    Scrn = _MK_FP( seg, 0 );
    ScreenPage( 0 );

} /* ScreenInit */

/*
 * ScreenFini - done with the screen
 */
void ScreenFini( void )
{
    KeyboardFini();
    if( QNXCon != NULL ) {
        console_close( QNXCon );
    }

} /* ScreenFini */

/*
 * ChkExtendedKbd - look for extended keyboard type
 */
void ChkExtendedKbd( void )
{
    EditFlags.ExtendedKeyboard = true;

} /* ChkExtendedKbd */

/*
 * MemSize - return amount of dos memory left (in 16 byte paragraphs)
 */
long MemSize( void )
{
    return( 0 );
} /* MemSize */

/*
 * ScreenPage - set the screen page to active/inactive
 */
void ScreenPage( int page )
{
    PageCnt += page;

} /* ScreenPage */

/*
 * ShiftDown - test if shift key is down
 */
bool ShiftDown( void )
{
    return( false );

} /* ShiftDown */

static bool hadCapsLock;

/*
 * TurnOffCapsLock - switch off caps lock
 */
void TurnOffCapsLock( void )
{
    hadCapsLock = false;

} /* TurnOffCapsLock */

/*
 * DoGetDriveType - get the type of drive A-Z
 */
drive_type DoGetDriveType( int drv )
{
    drv = drv;
    return( DRIVE_TYPE_NONE );

} /* DoGetDriveType */

/*
 * MyDelay - delay a specified number of milliseconds
 */
void MyDelay( int ms )
{
    delay( ms );

} /* MyDelay */

/*
 * SetCursorBlinkRate - set the current blink rate for the cursor
 */
void SetCursorBlinkRate( int cbr )
{
    EditVars.CursorBlinkRate = cbr;

} /* SetCursorBlinkRate */

vi_key GetKeyboard( void )
{
    return( GetVIKey( BIOSGetKeyboard( NULL ), 0, false ) );
}

bool KeyboardHit( void )
{
    return( BIOSKeyboardHit() );
}

void MyVioShowBuf( size_t offset, unsigned nchars )
{
    BIOSUpdateScreen( offset, nchars );
}
