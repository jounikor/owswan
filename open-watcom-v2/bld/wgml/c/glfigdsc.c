/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2004-2013 The Open Watcom Contributors. All Rights Reserved.
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
* Description: WGML implement :FIGDESC tag for LAYOUT processing
*
****************************************************************************/

#include "wgml.h"

#include "clibext.h"

/***************************************************************************/
/*   :FIGDESC attributes                                                   */
/***************************************************************************/
const   lay_att     figdesc_att[3] =
    { e_pre_lines, e_font, e_dummy_zero };

/*********************************************************************************/
/*Define the characteristics of the figure description entity.                   */
/*:FIGDESC                                                                       */
/*        pre_lines = 1                                                          */
/*        font = 0                                                               */
/*                                                                               */
/*pre_lines This attribute accepts vertical space units. A zero value means that */
/*no lines are skipped. If the skip value is a line unit, it is multiplied       */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for          */
/*more information). The resulting number of lines are skipped                   */
/*before the figure description. If the document entity starts a new             */
/*page, the specified number of lines are still skipped. The pre-lines           */
/*value is not merged with the previous document entity's post-skip              */
/*value. If the previous tag was :figcap, this value is ignored.                 */
/*                                                                               */
/*font This attribute accepts a non-negative integer number. If a font           */
/*number is used for which no font has been defined, WATCOM                      */
/*Script/GML will use font zero. The font numbers from zero to three             */
/*correspond directly to the highlighting levels specified by the                */
/*highlighting phrase GML tags. The font attribute defines the font of           */
/*the figure description. The font value is linked to the pre_lines              */
/*attribute (see "Font Linkage" on page 77).                                     */
/*********************************************************************************/


/***************************************************************************/
/*  lay_figdesc                                                            */
/***************************************************************************/

void    lay_figdesc( lay_tag ltag )
{
    char        *   p;
    condcode        cc;
    int             k;
    lay_att         curr;
    att_args        l_args;
    int             cvterr;

    /* unused parameters */ (void)ltag;

    p = scan_start;
    cvterr = false;

    if( !GlobFlags.firstpass ) {
        scan_start = scan_stop;
        eat_lay_sub_tag();
        return;                         // process during first pass only
    }
    if( ProcFlags.lay_xxx != el_figdesc ) {
        ProcFlags.lay_xxx = el_figdesc;
    }
    cc = get_lay_sub_and_value( &l_args );  // get att with value
    while( cc == pos ) {
        cvterr = -1;
        for( k = 0, curr = figdesc_att[k]; curr > 0; k++, curr = figdesc_att[k] ) {

            if( !strnicmp( att_names[curr], l_args.start[0], l_args.len[0] ) ) {
                p = l_args.start[1];

                switch( curr ) {
                case   e_pre_lines:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.figdesc.pre_lines );
                    break;
                case   e_font:
                    cvterr = i_font_number( p, curr, &layout_work.figdesc.font );
                    if( layout_work.figdesc.font >= wgml_font_cnt ) {
                        layout_work.figdesc.font = 0;
                    }
                    break;
                default:
                    out_msg( "WGML logic error.\n");
                    cvterr = true;
                    break;
                }
                if( cvterr ) {          // there was an error
                    err_count++;
                    g_err( err_att_val_inv );
                    file_mac_info();
                }
                break;                  // break out of for loop
            }
        }
        if( cvterr < 0 ) {
            err_count++;
            g_err( err_att_name_inv );
            file_mac_info();
        }
        cc = get_lay_sub_and_value( &l_args );  // get att with value
    }
    scan_start = scan_stop;
    return;
}

