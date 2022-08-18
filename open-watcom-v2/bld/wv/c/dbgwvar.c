/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Variables window (Locals/Watches).
*
****************************************************************************/



#include <limits.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgstk.h"
#include "dbgerr.h"
#include "dbgadget.h"
#include "dbgitem.h"
#include "dlgvarx.h"
#include "wspawn.h"
#include "dbgscan.h"
#include "dui.h"
#include "strutil.h"
#include "dbgutil.h"
#include "dbgexpr4.h"
#include "dbgexpr2.h"
#include "dbgexpr.h"
#include "dbgprint.h"
#include "dbgparse.h"
#include "wndsys.h"
#include "dbgprog.h"
#include "addarith.h"
#include "dlgexpr.h"
#include "dbgwglob.h"
#include "dbgwinsp.h"
#include "dlgnewws.h"
#include "dbgwvar.h"
#include "dbgchopt.h"
#include "dipinter.h"
#include "menudef.h"


#define WndVar( wnd )       ((var_window *)WndExtra( wnd ))
#define WndVarInfo( wnd )   (&WndVar( wnd )->i)

#define INDENT_AMOUNT           2
#define REASONABLE_NAME_WIDTH   30

typedef struct {
    var_info        i;
    gui_ord         last_width;             // how wide were we last resize?
    gui_ord         name_end;               // the length of the longest name
    var_type        vtype;                  // type of window : locals, expression, etc
    boolbit         initialized     : 1;    // is it just opened
    boolbit         show_whole_expr : 1;    // show foo->bar versus just .bar
} var_window;

typedef struct {
    int         scroll;
    wnd_row     row;
    wnd_piece   piece;
} var_wnd_data;

typedef void            VARDIRTRTN( a_window, int );

extern void             WndVarNewWindow( char * );

static char **VarNames[] = {
    #define pick(e,name,wndclass,icon)    name,
    #include "_dbgvar.h"
    #undef pick
};

static wnd_class_wv VarWndClass[] = {
    #define pick(e,name,wndclass,icon)    wndclass,
    #include "_dbgvar.h"
    #undef pick
};

static gui_resource *VarIcons[] = {
    #define pick(e,name,wndclass,icon)    icon,
    #include "_dbgvar.h"
    #undef pick
};

static WNDMENU      VarMenuItem;

static gui_menu_struct VarTypeMenu[] = {
    #include "mvartype.h"
};

static gui_menu_struct VarClassMenu[] = {
    #include "mvarclas.h"
};

static gui_menu_struct VarShowMenu[] = {
    #include "mvarshow.h"
};

static gui_menu_struct VarOptMenu[] = {
    #include "mvaropt.h"
};

static gui_menu_struct VarMenu[] = {
    #include "menuvar.h"
};

static void     VarSetWidth( a_window wnd )
/*
    Always leave room for a vertical scroll bar. It's most annoying
    having the window repaint whenever it appears/disappears.
 */
{
    var_window  *var = WndVar( wnd );

    var->last_width = WndWidth( wnd );
    if( VarRowTotal( &var->i ) <= WndRows( wnd ) ) {
        var->last_width -= WndScrollBarWidth( wnd );
    }
    var->last_width -= WndAvgCharX( wnd ) / 2;

}

static  void    VarRepaint( a_window wnd )
{
    var_window  *var = WndVar( wnd );

    VarAllNodesInvalid( &var->i );
    VarKillExprSPCache( &var->i );
    WndSetThumb( wnd );
    WndNoSelect( wnd );
    WndSetRepaint( wnd );
    WndResetScroll( wnd );
}


bool    WndVarAdd( a_window wnd, const char *name, unsigned len, bool expand )
{
    var_node    *v;

    v = VarAdd1( WndVarInfo( wnd ), name, len, expand, false );
    VarRepaint( wnd );
    return( v != NULL );
}


static wnd_row VarNumRows( a_window wnd )
{
    return( VarRowTotal( WndVarInfo( wnd ) ) );
}

