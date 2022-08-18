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
* Description:  Generate browsing information.
*
****************************************************************************/


#include "ftnstd.h"
#include "walloca.h"
#include "errcod.h"
#include "global.h"
#include "cpopt.h"
#include "progsw.h"
#include "brseinfo.h"
#include "dw.h"
#include "astype.h"
#include "browscli.h"
#include "fmemmgr.h"
#include "types.h"
#include "ferror.h"
#include "dwarfid.h"
#include "mkname.h"
#include "filescan.h"
#include "rstutils.h"

#include "clibext.h"


// linked list storage facility
typedef struct sym_list {
    struct sym_list     *link;                  // link
    sym_id              id;                     // symbol entry id
    dw_handle           dbh;                    // dwarf handle
    struct sym_list     *children;              // children of this entry
} sym_list;

typedef dw_handle (*func)( sym_id ste_ptr, dw_handle handle );

static void             BIAdd2List(sym_list **,sym_id,dw_handle);
static void             BIWalkList(sym_list **,func,int);
static dw_handle        BIGetAnyType(sym_id);
static dw_handle        BIGetType(sym_id);
static dw_handle        BIGetSPType(sym_id);
static dw_handle        BIGetArrayType(sym_id);
static dw_handle        BIGetStructType(sym_id,dw_handle);
static dw_handle        BILateRefSym(sym_id,dw_handle);
static dw_handle        BIStartStructType(sym_id,int);
static dw_handle        BIGetUnionType(sym_id);
static void             BIOutSP(sym_id);
static void             BISolidifyFunction(sym_id,dw_handle);
static void             BIOutDummies(entry_pt *);
static void             BIOutDeclareSP(sym_id,long);
static void             BIOutSF(sym_id);
static void             BIOutSPDumInfo(sym_id);
static void             BIOutVar(sym_id);
static void             BIOutConst(sym_id);
static void             BIDumpAllEntryPoints(entry_pt *,int);
static dw_handle        BIGetHandle(sym_id);
static void             BISetHandle(sym_id,dw_handle);
static char *           BIMKFullPath(const char *);
static void             BIInitBaseTypes(void);

static char             BrowseExtn[] = { "mbr" };

static dw_client        cBIId;
static dw_loc_handle    justJunk;
static char             fullPathName[PATH_MAX + 1];
static dw_handle        subProgTyHandle;
static unsigned_32      currState = 0;
static sym_list         *fixStructs = NULL;
static sym_list         *fixSubParms = NULL;

static dw_handle        baseTypes[LAST_BASE_TYPE + 1];

/* Forward declarations */
static void BIRefSymbol( dw_handle handle );


#define BI_STATE_IN_COMMON_BLOCK        0x00000001
#define BI_STATE_IN_STMT_FUNC           0x00000002
#define BI_STATE_RESOLVED               0x00000004
#define BI_STATE_IN_SCOPE               0x00000008
#define BI_NAMELIST_UNDEFINED           0x00000010

static  bool            BrInitialized;

#if _CPU == 8086
 #define ARCHITECTURE   sizeof( short )
#else
 #define ARCHITECTURE   sizeof( long )
#endif

#define _GenerateBrInfo()     ((Options & OPT_BROWSE) && \
                                 (BrInitialized) && \
                                 (ProgSw & PS_DONT_GENERATE))

#define _isFundamentalType( typ ) \
                        (((int)typ >= FIRST_BASE_TYPE) && ((int)typ <= LAST_BASE_TYPE))

