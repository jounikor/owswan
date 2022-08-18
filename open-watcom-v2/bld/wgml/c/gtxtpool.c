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
* Description:  WGML utility functions for alloc / free / reuse of
*                           different structs
*
*       add_ban_col_to_pool         prepare reuse of ban_column instance(s)
*       add_box_col_set_to_pool     prepare reuse of box_col_set instance(s)
*       add_box_col_stack_to_pool   prepare reuse of box_col_stack instance(s)
*       add_doc_col_to_pool         prepare reuse of doc_column instance(s)
*       add_doc_el_to_pool          prepare reuse of doc_element instance(s)
*       add_tag_cb_to_pool          add nested tag cb
*       add_text_chars_to_pool      prepare reuse of text_chars instance(s)
*       add_text_line_to_pool       prepare reuse of a text_line instance(s)
*       alloc_ban_col               create a ban_column instance
*       alloc_box_col_set           create a box_col_set instance
*       alloc_box_col_stack         create a box_col_stack instance
*       alloc_doc_col               create a doc_column instance
*       alloc_doc_el                create a doc_element instance
*       alloc_tag_cb                nested tag cb
*       alloc_text_chars            create a text_chars instance
*       alloc_text_line             create a text_line instance
*       free_pool_storage           do free for all pools
*
****************************************************************************/

#include "wgml.h"

/***************************************************************************/
/*  allocate / reuse and init a text_chars instance                        */
/*      optionally fill in text                                            */
/***************************************************************************/
text_chars  *alloc_text_chars( const char *text, size_t cnt, font_number font )
{
    text_chars   *   curr;
    text_chars   *   prev;

    curr = text_pool;
    while( (curr != NULL) && (curr->length <= cnt) ) {
        prev = curr;
        curr = curr->next;
    }
    if( curr != NULL ) {                // we found one large enough
        if( curr == text_pool ) {       // first is large enough
            text_pool = curr->next;
        } else {
            prev->next = curr->next;    // unchain curr
        }
    } else {                            // no one large enough found
        curr = mem_alloc( sizeof( *curr ) + cnt );
        curr->length = cnt;             // set max text size

        if( text_pool == NULL ) {       // alloc 10 text_chars if pool empty
            int k;

#define text_def    16              // default length if allocated in advance

            text_pool = mem_alloc( sizeof( *prev ) + text_def );
            prev = text_pool;
            for( k = 0; k < 10; k++ ) {
                prev->length = text_def;
                prev->next = mem_alloc( sizeof( *prev ) + text_def );
                prev = prev->next;
            }
            prev->next = NULL;
            prev->length = text_def;
        }
    }

    curr->prev = NULL;
    curr->next = NULL;
    curr->type = norm;
    curr->font = font;
    curr->width = 0;
    if( text != NULL ) {                   // text supplied
        memcpy( curr->text, text, cnt ); // yes copy text
        curr->count = cnt;              // set current size
    } else {
        curr->count = 0;                // init current size
        curr->text[0] = 0;
    }
    curr->text[cnt] = 0;

    return( curr );
}


/***************************************************************************/
/*  add text_chars instance(s) to free pool for reuse                      */
/***************************************************************************/

void    add_text_chars_to_pool( text_line * a_line )
{
    text_chars      *   tw;

    if( (a_line == NULL) || (a_line->first == NULL) ) {
        return;
    }
    // free text_chars in pool only have a valid next ptr

    for( tw = a_line->first; tw->next != NULL; tw = tw->next ); //empty
    tw->next = text_pool;
    text_pool = a_line->first;

}


/***************************************************************************/
/*  allocate / reuse a text_line  instance                                 */
/***************************************************************************/

text_line   * alloc_text_line( void )
{
    text_line   *   curr;
    text_line   *   prev;
    int             k;

    curr = line_pool;
    if( curr != NULL ) {                // there is one to use
        line_pool = curr->next;
    } else {                            // pool is empty
        curr = mem_alloc( sizeof( text_line ) );

        line_pool = mem_alloc( sizeof( *prev ) );
        prev = line_pool;
        for( k = 0; k < 10; k++ ) {     // alloc 10 text_lines if pool empty
            prev->next = mem_alloc( sizeof( *prev ) );
            prev = prev->next;
        }
        prev->next = NULL;
    }

    curr->first = NULL;
    curr->last = NULL;
    curr->line_height = 0;
    curr->next = NULL;
    curr->y_address = 0;

    return( curr );
}


/***************************************************************************/
/*  add a text_line instance to free pool for reuse                        */
/***************************************************************************/

void    add_text_line_to_pool( text_line * a_line )
{

    if( a_line == NULL ) {
        return;
    }

    a_line->next = line_pool;
    line_pool = a_line;
}


/***************************************************************************/
/*  allocate / reuse a ban_column instance                                 */
/***************************************************************************/