static  void    VarModify( a_window wnd, wnd_row row, wnd_piece piece )
{
    var_node            *v;
    type_kind           class;
    var_window          *var = WndVar( wnd );
    bool                ok;
    bool                followable;
    mad_radix           old_radix;

    if( row < 0 ) {
        if( var->vtype == VAR_WATCH || var->vtype == VAR_VARIABLE ) {
            VarMenuItem( wnd, MENU_VAR_NEW_EXPRESSION, row, piece );
        }
        return;
    }
    VarErrState();
    VarKillExprSPCache( &var->i );
    v = VarFindRow( &var->i, row );
    if( v == NULL ) {
        v = VarFindRowNode( &var->i, row );
        if( v == NULL )
            return;
        if( piece != VAR_PIECE_GADGET && piece != VAR_PIECE_NAME )
            return;
        if( v->expand != NULL || v->node_type == NODE_INHERIT ) {
            VarExpandRow( &var->i, v, row );
            WndNewCurrent( wnd, row, VAR_PIECE_NAME );
            VarRepaint( wnd );
        }
        return;
    }
    followable = VarGetStackClass( &class );
    switch( piece ) {
    case VAR_PIECE_GADGET:
    case VAR_PIECE_NAME:
        if( VarExpandable( class ) || followable || v->expand != NULL ) {
            VarExpandRow( &var->i, v, row );
            WndNewCurrent( wnd, row, VAR_PIECE_NAME );
            VarRepaint( wnd );
        }
        break;
    case VAR_PIECE_VALUE:
        if( !VarExpandable( class ) ) {
            char *value = DbgAlloc( TXT_LEN );
            char *title = DbgAlloc( TXT_LEN );
            old_radix = VarNewCurrRadix( v );
            ExprValue( ExprSP );
            VarBuildName( &var->i, v, false );
            StrCopy( TxtBuff, title );
            VarPrintText( &var->i, value, PrintValue, TXT_LEN );
            VarKillExprSPCache( &var->i );
            v = VarFindRow( &var->i, row );
            FreezeStack();
            ok = DlgAnyExpr( title, value, TXT_LEN );
            UnFreezeStack( false );
            if( ok )
                VarDoAssign( &var->i, v, value );
            NewCurrRadix( old_radix );
            WndRowDirty( wnd, row );
            DbgFree( value );
            DbgFree( title );
        }
        break;
    }
    VarDoneRow( &var->i );
    VarOldErrState();
}

static  void    ExpandRowIfPossible( a_window wnd, wnd_row row, wnd_piece piece )
{
    var_node            *v;
    type_kind           class;
    var_window          *var = WndVar( wnd );
    bool                followable;

    VarErrState();
    VarKillExprSPCache( &var->i );
    v = VarFindRow( &var->i, row );

    followable = VarGetStackClass( &class );
    switch( piece ) {
    case VAR_PIECE_GADGET:
    case VAR_PIECE_NAME:
        if( VarExpandable( class ) || followable || v->expand != NULL ) {
            VarExpandRowNoCollapse( &var->i, v, row );
            /* We do NOT want to set current or induce a repaint yet */
            /* WndNewCurrent( wnd, row, VAR_PIECE_NAME ); */
            /* VarRepaint( wnd ); */
        }
        break;
    }
    VarDoneRow( &var->i );
    VarOldErrState();
}


static bool VarEdit( a_window wnd, var_node *v )
{
    var_window  *var = WndVar( wnd );

    if( v == NULL ) {
        TxtBuff[0] = NULLCHAR;
    } else {
        strcpy( TxtBuff, VarNodeExpr( v ) );
    }
    VarRepaint( wnd );
    DlgNewWithSym( LIT_ENG( New_Expression ), TxtBuff, TXT_LEN );
    if( TxtBuff[0] == NULLCHAR )
        return( false );
    VarAddNodeToScope( &var->i, v, TxtBuff );
    VarRepaint( wnd );
    return( true );
}

#if 0
static void VarMoveToRoot( a_window wnd, int row, var_node *v )
{
    var_window  *var = WndVar( wnd );
    int         new_row;

    new_row = VarFindRootRow( &var->i, v, row );
    if( new_row == row )
        return;
    WndMoveCurrent( wnd, new_row, VAR_PIECE_NAME );
}
#endif


static void VarSetOptions( var_window *var )
{
    var->show_whole_expr = _IsOn( SW_VAR_WHOLE_EXPR );
    VarDisplaySetMembers( &var->i, _IsOn( SW_VAR_SHOW_MEMBERS ) );
}