void    BIInit( void ) {
//================

    dw_init_info        init_dwl;
    dw_cu_info          cu;

    BrInitialized = true;
    if( !_GenerateBrInfo() )
        return;
    init_dwl.language = DWLANG_FORTRAN;
    init_dwl.compiler_options = DW_CM_BROWSER | DW_CM_UPPER;
    init_dwl.producer_name = DWARF_PRODUCER_ID " V1";
    init_dwl.abbrev_sym = NULL;
    if( !setjmp( init_dwl.exception_handler ) ) {
        CLIInit( &(init_dwl.funcs), MEM_SECTION );
        cBIId = DWInit( &init_dwl );
        justJunk = DWLocFini( cBIId, DWLocInit( cBIId ) );
        cu.source_filename=BIMKFullPath( CurrFile->name );
        cu.directory=".";
        cu.flags = true;
        cu.offset_size = ARCHITECTURE;
        cu.segment_size = 0;
        cu.model = DW_MODEL_NONE;
        cu.inc_list = NULL;
        cu.inc_list_len = 0;
        cu.dbg_pch = NULL;
        DWBeginCompileUnit( cBIId, &cu );
        BISetSrcFile();
    } else {
        BrInitialized = false;
        Error( SM_BROWSE_ERROR );
    }
    BIInitBaseTypes();
}

void    BIEnd( void ) {
//===============

    char        fn[_MAX_PATH];

    if( !_GenerateBrInfo() )
        return;
    MakeName( SDFName( SrcName ), BrowseExtn, fn );
    DWEndCompileUnit( cBIId );
    DWLocTrash( cBIId, justJunk );
    DWFini( cBIId );
    CLIDump( fn );
    CLIFini();
}

void    BIStartSubProg( void ) {
//========================
}

void    BIStartSubroutine( void ) {
//===========================

    if( _GenerateBrInfo() ) {
        if( (SubProgId->u.ns.flags & SY_SUBPROG_TYPE) != SY_BLOCK_DATA ) {
            BIOutSrcLine();
            BIOutSP( SubProgId );
        }
        currState |= BI_STATE_IN_SCOPE;
    }
}

void    BIFiniStartOfSubroutine( void ) {
//=================================

    if( _GenerateBrInfo() && (currState & BI_STATE_IN_SCOPE) ) {
        if( (SubProgId->u.ns.flags & SY_SUBPROG_TYPE) == SY_FUNCTION ) {
            BISolidifyFunction( SubProgId, subProgTyHandle );
        }
        if( (SubProgId->u.ns.flags & SY_SUBPROG_TYPE) != SY_BLOCK_DATA ) {
            if( currState & BI_STATE_IN_SCOPE ) {
                BIOutDummies( Entries );
            }
        }
        BIWalkList( &fixSubParms, &BILateRefSym, true );
        currState |= BI_STATE_RESOLVED;
    }
}

void    BIEndBlockData( void ) {
//==============================

    if( _GenerateBrInfo() ) {
        currState &= ~BI_STATE_IN_COMMON_BLOCK;
        DWEndLexicalBlock ( cBIId );
    }
}

void    BIEndSubProg( void ) {
//======================

    if( _GenerateBrInfo() ) {
        if( (SubProgId->u.ns.flags & SY_SUBPROG_TYPE) == SY_BLOCK_DATA ) {
            BIEndBlockData();
        } else {
            BIDumpAllEntryPoints( Entries, 0 );
            DWEndSubroutine ( cBIId );
        }
        currState &= ~(BI_STATE_IN_SCOPE | BI_STATE_RESOLVED);
    }
}

void BIResolveUndefTypes( void ) {
//==========================

    if( _GenerateBrInfo() ) {
        BIWalkList( &fixStructs, &BIGetStructType, true );
    }
}

void    BIEndSF( sym_id ste_ptr ) {
//=================================

    if( _GenerateBrInfo() ) {
        DWEndSubroutine ( cBIId );
        BIRefSymbol( BIGetHandle( ste_ptr ) );
        currState &= ~BI_STATE_IN_STMT_FUNC;
    }
}

void    BIStartRBorEP( sym_id ste_ptr ) {
//=======================================

    if( _GenerateBrInfo() ) {
        BIOutSP( ste_ptr );
        if( ste_ptr->u.ns.flags & SY_SENTRY ) {
            BIOutDummies( ArgList );
        }
    }
}

void    BIEndRBorEP( void ) {
//===========================

    if( _GenerateBrInfo() ) {
        DWEndSubroutine ( cBIId );
    }
}

