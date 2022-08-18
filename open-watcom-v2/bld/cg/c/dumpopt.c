/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Dump internal optimizer data.
*
****************************************************************************/


#include "_cgstd.h"
#include "optwif.h"
#include "inslist.h"
#include "rtrtn.h"
#include "dumpio.h"
#include "dumpopt.h"
#include "feprotos.h"


static  void            DoData( oc_entry *instr );
static  void            DoLabel( oc_handle *instr );
static  void            DoRef( oc_handle *instr );

static const char *CNames[] = {
    #define pick_class(x) #x ,
    #include "occlasss.h"
    #undef pick_class
    ""
};

#if _TARGET & _TARG_INTEL

static const char *Conds[] = {
    "jo   ",
    "jno  ",
    "jb   ",
    "jae  ",
    "je   ",
    "jne  ",
    "jbe  ",
    "ja   ",
    "js   ",
    "jns  ",
    "jp   ",
    "jnp  ",
    "jl   ",
    "jge  ",
    "jle  ",
    "jg   ",
    "jmp  ",
    ""
};

static  const char  *CondName( oc_jcond *oc ) {
/*********************************************/

    return( Conds[oc->cond] );
}

#else

static const char *Conds[] = {
    "je   ",
    "jne  ",
    "je   ",
    "jne  ",
    "jg   ",
    "jle  ",
    "jl   ",
    "jge  ",
    ""
};

static  const char  *CondName( oc_jcond *oc ) {
/*********************************************/

    return( Conds[oc->cond - FIRST_CONDITION] );
}

#endif

static  bool    LblName( label_handle lbl, bool no_prefix )
/*********************************************************/
{
    if( !ValidLbl( lbl ) )
        return( false );
    if( no_prefix ) {
        if( lbl->lbl.sym == NULL ) {
            return( false );
        }
    } else {
        DumpChar( 'L' );
        DumpPtr( lbl );
        if( lbl->lbl.sym == NULL ) {
            return( true );
        }
    }
    DumpChar( '(' );
    if( AskIfRTLabel( lbl ) ) {
        DumpXString( AskRTName( SYM2RTIDX( lbl->lbl.sym ) ) );
    } else if( AskIfCommonLabel( lbl ) ) {
        DumpLiteral( "Common import => [" );
        DumpUInt( (unsigned)(pointer_uint)lbl->lbl.sym );
        DumpLiteral( "] " );
    } else {
        DumpXString( FEName( lbl->lbl.sym ) );
    }
    DumpChar( ')' );
    return( true );
}


static  void    CheckAttr( oc_class cl ) {
/****************************************/

    if( cl & ATTR_FAR ) {
        DumpLiteral( "far " );
    }
    if( cl & ATTR_SHORT ) {
        DumpLiteral( "short " );
    }
    if( cl & ATTR_POP ) {
        DumpLiteral( "popping " );
    }
    if( cl & ATTR_FLOAT ) {
        DumpLiteral( "floating " );
    }
}


static  void    DoInfo( any_oc *oc ) {
/**************************************/

    switch( oc->oc_header.class & INFO_MASK ) {
    case INFO_LINE:
        DumpLiteral( "LINE " );
        DumpInt( oc->oc_linenum.line );
        if( oc->oc_linenum.label_line ) {
            DumpLiteral( " (Label)" );
        }
        break;
    case INFO_LDONE:
        DumpLiteral( "LDONE " );
        LblName( oc->oc_handle.handle, false );
        break;
    case INFO_DEAD_JMP:
        DumpLiteral( "DEAD  " );
        DumpLiteral( "jmp  " );
        DoRef( &(oc->oc_handle) );
        break;
    case INFO_DBG_RTN_BEG:
        DumpLiteral( "RTN BEGIN " );
        DumpPtr( oc->oc_debug.ptr );
        break;
    case INFO_DBG_BLK_BEG:
        DumpLiteral( "BLOCK BEGIN " );
        DumpPtr( oc->oc_debug.ptr );
        break;
    case INFO_DBG_PRO_END:
        DumpLiteral( "PROLOG END " );
        DumpPtr( oc->oc_debug.ptr );
        break;
    case INFO_DBG_EPI_BEG:
        DumpLiteral( "EPILOG BEGIN " );
        DumpPtr( oc->oc_debug.ptr );
        break;
    case INFO_DBG_BLK_END:
        DumpLiteral( "BLOCK END " );
        DumpPtr( oc->oc_debug.ptr );
        break;
    case INFO_DBG_RTN_END:
        DumpLiteral( "RTN END " );
        DumpPtr( oc->oc_debug.ptr );
        break;
    case INFO_SELECT:
        DumpLiteral( "SELECT TABLE " );
        if( oc->oc_select.starts ) {
            DumpLiteral( "STARTS" );
        } else {
            DumpLiteral( "ENDS" );
        }
        break;
    default:
        DumpLiteral( "*** unknown info ***" );
        break;
    }
}


