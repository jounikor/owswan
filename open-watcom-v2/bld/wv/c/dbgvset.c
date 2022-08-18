/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Management of program variables display settings.
*
****************************************************************************/


#include <stdlib.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgstk.h"
#include "dbgerr.h"
#include "dbglit.h"
#include "dbgitem.h"
#include "dbgvar.h"
#include "dui.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgmain.h"
#include "dbgshow.h"
#include "dbgsetfg.h"


#define TYPE_DEFS \
    pick( "Ontop",       TY_ONTOP       ) \
    pick( "Hastop",      TY_HASTOP      ) \
    pick( "Autoexpand",  TY_AUTOEXPAND  ) \
    pick( "ISstruct",    TY_ISSTRUCT    ) \
    pick( "HEx",         TY_HEX         ) \
    pick( "Decimal",     TY_DECIMAL     ) \
    pick( "Character",   TY_CHARACTER   ) \
    pick( "STRIng",      TY_STRING      ) \
    pick( "Pointer",     TY_POINTER     ) \
    pick( "HIde",        TY_HIDE        ) \
    pick( "Field",       TY_FIELD       ) \
    pick( "STRUct",      TY_STRUCT      ) \
    pick( "NOCODe",      TY_NOCODE      ) \
    pick( "NOInherit",   TY_NOINHERIT   ) \
    pick( "NOCOMpiler",  TY_NOCOMPILER  ) \
    pick( "NOPRIvate",   TY_NOPRIVATE   ) \
    pick( "NOPROtected", TY_NOPROTECTED ) \
    pick( "NOSTAtic",    TY_NOSTATIC    ) \
    pick( "CODe",        TY_CODE        ) \
    pick( "INherit",     TY_INHERIT     ) \
    pick( "COMpiler",    TY_COMPILER    ) \
    pick( "PRIvate",     TY_PRIVATE     ) \
    pick( "PROtected",   TY_PROTECTED   ) \
    pick( "STAtic",      TY_STATIC      )

enum {
    #define pick(t,e)   e,
    TYPE_DEFS
    #undef pick
};

extern type_display     *TypeDisplay;

static const char TypeSettings[] = {
    #define pick(t,e)   t "\0"
    TYPE_DEFS
    #undef pick
};