void    BIStartComBlock( sym_id ste_ptr )
//=======================================
{
    if( _GenerateBrInfo() ) {
        DWDeclPos( cBIId, CurrFile->rec, 0 );
        currState |= BI_STATE_IN_COMMON_BLOCK;
        DWIncludeCommonBlock( cBIId, DWBeginCommonBlock( cBIId, justJunk, 0, ste_ptr->u.ns.name, 0 ) );

    }
}

void    BIEndComBlock( void ) {
//=============================

    if( _GenerateBrInfo() ) {
        currState &= ~BI_STATE_IN_COMMON_BLOCK;
        DWEndCommonBlock ( cBIId );
    }
}

void    BIStartBlockData( sym_id ste_ptr )
//========================================
{
    if( _GenerateBrInfo() ) {
        DWDeclPos( cBIId, CurrFile->rec, 0 );
        DWBeginLexicalBlock( cBIId, 0, ste_ptr->u.ns.name );
    }
}

void    BIOutComSymbol( sym_id ste_ptr ) {
//========================================

    if( _GenerateBrInfo() ) {
        BIOutVar( ste_ptr );
    }
}

void    BIOutNameList( sym_id ste_ptr ) {
//=======================================

    char        name[33];
    grp_entry   *ge;
    dw_handle   var;

    if( _GenerateBrInfo() ) {
        if( !( ste_ptr->u.nl.dbh ) ) {
            strncpy( name, ste_ptr->u.nl.name, ste_ptr->u.nl.name_len );
            name[ste_ptr->u.nl.name_len] = 0;
            BIOutSrcLine();
            for( ge = ste_ptr->u.nl.group_list; ge != NULL; ge = ge->link ) {
                var = BIGetHandle( ge->sym );
                if( !var ) {
                    BIOutSymbol( ge->sym );
                }
            }
            ste_ptr->u.nl.dbh = DWNameListBegin( cBIId, name );
            for( ge = ste_ptr->u.nl.group_list; ge != NULL; ge = ge->link ) {
                DWNameListItem( cBIId, BIGetHandle( ge->sym ) );
            }
            DWEndNameList( cBIId );
        }
        BIRefSymbol( ste_ptr->u.nl.dbh );
    }
}