void    DumpOc( ins_entry *ins )
/******************************/
{
    DumpPtr( ins );
    DumpChar( ' ' );
    DumpString(  CNames[_Class( ins )] );
    DumpChar( ' ' );
    if( _Class( ins ) != OC_INFO ) {
        CheckAttr( ins->oc.oc_header.class );
    }
    switch( _Class( ins ) ) {
    case OC_INFO:
        DoInfo ( &ins->oc );
        break;
    case OC_CODE:
        DoData ( &ins->oc.oc_entry );
        break;
    case OC_DATA:
        DoData ( &ins->oc.oc_entry );
        break;
    case OC_IDATA:
        DoData( &ins->oc.oc_entry );
        break;
    case OC_BDATA:
        DoData ( &ins->oc.oc_entry );
        break;
    case OC_LABEL:
        DoLabel( &ins->oc.oc_handle );
        break;
    case OC_LREF:
        DumpLiteral( "dw   " );
        DoRef  ( &ins->oc.oc_handle );
        break;
    case OC_CALL:
        DumpLiteral( "call " );
        DoRef  ( &ins->oc.oc_handle );
        break;
    case OC_CALLI:
        DoData ( &ins->oc.oc_entry );
        break;
    case OC_JCOND:
        DumpString( CondName( &ins->oc.oc_jcond ) );
        DoRef( &ins->oc.oc_handle );
        break;
    case OC_JCONDI:
        DoData( &ins->oc.oc_entry );
        break;
    case OC_JMP:
        DumpLiteral( "jmp  " );
        DoRef  ( &ins->oc.oc_handle );
        break;
    case OC_JMPI:
        DoData ( &ins->oc.oc_entry );
        break;
    case OC_RET:
        DumpInt( ins->oc.oc_ret.pops );
        DumpNL();
        break;
#if _TARGET & _TARG_RISC
    case OC_RCODE:
        DumpPtr( (pointer)(pointer_uint)ins->oc.oc_rins.opcode );
        if( _HasReloc( &ins->oc.oc_rins ) ) {
            DumpLiteral( " [ " );
            LblName( ins->oc.oc_rins.sym, false );
            DumpChar( ',' );
            DumpInt( ins->oc.oc_rins.reloc );
            DumpLiteral( " ] " );
        }
        break;
#endif
    default:
        DumpLiteral( "*** unknown class ***" );
        break;
    }
    DumpNL();
}


static  void    DoData( oc_entry *instr ) {
/*****************************************/

    uint        len;

    for( len = 0; len < instr->hdr.reclen - offsetof( oc_entry, data ); ++len ) {
        DumpByte( instr->data[len] );
        DumpChar( ' ' );
    }
}


static  void    DoLabel( oc_handle *instr ) {
/*******************************************/

    label_handle    lbl;

    lbl = instr->handle;
    DumpLiteral( "align=<" );
    DumpByte( instr->hdr.objlen + 1 );
    DumpLiteral( "> " );
    while( LblName( lbl, false ) ) {
        lbl = lbl->alias;
        if( lbl == NULL )
            break;
        DumpChar( ' ' );
    }
#if _TARGET & _TARG_RISC
    if( instr->line != 0 ) {
        DumpLiteral( "line=<" );
        DumpInt( instr->line );
        DumpLiteral( "> " );
    }
#endif
}


static  void    DoRef( oc_handle *instr )
/***************************************/
{
    LblName( instr->handle, false );
}


void    DumpLbl( label_handle lbl )
/*********************************/
{
    ins_entry   *ref;

    if( !ValidLbl( lbl ) )
        return;
    if( lbl->lbl.sym != NULL ) {
        if( LblName( lbl, true ) ) {
            DumpLiteral( " " );
        }
    }
    DumpLiteral( "addr==" );
    Dump8h( lbl->lbl.address );
    DumpLiteral( ", patch==" );
    DumpPtr( lbl->lbl.patch );
    DumpChar( ' ' );
    if( _TstStatus( lbl, CODELABEL ) ) {
        DumpLiteral( "CODE " );
    }
    if( _TstStatus( lbl, KEEPLABEL ) ) {
        DumpLiteral( "KEEP " );
    }
    if( _TstStatus( lbl, DYINGLABEL ) ) {
        DumpLiteral( "DYING " );
    }
    if( _TstStatus( lbl, SHORTREACH ) ) {
        DumpLiteral( "S-REACH " );
    }
    if( _TstStatus( lbl, CONDEMNED ) ) {
        DumpLiteral( "CONDEMNED " );
    }
    if( _TstStatus( lbl, RUNTIME ) ) {
        DumpLiteral( "RT " );
    }
    if( _TstStatus( lbl, REDIRECTION ) ) {
        DumpLiteral( "REDIR " );
    }
    if( _TstStatus( lbl, UNIQUE ) ) {
        DumpLiteral( "UNIQUE " );
    }
    DumpNL();
    if( lbl->ins != NULL ) {
        DumpLiteral( "ins==" );
        DumpPtr( lbl->ins );
        DumpChar( ' ' );
    }
    if( lbl->redirect != NULL ) {
        DumpLiteral( "redir==" );
        DumpPtr( lbl->redirect );
        DumpChar( ' ' );
    }
    ref = lbl->refs;
    if( ref != NULL ) {
        DumpLiteral( "ref==" );
        for(;;) {
            DumpPtr( ref );
            DumpLiteral( "  " );
            ref = _LblRef( ref );
            if( ref == NULL ) {
                break;
            }
        }
    }
    DumpNL();
}


void    DownOpt( ins_entry *instr, uint num )
/*******************************************/
{
    DumpLiteral( "--------<Queue>-------" );
    DumpNL();
    for( ; instr != NULL && num-- > 0; instr = instr->ins.next ) {
        DumpOc( instr );
    }
}


void    UpOpt( ins_entry *ins, uint last )
/****************************************/
{
    uint        size;

    for( size = last; size > 0; --size ) {
        ins = ins->ins.prev;
        if( ins == NULL ) {
            break;
        }
    }
    DownOpt( ins, last + 1 );
}


void    DumpOpt( void )
/*********************/
{
    DownOpt( FirstIns, ~0U );
}