static char *Attributes( type_display *curr, char *p )
{
    type_display        *alias;

    if( curr->is_struct ) {
        // On load some our fields are set to the alias' fields
        // but we don't want to override that, so this must come FIRST
        for( alias = curr->alias; alias != curr; alias = alias->alias ) {
            if( !alias->is_field && alias->is_struct ) {
                p = GetCmdEntry( TypeSettings, TY_ISSTRUCT, p );
                p = StrCopy( " ", p );
                p = StrCopy( alias->name, p );
                p = StrCopy( " ", p );
                break;
            }
        }
    }
    if( curr->on_top ) {
        p = GetCmdEntry( TypeSettings, TY_ONTOP, p );
        p = StrCopy( " ", p );
    }
    if( curr->has_top ) {
        p = GetCmdEntry( TypeSettings, TY_HASTOP, p );
        p = StrCopy( " ", p );
    }
    if( curr->autoexpand ) {
        p = GetCmdEntry( TypeSettings, TY_AUTOEXPAND, p );
        p = StrCopy( " ", p );
    }
    if( curr->display & VARDISP_HEX ) {
        p = GetCmdEntry( TypeSettings, TY_HEX, p );
        p = StrCopy( " ", p );
    }
    if( curr->display & VARDISP_DECIMAL ) {
        p = GetCmdEntry( TypeSettings, TY_DECIMAL, p );
        p = StrCopy( " ", p );
    }
    if( curr->display & VARDISP_CHARACTER ) {
        p = GetCmdEntry( TypeSettings, TY_CHARACTER, p );
        p = StrCopy( " ", p );
    }
    if( curr->display & VARDISP_STRING ) {
        p = GetCmdEntry( TypeSettings, TY_STRING, p );
        p = StrCopy( " ", p );
    }
    if( curr->display & VARDISP_POINTER ) {
        p = GetCmdEntry( TypeSettings, TY_POINTER, p );
        p = StrCopy( " ", p );
    }
#ifdef I_EVER_SOLVE_THIS_THORNY_HIDE_PROBLEM
    if( curr->display & VARDISP_HIDE ) {
        p = GetCmdEntry( TypeSettings, TY_HIDE, p );
        p = StrCopy( " ", p );
    }
#endif
    if( VarDisplayIsHidden( NULL, VARNODE_CODE ) !=
      ( ( ( curr->hide & VARNODE_CODE ) != 0 ) ) ) {
        if( curr->hide & VARNODE_CODE ) {
            p = GetCmdEntry( TypeSettings, TY_NOCODE, p );
        } else {
            p = GetCmdEntry( TypeSettings, TY_CODE, p );
        }
        p = StrCopy( " ", p );
    }
    if( VarDisplayIsHidden( NULL, VARNODE_INHERIT ) !=
      ( ( ( curr->hide & VARNODE_INHERIT ) != 0 ) ) ) {
        if( curr->hide & VARNODE_INHERIT ) {
            p = GetCmdEntry( TypeSettings, TY_NOINHERIT, p );
        } else {
            p = GetCmdEntry( TypeSettings, TY_INHERIT, p );
        }
        p = StrCopy( " ", p );
    }
    if( VarDisplayIsHidden( NULL, VARNODE_COMPILER ) !=
      ( ( ( curr->hide & VARNODE_COMPILER ) != 0 ) ) ) {
        if( curr->hide & VARNODE_COMPILER ) {
            p = GetCmdEntry( TypeSettings, TY_NOCOMPILER, p );
        } else {
            p = GetCmdEntry( TypeSettings, TY_COMPILER, p );
        }
        p = StrCopy( " ", p );
    }
    if( VarDisplayIsHidden( NULL, VARNODE_PRIVATE ) !=
      ( ( ( curr->hide & VARNODE_PRIVATE ) != 0 ) ) ) {
        if( curr->hide & VARNODE_PRIVATE ) {
            p = GetCmdEntry( TypeSettings, TY_NOPRIVATE, p );
        } else {
            p = GetCmdEntry( TypeSettings, TY_PRIVATE, p );
        }
        p = StrCopy( " ", p );
    }
    if( VarDisplayIsHidden( NULL, VARNODE_PROTECTED ) !=
      ( ( ( curr->hide & VARNODE_PROTECTED ) != 0 ) ) ) {
        if( curr->hide & VARNODE_PROTECTED ) {
            p = GetCmdEntry( TypeSettings, TY_NOPROTECTED, p );
        } else {
            p = GetCmdEntry( TypeSettings, TY_PROTECTED, p );
        }
        p = StrCopy( " ", p );
    }
    if( VarDisplayIsHidden( NULL, VARNODE_STATIC ) !=
      ( ( ( curr->hide & VARNODE_STATIC ) != 0 ) ) ) {
        if( curr->hide & VARNODE_STATIC ) {
            p = GetCmdEntry( TypeSettings, TY_NOSTATIC, p );
        } else {
            p = GetCmdEntry( TypeSettings, TY_STATIC, p );
        }
        p = StrCopy( " ", p );
    }
    return( p );
}

static void oops( void )
{
    Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_SET ) );
}

static void ScanLeftBrace( void )
/*******************************/
{
    if( CurrToken != T_LEFT_BRACE )
        oops();
    Scan();
}

static bool ScanRightBrace( void )
/********************************/
{
    if( CurrToken != T_RIGHT_BRACE )
        return( false );
    Scan();
    return( true );
}

static char *ScanName( void )
/***************************/
{
    const char  *start;
    size_t      len;

    ScanItem( true, &start, &len );
    memcpy( TxtBuff, start, len );
    TxtBuff[len] = NULLCHAR;
    return( TxtBuff );
}


