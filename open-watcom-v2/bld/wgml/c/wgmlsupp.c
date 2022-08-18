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
* Description:  Defines functions moved out of wgml.c so they can be linked
*               into the research programs without including main() from wgml.c
*                   free_inc_fp()
*                   free_resources()
*                   free_some_mem()
*                   get_line()
*                   get_macro_line()
*                   g_banner()
*                   inc_inc_level()
*                   my_exit()
*                   reopen_inc_fp()
*                   show_include_stack()
****************************************************************************/

#include <errno.h>
#include "wgml.h"
#include "banner.h"

#include "clibext.h"

#define CRLF            "\n"


/***************************************************************************/
/*  Output Banner if wanted and not yet done                               */
/***************************************************************************/

void g_banner( void )
{
    if( !(GlobFlags.bannerprinted | GlobFlags.quiet) ) {
        out_msg( banner1w( "Script/GML", _WGML_VERSION_ ) CRLF );
        out_msg( banner2 CRLF );
        out_msg( banner3 CRLF );
        out_msg( banner3a CRLF );
        out_msg( "Compiled with WATCOMC " _MACROSTR( __WATCOMC__ )
                 " " __DATE__ " " __TIME__ CRLF );
        mem_banner();
        GlobFlags.bannerprinted = 1;
    }
}


/***************************************************************************/
/*  increment include level                                                */
/***************************************************************************/

void    inc_inc_level( void )
{
    inc_level++;                        // start new level
    if( inc_level > max_inc_level ) {
        max_inc_level = inc_level;      // record highest level
    }
}


/***************************************************************************/
/*  Program end                                                            */
/***************************************************************************/

void my_exit( int rc )
{
    exit( rc );
}


/***************************************************************************/
/*  Try to close an opened include file                                    */
/***************************************************************************/

static  bool    free_inc_fp( void )
{
    inputcb *   ip;
    filecb  *   cb;
    int         rc;

    ip = input_cbs;
    while( ip != NULL ) {              // as long as input stack is not empty
        if( ip->fmflags & II_file ) {   // if file (not macro)
            if( (cb = ip->s.f) != NULL ) {
                if( (cb->flags & FF_open) ) {   // and file is open
                    rc = fgetpos( cb->fp, &cb->pos );
                    if( rc != 0 ) {
                        g_err( err_file_io, strerror( errno ), cb->filename );
                        err_count++;
                        g_suicide();
                    }
                    rc = fclose( cb->fp );
                    if( rc != 0 ) {
                        g_err( err_file_io, strerror( errno ), cb->filename );
                        err_count++;
                        g_suicide();
                    }
                    cb->flags &= ~FF_open;
                    return( true );
                }
            }
        }
        ip = ip->prev;                  // next higher input level
    }
    return( false );                    // nothing to close
}


/***************************************************************************/
/* reopen a file which was closed due to resource shortage and perhaps     */
/* close another one                                                       */
/***************************************************************************/

static void reopen_inc_fp( filecb *cb )
{
    int         rc;
    int         erc2;

    if( (cb->flags & FF_open) == 0 ) {
        for( ;; ) {
            cb->fp = fopen( cb->filename, "rb" );
            if( cb->fp != NULL ) break;
            erc2 = errno;
            if( errno != ENOMEM && errno != ENFILE && errno != EMFILE ) break;
            if( !free_inc_fp() ) break; // try closing an include file
        }
        if( cb->fp != NULL ) {
            rc = fsetpos( cb->fp, &cb->pos );
            if( rc != 0 ) {
                g_err( err_file_io, strerror( errno ), cb->filename );
                err_count++;
                g_suicide();
            }
            cb->flags |= FF_open;
        } else {
            g_err( err_file_io, strerror( erc2 ), cb->filename );
            err_count++;
            g_suicide();
        }
    }
    return;
}

/***************************************************************************/
/*  Report resource exhaustion: may eventually try to correct the problem  */
/***************************************************************************/

bool    free_resources( int in_errno )
{
    if( in_errno == ENOMEM) {
        g_err( err_no_memory );
    } else {
        g_err( err_no_handles );
    }
    err_count++;
    return( false );
}

