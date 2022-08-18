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
* Description:  Japanese dialogs interface.
*
****************************************************************************/


#ifndef _JDLG_H_INCLUDED
#define _JDLG_H_INCLUDED

#include "_windlg.h"


extern bool JDialogInit( void );
extern void JDialogFini( void );
extern INT_PTR JDialogBox( HINSTANCE hinst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc );
extern INT_PTR JDialogBoxIndirect( HINSTANCE hinst, TEMPLATE_HANDLE dlgtemplate, HWND hwndOwner, DLGPROC dlgproc );
extern INT_PTR JDialogBoxParam( HINSTANCE hinst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit );
extern INT_PTR JDialogBoxIndirectParam( HINSTANCE hinst, TEMPLATE_HANDLE dlgtemplate, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit );

extern HWND JCreateDialogIndirect( HINSTANCE hinst, TEMPLATE_HANDLE dlgtemplate, HWND hwndOwner, DLGPROC dlgproc );
extern HWND JCreateDialogIndirectParam( HINSTANCE hinst, TEMPLATE_HANDLE dlgtemplate, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit );
extern HWND JCreateDialog( HINSTANCE hinst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc );
extern HWND JCreateDialogParam( HINSTANCE hinst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit );

extern bool JDialogGetJFont( char **facename, WORD *pointsize );

#endif /* _JDLG_H_INCLUDED */