static int ScanAttribute( type_display *type, int token )
{
    bool        dirty = true;
    switch( token ) {
    case TY_FIELD:
        dirty = false;
        break;
    case TY_ONTOP:
        type->on_top = true;
        break;
    case TY_HASTOP:
        type->has_top = true;
        dirty = false;
        break;
    case TY_AUTOEXPAND:
        type->autoexpand = true;
        break;
    case TY_ISSTRUCT:
        VarDisplayAlias( type, VarDisplayAddStruct( ScanName() ) );
        dirty = false;
        /* fall through */
    case TY_HEX:
        type->display |= VARDISP_HEX;
        break;
    case TY_DECIMAL:
        type->display |= VARDISP_DECIMAL;
        break;
    case TY_CHARACTER:
        type->display |= VARDISP_CHARACTER;
        break;
    case TY_STRING:
        type->display |= VARDISP_STRING;
        break;
    case TY_POINTER:
        type->display |= VARDISP_POINTER;
        break;
    case TY_HIDE:
#ifdef I_EVER_SOLVE_THIS_THORNY_HIDE_PROBLEM
        type->display |= VARDISP_HIDE;
#endif
        break;
    case TY_NOCODE:
        type->hide |= VARNODE_CODE;
        break;
    case TY_NOINHERIT:
        type->hide |= VARNODE_INHERIT;
        break;
    case TY_NOCOMPILER:
        type->hide |= VARNODE_COMPILER;
        break;
    case TY_NOPRIVATE:
        type->hide |= VARNODE_PRIVATE;
        break;
    case TY_NOPROTECTED:
        type->hide |= VARNODE_PROTECTED;
        break;
    case TY_NOSTATIC:
        type->hide |= VARNODE_STATIC;
        break;
    case TY_CODE:
        type->hide &= ~VARNODE_CODE;
        break;
    case TY_INHERIT:
        type->hide &= ~VARNODE_INHERIT;
        break;
    case TY_COMPILER:
        type->hide &= ~VARNODE_COMPILER;
        break;
    case TY_PRIVATE:
        type->hide &= ~VARNODE_PRIVATE;
        break;
    case TY_PROTECTED:
        type->hide &= ~VARNODE_PROTECTED;
        break;
    case TY_STATIC:
        type->hide &= ~VARNODE_STATIC;
        break;
    default:
        oops();
    }
    if( dirty )
        VarDisplayDirty( type );
    return( token );
}


void TypeSet( void )
/******************/
{
    type_display        *parent;
    type_display        *field;

    ScanLeftBrace();
    while( !ScanRightBrace() ) {
        if( ScanCmd( TypeSettings ) != TY_STRUCT )
            oops();
        parent = VarDisplayAddStruct( ScanName() );
        ScanLeftBrace();
        while( !ScanRightBrace() ) {
            if( ScanAttribute( parent, ScanCmd( TypeSettings ) ) == TY_FIELD ) {
                field = VarDisplayAddField( parent, ScanName() );
                ScanLeftBrace();
                while( !ScanRightBrace() ) {
                    if( ScanAttribute( field, ScanCmd( TypeSettings ) ) == TY_FIELD ) {
                        oops();
                    }
                }
            }
        }
    }
    ReqEOC();
}

void TypeConf( void )
/*******************/
{
    type_display *curr;
    type_display *fcurr;

    StrCopy( " {", StrCopy( NameBuff, TxtBuff ) );
    DUIDlgTxt( TxtBuff );
    for( curr = TypeDisplay; curr != NULL; curr = curr->next ) {
        if( !curr->dirty )
            continue;
        Attributes( curr,
                    StrCopy( " { ",
                    StrCopy( curr->name,
                    StrCopy( " ",
                    GetCmdEntry( TypeSettings, TY_STRUCT,
                    StrCopy( "  ", TxtBuff ) ) ) ) ) );
        DUIDlgTxt( TxtBuff );
        for( fcurr = curr->fields; fcurr != NULL; fcurr = fcurr->next ) {
            if( !fcurr->dirty )
                continue;
            StrCopy( "}",
            Attributes( fcurr,
            StrCopy( " { ",
            StrCopy( fcurr->name,
            StrCopy( " ",
            GetCmdEntry( TypeSettings, TY_FIELD,
            StrCopy( "   ", TxtBuff ) ) ) ) ) ) );
            DUIDlgTxt( TxtBuff );
        }
        DUIDlgTxt( "  }" );
    }
    DUIDlgTxt( "}" );
}