static void VarInitPopup( a_window wnd, var_window *var, var_node *v )
{
    type_kind           class;
    bool                pointer;
    bool                noedit;

    WndMenuGrayAll( wnd );
    WndMenuEnable( wnd, MENU_VAR_OPTIONS, true );
    WndMenuEnable( wnd, MENU_VAR_CLASS, true );
    WndMenuEnable( wnd, MENU_VAR_TYPE, true );
    WndMenuEnable( wnd, MENU_VAR_SHOW_MEMBER, true );
    WndMenuEnable( wnd, MENU_VAR_SHOW_WHOLE_EXPR, true );
    noedit = ( var->vtype == VAR_LOCALS || var->vtype == VAR_FILESCOPE );
    WndMenuIgnore( wnd, MENU_VAR_EDIT_EXPRESSION, noedit );
    WndMenuIgnore( wnd, MENU_VAR_NEW_EXPRESSION, noedit );
    WndMenuIgnore( wnd, MENU_VAR_DELETE, noedit );
    if( !noedit ) {
        WndMenuEnable( wnd, MENU_VAR_EDIT_EXPRESSION, v != NULL && v->parent == NULL );
        WndMenuEnable( wnd, MENU_VAR_NEW_EXPRESSION, true );
        WndMenuEnable( wnd, MENU_VAR_DELETE, v != NULL && v->parent == NULL );
    }
    WndMenuEnable( wnd, MENU_VAR_WATCH, v != NULL );
    WndMenuEnable( wnd, MENU_VAR_INSPECT, v != NULL );

    if( v != NULL && !VarError ) {
        VarGetStackClass( &class );
        pointer = VarIsPointer( class );
        WndMenuEnable( wnd, MENU_VAR_TYPE, true );
        WndMenuEnable( wnd, MENU_VAR_SHOW, true );
        WndMenuEnable( wnd, MENU_VAR_CLASS, true );
        WndMenuEnable( wnd, MENU_VAR_OPTIONS, true );
//      WndMenuEnable( wnd, MENU_VAR_SHOW_ROOT, v->parent != NULL );
        WndMenuEnable( wnd, MENU_VAR_ARRAY_EXPAND, pointer && v->expand == NULL );
        WndMenuEnable( wnd, MENU_VAR_INSPECT_POINTER, pointer );
        WndMenuEnable( wnd, MENU_VAR_INSPECT_CODE, pointer );
        WndMenuEnable( wnd, MENU_VAR_FIELD_TOP, v->node_type == NODE_FIELD );
        WndMenuEnable( wnd, MENU_VAR_INSPECT_MEMORY, VarIsLValue() );
        WndMenuEnable( wnd, MENU_VAR_WATCH, true );
        WndMenuEnable( wnd, MENU_VAR_INSPECT, true );
#ifdef I_EVER_SOLVE_THE_THORNY_HIDE_PROBLEM
        WndMenuEnable( wnd, MENU_VAR_HIDE, v->node_type == NODE_FIELD );
        WndMenuEnable( wnd, MENU_VAR_UNHIDE, v->expand != NULL );
#endif
        WndMenuEnable( wnd, MENU_VAR_SHOW_TYPE, true );

        WndMenuEnable( wnd, MENU_VAR_POINTER, pointer );
        WndMenuEnable( wnd, MENU_VAR_STRING, pointer );
        if( v->expand == NULL ) {
            if( !VarExpandable( class ) ) {
                WndMenuEnable( wnd, MENU_VAR_MODIFY, true );
                WndMenuEnable( wnd, MENU_VAR_BREAK, true );
                WndMenuEnable( wnd, MENU_VAR_ALLHEX, VarParentIsArray( v ) );
                WndMenuEnable( wnd, MENU_VAR_ALLDECIMAL, VarParentIsArray( v ) );
                WndMenuEnable( wnd, MENU_VAR_HEX, !pointer );
                WndMenuEnable( wnd, MENU_VAR_DECIMAL, !pointer );
                WndMenuEnable( wnd, MENU_VAR_CHAR, !pointer );
            }
        }
        /* Enable even if already expanded */
        if( VarExpandable( class ) ) {
            WndMenuEnable( wnd, MENU_VAR_EXPAND_ALL, true );
        }
        if( VarDisplayIsStruct( v ) ) {
            WndMenuEnable( wnd, MENU_VAR_SHOW_CODE, true );
            WndMenuEnable( wnd, MENU_VAR_SHOW_INHERIT, true );
            WndMenuEnable( wnd, MENU_VAR_SHOW_COMPILER, true );
            WndMenuEnable( wnd, MENU_VAR_SHOW_PRIVATE, true );
            WndMenuEnable( wnd, MENU_VAR_SHOW_PROTECTED, true );
            WndMenuEnable( wnd, MENU_VAR_SHOW_STATIC, true );
            WndMenuCheck( wnd, MENU_VAR_SHOW_CODE, !VarDisplayIsHidden( v, VARNODE_CODE ) );
            WndMenuCheck( wnd, MENU_VAR_SHOW_INHERIT, !VarDisplayIsHidden( v, VARNODE_INHERIT ) );
            WndMenuCheck( wnd, MENU_VAR_SHOW_COMPILER, !VarDisplayIsHidden( v, VARNODE_COMPILER ) );
            WndMenuCheck( wnd, MENU_VAR_SHOW_PRIVATE, !VarDisplayIsHidden( v, VARNODE_PRIVATE ) );
            WndMenuCheck( wnd, MENU_VAR_SHOW_PROTECTED, !VarDisplayIsHidden( v, VARNODE_PROTECTED ) );
            WndMenuCheck( wnd, MENU_VAR_SHOW_STATIC, !VarDisplayIsHidden( v, VARNODE_STATIC ) );
        }
    }
    WndMenuCheck( wnd, MENU_VAR_SHOW_WHOLE_EXPR, var->show_whole_expr );
    WndMenuCheck( wnd, MENU_VAR_SHOW_MEMBER, VarDisplayShowMembers( &var->i ) );
    WndMenuCheck( wnd, MENU_VAR_FIELD_TOP, v != NULL && VarDisplayedOnTop( v ) );
}