void    BIOutSymbol( sym_id ste_ptr ) {
//=====================================

// define/declare/reference a symbol

    dw_handle   temp;

    if( !_GenerateBrInfo() )
        return;
    if( (currState & BI_STATE_IN_SCOPE) == 0 ) {
        BISetHandle( ste_ptr, 0 );
        return;
    }
    BIOutSrcLine();
    if( (ste_ptr->u.ns.flags & SY_REFERENCED) == 0 ) {
        if( (ste_ptr->u.ns.flags & SY_CLASS) == SY_SUBPROGRAM ) {
            if( (ste_ptr->u.ns.flags & SY_SUBPROG_TYPE) == SY_STMT_FUNC ) {
                if( (ASType & AST_ASF) &&
                        (currState & BI_STATE_IN_STMT_FUNC) == 0 ) { //if defining s.f.
                    BIOutSF( ste_ptr );
                } else {
                    BIRefSymbol( BIGetHandle( ste_ptr ) );
                }
            } else if( (ste_ptr->u.ns.flags & SY_SUBPROG_TYPE) == SY_REMOTE_BLOCK ) {
                BIOutDeclareSP( ste_ptr, 0 );
            } else {
                if( (ste_ptr->u.ns.flags & SY_PS_ENTRY) == 0 ) {
                    BIOutDeclareSP( ste_ptr, DW_FLAG_GLOBAL );
                } else {
                    BIRefSymbol( BIGetHandle( ste_ptr ) );
                }
            }
        } else if( (ste_ptr->u.ns.flags & SY_CLASS) == SY_VARIABLE ) {
                if( ste_ptr->u.ns.flags & SY_SUB_PARM ) {
                    if( currState & BI_STATE_RESOLVED ) {
                        BIRefSymbol( BIGetHandle( ste_ptr ) );
                    } else {
                        BIAdd2List( &fixSubParms, ste_ptr, CurrFile->rec );
                    }
                } else if( (ste_ptr->u.ns.flags & SY_SPECIAL_PARM) == 0 ) {
                    if( (ste_ptr->u.ns.flags & SY_IN_COMMON) == 0 ||
                       (currState & BI_STATE_IN_COMMON_BLOCK) ) {
                        BIOutVar( ste_ptr );
                    } else if( (ste_ptr->u.ns.flags & (SY_DATA_INIT | SY_IN_DIMEXPR)) == 0 ||
                                (ste_ptr->u.ns.flags & SY_IN_COMMON) == 0 ) {
                        BIRefSymbol( BIGetHandle( ste_ptr ) );
                    }
                } else if( currState & BI_STATE_IN_STMT_FUNC ) {
                    BIRefSymbol( BIGetHandle( ste_ptr->u.ns.si.ms.sym ) );
                }
        } else if( (ste_ptr->u.ns.flags & SY_CLASS) == SY_PARAMETER ) {
            BIOutConst( ste_ptr );
        }
    } else {
        // Do we need to use the magic symbol when referencing?
        if( (currState & BI_STATE_IN_STMT_FUNC) &&
             (ste_ptr->u.ns.flags & SY_SPECIAL_PARM) ) {
            BIRefSymbol( BIGetHandle( ste_ptr->u.ns.si.ms.sym ) );
        } else if( (ste_ptr->u.ns.flags & SY_SUB_PARM) &&
                  ( (currState & BI_STATE_RESOLVED) == 0 ) ) {
            BIAdd2List( &fixSubParms, ste_ptr, CurrFile->rec );
        } else {
            temp = BIGetHandle( ste_ptr );
            if( temp ) {
                BIRefSymbol( temp );
            } else {
                // Consider:    data ( x(i), i=1,3 ) ...
                //              do 666 i = 1, 3
                //               ....
                //      666      continue
                // The variable has yet to be declared since it
                // was first referenced before the sub prog definition
                // so we must turn of the reference bit,
                // Dump thhe symbol
                // and set the bit on again
                ste_ptr->u.ns.flags &= ~SY_REFERENCED;
                BIOutSymbol( ste_ptr );
                ste_ptr->u.ns.flags |= SY_REFERENCED;
            }
        }
    }

}

void BISetSrcFile( void ) {
//===================

// Set Current Source Line

    char        *name;

    if( _GenerateBrInfo() ) {
        if( (ProgSw & PS_FATAL_ERROR) == 0 && CurrFile ) {
            name = BIMKFullPath( CurrFile->name );
            DWSetFile( cBIId, name );
            DWDeclFile( cBIId, name );
            DWLineNum( cBIId, DW_LN_DEFAULT, CurrFile->rec, 0, 0 );
            DWDeclPos( cBIId, CurrFile->rec, 0 );
        }
    }
}

void BIOutSrcLine( void ) {
//===================

// Set Current Source Line

    if( _GenerateBrInfo() ) {
        DWLineNum( cBIId, DW_LN_DEFAULT, SrcRecNum, 0, 0 );
        DWDeclPos( cBIId, CurrFile->rec, 0 );
    }
}


static dw_handle BIGetHandle( sym_id ste_ptr) {
//=============================================

    return( ste_ptr->u.ns.u3.dbh );
}


static void BISetHandle( sym_id ste_ptr, dw_handle handle ) {
//===========================================================

    ste_ptr->u.ns.u3.dbh = handle;
}


static void BIRefSymbol( dw_handle handle ) {
//===========================================

    if( _GenerateBrInfo() ) {
        DWReference( cBIId, SrcRecNum, 0, handle );
    }
}


static dw_handle BILateRefSym( sym_id ste_ptr, dw_handle handle ) {
//=================================================================

    unsigned_32     temp = SrcRecNum;

    SrcRecNum = handle;
    BIRefSymbol( BIGetHandle( ste_ptr ) );
    SrcRecNum = temp;
    return( 0 );
}