/***************************************************************************/
/* free_layout_banner  free banner and banregion, the only parts of the    */
/* layout dynamically allocated                                            */
/***************************************************************************/
void    free_layout_banner( void )
{
    banner_lay_tag  * ban;
    banner_lay_tag  * ban1;
    region_lay_tag  * reg;

    ban = layout_work.banner;
    while( ban != NULL ) {
        reg = ban->region;
        while( reg != NULL ) {
            ban->region = reg->next;
            mem_free( reg );
            reg = ban->region;
        }
        ban1 = ban->next;
        mem_free( ban );
        ban = ban1;
    }
}

/***************************************************************************/
/*  free some buffers                                                      */
/***************************************************************************/

void    free_some_mem( void )
{

    if( token_buf != NULL ) {
        mem_free( token_buf );
    }
    if( alt_ext != NULL ) {
        mem_free( alt_ext );
    }
    if( def_ext != NULL ) {
        mem_free( def_ext );
    }
    if( master_fname != NULL ) {
        mem_free( master_fname );
    }
    if( master_fname_attr != NULL ) {
        mem_free( master_fname_attr );
    }
    if( dev_name != NULL ) {
        mem_free( dev_name );
    }
    if( lay_files != NULL ) {
        laystack * lwk;

        while( lay_files != NULL ) {
            lwk = lay_files->next;
            mem_free( lay_files );
            lay_files = lwk;
        }
    }
    if( out_file != NULL ) {
        mem_free( out_file );
    }
    if( out_file_attr != NULL ) {
        mem_free( out_file_attr );
    }
    if( global_dict != NULL ) {
        free_dict( &global_dict );
    }
    if( macro_dict != NULL ) {
        free_macro_dict( &macro_dict );
    }
    if( ref_dict != NULL ) {
        free_ref_dict( &ref_dict );
    }
    if( iref_dict != NULL ) {
        free_ref_dict( &iref_dict );
    }
    if( fig_dict != NULL ) {
        free_ref_dict( &fig_dict );
    }
    if( fn_dict != NULL ) {
        free_ref_dict( &fn_dict );
    }
    if( tag_dict != NULL ) {
        free_tag_dict( &tag_dict );
    }
    if( index_dict != NULL ) {
        free_index_dict( &index_dict );
    }
    if( buff2 != NULL ) {
        mem_free( buff2 );
    }
    if( workbuf != NULL ) {
        mem_free( workbuf );
    }
    if( box_line != NULL ) {
        add_box_col_stack_to_pool( box_line );
    }
    if( cur_line != NULL ) {
        add_box_col_set_to_pool( cur_line );
    }
    if( prev_line != NULL ) {
        add_box_col_set_to_pool( prev_line );
    }
    if( t_line != NULL ) {
        add_text_chars_to_pool( t_line );
        add_text_line_to_pool( t_line );
    }
    if( t_element != NULL ) {
        clear_doc_element( t_element );
    }
    if( t_page.top_ban != NULL ) {
        if( t_page.top_ban->first != NULL ) {
            clear_doc_element( t_page.top_ban->first );
            add_doc_el_to_pool( t_page.top_ban->first );
        }
        add_ban_col_to_pool( t_page.top_ban );
    }
    if( t_page.page_width != NULL ) {
        clear_doc_element( t_page.page_width );
        add_doc_el_to_pool( t_page.page_width );
    }
    if( t_page.main != NULL ) {
        if( t_page.main->main != NULL ) {
            clear_doc_element( t_page.main->main );
            add_doc_el_to_pool( t_page.main->main );
        }
        if( t_page.main->bot_fig != NULL ) {
            clear_doc_element( t_page.main->bot_fig );
            add_doc_el_to_pool( t_page.main->bot_fig );
        }
        if( t_page.main->footnote != NULL ) {
            clear_doc_element( t_page.main->footnote );
            add_doc_el_to_pool( t_page.main->footnote );
        }
        add_doc_col_to_pool( t_page.main );
    }
    if( t_page.bot_ban != NULL ) {
        if( t_page.bot_ban->first != NULL ) {
            clear_doc_element( t_page.bot_ban->first );
            add_doc_el_to_pool( t_page.bot_ban->first );
        }
        add_ban_col_to_pool( t_page.bot_ban );
    }
    if( n_page.col_top != NULL ) {
        clear_doc_element( n_page.col_top );
        add_doc_el_to_pool( n_page.col_top );
    }
    if( n_page.col_main != NULL ) {
        clear_doc_element( n_page.col_main );
        add_doc_el_to_pool( n_page.col_main );
    }
    if( n_page.col_bot != NULL ) {
        clear_doc_element( n_page.col_bot );
        add_doc_el_to_pool( n_page.col_bot );
    }
    if( n_page.col_fn != NULL ) {
        clear_doc_element( n_page.col_fn );
        add_doc_el_to_pool( n_page.col_fn );
    }

    free_layout_banner();

    free_pool_storage();

}