static void VarMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    var_node            *v;
    dlg_var_expand      varx;
    long                start;
    int                 dummy;
    array_info          ainfo;
    char                *name;
    bool                need_reset;
    var_window          *var;

    /* unused parameters */ (void)piece;

    var = WndVar( wnd );
    need_reset = VarErrState();
    v = VarFindRow( &var->i, row );
    if( v == NULL && VarError ) {
        v = VarFindRoot( &var->i, row, &dummy );
    }
    switch( id ) {
    case MENU_INITIALIZE:
        VarInitPopup( wnd, var, v );
        break;
    case MENU_VAR_SHOW_WHOLE_EXPR:
        var->show_whole_expr = !var->show_whole_expr;
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_MEMBER:
        VarDisplaySetMembers( &var->i, !VarDisplayShowMembers( &var->i ) );
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_CODE:
        VarDisplayFlipHide( v, VARNODE_CODE );
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_INHERIT:
        VarDisplayFlipHide( v, VARNODE_INHERIT );
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_COMPILER:
        VarDisplayFlipHide( v, VARNODE_COMPILER );
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_PRIVATE:
        VarDisplayFlipHide( v, VARNODE_PRIVATE );
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_PROTECTED:
        VarDisplayFlipHide( v, VARNODE_PROTECTED );
        VarRepaint( wnd );
        break;
    case MENU_VAR_SHOW_STATIC:
        VarDisplayFlipHide( v, VARNODE_STATIC );
        VarRepaint( wnd );
        break;
#if 0
    case MENU_VAR_SHOW_ROOT:
        VarMoveToRoot( wnd, row, v );
        break;
#endif
    case MENU_VAR_HEX:
        VarDisplaySetHex( v );
        VarRepaint( wnd );
        break;
    case MENU_VAR_ALLHEX:
        VarDisplaySetArrayHex( v->parent );
        VarRepaint( wnd );
        break;
    case MENU_VAR_DECIMAL:
        VarDisplaySetDecimal( v );
        VarRepaint( wnd );
        break;
    case MENU_VAR_ALLDECIMAL:
        VarDisplaySetArrayDec( v->parent );
        VarRepaint( wnd );
        break;
    case MENU_VAR_CHAR:
        VarDisplaySetChar( v );
        VarRepaint( wnd );
        break;
    case MENU_VAR_STRING:
        if( v->expand != NULL )
            VarDeExpand( v );
        VarDisplaySetString( v );
        VarRepaint( wnd );
        break;
    case MENU_VAR_POINTER:
        if( v->expand != NULL )
            VarDeExpand( v );
        VarDisplaySetPointer( v );
        VarRepaint( wnd );
        break;
    case MENU_VAR_INSPECT_CODE:
        need_reset = VarOldErrState();
        break;
    case MENU_VAR_FIELD_TOP:
        VarDisplayOnTop( v, !VarDisplayedOnTop( v ) );
        VarRepaint( wnd );
        break;
#ifdef I_EVER_SOLVE_THE_THORNY_HIDE_PROBLEM
    case MENU_VAR_HIDE:
        VarDisplayHide( v );
        VarRepaint( wnd );
        break;
    case MENU_VAR_UNHIDE:
        VarDisplayUnHide( v );
        VarRepaint( wnd );
        break;
#endif
    case MENU_VAR_SHOW_TYPE:
        VarDisplayType( v, TxtBuff, TXT_LEN );
        DUIMsgBox( TxtBuff );
        break;
    case MENU_VAR_INSPECT_POINTER:
        need_reset = VarOldErrState();
        VarInspectPointer();
        break;
    case MENU_VAR_INSPECT_MEMORY:
        need_reset = VarOldErrState();
        VarInspectMemory();
        break;
    case MENU_VAR_BREAK:
        VarBreakOnWrite( &var->i, v );
        break;
    case MENU_VAR_MODIFY:
        VarModify( wnd, row, VAR_PIECE_VALUE );
        break;
    case MENU_VAR_ARRAY_EXPAND:
        ExprValue( ExprSP );
        varx.start = 0;
        varx.end = 0;
        start = 0;
        switch( ExprSP->ti.kind ) {
        case TK_ARRAY:
            DIPTypeArrayInfo( ExprSP->th, ExprSP->lc, &ainfo, NULL );
            start = ainfo.low_bound;
            varx.start = start;
            varx.end = varx.start + ainfo.num_elts - 1;
            /* fall through */
        case TK_POINTER:
            {
                bool rc;

                FreezeStack();
                VarRepaint( wnd ); // set early so we flush redundant repaints
                rc = DlgVarExpand( &varx );
                UnFreezeStack( false );
                if( rc ) {
                    VarDeExpand( v );
                    VarExpand( &var->i, v, varx.start - start, varx.end - start );
                }
                VarRepaint( wnd ); // again in case we got a repaint while dlg up
            }
        }
        break;
    case MENU_VAR_INSPECT:
        VarBuildName( &var->i, v, false );
        name = DupStr( TxtBuff );
        WndInspectExprSP( name );
        WndFree( name );
        break;
    case MENU_VAR_WATCH:
        VarAddWatch( &var->i, v );
        break;
    case MENU_VAR_EDIT_EXPRESSION:
        VarEdit( wnd, v );
        break;
    case MENU_VAR_NEW_EXPRESSION:
        if( !VarEdit( wnd, NULL ) )
            break;
        WndScrollBottom( wnd );
        break;
    case MENU_VAR_DELETE:
        VarRepaint( wnd );
        VarDeExpand( v );
        VarDelete( &var->i, v );
        break;
    case MENU_VAR_EXPAND_ALL:
        {
            int expand_row = row;
            int num_rows = VarNumRows( wnd );
            var_node * v_sibling = NULL;

            /*
             *  If we are a root node, then we have no sibling. If we have a parent, then we may have a sibling but we may
             *  also by the last leaf node of our parent so we need to check out to see who our parents next sibling is and
             *  stop there.
             */
            if( v->parent != NULL ) {
                var_node    *v_iter;

                for( v_iter = v->parent->expand; v_iter != NULL; v_iter = v_iter->next ) {
                    if( v_iter == v ) {
                        v_sibling = v_iter->next;
                        break;
                    }
                }

                if( NULL == v_sibling ) {   /* last element, but may be more following. track grandparent */
                    if( v->parent->parent != NULL ) {
                        for( v_iter = v->parent->parent->expand; v_iter != NULL; v_iter = v_iter->next ) {
                            if( v_iter == v->parent ) {
                                v_sibling = v_iter->next;
                                break;
                            }
                        }
                    }
                }
            }

            for( ; expand_row < num_rows; expand_row++ ) {
                var_node    *v_next;

                v_next = VarFindRowNode( &var->i, expand_row );
                if( v_next == v_sibling )
                    break;
                ExpandRowIfPossible( wnd, expand_row, VAR_PIECE_GADGET );
                num_rows = VarNumRows( wnd );
            }
        }
        VarRepaint( wnd );
        break;
    }
    if( need_reset )
        VarOldErrState();
    VarDoneRow( &var->i );
}