ban_column * alloc_ban_col( void )
{
    ban_column  *   curr;
    ban_column  *   prev;
    int             k;

    curr = ban_col_pool;
    if( curr != NULL ) {                // there is one to use
        ban_col_pool = curr->next;
    } else {                            // pool is empty
        curr = mem_alloc( sizeof( ban_column ) );

        ban_col_pool = mem_alloc( sizeof( *prev ) );
        prev = ban_col_pool;
        for( k = 0; k < 10; k++ ) {     // alloc 10 ban_columnss if pool empty
            prev->next = mem_alloc( sizeof( *prev ) );
            prev = prev->next;
        }
        prev->next = NULL;
    }

    curr->next = NULL;
    curr->first = NULL;

    return( curr );
}


/***************************************************************************/
/*  add a ban_column instance to free pool for reuse                       */
/***************************************************************************/

void add_ban_col_to_pool( ban_column * a_column )
{

    if( a_column == NULL ) {
        return;
    }

    a_column->next = ban_col_pool;
    ban_col_pool = a_column;
}


/***************************************************************************/
/*  allocate / reuse a box_col_set instance                                */
/***************************************************************************/

box_col_set * alloc_box_col_set( void )
{
    box_col_set *   curr;
    int             k;

    if( box_col_set_pool == NULL ) {                // pool is empty
        box_col_set_pool = mem_alloc( sizeof( *curr ) );
        curr = box_col_set_pool;
        curr->current = 0;
        curr->length = BOXCOL_COUNT;
        curr->cols = mem_alloc( BOXCOL_COUNT * sizeof( box_col ));
        for( k = 0; k < 10; k++ ) {     // alloc 10 box_col_sets if pool empty
            curr->next = mem_alloc( sizeof( *curr ) );
            curr = curr->next;
            curr->current = 0;
            curr->length = BOXCOL_COUNT;
            curr->cols = mem_alloc( BOXCOL_COUNT * sizeof( box_col ));
        }
        curr->next = NULL;
    }
    curr = box_col_set_pool;
    box_col_set_pool = curr->next;
    curr->next = NULL;
    curr->current = 0;                  // clear before returning
    memset( curr->cols, 0, curr->length * sizeof( box_col ));

    return( curr );
}


/***************************************************************************/
/*  add a linked list of box_col_set instances to free pool for reuse      */
/***************************************************************************/

void    add_box_col_set_to_pool( box_col_set * a_set )
{
    box_col_set * tw;

    if( a_set == NULL ) {
        return;
    }

    for( tw = a_set; tw->next != NULL; tw = tw->next ); //empty
    tw->next = box_col_set_pool;
    box_col_set_pool = a_set;
}


/***************************************************************************/
/*  allocate / reuse a box_col_stack instance                              */
/***************************************************************************/

box_col_stack * alloc_box_col_stack( void )
{
    box_col_stack   *   curr;
    int                 k;

    if( box_col_stack_pool == NULL ) {                // pool is empty
        box_col_stack_pool = mem_alloc( sizeof( *curr ) );
        curr = box_col_stack_pool;
        curr->first = NULL;
        for( k = 0; k < 10; k++ ) {     // alloc 10 box_col_sets if pool empty
            curr->next = mem_alloc( sizeof( *curr ) );
            curr = curr->next;
            curr->first = NULL;
        }
        curr->next = NULL;
    }
    curr = box_col_stack_pool;
    box_col_stack_pool = curr->next;
    curr->next = NULL;
    curr->first = NULL;                 // clear before returning
    curr->had_cols = false;
    curr->inner_box = false;

    return( curr );
}


/***************************************************************************/
/*  add a linked list of box_col_stack instances to free pool for reuse    */
/***************************************************************************/

void    add_box_col_stack_to_pool( box_col_stack * a_stack )
{
    box_col_stack * tw;

    if( a_stack == NULL ) {
        return;
    }

    for( tw = a_stack; tw->next != NULL; tw = tw->next ); //empty
    tw->next = box_col_stack_pool;
    box_col_stack_pool = a_stack;
}


/***************************************************************************/
/*  allocate / reuse a doc_column instance                                 */
/***************************************************************************/

doc_column * alloc_doc_col( void )
{
    doc_column  *   curr;

    curr = doc_col_pool;
    if( curr != NULL ) {                // there is one to use
        doc_col_pool = curr->next;
    } else {                            // pool is empty
        curr = mem_alloc( sizeof( doc_column ) );

        doc_col_pool = mem_alloc( sizeof( doc_column ) );
        doc_col_pool->next = NULL;
    }

    curr->next = NULL;
    curr->fig_top = g_page_bottom;
    curr->fn_top = g_page_bottom;
    curr->main_top = t_page.main_top;
    curr->main = NULL;
    curr->bot_fig = NULL;
    curr->footnote = NULL;

    return( curr );
}


/***************************************************************************/
/*  add a doc_column instance to free pool for reuse                       */
/***************************************************************************/

void add_doc_col_to_pool( doc_column * a_column )
{

    if( a_column == NULL ) {
        return;
    }

    a_column->next = doc_col_pool;
    doc_col_pool = a_column;
}


/***************************************************************************/
/*  allocate / reuse a doc_element instance                                */
/***************************************************************************/