static void BIOutDummies( entry_pt *dum_lst ) {
//=============================================

    parameter           *curr_parm;

    if( !dum_lst ) {
        return;
    }
    for( curr_parm = dum_lst->parms; curr_parm != NULL; curr_parm = curr_parm->link ) {
        if( (curr_parm->flags & ARG_STMTNO) == 0 ) {
            BIOutSPDumInfo( curr_parm->id );
        }
    }
}


static void BIDumpAllEntryPoints( entry_pt *dum_lst, int level ) {
//================================================================

    // Close up all entry points for the subprogram.
    // This is done recursively since scope might be a factor
    // in the future

    if( !dum_lst ) {
        return;
    }
    BIDumpAllEntryPoints( dum_lst->link, level + 1 );
    if( level ) {
        BIEndRBorEP();
    }
}


static void BIOutSP( sym_id ste_ptr )
//===================================
// Dump the subprogram.
{
    uint        flags = 0;
    dw_handle   fret;

    DWDeclPos( cBIId, CurrFile->rec, 0 );
    if( (ste_ptr->u.ns.flags & SY_SUBPROG_TYPE) == SY_FUNCTION ) {
        if( ste_ptr->u.ns.flags & SY_SENTRY ) {
            fret = subProgTyHandle;
        } else {
            if( ste_ptr->u.ns.u1.s.typ == FT_STRUCTURE ) {
                fret = BIStartStructType( ste_ptr, false );
            } else {
                fret = DWHandle( cBIId, DW_ST_NONE );
            }
            subProgTyHandle = fret;
        }
    } else {
        fret = BIGetSPType( ste_ptr );
    }
    if( ste_ptr->u.ns.flags & SY_SENTRY ) {
        fret = DWBeginEntryPoint( cBIId, fret, justJunk, 0, ste_ptr->u.ns.name, 0, flags );
    } else {
        if( (ste_ptr->u.ns.flags & SY_SUBPROG_TYPE) == SY_PROGRAM ) {
            flags |= DW_FLAG_MAIN;
        }
        fret = DWBeginSubroutine( cBIId, 0, fret, justJunk, 0, 0,
                    0, 0, ste_ptr->u.ns.name, 0, flags );

    }
    BISetHandle( ste_ptr, fret );
}


static int BIMapType( TYPE typ ) {
//===============================

// Map our type to a DWARF fundamental type

    switch( typ ) {
    case( FT_LOGICAL_1 ):
    case( FT_LOGICAL ):         return( DW_FT_BOOLEAN );
    case( FT_INTEGER_1 ):
    case( FT_INTEGER_2 ):
    case( FT_INTEGER ):         return( DW_FT_SIGNED );
    case( FT_REAL ):
    case( FT_DOUBLE ):
    case( FT_TRUE_EXTENDED ):   return( DW_FT_FLOAT );
    case( FT_COMPLEX ):
    case( FT_DCOMPLEX ):
    case( FT_TRUE_XCOMPLEX ):   return( DW_FT_COMPLEX_FLOAT );
    case( FT_CHAR ):            return( DW_FT_UNSIGNED_CHAR );
    }
    return( 0 );
}


static dw_handle BIMakeFundamental( TYPE typ ) {
//=============================================

// create a new fundamental handle seperate from the one created at birth

    return( DWFundamental(cBIId, TypeKW(typ), BIMapType(typ), TypeSize(typ)) );
}


static void BISolidifyFunction( sym_id ste_ptr, dw_handle handle ) {
//==================================================================

//  solidify the function type;

    if( ste_ptr->u.ns.u1.s.typ != FT_STRUCTURE ) {
        DWHandleSet( cBIId, handle );
    }
    if( _isFundamentalType( ste_ptr->u.ns.u1.s.typ ) ) {
        // since we now emit our fundamentals at init time, we must explicitly
        // create another fundemntal handle rather than using the ones created
        // at birth.  This is necessary because we must set next handle emitted
        // to that type
        BIMakeFundamental( ste_ptr->u.ns.u1.s.typ );
    } else {
        BIGetSPType( ste_ptr );
    }
}