static void FmtName( var_window *var, var_node *v, wnd_line_piece *line,
                     a_window wnd, int depth, int inherited, int row )
{
    gui_ord     name_len;

    line->indent = MaxGadgetLength;
    if( !var->show_whole_expr ) {
        inherited += depth;
    }
    line->indent += ( INDENT_AMOUNT * inherited ) * WndAvgCharX( wnd );
    line->text = TxtBuff;
    VarBuildName( &var->i, v, !var->show_whole_expr );
    name_len = WndExtentX( wnd, TxtBuff );
    var->name_end = name_len + line->indent;
    var->i.name_end_row = row;
}

static  bool    VarGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    var_window  *var = WndVar( wnd );
    var_node    *v;
    int         depth, inherited;
    int         indent;
    gui_ord     good_size;
//    bool        on_rhs;
    int         outdent;
    var_node    *parent;

    if( piece >= VAR_PIECE_LAST ) {
        v = VarGetDisplayPiece( &var->i, row, VAR_PIECE_NAME, &depth, &inherited );
        if( v == NULL )
            return( false );
        line->text = "";
        line->tabstop = false;
        line->static_text = true;
        if( !var->show_whole_expr ) {
            inherited += depth;
        }
        outdent = piece - VAR_PIECE_LAST + 1;
        line->indent = MaxGadgetLength + ( INDENT_AMOUNT * ( inherited - outdent ) ) * WndAvgCharX( wnd );
        if( outdent > inherited )
            return( false );
        if( outdent == 1 ) {
            if( v->parent == NULL )
                return( false );
            if( VarNextVisibleSibling( &var->i, v ) != NULL ) {
                line->draw_line_hook = true;
            } else {
                line->draw_hook = true;
            }
        } else {
            parent = v->parent;
            while( --outdent > 1 ) {
                parent = parent->parent;
            }
            if( VarNextVisibleSibling( &var->i, parent ) != NULL ) {
                line->vertical_line = true;
            }
        }
        return( true );
    }
    v = VarGetDisplayPiece( &var->i, row, piece, &depth, &inherited );
    if( v == NULL )
        return( false );
    line->tabstop = true;
    switch( piece ) {
    case VAR_PIECE_GADGET:
        line->tabstop = false;
        if( WndDoingSearch )
            break;
        if( v->gadget == VARGADGET_NONE ) {
            line->text = LIT_ENG( Empty );
        } else {
            wnd_gadget_type gadgets[] =
                 { WND_GADGET_NONE, GADGET_OPEN,
                     GADGET_CLOSED, GADGET_POINTS, GADGET_UNPOINTS,
                     GADGET_BAD_POINTS, GADGET_INHERIT_OPEN,
                     GADGET_INHERIT_CLOSED };
            SetGadgetLine( wnd, line, gadgets[v->gadget] );
            if( v->gadget == VARGADGET_BADPOINTS ) {
                line->attr = WND_PLAIN;
            }
        }
        break;
    case VAR_PIECE_NAME:
        FmtName( var, v, line, wnd, depth, inherited, row );
        break;
    case VAR_PIECE_VALUE:
        if( !WndDoingSearch && var->i.name_end_row != row ) {
            FmtName( var, v, line, wnd, depth, inherited, row );
        }
        line->text = v->value;
        if( v->standout ) {
            line->attr = WND_STANDOUT;
        }
//        on_rhs = true;
        indent = var->last_width - WndExtentX( wnd, line->text );
        good_size = REASONABLE_NAME_WIDTH * WndAvgCharX( wnd );
        if( indent < 0 )
            indent = 0;
        if( indent >= good_size ) {
            indent = good_size;
//            on_rhs = false;
        }
        if( indent < var->name_end + WndAvgCharX( wnd ) ) {
            indent = var->name_end + WndAvgCharX( wnd );
        }
        line->indent = indent;
        break;
    }
    return( true );
}