doc_element * alloc_doc_el(  element_type type )
{
    doc_element *   curr;
    int             k;

    if( doc_el_pool == NULL ) {         // pool is empty
        doc_el_pool = mem_alloc( sizeof( *curr ) );
        curr = doc_el_pool;
        for( k = 0; k < 10; k++ ) {     // alloc 10 doc_els if pool empty
            curr->next = mem_alloc( sizeof( *curr ) );
            curr = curr->next;
        }
        curr->next = NULL;
    }
    curr = doc_el_pool;
    doc_el_pool = curr->next;

    curr->next = NULL;
    curr->blank_lines = 0;
    curr->depth = 0;
    curr->subs_skip = 0;
    curr->top_skip = 0;
    curr->type = type;

    switch( type ) {
    case el_binc :
        curr->element.binc.cur_left = 0;
        curr->element.binc.depth = 0;
        curr->element.binc.y_address = 0;
        curr->element.binc.at_top = false;
        curr->element.binc.has_rec_type = false;
        curr->element.binc.file[0] = '\0';
        break;
    case el_dbox :
        curr->element.dbox.h_start = 0;
        curr->element.dbox.v_start = 0;
        curr->element.dbox.h_len = 0;
        curr->element.dbox.v_len = 0;
        break;
    case el_graph :
        curr->element.graph.cur_left = 0;
        curr->element.graph.depth = 0;
        curr->element.graph.scale = 0;
        curr->element.graph.width = 0;
        curr->element.graph.y_address = 0;
        curr->element.graph.xoff = 0;
        curr->element.graph.yoff = 0;
        curr->element.graph.at_top = false;
        curr->element.graph.file[0] = '\0';
        break;
    case el_hline :
        curr->element.hline.h_start = 0;
        curr->element.hline.v_start = 0;
        curr->element.hline.h_len = 0;
        break;
    case el_text :
        curr->element.text.spacing = 0;
        curr->element.text.first = NULL;
        curr->element.text.overprint = false;
        curr->element.text.force_op = false;
        break;
    case el_vline :
        curr->element.vline.h_start = 0;
        curr->element.vline.v_start = 0;
        curr->element.vline.v_len = 0;
        curr->element.vline.twice = true;
        break;
    default :
        internal_err( __FILE__, __LINE__ );
    }

    return( curr );
}


/***************************************************************************/
/*  add a doc_element instance to free pool for reuse                      */
/***************************************************************************/

void    add_doc_el_to_pool( doc_element * a_element )
{

    if( a_element == NULL ) {
        return;
    }

    a_element->next = doc_el_pool;
    doc_el_pool = a_element;
}


/***************************************************************************/
/*  allocate / reuse a tag_cb     instance                                 */
/***************************************************************************/

tag_cb  * alloc_tag_cb( void )
{
    tag_cb  *   curr;
    tag_cb  *   prev;
    int         k;

    curr = tag_pool;
    if( curr != NULL ) {                // there is one to use
        tag_pool = curr->prev;
    } else {                            // pool is empty
        curr = mem_alloc( sizeof( tag_cb ) );

        prev = mem_alloc( sizeof( *prev ) );
        tag_pool = prev;
        for( k = 0; k < 10; k++ ) { // alloc 10 tag_cb if pool empty
            prev->prev = mem_alloc( sizeof( *prev ) );
            prev = prev->prev;
        }
        prev->prev = NULL;
    }
    curr->prev = NULL;
    curr->gtag = GML_TAG_NONE;

    return( curr );
}


/***************************************************************************/
/*  add a tag_cb    instance to free pool for reuse                        */
/***************************************************************************/

void    add_tag_cb_to_pool( tag_cb * a_cb )
{
    nest_stack  *   ns;
    nest_stack  *   nsv;

    if( a_cb == NULL ) {
        return;
    }
    for( ns = a_cb->p_stack; ns != NULL; ns = nsv ) {
        nsv = ns->prev;
        mem_free( ns );
    }
    a_cb->prev = tag_pool;
    tag_pool = a_cb;
}


/***************************************************************************/
/*  free all elements of our storage pools                                 */
/***************************************************************************/

void    free_pool_storage( void )
{
    void    *   v;
    void    *   wv;

    for( v = text_pool; v != NULL; ) {
        wv = ( (text_chars *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = line_pool; v != NULL; ) {
        wv = ( (text_line *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = ban_col_pool; v != NULL; ) {
        wv = ( (ban_column *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = box_col_set_pool; v != NULL; ) {
        if( ((box_col_set *) v)->cols != NULL ) {
            mem_free( ((box_col_set *) v)->cols );
        }
        wv = ( (box_col_set *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = box_col_stack_pool; v != NULL; ) {
        wv = ( (box_col_stack *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = doc_col_pool; v != NULL; ) {
        wv = ( (doc_column *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = doc_el_pool; v != NULL; ) {
        wv = ( (doc_element *) v)->next;
        mem_free( v );
        v = wv;
    }

    for( v = tag_pool; v != NULL; ) {
        wv = ( (tag_cb *) v)->prev;
        mem_free( v );
        v = wv;
    }
}