static void BIOutDeclareSP( sym_id ste_ptr, long flags )
//======================================================
// Dump the name of an external or intrinsic function. and its data
{
    dw_handle           handle;

    if( ste_ptr == SubProgId )
        return;
    flags |= DW_FLAG_DECLARATION;

    handle = DWBeginSubroutine( cBIId, 0, BIGetSPType( ste_ptr ), justJunk,
                 0, 0, 0, 0, ste_ptr->u.ns.name, 0, flags );
    DWEndSubroutine ( cBIId );
    BISetHandle( ste_ptr, handle );
    BIRefSymbol( handle );
}


static void BIOutSF( sym_id ste_ptr ) {
//=====================================

// Dump the name of a statement function. and its data

    sf_parm             *tmp;

    BIOutSP( ste_ptr );
    for( tmp = ste_ptr->u.ns.si.sf.header->parm_list; tmp != NULL; tmp = tmp->link ) {
        BIOutSPDumInfo( tmp->actual );
    }
    currState |= BI_STATE_IN_STMT_FUNC;
}


static void BIOutSPDumInfo( sym_id ste_ptr )
//==========================================
// Dump the name of a subprogram dummy argument.
{
    dw_handle           handle;

    handle = DWFormalParameter( cBIId, BIGetAnyType( ste_ptr ), 0, 0,
                                ste_ptr->u.ns.name, DW_DEFAULT_NONE );
    BIRefSymbol( handle );
    BISetHandle( ste_ptr, handle );
}


static void BIOutVar( sym_id ste_ptr )
//====================================
// Dump the name of a variable.
{
    dw_handle           handle;

    handle = DWVariable(cBIId, BIGetAnyType(ste_ptr), 0, 0, 0,
                        ste_ptr->u.ns.name, 0, 0 );
    BIRefSymbol( handle );
    BISetHandle( ste_ptr, handle );
}


static void BIOutConst( sym_id ste_ptr )
//======================================
// Dump the name of a variable.
{
    dw_handle           handle;
    void                *value;

    if( ste_ptr->u.ns.u1.s.typ == FT_CHAR ) {
        value = &(ste_ptr->u.ns.si.pc.value->u.lt.value);
    } else {
        value = &(ste_ptr->u.ns.si.pc.value->u.cn.value);
    }
    handle = DWConstant(cBIId, BIGetAnyType(ste_ptr), value,
                        ste_ptr->u.ns.xt.size, 0,
                        ste_ptr->u.ns.name, 0, 0 );
    BIRefSymbol( handle );
    BISetHandle( ste_ptr, handle );
}


static dw_handle        BIGetAnyType( sym_id ste_ptr ) {
//======================================================

// return any and all type

    if( ste_ptr->u.ns.flags & SY_SUBSCRIPTED ) {
        return( BIGetArrayType( ste_ptr ) );
    } else {
        return( BIGetType( ste_ptr ) );
    }
}


static dw_handle BIGetSPType( sym_id ste_ptr ) {
//==============================================

    switch( ste_ptr->u.ns.flags & SY_SUBPROG_TYPE ) {
    case( SY_SUBROUTINE ) :
    case( SY_REMOTE_BLOCK ):
    case( SY_BLOCK_DATA ) :     return( 0 );
    case( SY_FUNCTION ) :
    case( SY_PROGRAM ) :
    case( SY_STMT_FUNC ) :
    case( SY_FN_OR_SUB ) :
        if( ste_ptr->u.ns.u1.s.typ == FT_STRUCTURE && ste_ptr->u.ns.xt.record == NULL ) {
            return( BIStartStructType( ste_ptr, true ) );
        }
        return( BIGetType( ste_ptr ) );
    }
    return( 0 );
}