static void VarSaveWndToScope( a_window wnd )
{
    var_wnd_data    *wnd_data;
    var_window      *var = WndVar( wnd );

    wnd_data = var->i.s->wnd_data;
    if( wnd_data == NULL ) {
        wnd_data = WndMustAlloc( sizeof( var_wnd_data ) );
        var->i.s->wnd_data = wnd_data;
    }
    wnd_data->scroll = WndTop( wnd );
    WndGetCurrent( wnd, &wnd_data->row, &wnd_data->piece );
}

static void VarRestoreWndFromScope( a_window wnd )
{
    var_wnd_data    *wnd_data;
    var_window      *var = WndVar( wnd );

    wnd_data = var->i.s->wnd_data;
    if( wnd_data == NULL || wnd_data->row == WND_NO_ROW ) {
        WndVScrollAbs( wnd, 0 );
        WndFirstCurrent( wnd );
    } else {
        WndVScrollAbs( wnd, wnd_data->scroll );
        WndNewCurrent( wnd, wnd_data->row, wnd_data->piece );
    }
}

static bool VarInfoWndRefresh( var_type vtype, var_info *i, address *addr, a_window wnd )
{
    scope_list      *nested, *new;
    scope_state     *s, *outer;
    bool            repaint;
    scope_block     noscope;
    bool            havescope;

    repaint = false;
    *addr = NilAddr;
    switch( vtype ) {
    case VAR_FILESCOPE:
        if( i->s->mod != ContextMod ) {
            repaint = true;
            VarSaveWndToScope( wnd );
            noscope.start = NilAddr;
            noscope.len = 0;
            noscope.unique = 0;
            NewScope( i, &noscope, ContextMod, &repaint );
            VarRestoreWndFromScope( wnd );
        }
        break;
    case VAR_LOCALS:
        _AllocA( nested, sizeof( *nested ) );
        outer = NULL;
        nested->next = NULL;
        havescope = true;
        noscope.start = NilAddr;
        noscope.len = 0;
        noscope.unique = 0;
        if( DeAliasAddrScope( ContextMod, Context.execution, &nested->scope ) == SR_NONE ) {
            nested->scope = noscope;
            repaint = true;
            havescope = false;
        }
        if( !SameScope( &nested->scope, i->s ) ) {
            repaint = true;
            VarSaveWndToScope( wnd );
            if( havescope ) {
                for( ;; ) {
                    _AllocA( new, sizeof( *new ) );
                    if( DIPScopeOuter( ContextMod, &nested->scope, &new->scope ) == SR_NONE )
                        break;
                    new->next = nested;
                    nested = new;
                }
            }
            while( nested != NULL ) {
                s = NewScope( i, &nested->scope, NO_MOD, &repaint );
                s->outer = outer;
                outer = s;
                nested = nested->next;
            }
            VarRestoreWndFromScope( wnd );
        }
        if( outer != NULL )
            *addr = outer->scope.addr;
        break;
    }
    return( repaint );
}

static  void    VarBegPaint( a_window wnd, wnd_row row, int num )
{
    var_window  *var = WndVar( wnd );

    /* unused parameters */ (void)row; (void)num;

    VarOkToCache( &var->i, true );
}


static  void    VarEndPaint( a_window wnd, wnd_row row, int num )
{
    var_window  *var = WndVar( wnd );

    /* unused parameters */ (void)row; (void)num;

    VarOkToCache( &var->i, false );
}


static void VarDirtyRow( a_window wnd, int row )
/**************************************************/
{
    WndRowDirty( wnd, row );
}