/***************************************************************************/
/*  get line from current macro                                            */
/***************************************************************************/
static  void    get_macro_line( void )
{
    macrocb *   cb;

    if( input_cbs->fmflags & II_file ) {// current input is file not macro
        g_err( err_logic_mac );
        show_include_stack();
        err_count++;
        g_suicide();
    }
    cb = input_cbs->s.m;

    if( cb->macline == NULL ) {         // no more macrolines
        input_cbs->fmflags |= II_eof;
        input_cbs->fmflags &= ~(II_sol | II_eol);
        cb->flags          |= FF_eof;
        *buff2              = '\0';
    } else {
        cb->lineno++;
        cb->flags          &= ~FF_eof;
        input_cbs->fmflags &= ~II_eof;
        strcpy( buff2, cb->macline->value );
        cb->macline         = cb->macline->next;
    }
}


/***************************************************************************/
/*  get line from current input ( file )                                   */
/*  skipping lines before the first one to process if neccessary           */
/*                                                                         */
/*  returns  false for EOF                                                 */
/***************************************************************************/
bool    get_line( bool display_line )
{
    filecb      *   cb;
    char        *   p;
    inp_line    *   pline;

    if( ProcFlags.reprocess_line ) {    // there was an unget
        ProcFlags.reprocess_line = false;   // only used for :LAYOUT
        return( !(input_cbs->fmflags & II_eof) );
    }
    if( input_cbs->hidden_head != NULL ) {  // line was previously split,
        strcpy( buff2, input_cbs->hidden_head->value ); // take next part
        pline = input_cbs->hidden_head;
        input_cbs->hidden_head = input_cbs->hidden_head->next;

        if( pline->sol ) {
            input_cbs->fmflags |=  II_sol;  // start of logical record
        } else {
            input_cbs->fmflags &= ~II_sol;  // not at start of input line
        }

        mem_free( pline );

        if( input_cbs->hidden_head == NULL ) {  // last part of split line
            input_cbs->hidden_tail = NULL;
            input_cbs->fmflags |= II_eol;
        }
    } else {
        if( input_cbs->pe_cb.count > 0 ) {  // .pe perform active
            strcpy( buff2, input_cbs->pe_cb.line );
            input_cbs->pe_cb.count--;
            if( input_cbs->pe_cb.count <= 0 ) {
                reset_pe_cb();
            }
        } else {
            if( input_cbs->fmflags & II_tag_mac ) {
                get_macro_line();       // input from macro line
            } else {
                cb = input_cbs->s.f;    // input from file
                if( !(cb->flags & FF_open) ) {
                    g_info( err_inf_reopen );
                    show_include_stack();
                    reopen_inc_fp( cb );
                }
                do {
                    fgetpos( cb->fp, &cb->pos );// remember position for label
                    p = fgets( buff2, buf_size, cb->fp );
                    if( p != NULL ) {
                        if( cb->lineno >= cb->linemax ) {
                            input_cbs->fmflags |= II_eof;
                            input_cbs->fmflags &= ~(II_sol | II_eol);
                            cb->flags |= FF_eof;
                            *buff2 = '\0';
                            break;
                        }
                        cb->lineno++;
                        input_cbs->fmflags |= (II_sol | II_eol);

                        if( cb->flags & FF_crlf ) {// try to delete CRLF at end
                            p += strlen( p ) - 1;
                            while( (*p == '\r') || (*p == '\n')  ) {
                                *p-- = '\0';
                            }
                        }
#if 1
                        if( ProcFlags.start_section && !ProcFlags.concat &&
                            (*buff2 == '\0') ) {
                            *buff2 = ' ';   // empty line gets 1 blank
                            *(buff2 + 1) = '\0';// requires more testing TBD
                        }
#endif
                    } else {
                        if( feof( cb->fp ) ) {
                            input_cbs->fmflags |= II_eof;
                            input_cbs->fmflags &= ~(II_sol | II_eol);
                            cb->flags |= FF_eof;
                            *buff2 = '\0';
                            break;
                        } else {
                            g_err( err_file_io, strerror( errno ), cb->filename );
                            err_count++;
                            g_suicide();
                        }
                    }
                } while( cb->lineno < cb->linemin );
            }
        }
    }

    buff2_lg = strlen( buff2 );
    *(buff2 + buff2_lg) = '\0';
    *(buff2 + buff2_lg + 1) = '\0';
    if( input_cbs->fmflags & II_file ) {
        input_cbs->s.f->usedlen = buff2_lg;
        if( GlobFlags.research ) {    // research mode
            if( ProcFlags.researchfile ) {  // for single file
                if( input_cbs->fmflags & II_research ) {// research active
                    if( research_to < input_cbs->s.f->lineno ) {
                        input_cbs->fmflags &= ~II_research;// end of research range
                    }
                } else {                // not (yet) active
                    if( research_from == input_cbs->s.f->lineno ) {
                        if( NULL != strstr( input_cbs->s.f->filename,
                                            research_file_name) ) {
                        input_cbs->fmflags |= II_research;// start of research range
                        }
                    }
                }
            } else {
                input_cbs->fmflags |= II_research;
            }
        }
    }

    if( !(input_cbs->fmflags & II_eof) ) {
        if( display_line && GlobFlags.firstpass
            && input_cbs->fmflags & II_research ) {
            printf( "%s\n", buff2 );
        }
    }
    return( !(input_cbs->fmflags & II_eof) );
}


