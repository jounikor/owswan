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
* Description:  WORD/DWORD static and edit fields.
*
****************************************************************************/


#include "commonui.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include "bool.h"
#include "font.h"
#include "dlgmod.h"

/*
 * SetDWORDStaticField - set a dialog item DWORD
 */
void SetDWORDStaticField( HWND hwnd, int id, DWORD reg )
{
    char        buff[32];

    sprintf( buff, "%08lx", reg );
    SetDlgMonoFont( hwnd, id );
    SetDlgItemText( hwnd, id, buff );

} /* SetDWORDStaticField */

/*
 * SetWORDStaticField - set a dialog item WORD
 */
void SetWORDStaticField( HWND hwnd, int id, WORD reg )
{
    char        buff[32];

    sprintf( buff, "%04x", reg );
    SetDlgMonoFont( hwnd, id );
    SetDlgItemText( hwnd, id, buff );

} /* SetWORDStaticField */

/*
 * SetDWORDEditField - set a dialog item DWORD
 */
void SetDWORDEditField( HWND hwnd, int id, DWORD reg )
{
    char        buff[32];

    sprintf( buff, "%08lx", reg );
    SetDlgMonoFont( hwnd, id );
    SetDlgItemText( hwnd, id, buff );

} /* SetDWORDEditFiled */

/*
 * SetWORDEditField - set a dialog item WORD
 */
void SetWORDEditField( HWND hwnd, int id, WORD reg )
{
    char        buff[32];

    sprintf( buff, "%04x", reg );
    SetDlgMonoFont( hwnd, id );
    SetDlgItemText( hwnd, id, buff );

} /* SetWORDEditFiled */

/*
 * GetDWORDEditField - set a dialog item DWORD
 */
void GetDWORDEditField( HWND hwnd, int id, DWORD *reg )
{
    char        buff[32];

    GetDlgItemText( hwnd, id, (LPSTR) buff, 32 );
    *reg = strtoul( buff, NULL, 16 );

} /* GetDWORDEditFiled */

/*
 * GetWORDEditField - set a dialog item WORD
 */
void GetWORDEditField( HWND hwnd, int id, WORD *reg )
{
    char        buff[32];

    GetDlgItemText( hwnd, id, (LPSTR) buff, 32 );
    *reg = (WORD)strtoul( buff, NULL, 16 );

} /* GetWORDEditFiled */
