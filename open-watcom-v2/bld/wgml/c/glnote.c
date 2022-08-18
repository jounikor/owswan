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
* Description: WGML implement :NOTE tag for LAYOUT processing
*
****************************************************************************/

#include "wgml.h"

#include "clibext.h"

/***************************************************************************/
/*   :NOTE   attributes                                                    */
/***************************************************************************/
const   lay_att     note_att[8] =
    { e_left_indent, e_right_indent, e_pre_skip, e_post_skip, e_font,
      e_spacing, e_note_string, e_dummy_zero };



/*********************************************************************************/
/*Define the characteristics of the note entity.                                 */
/*:NOTE                                                                          */
/*        left_indent = 0                                                        */
/*        right_indent = 0                                                       */
/*        pre_skip = 1                                                           */
/*        post_skip = 1                                                          */
/*        font = 2                                                               */
/*        spacing = 1                                                            */
/*        note_string = "NOTE: "                                                 */
/*                                                                               */
/*left_indent This attribute accepts any valid horizontal space unit. The left   */
/*indent value is added to the current left margin. The left margin will         */
/*be reset to its previous value at the end of the note.                         */
/*                                                                               */
/*right_indent This attribute accepts any valid horizontal space unit. The right */
/*indent value is subtracted from the current right margin. The right            */
/*margin will be reset to its previous value at the end of the note.             */
/*                                                                               */
/*pre_skip This attribute accepts vertical space units. A zero value means that  */
/*no lines are skipped. If the skip value is a line unit, it is multiplied       */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for          */
/*more information). The resulting amount of space is skipped before             */
/*the note. The pre-skip will be merged with the previous document               */
/*entity's post-skip value. If a pre-skip occurs at the beginning of an          */
/*output page, the pre-skip value has no effect.                                 */
/*                                                                               */
/*post_skip This attribute accepts vertical space units. A zero value means that */
/*no lines are skipped. If the skip value is a line unit, it is multiplied       */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for          */
/*more information). The resulting amount of space is skipped after              */
/*the note. The post-skip will be merged with the next document                  */
/*entity's pre-skip value. If a post-skip occurs at the end of an output         */
/*page, any remaining part of the skip is not carried over to the next           */
/*output page.                                                                   */
/*                                                                               */
/*font This attribute accepts a non-negative integer number. If a font           */
/*number is used for which no font has been defined, WATCOM                      */
/*Script/GML will use font zero. The font numbers from zero to three             */
/*correspond directly to the highlighting levels specified by the                */
/*highlighting phrase GML tags. The font attribute defines the font of           */
/*the text specified by the note_string attribute. The font value is             */
/*linked to the left_indent, right_indent, pre_skip and post_skip                */
/*attributes (see "Font Linkage" on page 77).                                    */
/*                                                                               */
/*spacing This attribute accepts a positive integer number. The spacing          */
/*determines the number of blank lines that are output between text              */
/*lines. If the line spacing is two, each text line will take two lines in       */
/*the output. The number of blank lines between text lines will                  */
/*therefore be the spacing value minus one. The spacing attribute                */
/*defines the line spacing within the note.                                      */
/*                                                                               */
/*note_string This attribute accepts a character string. The specified string    */
/*precedes the text of the note. The length of this string determines            */
/*indentation of the note text.                                                  */
/*********************************************************************************/


/***************************************************************************/
/*  lay_note                                                               */
/***************************************************************************/

void    lay_note( lay_tag ltag )
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
    if( ProcFlags.lay_xxx != el_note ) {
        ProcFlags.lay_xxx = el_note;
    }
    cc = get_lay_sub_and_value( &l_args );  // get att with value
    while( cc == pos ) {
        cvterr = -1;
        for( k = 0, curr = note_att[k]; curr > 0; k++, curr = note_att[k] ) {

            if( !strnicmp( att_names[curr], l_args.start[0], l_args.len[0] ) ) {
                p = l_args.start[1];

                switch( curr ) {
                case   e_left_indent:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.note.left_indent );
                    break;
                case   e_right_indent:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.note.right_indent );
                    break;
                case   e_pre_skip:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.note.pre_skip );
                    break;
                case   e_post_skip:
                    cvterr = i_space_unit( p, curr,
                                           &layout_work.note.post_skip );
                    break;
                case   e_font:
                    cvterr = i_font_number( p, curr, &layout_work.note.font );
                    if( layout_work.note.font >= wgml_font_cnt ) {
                        layout_work.note.font = 0;
                    }
                    break;
                case   e_spacing:
                    cvterr = i_int8( p, curr, &layout_work.note.spacing );
                    break;
                case   e_note_string:
                    cvterr = i_xx_string( p, curr, layout_work.note.string );
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