/***************************************************************************/
/*  output the filenames + lines which were included                       */
/***************************************************************************/

void    show_include_stack( void )
{
    inputcb *   ip;
    char        linestr[MAX_L_AS_STR];
    char        linemac[MAX_L_AS_STR];


    if( input_cbs != NULL ) {
        if( input_cbs->fmflags & II_tag_mac ) {
            ultoa( input_cbs->s.m->lineno, linestr, 10 );
            ultoa( input_cbs->s.m->mac->lineno, linemac, 10 );
            g_info( err_inf_mac_def, linestr, input_cbs->s.m->mac->name,
                    linemac, input_cbs->s.m->mac->mac_file_name);
//      } else {
//          ultoa( input_cbs->s.f->lineno, linestr, 10 );
//          g_info( inf_file_line, linestr, input_cbs->s.f->filename );
        }
        ip = input_cbs->prev;
    } else {
        ip = NULL;
    }
    out_msg( "\n" );
    while( ip != NULL ) {
        switch( ip->fmflags & II_input ) {
        case    II_file:
            ultoa( ip->s.f->lineno, linestr, 10 );
            g_info( err_inf_line_file, linestr, ip->s.f->filename );
            break;
        case    II_tag :
            g_info( err_inf_tag, ip->s.m->tag->name );
            // fallthrough
        case    II_macro :
            ultoa( ip->s.m->lineno, linestr, 10 );
            ultoa( ip->s.m->mac->lineno, linemac, 10 );
            g_info( err_inf_mac_def, linestr, ip->s.m->mac->name,
                    linemac, ip->s.m->mac->mac_file_name);
            break;
        default:
            g_info( err_inc_unknown );
            break;
        }
        ip = ip->prev;
    }
    return;
}

