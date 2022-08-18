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
* Description:  DWARF DIP module management function prototypes.
*
****************************************************************************/


extern dip_status       InitModMap( imp_image_handle * );
extern void             FiniModMap( imp_image_handle * );
extern imp_mod_handle   Dwarf2Mod( imp_image_handle *, drmem_hdl mod_handle );
extern imp_mod_handle   DwarfMod( imp_image_handle *, drmem_hdl dr_sym );
extern imp_mod_handle   CuTag2Mod( imp_image_handle *, drmem_hdl cu_tag );
extern size_t           NameCopy( char *buff, const char *from, size_t buff_size, size_t len );
extern walk_result      DFWalkModList( imp_image_handle *, DIP_IMP_MOD_WALKER wk, void *d );
extern walk_result      DFWalkModListSrc( imp_image_handle *, bool src, DIP_IMP_MOD_WALKER wk, void *d );
extern search_result    DFAddrMod( imp_image_handle *, address a, imp_mod_handle *im );
extern bool             ClearMods( imp_image_handle * );
extern void             SetModPubNames( imp_image_handle *, drmem_hdl mod_handle );