static void VarWndRefreshVisible( var_info *i, int top, int rows, VARDIRTRTN *dirty, a_window wnd )
/*************************************************************************************************/
{
    int             row;
    var_node        *v;
    char            *value;
    var_gadget_type gadget;
    bool            standout;
    bool            on_top;

    VarAllNodesInvalid( i );
    VarErrState();
    VarOkToCache( i, true );
    for( row = top; row < top + rows; ++row ) {
        v = VarFindRow( i, row );
        if( v == NULL ) {
            v = VarFindRowNode( i, row );
            if( v == NULL ) {
                break;
            }
        } else {
            ExprValue( ExprSP );
        }

        gadget = VarGetGadget( v );
        if( gadget != v->gadget )
            dirty( wnd, row );
        VarSetGadget( v, gadget );

        on_top = VarGetOnTop( v );
        if( on_top != v->on_top )
            dirty( wnd, row );
        VarSetOnTop( v, on_top );

        value = VarGetValue( i, v );
        standout = false;
        if( v->value != NULL ) {
            standout = ( strcmp( value, v->value ) != 0 );
        }
        if( v->value == NULL || v->standout || standout )
            dirty( wnd, row );
        v->standout = standout;
        VarSetValue( v, value );
        VarDoneRow( i );
    }
    VarOkToCache( i, false );
    VarOldErrState();
}

static  void VarRefresh( a_window wnd )
{
    var_window  *var = WndVar( wnd );
    address     addr;
    bool        repaint;
    char        *p;

    repaint = false;
    if( !var->initialized ||
      ( UpdateFlags & (UP_MEM_CHANGE | UP_STACKPOS_CHANGE | UP_CSIP_CHANGE | UP_REG_CHANGE) ) ) {
        var->initialized = true;
        repaint = VarInfoWndRefresh( var->vtype, &var->i, &addr, wnd );
        if( var->vtype == VAR_LOCALS ) {
            p = StrCopy( LIT_DUI( WindowLocals ), TxtBuff );
            if( !IS_NIL_ADDR( addr ) ) {
                p = StrCopy( " (", p );
                p = CnvNearestAddr( addr, p, TXT_LEN - ( p - TxtBuff ) );
                p = StrCopy( ")", p );
            }
            WndSetTitle( wnd, TxtBuff );
        }
        VarWndRefreshVisible( &var->i, WndTop( wnd ), WndRows( wnd ), VarDirtyRow, wnd );
    }
    if( UpdateFlags & UP_VAR_DISPLAY ) {
        VarDisplayUpdate( &var->i );
        repaint = true;
    }
    if( repaint || ( UpdateFlags & (UP_RADIX_CHANGE | UP_SYM_CHANGE) ) != 0 ) {
        VarRepaint( wnd );
    }
}


static bool VarWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    var_window  *var = WndVar( wnd );
    gui_ord     old_width;
    int         delta;

    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        VarInitInfo( &var->i );
        VarSetOptions( var );
        VarSetWidth( wnd );
        var->initialized = false;
        VarRefresh( wnd );
        VarRepaint( wnd );
        WndSetKeyPiece( wnd, VAR_PIECE_NAME );
        return( true );
    case GUI_RESIZE :
        old_width = var->last_width;
        VarSetWidth( wnd );
        delta = old_width - var->last_width;
        if( delta < 0 )
            delta = -delta;
        if( delta >= 50 ) { // BIG kludge. To be removed
            VarRepaint( wnd );
        }
        return( true );
    case GUI_DESTROY :
        VarFiniInfo( &var->i );
        WndFree( var );
        return( true );
    }
    return( false );
}


static bool VarDoClass( wnd_class_wv wndclass, bool (*rtn)( var_info*, void* ), void *cookie )
{
    a_window    wnd;

    for( wnd = WndFindClass( NULL, wndclass );
         wnd != NULL; wnd = WndFindClass( wnd, wndclass ) ) {
        if( rtn( WndVarInfo( wnd ), cookie ) ) {
            return( true );
        }
    }
    return( false );
}


static bool VarDoAll( bool (*rtn)(var_info *, void *), void *cookie )
{
    var_type    i;

    for( i = 0; i < NUM_VAR_TYPE; ++i ) {
        if( VarDoClass( VarWndClass[i], rtn, cookie ) ) {
            return( true );
        }
    }
    return( false );
}


bool VarInfoRelease( void )
{
    return( VarDoAll( VarDeleteAScope, NULL ) );
}


void VarUnMapScopes( image_entry *image )
{
    VarDoAll( VarUnMap, image );
}


void VarFreeScopes( void )
{
    VarDoAll( VarDeleteAllScopes, NULL );
}


static void DoVarChangeOptions( a_window wnd )
{
    VarSetOptions( WndVar( wnd ) );
    VarRepaint( wnd );
}

static void VarWndDoAll( void (*rtn)( a_window ) )
{
    var_type    i;

    for( i = 0; i < NUM_VAR_TYPE; ++i ) {
        WndForAllClass( VarWndClass[i], rtn );
    }
}

void VarReMapScopes( image_entry *image )
{
    VarDoAll( VarReMap, image );
    VarWndDoAll( VarRepaint );
}


