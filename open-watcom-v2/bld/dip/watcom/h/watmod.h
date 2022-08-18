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


extern section_info     *FindInfo( imp_image_handle *, imp_mod_handle im );
extern mod_dbg_info     *ModPointer( imp_image_handle *, imp_mod_handle im );
extern dip_status       AdjustMods( imp_image_handle *, section_info *inf, unsigned long adjust );
extern void             SetModBase( imp_image_handle * );
extern void             ModInfoFini( section_info *inf );
extern unsigned         ModInfoSplit( imp_image_handle *, info_block *blk, section_info *inf );
extern word             ModOff2Idx( section_info *inf, word off );
extern walk_result      MyWalkModList( imp_image_handle *, DIP_INT_MOD_WALKER *wk, void *d );
extern size_t           PrimaryCueFile( imp_image_handle *, imp_cue_handle *, char *buff, size_t buff_size );
