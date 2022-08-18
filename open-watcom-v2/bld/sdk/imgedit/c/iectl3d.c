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


#include "imgedit.h"

/*
 * IECtl3dInit
 */
bool IECtl3dInit( HINSTANCE inst )
{
    bool ok;

    ok = WRCtl3dRegister( inst );
    if( ok ) {
        ok = WRCtl3dAutoSubclass( inst );
    }

    return( ok );

} /* IECtl3dInit */

/*
 * IECtl3dFini
 */
void IECtl3dFini( HINSTANCE inst )
{
    WRCtl3dUnregister( inst );

} /* IECtl3dFini */

/*
 * IECtl3dColorChange
 */
void IECtl3dColorChange( void )
{
    WRCtl3dColorChange();

} /* IECtl3dColorChange */

/*
 * IECtl3dSubclassDlg
 */
void IECtl3dSubclassDlg( HWND win, WORD w )
{
    WRCtl3dSubclassDlg( win, w );

} /* IECtl3dSubclassDlg */

/*
 * IECtl3dSubclassDlgAll
 */
void IECtl3dSubclassDlgAll( HWND win )
{
    WRCtl3dSubclassDlgAll( win );

} /* IECtl3dSubclassDlg */
