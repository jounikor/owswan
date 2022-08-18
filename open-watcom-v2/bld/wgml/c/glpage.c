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
* Description: WGML implement :PAGE tag for LAYOUT processing
*
****************************************************************************/

#include "wgml.h"

#include "clibext.h"

/***************************************************************************/
/*   :PAGE  attributes                                                     */
/***************************************************************************/
const   lay_att     page_att[5] =
    { e_top_margin, e_left_margin, e_right_margin, e_depth, e_dummy_zero };

/***************************************************************************/
/*                                                                         */
/* Define the boundaries of the document on the output page.               */
/*                                                                         */
/*  Format:                                                                */
/*          :PAGE                                                          */
/*              top_margin = 0                                             */
/*              left_margin = '1i'                                         */
/*              right_margin = '7i'                                        */
/*              depth = '9.66i'                                            */
/*                                                                         */
/* top_margin   The top margin attribute specifies the amount of space     */
/*              between the top of the page and the start of the output    */
/*              text. This attribute accepts any valid vertical space unit.*/
/*                                                                         */
/* left_margin  The left margin attribute specifies the amount of space    */
/*              between the left side of the page and the start of the     */
/*              output text. This attribute accepts any valid horizontal   */
/*              space unit.                                                */
/*                                                                         */
/* right_margin The right margin attribute specifies the amount of space   */
/*              between the left side of the page and the right margin of  */
/*              the output text. This attribute accepts any valid          */
/*              horizontal space unit.                                     */
/*                                                                         */
/* depth        The depth attribute specifies the depth of the output page.*/
/*              Output text starts at the top margin and ends at the bottom*/
/*              margin of the page. The bottom margin is the sum of the    */
/*              top_margin and depth attribute values. This attribute      */
/*              accepts any valid vertical space unit.                     */
/***************************************************************************/


/***************************************************************************/
/*  lay_page                                                               */
/***************************************************************************/

void    lay_page( lay_tag ltag )
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
    if( ProcFlags.lay_xxx != el_page ) {
        ProcFlags.lay_xxx = el_page;
    }
    cc = get_lay_sub_and_value( &l_args );  // get one with value
    while( cc == pos ) {
        cvterr = -1;
        for( k = 0, curr = page_att[k]; curr > 0; k++, curr = page_att[k] ) {

            if( !strnicmp( att_names[curr], l_args.start[0], l_args.len[0] ) ) {
                p = l_args.start[1];

                switch( curr ) {
                case   e_top_margin:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.page.top_margin );
                    break;
                case   e_left_margin:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.page.left_margin );
                    break;
                case   e_right_margin:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.page.right_margin );
                    break;
                case   e_depth:
                    cvterr = i_space_unit( p, curr, &layout_work.page.depth );
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
        cc = get_lay_sub_and_value( &l_args );  // get one with value
    }
    scan_start = scan_stop;
    return;
}