static dw_handle BIGetType( sym_id ste_ptr ) {
//============================================

// Get the Symbol's NON COMPOUND DWARF TYPE,

    TYPE        typ = ste_ptr->u.ns.u1.s.typ;
    dw_handle   ret = 0;

    switch( typ ) {
    case( FT_LOGICAL_1 ):
    case( FT_LOGICAL ):
    case( FT_INTEGER_1 ):
    case( FT_INTEGER_2 ):
    case( FT_INTEGER ):
    case( FT_HEX ):
    case( FT_REAL ):
    case( FT_DOUBLE ):
    case( FT_TRUE_EXTENDED ):
    case( FT_COMPLEX ):
    case( FT_DCOMPLEX ):
    case( FT_TRUE_XCOMPLEX ):
        ret = baseTypes[typ];
        break;
    case( FT_CHAR ):
        ret = DWString(cBIId, 0, ste_ptr->u.ns.xt.size, "", 0, 0);
        break;
    case( FT_UNION ):
        ret = BIGetUnionType( ste_ptr );
        break;
    case( FT_STRUCTURE ):
        ret = BIGetStructType( ste_ptr, 0 );
        break;
    }
    DWDeclPos( cBIId, CurrFile->rec, 0 );
    return( ret );
}


static dw_handle BIGetBaseType( TYPE typ ) {
//=========================================

// Get initialized base type

    DWDeclPos( cBIId, CurrFile->rec, 0 );
    return( baseTypes[typ] );
}


static dw_handle BIGetArrayType( sym_id ste_ptr ) {
//=================================================

// Get An array type of a named symbol

    int         dim_cnt;
    dw_dim_info data;
    intstar4    *bounds;
    dw_handle   ret;

    dim_cnt = _DimCount( ste_ptr->u.ns.si.va.u.dim_ext->dim_flags );
    data.index_type = BIGetBaseType( FT_INTEGER );
    bounds = &ste_ptr->u.ns.si.va.u.dim_ext->subs_1_lo;
    ret = DWBeginArray( cBIId, BIGetType( ste_ptr ), 0, NULL, 0, 0 );
    while( dim_cnt-- > 0 ) {
        data.lo_data = *bounds++;
        data.hi_data = *bounds++;
        DWArrayDimension( cBIId, &data );
    }
    DWDeclPos( cBIId, CurrFile->rec, 0 );
    DWEndArray( cBIId );
    return( ret );
}


static dw_handle BIGetStructType( sym_id ste_ptr, dw_handle handle ) {
//====================================================================

// get a struct type of a non named symbol

    struct field        *fields;
    dw_handle           ret;
    sym_id              data = alloca( sizeof( named_symbol ) + MAX_SYMLEN );
    char                *name;
    char                buffer[MAX_SYMLEN+1];
    long                un = 0;

    if( handle ) {
        ret = handle;
    } else if( ste_ptr->u.ns.xt.record->dbh ) {
        return( ste_ptr->u.ns.xt.record->dbh );
    } else {
        // consider: record /bar/ function x()
        //                       ....
        // we want to use the handle we used when the function
        // was defined
        if( ste_ptr->u.ns.xt.record == SubProgId->u.ns.xt.record ) {
            ret = subProgTyHandle;
        } else {
            ret = DWStruct( cBIId, DW_ST_STRUCT );
        }
    }
    ste_ptr->u.ns.xt.record->dbh = ret;
    DWDeclPos( cBIId, CurrFile->rec, 0 );
    memset( buffer, 0, sizeof( buffer ) );
    DWBeginStruct( cBIId, ret, ste_ptr->u.ns.xt.record->size,
                        ste_ptr->u.ns.xt.record->name, 0, 0 );
    for( fields = ste_ptr->u.ns.xt.record->fl.fields; fields != NULL; fields = &fields->link->u.fd ) {
        data->u.ns.u1.s.typ = fields->typ;
        data->u.ns.xt.record = fields->xt.record;
        if( fields->typ == FT_UNION ) {
            data->u.ns.si.va.u.dim_ext = NULL;
            data->u.ns.u2.name_len = 0;
            data->u.ns.name[0] = NULLCHAR;
            un++;
            name = data->u.ns.name;
        } else {
            data->u.ns.si.va.u.dim_ext = fields->dim_ext;
            data->u.ns.u2.name_len = fields->name_len;
            strncpy( data->u.ns.name, fields->name, fields->name_len );
            data->u.ns.name[fields->name_len] = NULLCHAR;
            name = NULL;
            if( data->u.ns.name[0] != NULLCHAR ) {
                name = data->u.ns.name;
            }
        }
        if( data->u.ns.si.va.u.dim_ext ) {
            DWAddField(cBIId, BIGetArrayType(data), justJunk, name, 0);
        } else {
            DWAddField( cBIId, BIGetType( data ), justJunk, name, 0 );
        }
    }
    DWEndStruct( cBIId );
    return( ret );
}