void VarChangeOptions( void )
{
    VarDisplaySetHidden( NULL, VARNODE_CODE, _IsOff( SW_VAR_SHOW_CODE ) );
    VarDisplaySetHidden( NULL, VARNODE_INHERIT, _IsOff( SW_VAR_SHOW_INHERIT ) );
    VarDisplaySetHidden( NULL, VARNODE_COMPILER, _IsOff( SW_VAR_SHOW_COMPILER ) );
    VarDisplaySetHidden( NULL, VARNODE_PRIVATE, _IsOff( SW_VAR_SHOW_PRIVATE ) );
    VarDisplaySetHidden( NULL, VARNODE_PROTECTED, _IsOff( SW_VAR_SHOW_PROTECTED ) );
    VarDisplaySetHidden( NULL, VARNODE_STATIC, _IsOff( SW_VAR_SHOW_STATIC ) );
    VarWndDoAll( DoVarChangeOptions );
}

static bool ChkUpdate( void )
{
    return( UpdateFlags & (UP_VAR_DISPLAY | UP_MEM_CHANGE | UP_STACKPOS_CHANGE | UP_CSIP_CHANGE | UP_REG_CHANGE | UP_RADIX_CHANGE | UP_SYM_CHANGE) );
}

wnd_info VarInfo = {
    VarWndEventProc,
    VarRefresh,
    VarGetLine,
    VarMenuItem,
    NoVScroll,
    VarBegPaint,
    VarEndPaint,
    VarModify,
    VarNumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( VarMenu )
};

static  a_window        DoWndVarOpen( var_type vtype )
{
    var_window  *var;
    a_window    wnd;

    var = WndMustAlloc( sizeof( var_window ) );
    var->vtype = vtype;
    wnd = DbgWndCreate( *VarNames[vtype], &VarInfo, VarWndClass[vtype], var, VarIcons[vtype] );
    if( wnd != NULL )
        WndClrSwitches( wnd, WSW_ONLY_MODIFY_TABSTOP );
    return( wnd );
}

a_window WndVarOpen( void )
{
    return( DoWndVarOpen( VAR_VARIABLE ) );
}

a_window WndWatOpen( void )
{
    return( DoWndVarOpen( VAR_WATCH ) );
}

a_window WndLclOpen( void )
{
    return( DoWndVarOpen( VAR_LOCALS ) );
}

a_window WndFSVOpen( void )
{
    return( DoWndVarOpen( VAR_FILESCOPE ) );
}

static  void    DoGraphicDisplay( void )
{
    const char  *name;
    unsigned    len;
    a_window    wnd;

    wnd = WndVarOpen();
    while( !ScanEOC() ) {
        if( CurrToken == T_COMMA )
            Scan();
        name = ScanPos();
        ChkExpr();
        len = ScanPos() - name;
        if( CurrToken != T_COMMA )
            ReqEOC();
        WndVarAdd( wnd, name, len, false );
    }
    WndFirstCurrent( wnd );
    WndFreshAll();
    WndShrinkToMouse( wnd, &WndLong );
    ReqEOC();
}

void GraphicDisplay( void )
{
    Spawn( DoGraphicDisplay );
}

var_node *VarGetDisplayPiece( var_info *i, wnd_row row, wnd_piece piece, int *pdepth, int *pinherit )
{
    var_node    *row_v;
    var_node    *v;

    if( piece >= VAR_PIECE_LAST )
        return( NULL );
    if( VarFirstNode( i ) == NULL )
        return( NULL );
    if( row >= VarRowTotal( i ) )
        return( NULL );
    row_v = VarFindRowNode( i, row );
    if( !row_v->value_valid ) {
        VarSetValue( row_v, LIT_ENG( Quest_Marks ) );
        row_v->value_valid = false;
    }
    if( !row_v->gadget_valid ) {
        VarSetGadget( row_v, VARGADGET_NONE );
        row_v->gadget_valid = false;
    }
    v = row_v;
    if( piece == VAR_PIECE_NAME ||
        ( piece == VAR_PIECE_GADGET && row_v->gadget_valid ) ||
        ( piece == VAR_PIECE_VALUE && row_v->value_valid ) ) {
        VarError = false;
    } else if( _IsOff( SW_TASK_RUNNING ) ) {
        if( row == i->exprsp_cacherow && i->exprsp_cache != NULL ) {
            VarError = false;
            v = i->exprsp_cache;
        } else if( row == i->exprsp_cacherow && i->exprsp_cache_is_error ) {
            VarError = true;
            v = NULL;
        } else {
            VarErrState();
            v = VarFindRow( i, row );
            VarOldErrState();
            i->exprsp_cacherow = row;
            i->exprsp_cache = v;
            i->exprsp_cache_is_error = VarError;
        }
        if( v == NULL ) {
            if( !VarError )
                return( NULL );
            v = row_v;
        }
        VarNodeInvalid( v );
        VarErrState();
        ExprValue( ExprSP );
        VarSetGadget( v, VarGetGadget( v ) );
        VarSetOnTop( v, VarGetOnTop( v ) );
        VarSetValue( v, VarGetValue( i, v ) );
        VarOldErrState();
        VarDoneRow( i );
    }
    VarGetDepths( i, v, pdepth, pinherit );
    return( v );
}