static dw_handle BIGetUnionType( sym_id ste_ptr ) {
//=================================================

// get a union type of a non named symbol

    struct fstruct      *fs;
    dw_handle           ret;
    sym_id              sym;
    size_t              max = 0;
    int                 map = 0;
    char                buff[15];

    ret = DWStruct( cBIId, DW_ST_UNION );
    // find the largest size of map
    for( fs = ste_ptr->u.ns.xt.record; fs != NULL; fs = &fs->link->u.sd ) {
        if( max < fs->size ) {
            max = fs->size;
        }
    }
    // Start the union declaration
    DWDeclPos( cBIId, CurrFile->rec, 0 );
    DWBeginStruct( cBIId, ret, max, ste_ptr->u.ns.name, 0, 0 );
    sym = FMemAlloc( sizeof( named_symbol ) + sizeof( buff ) );
    sym->u.ns.xt.record = FMemAlloc( sizeof( fstruct) + sizeof( buff ) );
    for( fs = ste_ptr->u.ns.xt.record; fs != NULL; fs = &fs->link->u.sd ) {
        memset( sym->u.ns.xt.record, 0, sizeof( fstruct ) + sizeof( buff ) );
        memcpy( sym->u.ns.xt.record, fs, sizeof( fmap ) );
        sym->u.ns.si.va.u.dim_ext = NULL;
        sym->u.ns.u1.s.typ = FT_STRUCTURE;
        sym->u.ns.u2.name_len = sprintf( buff, "MAP%d", map );
        strcpy( sym->u.ns.name, buff);
        sym->u.ns.xt.record->name_len = sym->u.ns.u2.name_len;
        strcpy( sym->u.ns.xt.record->name, buff );
        map++;
        DWAddField( cBIId, BIGetType( sym ), justJunk, sym->u.ns.name, 0 );
    }
    FMemFree( sym->u.ns.xt.record );
    FMemFree( sym );
    DWEndStruct( cBIId );
    return( ret );
}


static dw_handle BIStartStructType( sym_id ste_ptr, int add ) {
//=============================================================

// start a struct type of a function definition

    dw_handle   ret;

    ret = DWStruct( cBIId, DW_ST_STRUCT );
    if( add ) {
        BIAdd2List( &fixStructs, ste_ptr, ret );
    }
    return( ret );
}


static void BIAdd2List( sym_list **list, sym_id ste_ptr, dw_handle handle ) {
//===========================================================================

    sym_list    *tmp;

    tmp = FMemAlloc( sizeof( sym_list ) );
    tmp->id = ste_ptr;
    tmp->dbh = handle;
    tmp->link = *list;
    tmp->children = NULL;
    *list = tmp;
}


static void BIWalkList( sym_list **list, func action, int nuke_list ) {
//=====================================================================

    sym_list    *tmp;

    for( tmp = *list; tmp != NULL; ) {
        action( tmp->id, tmp->dbh );
        tmp = tmp->link;
        if( nuke_list ) {
            FMemFree( *list );
            *list = tmp;
        }
    }
}


static char *BIMKFullPath( const char *path ) {
//=============================================

    return( _fullpath( fullPathName, path, PATH_MAX ) );
}


static void BIInitBaseTypes( void ) {
//===================================

    TYPE    x;

    // assume that LAST_BASE_TYPE is the last fundamental type
    // and types from FIRST_BASE_TYPE to LAST_BASE_TYPE are all fundamental
    // base types
    for( x = FIRST_BASE_TYPE; x <= LAST_BASE_TYPE; x++ ) {
        baseTypes[x] = BIMakeFundamental( x );
    }
}
