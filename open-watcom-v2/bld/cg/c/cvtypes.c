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
* Description:  Emit CodeView type information.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "zoiks.h"
#include "cvdbg.h"
#include "data.h"
#include "types.h"
#include "utils.h"
#include "objout.h"
#include "cvsyms.h"
#include "cvtypes.h"
#include "dbsupp.h"
#include "feprotos.h"
#include "cgprotos.h"

struct lf_info {
     char       size;
     lf_values  code;
} lf_info;

static struct lf_info LFInfo[LFG_LAST] = {
    #define _LFMAC( n, N, c )    { sizeof( lf_##n ), c },
    #include "cv4types.h"
    #undef _LFMAC
};

static  void    BuffWrite( cv_out *out, void *to )
/************************************************/
{
    unsigned    len;
    segment_id  old_segid;

    len = (byte *)to - out->beg;
    old_segid = SetOP( out->segid );
    DataBytes( len, out->beg );
    out->beg = to;
    SetOP( old_segid );
}

static  void   BuffSkip( cv_out *out, void *to )
/**********************************************/
{
    out->beg = to;
}

static  void    buffEnd( cv_out *out )
/************************************/
{
    unsigned    len;
    segment_id  old_segid;

    len = out->ptr - out->beg;
    old_segid = SetOP( out->segid );
    DataBytes( len, out->beg );
    SetOP( old_segid );
}

static  void    *BuffInc( cv_out *out, int size )
/***********************************************/
{
    void    *ptr;

    ptr = out->ptr;
    out->ptr += size;
    return( ptr );
}

static  void  *AlignBuff( cv_out *out )
/*** round out->ptr up to align size ***/
{
    int     len;
//    int     len4;
    byte    *ptr;

    ptr = out->ptr;
    len = ptr - out->buff;
//    len4 = ((len+CV_ALIGN-1)& -CV_ALIGN);
    len = _RoundUp( len, CV_ALIGN ) - len;
    for( ; len > 0; --len ) {
        *ptr = (LF_PAD0 | len);
        ++ptr;
    }
    out->ptr = ptr;
    return( ptr );
}


static  void    SegReloc( segment_id segid, cg_sym_handle sym )
/*************************************************************/
{
    segment_id  old_segid;

    old_segid = SetOP( segid );
    FEPtrBase( sym );
    SetOP( old_segid );
}

static  void    *StartType( cv_out *out, lfg_index what )
/*******************************************************/
{
    lf_common   *ptr;

    ptr = (lf_common *)out->ptr;
    out->ptr += LFInfo[what].size;
    ptr->code = LFInfo[what].code;
    ++ptr; /* start of ct_... */
    return( ptr );
}

static  void    NewTypeString( cv_out *out )
/******************************************/
{
    out->segid = CVTypes;
    out->ptr = &out->buff[sizeof( u2 )];  /*skip length*/
    out->beg = out->buff;
}

static  void    NewType( cv_out *out )
/************************************/
{
    out->ptr = out->buff;   /* no length field */
    out->beg = out->buff;
}

static  int  EndSub( cv_out *out )
/*** write out a member of a subfield (don't write a length) ***/
/* return length so it can be backpatched to list head       ***/
/* reset buff to start **/
{
    unsigned        len;
    segment_id      old_segid;
//    long_offset     here;

    AlignBuff( out );
    len = out->ptr - out->buff;
    if( _IsModel( DBG_TYPES ) ) {
        old_segid = SetOP( CVTypes );
//        here = AskBigLocation();
        DataBytes( len, out->buff );
        SetOP( old_segid );
    }
    out->ptr = out->buff;
    return( len );
}

static  long_offset   EndTypeString( cv_out *out )
/************************************************/
{
    segment_id      old_segid;
    unsigned        len;
    long_offset     here = 0;

    if( _IsModel( DBG_TYPES ) ) {
        AlignBuff( out );
        len = out->ptr - out->buff;
        *((u2 *)&out->buff[0]) = len - sizeof( u2 );  /* set type rec len*/
        old_segid = SetOP( CVTypes );
        here = AskBigLocation();
        DataBytes( len, out->buff );
        SetOP( old_segid );
    }
    return( here );
}

void    CVEndType( cv_out *out )
/******************************/
{
    int     len;

    AlignBuff( out );
    len = out->ptr - out->buff;
    *((u2 *)&out->buff[0]) = len-sizeof( u2 );  /* set type rec len*/
}

static  void    PatchLen( long_offset where, u2 what )
/********** back patch field list length ************/
{
    long_offset         here;
    segment_id          old_segid;

    old_segid = SetOP( CVTypes );
    here = AskBigLocation();
    SetBigLocation( where );
    DataShort( what );
    SetBigLocation( here );
    SetOP( old_segid );
}

static  void PutFld2( cv_out *out, short num )
/********************************************/
{
    *((short*)out->ptr) = num;  /* set an imbedded num */
    out->ptr += sizeof( short );
}

static void PutFldSized( cv_out *out, int size, unsigned_32 val )
/***************************************************************/
{
    switch( size ) {
    case 1:
        *((u1*)out->ptr) = val;  /* out 1 */
        out->ptr += sizeof( u1 );
        break;
    case 2:
        *((u2*)out->ptr) = val;  /* out 2 */
        out->ptr += sizeof( u2 );
        break;
    case 4:
        *((u4*)out->ptr) = val;   /* out 4 */
        out->ptr += sizeof( u4 );
        break;
    }
}

static void PutLFInt( cv_out *out, cv_psize size, unsigned_32 val )
/*****************************************************************/
{
    switch( size ) {
    case CV_IB1:
        *((u1*)out->ptr) = val;  /* out 1 */
        out->ptr += sizeof( u1 );
        break;
    case CV_IB2:
        *((u2*)out->ptr) = val;  /* out 2 */
        out->ptr += sizeof( u2 );
        break;
    case CV_IB4:
        *((u4*)out->ptr) = val;   /* out 4 */
        out->ptr += sizeof( u4 );
        break;
    }
}

static lf_values   LFIntType( int size )
/**************************************/
{
    lf_values itipe ;

    switch( size ) {
    case 4:
        itipe = LF_TINT4;
        break;
    case 2:
        itipe = LF_TSHORT;
        break;
    case 1:
        itipe = LF_TCHAR;
        break;
    default:
        itipe = 0;
        Zoiks( ZOIKS_106 ); /* bad pointer */
    }
    return( itipe );
}

void CVPutINum( cv_out *out, signed_32 num )
/******************************************/
{
    byte       *ptr;
#define LC( what, to )   *((to *)&what)
    if( num >= 0 && num < 0x00008000 ){
        *((u2*)out->ptr) = num;  /* out num as is */
        out->ptr += sizeof( u2 );
        return;
    }
    ptr = out->ptr;
    if( num >= -128 && num <= 127 ) {
        LC( ptr[0],u2 ) = LF_CHAR;
        LC( ptr[2],u1 ) = num;
        ptr += sizeof(u2) + sizeof( u1 );
    }else if( num >= -32768 && num <= 32767 ) {
        LC( ptr[0],u2 ) = LF_SHORT;
        LC( ptr[2],u2 ) = num;
        ptr += sizeof(u2) + sizeof( u2 );
    }else if( num > 0 && num <= 0x0000ffff ){
        LC( ptr[0],u2 ) = LF_USHORT;
        LC( ptr[2],u2 ) = num;
        ptr += sizeof(u2) + sizeof( u2 );
    } else {
        LC( ptr[0],u2 ) = LF_LONG;
        LC( ptr[2],u4 ) = num;
        ptr += sizeof(u2) + sizeof( u4 );
    }
    out->ptr = ptr;
#undef LC
}

void        CVPutINum64( cv_out *out, signed_64 val )
/***************************************************/
{
    byte       *ptr;
#define LC( what, to )   *((to *)&what)
    if( val.u._32[I64HI32] == 0 || val.u._32[I64HI32] == -1 ) {
        CVPutINum( out, val.u._32[I64LO32] );
    } else {
        ptr = out->ptr;
        LC( ptr[0],u2 ) = LF_QUADWORD;
        LC( ptr[2],u4 ) = val.u._32[I64LO32];
        LC( ptr[6],u4 ) = val.u._32[I64HI32];
        ptr += sizeof(u2) + sizeof( u8 );
        out->ptr = ptr;
    }
#undef LC
}

void        CVPutStr( cv_out *out, const char *str )
/**************************************************/
{
    size_t      len;

    len = strlen( str );
    if( len > 255 )
        len = 255;
    *(out->ptr++) = len;
    memcpy( out->ptr, str, len );
    out->ptr += len;
}

void        CVPutNullStr( cv_out *out )
/*************************************/
{
    *(out->ptr++) = 0;
}


static  lf_values   LFSignedSize( signed_32 num )
/***********************************************/
{
    cv_primitive    index;

    index.s = 0;
    index.f.mode = CV_DIRECT;
    index.f.type = CV_SIGNED;
    if( num >= -128 && num <= 127 ) {
        index.f.size = CV_IB1;
    } else if( num >= -32768 && num <= 32767 ) {
        index.f.size = CV_IB2;
    } else {
        index.f.size =  CV_IB4;
    }
    return( index.s );
}

static  lf_values    LFSignedRange( signed_32 lo, signed_32 hi )
/**************************************************************/
{
    cv_primitive    index;

    index.s = 0;
    index.f.mode = CV_DIRECT;
    index.f.type = CV_SIGNED;
    if( lo >= -128 && hi <= 127 ) {
        index.f.size = CV_IB1;
    } else if( lo >= -32768 && hi <= 32767 ) {
         index.f.size = CV_IB2;
    } else {
         index.f.size =  CV_IB4;
    }
    return( index.s );
}

dbg_type    CVFtnType( const char *name, dbg_ftn_type tipe )
/**********************************************************/
{
    unsigned        size;
    cv_primitive    index;

    /* unused parameters */ (void)name;

    index.s = 0;
    index.f.mode = CV_DIRECT;
    index.f.type = CV_COMPLEX;
    size = (tipe & 0x0f)+1;
    if( size == 4 ){
        index.f.size = CV_RC32;
    }else if( size == 8 ){
        index.f.size = CV_RC32;
    }
    return( index.s );
}

#define MAX_REAL_NAME  7
static char const RealNames[MAX_REAL_NAME][17] = {
    "int",
    "unsigned int",
    "signed int",
    "char",
    "__int64",
    "unsigned __int64",
    "signed __int64",
};

dbg_type    CVScalar( const char *name, cg_type tipe )
/****************************************************/
{
    type_def          *tipe_addr;
    int                length;
    cv_primitive       index;
    int                count;

    index.s = 0;  /* set bits to 0 */
    if( strcmp( name, "__segment" ) == 0 ){
        index.s = LF_TSEGMENT;
    }else if( strcmp( name, "void" ) == 0 ){
        index.s = LF_TVOID;
    } else {
        tipe_addr = TypeAddress( tipe );
        length = tipe_addr->length;
        index.f.mode =  CV_DIRECT;
        if( tipe_addr->attr & TYPE_FLOAT ) {
            index.f.type = CV_REAL;
            if( length == 4 ) {
                index.f.size  =  CV_RC32;
            } else if( length == 8 ) {
                index.f.size = CV_RC64;
            }
        } else {
            for( count = 0; count < MAX_REAL_NAME; ++count ) {
                if( strcmp( name, RealNames[count] ) == 0 ) {
                    index.f.type =  CV_REALLYINT;
                    if( length == 1 ) {
                        index.f.size = 0; /* assume char (could be wide char )*/
                    } else {
                        if( (tipe_addr->attr & TYPE_SIGNED) == 0 ) {
                            ++length;
                        }
                        index.f.size = length;
                    }
                    goto done;
                }
            }
            if( tipe_addr->attr & TYPE_SIGNED ) {
                index.f.type =  CV_SIGNED;
            } else {
                index.f.type = CV_UNSIGNED;
            }
            switch( length ) {
            case 1:
                index.f.size = CV_IB1;
                break;
            case 2:
                index.f.size = CV_IB2;
                break;
            case 4:
                index.f.size = CV_IB4;
                break;
            case 8:
                index.f.size = CV_IB8;
                break;
            }
        }
    }
done:;
    return( index.s );
}

enum scope_name {
    SCOPE_TYPEDEF = 0,
    SCOPE_STRUCT  = 1,
    SCOPE_UNION   = 2,
    SCOPE_ENUM    = 3,
    SCOPE_CLASS   = 4,
    SCOPE_MAX
};

static char const ScopeNames[SCOPE_MAX][7] = {
    "",
    "struct",
    "union",
    "enum",
    "class"
};

char const *CVScopeName( dbg_type scope )
{
    return( ScopeNames[scope] );
}

dbg_type    CVScope( const char *name )
/*************************************/
{
    enum scope_name index;

    for( index = 0; index < SCOPE_MAX; ++index ) {
        if( strcmp( name, ScopeNames[index] ) == 0 ) {
            break;
        }
    }
    return( index );
}


void    CVDumpName( dbg_name name, dbg_type tipe )
/************************************************/
{
    cv_out          out[1];
    ct_modifier     *mod;
    long_offset     here;

// we are going to loose the name so a fix is needed in the interface
    if( tipe == DBG_FWD_TYPE ) {
        NewTypeString( out );
        tipe = ++TypeIdx;
        name->refno = tipe;
        if( _IsModel( DBG_TYPES ) ) {
            mod = StartType( out, LFG_MODIFIER );
            mod->attr.s = 0;
            mod->index = tipe;
            here = EndTypeString( out );
            name->patch.segid = CVTypes;
            name->patch.offset = 2 + here + offsetof(lf_modifier, f.index ) ;
        }
    }
    name->refno = tipe;
}

void CVBackRefType( dbg_name name, dbg_type tipe )
/************************************************/
{
    long_offset     here;
    segment_id      old_segid;

    old_segid = SetOP( name->patch.segid );
    here = AskBigLocation();
    SetBigLocation( name->patch.offset );
    DataShort( tipe );
    SetBigLocation( here );
    SetOP( old_segid );
    name->refno = tipe;
}

dbg_type    CVArray( dbg_type dims, dbg_type base )
/*************************************************/
{
    cv_out          out[1];
    ct_dimarray     *array;
    dbg_type        ret;

// Need the length to make things simple
    NewTypeString( out );
    ret = ++TypeIdx;
    array = StartType( out, LFG_DIMARRAY );
    array->utype = base;
    array->diminfo = dims;
    CVPutNullStr( out );
    EndTypeString( out );
    return( ret );
}

dbg_type    CVArraySize( offset size, unsigned_32 hi, dbg_type base )
/*******************************************************************/
{
    cv_out      out[1];
    ct_array    *array;
    dbg_type    ret;

// Need the length to make things simple
    NewTypeString( out );
    ret = ++TypeIdx;
    array = StartType( out, LFG_ARRAY );
    array->elemtype = base;
    array->idxtype  = LFSignedSize( hi );
    CVPutINum( out, size );
    CVPutNullStr( out );
    EndTypeString( out );
    return( ret );
}

static  lf_values   ArrayDim( unsigned_32  hi )
/************ Make 1 dim array bound *********/
{
    cv_out          out[1];
    ct_dimconu      *dim;
    cv_primitive    index;
    lf_values       ret;

    NewTypeString( out );
    ret = ++TypeIdx;
    dim = StartType( out, LFG_DIMCONU );
    index.s = LFSignedSize( hi );
    dim->rank = 1;
    dim->index = index.s;
    PutLFInt( out, index.f.size, hi );
    EndTypeString( out );
    return( ret );
}

dbg_type    CVCharBlock( unsigned_32 len )
/****************************************/
{
    return( CVArraySize( len, len, LF_TRCHAR ) );
}

static dbg_type  OutBckSym( back_handle bck, int off, dbg_type tipe )
/*******************************************************************/
{
    cv_out          out[1];

    NewTypeString( out );
    StartType( out, LFG_REFSYM );
    CVOutBck( out, bck, off, tipe );
    return( ++TypeIdx );
}

static dbg_type  OutBckCon( int val, dbg_type tipe )
/**************************************************/
{
    cv_out          out[1];

    NewTypeString( out );
    StartType( out, LFG_REFSYM );
    CVOutSymICon( out, "_bck_con", val, tipe );
    return( ++TypeIdx );
}

dbg_type    CVIndCharBlock( back_handle len, cg_type len_type, int off )
/**********************************************************************/
{
    dbg_type        itipe;
    dbg_type        symref;
    cv_out          out[1];
    ct_dimvaru      *dim;
    cv_primitive    index;
    lf_values       ret;
    type_def        *tipe_addr;

    tipe_addr = TypeAddress( len_type );
    itipe = LFIntType( tipe_addr->length );
    symref = OutBckSym( len, off, itipe );
    NewTypeString( out );
    ret = ++TypeIdx;
    dim = StartType( out, LFG_DIMVARU );
    index.s = itipe;
    dim->rank = 1;
    dim->index = index.s;
    PutFld2( out, symref );
    EndTypeString( out );
    ret = CVArray( ret, LF_TCHAR );
    return( ret );
}

typedef struct {
    union {
      cg_sym_handle s;
      name          *n;
    } v;
    int         o;
    enum {
        EXPR_NONE     = 0x00,
        EXPR_NAME     = 0x01,
        EXPR_SYM      = 0x02,
        EXPR_VAR      = (EXPR_NAME | EXPR_SYM ),
    } state;
} fold_leaf;

enum{ FOLD_EXPR = 10 };
typedef struct {
    fold_leaf *stk;
    fold_leaf ops[FOLD_EXPR];
    bool  error;
} fold_expr;

static  void    DoLocFold( dbg_loc loc, fold_expr *what )
/*******************************************************/
{
    offset      disp;
    fold_leaf   tmp;
    fold_leaf   *stk;

    if( loc->next != NULL ) {
        DoLocFold( loc->next, what );
    }
    if( what->error ) {
        return;
    }
    stk = what->stk;
    if( stk <= what->ops ){
        what->error = true;
        return;
    }
    switch( loc->class & 0xf0 ) {
    case LOC_CONSTANT:
    case LOC_BP_OFFSET:
        --stk;
        if( (loc->class & 0xf0) == LOC_BP_OFFSET ) {
            stk[0].v.n  = loc->u.be_sym->v.symbol;
            stk[0].state = EXPR_NAME;
            stk[0].o = 0;
        } else {
            if( loc->class == LOC_MEMORY ) {
                stk[0].v.s = loc->u.fe_sym;
                stk[0].state = EXPR_SYM;
            } else {
                stk[0].v.s = 0;
                disp = loc->u.val;
                stk[0].o = disp;
                stk[0].state = EXPR_NONE;
            }
        }
        break;
    case LOC_OPER:
        switch( loc->class & 0x0f ) {
        case LOP_ADD:
            if( (stk[0].state & stk[1].state) & EXPR_VAR ) {
                what->error = true;
                return;
            }
            if( stk[1].state & EXPR_VAR  ){
                stk[0].v = stk[1].v;
                stk[0].state |= stk[1].state;
            }
            stk[0].o += stk[1].o;
            ++stk;
            break;
        case LOP_XCHG:
            tmp = stk[0];
            stk[0] = stk[1];
            stk[1] = tmp;
            break;
        default:
           what->error = true;
           break;
        }
        break;
    default:
       what->error = true;
       break;
    }
    what->stk = stk;
}

static bool FoldExpr( dbg_loc loc, fold_leaf *ret )
/*************************************************/
{
    fold_expr      expr;

    expr.stk = &expr.ops[FOLD_EXPR];
    expr.error = false;
    DoLocFold( loc, &expr );
    if( !expr.error ) {
        if( expr.stk == &expr.ops[FOLD_EXPR - 1] ) {
            *ret = expr.ops[FOLD_EXPR - 1];
            return( true );
        }
    }
    return( false );
}

dbg_type    CVLocCharBlock( dbg_loc loc, cg_type len_type )
/*********************************************************/
{
    dbg_type        itipe;
    dbg_type        symref;
    cv_out          out[1];
    ct_dimvaru      *dim;
    lf_values       ret;
    type_def        *tipe_addr;
    fold_leaf       tmp;

    tipe_addr = TypeAddress( len_type );
    itipe = LFIntType( tipe_addr->length );
    if( FoldExpr( loc, &tmp ) ) {
        if( tmp.state & EXPR_NAME ) {
            NewTypeString( out );
            symref = ++TypeIdx;
            StartType( out, LFG_REFSYM );
            CVOutLocal( out, tmp.v.n, tmp.o, itipe );
        } else {
            symref = OutBckSym( FEBack( tmp.v.s ), tmp.o, itipe );
        }
    } else {
        symref = OutBckCon( 1, itipe );
    }
    NewTypeString( out );
    ret = ++TypeIdx;
    dim = StartType( out, LFG_DIMVARU );
    dim->rank = 1;
    dim->index = itipe;
    PutFld2( out, symref );
    EndTypeString( out );
    ret = CVArray( ret, LF_TRCHAR );
    return( ret );
}

dbg_type    CVFtnArray( back_handle dims, cg_type lo_bound_tipe,
                 cg_type num_elts_tipe, int off, dbg_type base )
/**************************************************************/
{
    dbg_type        itipe;
    dbg_type        symref[2];
    cv_out          out[1];
    ct_dimvarlu     *dim;
    lf_values       ret;
    type_def        *tipe_addr;

    /* unused parameters */ (void)num_elts_tipe;

    tipe_addr = TypeAddress( lo_bound_tipe );
    itipe = LFIntType( tipe_addr->length );
    symref[0] = OutBckSym( dims, off, itipe );
    symref[1] = OutBckSym( dims, off+tipe_addr->length, itipe );
    NewTypeString( out );
    ret = ++TypeIdx;
    dim = StartType( out, LFG_DIMVARLU );
    dim->rank = 1;
    dim->index = itipe;
    PutFld2( out, symref[0] );
    PutFld2( out, symref[1] );
    EndTypeString( out );
    ret = CVArray( ret, base );
    return( ret );
}



dbg_type    CVIntArray( unsigned_32 hi, dbg_type base )
/*****************************************************/
{
    cv_out          out[1];
    ct_dimarray     *array;
    dbg_type        ret;
    lf_values       dim;

// Need the length to make things simple
    dim = ArrayDim( hi );
    NewTypeString( out );
    ret = ++TypeIdx;
    array = StartType( out, LFG_DIMARRAY );
    array->utype = base;
    array->diminfo = dim;
    CVPutNullStr( out );
    EndTypeString( out );
    return( ret );
}



static  lf_values   ArrayDimL( signed_32 low, signed_32 hi )
/************ Make 1 dim array bound **********************/
{
    cv_out          out[1];
    ct_dimconlu     *dim;
    cv_primitive    index;
    lf_values       ret;

    NewTypeString( out );
    ret = ++TypeIdx;
    dim = StartType( out, LFG_DIMCONLU );
    index.s = LFSignedRange( low, hi );
    dim->rank = 1;
    dim->index = index.s;
    PutLFInt( out, index.f.size, (offset)low );
    PutLFInt( out, index.f.size, (offset)hi );
    EndTypeString( out );
    return( ret );
}

static  dbg_type    CVDimVarLU( dbg_array ar )
/********************************************/
{
    cv_out         out[1];
    ct_dimvarlu    *var;
    dbg_type       itipe;
    dbg_type       symref[2];
    type_def       *tipe_addr;
    dim_any        *dim;
    dim_any        *next;

    NewTypeString( out );
    var = StartType( out, LFG_DIMVARLU );
    var->rank = ar->num;
    var->index = LF_TINT4;
    tipe_addr = NULL;
    itipe = 0;
    for( dim = ar->list; dim != NULL; dim = next ) {
        next = dim->entry.next;
        switch( dim->entry.kind ) {
        case DIM_VAR:
            if( tipe_addr == NULL ){
                tipe_addr = TypeAddress( dim->var.lo_bound_tipe );
                itipe = LFIntType( tipe_addr->length );
            }
            symref[0] = OutBckSym( dim->var.dims, dim->var.off, itipe );
            symref[1] = OutBckSym( dim->var.dims, dim->var.off + tipe_addr->length, itipe );
            break;
        case DIM_CON:
            symref[0] = OutBckCon( dim->con.lo, dim->con.idx );
            symref[1] = OutBckCon( dim->con.hi, dim->con.idx );
            break;
        default:
            symref[0] = 0;
            symref[1] = 0;
            Zoiks( ZOIKS_106 ); /* bad pointer */
            break;

        }
        PutFld2( out, symref[0] );
        PutFld2( out, symref[1] );
        CGFree( dim  );
    }
    ar->list = NULL;
    EndTypeString( out );
    return( ++TypeIdx );
}

static  dbg_type    CVDimConLU( dbg_array ar )
/********************************************/
{
    cv_out          out[1];
    ct_dimconlu     *con;
    dim_any         *dim;
    dim_any         *next;

    NewTypeString( out );
    con = StartType( out, LFG_DIMCONLU );
    con->rank = ar->num;
    con->index = LF_TINT4;
    for( dim = ar->list; dim != NULL; dim = next ) {
        next = dim->entry.next;
        switch( dim->entry.kind ) {
        case DIM_CON:
            PutFldSized( out, 4, dim->con.lo );
            PutFldSized( out, 4, dim->con.hi );
            break;
        default:
            Zoiks( ZOIKS_106 ); /* bad pointer */
            break;

        }
        CGFree( dim );
    }
    ar->list = NULL;
    EndTypeString( out );
    return( ++TypeIdx );
}

dbg_type    CVEndArray( dbg_array ar )
/************************************/
{
    dbg_type        dims;

    if( ar->is_variable ) {
        dims = CVDimVarLU( ar );
    } else {
        dims = CVDimConLU( ar );
    }
    return( CVArray( dims, ar->base ) );
}

dbg_type   CVSubRange( signed_32 lo, signed_32 hi, dbg_type base )
/****************************************************************/
/* not supported by CV */
{
    /* unused parameters */ (void)base;

    return( ArrayDimL( lo, hi ) );
}

static  cv_ptrtype  PtrClass( cg_type ptr_type )
/**********************************************/
{
    type_def        *tipe_addr;
    cv_ptrtype      ret = 0;

    tipe_addr = TypeAddress( ptr_type );
    switch( tipe_addr->refno ) {
    case TY_HUGE_POINTER:
        ret = CV_HUGE;
        break;
    case TY_LONG_POINTER:
    case TY_LONG_CODE_PTR:
        if( tipe_addr->length == 6 ) {
            ret = CV_FAR32;
        } else {
            ret = CV_FAR;
        }
        break;
    case TY_NEAR_POINTER:
    case TY_NEAR_CODE_PTR:
        if( tipe_addr->length == 4 ) {
            ret = CV_NEAR32;
        } else {
            ret = CV_NEAR;
        }
        break;
    }
    return( ret );
}

static  dbg_type    MkPtr( cg_type ptr_type, dbg_type base, cv_ptrmode mode )
/***************************************************************************/
{
    cv_out      out[1];
    dbg_type    ret;
    ct_pointer  *cvptr;

    NewTypeString( out );
    ret = ++TypeIdx;
    cvptr = StartType( out, LFG_POINTER );
    cvptr->attr.s = 0;
    cvptr->attr.f.mode = mode;
    cvptr->attr.f.type = PtrClass( ptr_type );
    cvptr->type = base;
    EndTypeString( out );
    return( ret );
}

dbg_type    CVDereference( cg_type ptr_type, dbg_type base )
/**********************************************************/
{
    return( MkPtr( ptr_type, base, CV_REF ) );
}

dbg_type    CVPtr( cg_type ptr_type, dbg_type base )
/**************************************************/
{
    dbg_type        ret;
    cv_primitive    index;

    index.s = base;
    if( index.s < CV_FIRST_USER_TYPE
     && index.s > LF_TSEGMENT && index.f.mode == 0 ) {
        switch( PtrClass( ptr_type ) ) {
        case CV_NEAR:
            index.f.mode =  CV_NEARP;
            break;
        case CV_FAR:
            index.f.mode =  CV_FARP;
            break;
        case CV_HUGE:
            index.f.mode =  CV_HUGEP;
            break;
        case CV_NEAR32:
            index.f.mode =  CV_NEAR32P;
            break;
        case CV_FAR32:
            index.f.mode =  CV_FAR32P;
            break;
        default:
            Zoiks( ZOIKS_106 ); /* bad pointer */
        }
        ret = index.s;
    } else {
        ret = MkPtr( ptr_type, base, CV_PTR );
    }
    return( ret );
}



static  dbg_type    CVBasedPtrK( cg_type ptr_type, dbg_type base,
                            cg_sym_handle sym, cv_based_kind kind )
/*****************************************************************/
{
    cv_out          out[1];
    dbg_type        ret;
    ct_pointer      *cvptr;
//    cv_ptrtype      ptype;
    void            *ptr1;
    void            *ptr2;

    /* unused parameters */ (void)ptr_type;

    //TODO: Need to do somthing about segments
    //TODO: Need to do somthing about BasePtr
    NewTypeString( out );
    ret = ++TypeIdx;
//    ptype = PtrClass( ptr_type );
    cvptr = StartType( out, LFG_POINTER );
    cvptr->attr.s = 0;
    cvptr->attr.f.mode = CV_PTR;
    cvptr->type = base;
    switch( kind ){
    case BASED_SELF:
        cvptr->attr.f.type = CV_BASESELF;
        CVEndType( out );
        break;
    case BASED_VOID:
        cvptr->attr.f.type = CV_BASETYPE;
        PutLFInt( out, CV_IB2, LF_TVOID );
        CVPutNullStr( out );
        CVEndType( out );
        break;
    case BASED_SEG:
        cvptr->attr.f.type = CV_BASESEG;
        ptr1 = out->ptr;
        PutLFInt( out, CV_IB2, 0 );
        ptr2 = out->ptr;
        CVEndType( out );
        BuffWrite( out, ptr1);
        SegReloc( CVTypes, sym );
        BuffSkip( out, ptr2 );
        break;
    case BASED_VALUE:
        cvptr->attr.f.type = CV_BASEVAL;
        ptr1 = out->beg;
        out->beg = out->ptr;
        CVOutSym( out, sym );
        out->beg = ptr1;
        CVEndType( out );
        break;
    }
    buffEnd( out );
    return( ret );
}

typedef struct {
    cg_sym_handle s;
    enum {
        SYM_NOT,
        SYM_SYM,
        SYM_IND,
        SYM_ZERO,
    } kind;
    bool indirect;
} based_leaf;

#define MAX_OP 2
typedef struct {
    based_leaf      *stk;
    based_leaf      ops[MAX_OP];
    cg_sym_handle   sym;
    int             count;
    enum {
        IS_NONE,
        IS_SEG,
        IS_VALUE,
        IS_VOID,
        IS_SELF,
        IS_ERROR,
    } state;
} based_expr;

static  void    DoLocBase( dbg_loc loc, based_expr *what )
/********************************************************/
{
    offset      disp;
    based_leaf  tmp;
    based_leaf  *stk;

    if( loc->next != NULL ) {
        DoLocBase( loc->next, what );
    }
    if( what->state == IS_ERROR ) {
        return;
    }
    stk = what->stk;
    switch( loc->class & 0xf0 ) {
    case LOC_CONSTANT:
    case LOC_BP_OFFSET:
        if( what->count < MAX_OP ) {
            ++what->count;
            --stk;
        } else {
            what->state = IS_ERROR;
            return;
        }
        if( (loc->class & 0xf0) == LOC_BP_OFFSET ) {
            stk[0].s  = loc->u.be_sym->v.symbol;
            stk[0].kind = SYM_SYM;
        } else {
            if( loc->class == LOC_MEMORY ) {
                stk[0].s = loc->u.fe_sym;
                stk[0].kind = SYM_SYM;
            } else {
                disp = loc->u.val;
                if( disp == 0 ){
                    stk[0].kind = SYM_ZERO;
                } else {
                    what->state = IS_ERROR;
                }
            }
        }
        what->stk = stk;
        break;
    case LOC_OPER:
        if( what->count == 0 ) {
            what->state = IS_ERROR;
        }
        switch( loc->class & 0x0f ) {
        case LOP_IND_2:
        case LOP_IND_4:
        case LOP_IND_ADDR_16:
        case LOP_IND_ADDR_32:
            if( stk[0].kind == SYM_SYM ) {
                stk[0].kind = SYM_IND;
                what->state = IS_VALUE;
                what->sym = stk[0].s;
            } else {
                 what->state = IS_ERROR;
            }
            break;
        case LOP_MK_FP:
            switch( stk[0].kind ) {
            case SYM_ZERO:
                what->state = IS_SELF;
                if( what->count == 2 ) {
                    switch( stk[1].kind ) {
                    case SYM_ZERO:
                        what->state = IS_VOID;
                        break;
                    case SYM_SYM:
                        what->state = IS_SEG;
                        what->sym= stk[1].s;
                        break;
                    case SYM_IND:
                        what->state = IS_VALUE;
                        what->sym= stk[1].s;
                        break;
                    default:
                       what->state = IS_ERROR;
                    }
                } else if( what->count != 1 ) {
                   what->state = IS_ERROR;
                }
               break;
            case SYM_IND:
                what->sym= stk[0].s;
                if( what->count == 2 ){
                    if( stk[1].kind == SYM_SYM ){
                        what->state = IS_VALUE;
                    } else {
                        what->state = IS_ERROR;
                    }
                }else if( what->count != 1 ){
                   what->state = IS_ERROR;
                }
               break;
            default:
               what->state = IS_ERROR;
               break;
            }
            break;
        case LOP_XCHG:
            if( what->count >= 2 ){
                tmp = stk[0];
                stk[0] = stk[1];
                stk[1] = tmp;
            }
            break;
        }
        break;
    default:
       what->state = IS_ERROR;
       break;
    }
}

dbg_type    CVBasedPtr( cg_type ptr_type, dbg_type base, dbg_loc loc_segment )
/****************************************************************************/
{
    based_expr     expr;
    uint           based_kind;
    dbg_type       ret;

    expr.stk = &expr.ops[MAX_OP];
    expr.count = 0;
    expr.state = IS_NONE;
    expr.sym = NULL;;
    DoLocBase( loc_segment, &expr );
    based_kind = 0;
    switch( expr.state ){
    case IS_ERROR:
        expr.sym = NULL;
        /* fall through */
    case IS_NONE:
    case IS_VOID:
        based_kind = BASED_VOID;
        break;
    case IS_SELF:
        based_kind = BASED_SELF;
        break;
    case IS_SEG:
        based_kind = BASED_SEG;
        break;
    case IS_VALUE:
        based_kind = BASED_VALUE;
        break;
    }
    ret = CVBasedPtrK( ptr_type, base, expr.sym, based_kind );
    return( ret );
}

static void MkBits( field_any *field )
/************************************/
{
    lf_values       biti;
    ct_bitfield     *bit;
    cv_out          out[1];

    for( ; field != NULL; field= field->entry.next ) {
        if( field->entry.field_type == FIELD_OFFSET ) {
            if( field->member.b_len != 0 ) {
                NewTypeString( out );
                biti = ++TypeIdx;
                bit = StartType( out, LFG_BITFIELD );
                bit->length = field->member.b_len;
                bit->position = field->member.b_strt;
                bit->type = field->member.base;
                EndTypeString( out );
                field->member.base = biti;
            }
        }
    }
}

static cv_access WVCVAccess( uint attr )
{
    cv_access ret = 0;

    switch( attr ){
    case FIELD_PUBLIC:
        ret = CV_PUBLIC;
        break;
    case FIELD_PROTECTED:
        ret = CV_PROTECTED;
        break;
    case FIELD_PRIVATE:
        ret = CV_PRIVATE;
        break;
    }
    return( ret );
}

static cv_mprop WVCVMProp( uint kind )
{
    cv_mprop ret = 0;

    switch( kind ){
    case METHOD_VANILLA:
        ret = CV_VANILLA;
        break;
    case METHOD_STATIC:
        ret = CV_STATIC;
        break;
    case METHOD_VIRTUAL:
        ret = CV_INTROVIRT;
        break;
    case METHOD_FRIEND:
        ret = CV_FRIEND;
        break;
    }
    return( ret );
}

static   cv_vtshape   CVVTShape( cg_type ptr_type )
/*************************************************/
{
    type_def        *tipe_addr;
    cv_vtshape      ret = 0;

    tipe_addr = TypeAddress( ptr_type );
    switch( tipe_addr->refno ) {
    case TY_HUGE_POINTER:
        ret = CV_VTFAR;
        break;
    case TY_LONG_POINTER:
    case TY_LONG_CODE_PTR:
        if( tipe_addr->length == 6 ){
            ret = CV_VTFAR32;
        } else {
            ret = CV_VTFAR;
        }
        break;
    case TY_NEAR_POINTER:
    case TY_NEAR_CODE_PTR:
        if( tipe_addr->length == 4 ){
            ret = CV_VTNEAR32;
        } else {
            ret = CV_VTNEAR;
        }
        break;
    }
    return( ret );
}

static lf_values MkVTShape( cv_out *out, field_vfunc *vf )
/********************************************************/
{
    lf_values       vshape;
    ct_vtshape      *vt;
    unsigned char   val;
    int             count;
    cv_vtshape      shape;

    NewTypeString( out );
    vt = StartType( out, LFG_VTSHAPE );
    vt->count = vf->vft_size;
    shape = CVVTShape( vf->vft_cgtype );
    val = 0;
    for( count = 0; count < vf->vft_size; ++count ) {
        if( (count & 1) == 0 ){
            val = shape << 4;
        } else {
            val |= shape;
            PutLFInt( out, CV_IB1, val );
        }
    }
    if( (count & 1) ) {
        PutLFInt( out, CV_IB1, val );
    }
    EndTypeString( out );
    vshape = ++TypeIdx;
    vf->base = CVPtr( vf->vft_cgtype, vshape );
    return( vshape );
}

static int  MkFlist( dbg_struct st )
/**********************************/
{
    field_any         *field;
    field_any         *old;
    cv_out            out[1];
    ct_subfield_ptrs  fld;
    cv_mlist          *a_mlist;
    long_offset       fstart;
    offset            disp;
    u2                len;
    int               count;
//    int               mlist;

    NewTypeString( out );
    StartType( out, LFG_FIELDLIST);
    fstart = EndTypeString( out );
    len = 2; /* size of code */
    count = 0;
//    mlist = TypeIdx;
    NewType( out ); /* reset buff for subfields */
    old = st->list;
    for( field = old; field != NULL; field = field->entry.next ) {
        switch( field->entry.field_type ) {
        case FIELD_INHERIT:
            if( field->bclass.u.adjustor->class == LOC_CONST_1 ) {
                disp = field->bclass.u.adjustor->u.val;
                switch( field->bclass.kind ) {
                case INHERIT_DBASE:
                    fld.a_bclass = StartType( out, LFG_BCLASS );
                    fld.a_bclass->type = field->bclass.base;
                    fld.a_bclass->attr.s = 0; /* zero bits */
                    fld.a_bclass->attr.f.access = CV_PUBLIC;
                    CVPutINum( out, disp );
                    break;
                case INHERIT_VBASE:
                case INHERIT_IVBASE:
                    if( field->bclass.kind == INHERIT_VBASE ){
                        fld.a_vbclass = StartType( out, LFG_VBCLASS );
                    } else {
                        fld.a_vbclass = StartType( out, LFG_IVBCLASS );
                    }
                    fld.a_vbclass->btype = field->bclass.base;
                    fld.a_vbclass->vtype = st->vtbl_type;
                    fld.a_vbclass->attr.s = 0; /* zero bits */
                    fld.a_vbclass->attr.f.access = CV_PUBLIC;
                    CVPutINum( out, st->vtbl_off );
                    CVPutINum( out, disp );
                }
                len += EndSub( out );  /* write out subfield accum length */
            }
            ++count;
            DBLocFini( field->bclass.u.adjustor );
            break;
        case FIELD_METHOD:{
            field_any        *curr;
            int               count1;

            count1 = 1;
            for( curr = field->entry.next; curr != NULL; curr = curr->entry.next ) {
                if( curr->entry.field_type != FIELD_METHOD )
                    break;
                if( strcmp( curr->method.name, field->method.name ) != 0 )
                    break;
                ++count1;
                field = curr;
            }
            fld.a_method = StartType( out, LFG_METHOD );
            fld.a_method->count = count1;
            fld.a_method->mList = ++TypeIdx;
            CVPutStr( out, field->method.name );
            len += EndSub( out );  /* write out subfield accum length */
            break ;
        }
        case FIELD_NESTED:
            fld.a_nestedtype = StartType( out, LFG_NESTEDTYPE );
            fld.a_nestedtype->index = field->nested.base;
            CVPutStr( out, field->nested.name );
            len += EndSub( out );  /* write out subfield accum length */
            break;
        case FIELD_VFUNC:
            if( field->vfunc.vfptr_off == 0 ){
                fld.a_vfunctab = StartType( out, LFG_VFUNCTAB );
                fld.a_vfunctab->type = field->vfunc.base;
            }
#if 0   // till cvpack is fixed
            else{
                fld.a_vfuncoff = StartType( out, LFG_VFUNCOFF );
                fld.a_vfuncoff->type = field->vfunc.base;
                fld.a_vfuncoff->offset = field->vfunc.vfptr_off;
            }
#endif
            len += EndSub( out );  /* write out subfield accum length */
            break;
        case FIELD_LOC:
            disp = LocSimpField( field->member.u.loc );
            if( disp != NO_OFFSET ){
                ++count;
                fld.a_member = StartType( out, LFG_MEMBER );
                fld.a_member->type = field->member.base;
                fld.a_member->attr.s = 0; /* zero bits */
                fld.a_member->attr.f.access = WVCVAccess( field->member.attr );
                CVPutINum( out, disp );
                CVPutStr( out, field->member.name );
                len += EndSub( out );  /* write out subfield accum length */
            }
            DBLocFini( field->member.u.loc );
            break;
        case FIELD_STFIELD:
            fld.a_stmember = StartType( out, LFG_STMEMBER );
            fld.a_stmember->type = field->stfield.base;
            fld.a_stmember->attr.s = 0; /* zero bits */
            fld.a_stmember->attr.f.access = WVCVAccess( field->stfield.attr );
            DBLocFini( field->stfield.loc );
            CVPutStr( out, field->stfield.name );
            len += EndSub( out );  /* write out subfield accum length */
            break;
        case FIELD_OFFSET:
            ++count;
            fld.a_member = StartType( out, LFG_MEMBER );
            fld.a_member->type = field->member.base;
            fld.a_member->attr.s = 0; /* zero bits */
            fld.a_member->attr.f.access = WVCVAccess( field->member.attr );
            CVPutINum( out, field->member.u.off );
            CVPutStr( out, field->member.name );
            len += EndSub( out );  /* write out subfield accum length */
            break;
        }
    }
    PatchLen( fstart, len ); /* fill in length */
    for( field = old; field != NULL; field = field->entry.next ) {
        if( field->entry.field_type == FIELD_METHOD ) {
            field_any       *curr;

            NewTypeString( out );
            a_mlist = StartType( out, LFG_METHODLIST );
            for( ;; ) {
                a_mlist->attr.s = 0; /* zero bits */
                a_mlist->attr.f.access = WVCVAccess( field->method.attr );
                a_mlist->attr.f.mprop = WVCVMProp( field->method.kind );
                a_mlist->type = field->method.base;
                if( field->method.kind == METHOD_VIRTUAL ) {
                    if( field->method.u.loc->class == LOC_CONST_1 ) {
                        disp = field->method.u.loc->u.val;
                    } else {
                        disp = NO_OFFSET;
                    }
                    PutLFInt( out, CV_IB4, disp );
                }
                DBLocFini( field->method.u.loc );
                curr = field->entry.next;
                if( curr == NULL )
                    break;
                if( curr->entry.field_type != FIELD_METHOD )
                    break;
                if( strcmp( curr->method.name, field->method.name ) != 0 )
                    break;
                field = curr;
                curr = curr->entry.next;
                a_mlist = BuffInc( out, sizeof( *a_mlist ) );
            }
            EndTypeString( out );
         }
    }
    for( ; old != NULL; old = field ) {
        field = old->entry.next;
        CGFree( old );
    }
    return( count );
}

static  field_any  *UnLinkMethod( field_any **owner, const char *name )
/*** UnLink method with name  ****************************************/
{
    field_any    *curr;

    for( ; (curr = *owner) != NULL; owner = &(*owner)->entry.next ) {
        if( curr->entry.field_type == FIELD_METHOD ){
            if( strcmp( curr->method.name, name ) == 0 ){
                *owner = curr->entry.next;
                break;
            }
        }
    }
    return( curr );
}

static  int   SortMethods( dbg_struct st )
/****************************************/
{
    field_any  *curr;
    field_any  *next;
    field_any  *add;
    int        overops;

    overops = 0;
    for( curr = st->list; curr != NULL; curr = curr->entry.next ) {
        if( curr->entry.field_type == FIELD_METHOD ) {
            add = UnLinkMethod( &curr->entry.next, curr->method.name );
            if( add != NULL ){
                overops = 1;
                next = curr->entry.next;
                curr->entry.next = add;
                add->entry.next = next;
            }
        }
    }
    return( overops );
}

static int  FlistCount( field_any *field )
/****************************************/
{
    int               count;
    offset            disp;

    count = 0;
    for( ; field != NULL; field = field->entry.next ) {
        switch( field->entry.field_type ) {
        case FIELD_INHERIT:
        case FIELD_METHOD:
        case FIELD_OFFSET:
        case FIELD_NESTED:
        case FIELD_STFIELD:
            ++count;
            break;
        case FIELD_LOC:
            disp = LocSimpField( field->member.u.loc );
            if( disp != NO_OFFSET ){
                ++count;
            }
            break;
        case FIELD_VFUNC:
            if( field->vfunc.vfptr_off == 0 ){
                ++count;
            }
            break;
        }
    }
    return( count );
}

dbg_type    CVEndStruct( dbg_struct st )
/**************************************/
{
    lf_values        flisti;
    lf_values        hd;
    lf_values        vshape;
    union {
        ct_structure s;
        ct_union     u;
    }          *head;
    int             count;
    cv_out          out[1];

    if( st->vtbl_type != DBG_NIL_TYPE ){
        lf_values   vtbl_type;

        vtbl_type  = CVArraySize( 0, 0, st->vtbl_type ); /* fake p@[0] vtbl */
        vtbl_type = MkPtr( st->ptr_type, vtbl_type, CV_PTR );
        st->vtbl_type = vtbl_type;
    }
    MkBits(  st->list );
    if( st->vf != NULL ) {
        vshape = MkVTShape( out, st->vf );
    } else {
        vshape = 0;
    }
    NewTypeString( out );
    hd = ++TypeIdx;
    flisti = ++TypeIdx;
    count = FlistCount( st->list );
    if( st->is_struct ) {
        head = StartType( out, LFG_STRUCTURE );
//      head->s.count  = st->num;
        head->s.count  = count;
        head->s.field = flisti;
        head->s.property.s = 0;
        head->s.property.f.cnested = st->is_cnested;
        head->s.property.f.isnested = st->is_nested;
        head->s.property.f.overops = SortMethods( st );
        head->s.dList = 0;
        head->s.vshape = vshape;
    } else {
        head = StartType( out, LFG_UNION );
        head->u.count  = st->num;
        head->u.field = flisti;
        head->u.property.s = 0;
        head->s.property.f.cnested = st->is_cnested;
        head->s.property.f.isnested = st->is_nested;
    }
    CVPutINum( out, st->size );
    CVPutStr( out, st->name );
    EndTypeString( out );
    count = MkFlist( st );
    return( hd );
}

static  const_entry  *RevEnums( const_entry *cons )
/*************************************************/
{
    const_entry *head;
    const_entry *next;

    head = NULL;
    for( ; cons != NULL; cons = next ) {
        next = cons->next;
        cons->next = head;
        head = cons;
    }
    return( head );
}

dbg_type    CVEndEnum( dbg_enum en )
/**********************************/
{
    const_entry     *cons;
    const_entry     *next;
    lf_values       fList;
    ct_enumerate    *mem;
    ct_enum         *head;
    lf_values       headi;
    int             count;
    long_offset     fstart;
    u2              len;
    cv_out          out[1];

    NewTypeString( out );
    fList = ++TypeIdx;
    StartType( out, LFG_FIELDLIST );
    fstart = EndTypeString( out );
    len = 2;
    en->list  = RevEnums( en->list );
    count = 0;
    NewType( out ); /* reset buff for subfields */
    for( cons = en->list; cons != NULL; cons = cons->next ) {
        mem = StartType( out, LFG_ENUMERATE );
        mem->attr.s = 0;
        CVPutINum64( out, cons->val );
        CVPutStr( out, cons->name );
        len += EndSub( out );  /* write out subfield accum length */
        ++count;
    }
    PatchLen( fstart, len ); /* fill in length */
    NewTypeString( out );
    headi = ++TypeIdx;
    head = StartType( out, LFG_ENUM );
    head->count = count;
    head->type = CVScalar( "", en->tipe );
    head->fList  = fList;
    head->property.s = 0;
    CVPutNullStr( out );
    EndTypeString( out );
    if( en->is_nested ){
        for( cons = en->list; cons != NULL; cons = cons->next ) {
            CVSymIConst64( cons->name, cons->val, headi );
        }
    }
    for( cons = en->list; cons != NULL; cons = next ) {
        next = cons->next;
        CGFree( cons );
    }
    return( headi );
}


dbg_type    CVEndProc( dbg_proc pr )
/**********************************/
{
    parm_entry  *parm;
    parm_entry  *next;
    ct_arglist  *args;
    lf_values   arglist;
    cv_calls    call;
    union{
        ct_procedure *a_procedure;
        ct_mfunction *a_mfunction;
    }f;
    lf_values   proci;
    cv_out      out[1];

    NewTypeString( out );
    arglist = ++TypeIdx;
    args = StartType( out, LFG_ARGLIST );
    args->argcount = pr->num;
    for( parm = pr->list; parm != NULL; parm = next ) {
        next = parm->next;
        PutFld2( out, parm->tipe );
        CGFree( parm );
    }
    EndTypeString( out );
#if _TARGET & ( _TARG_8086 | _TARG_80386 )
    if( pr->call == TY_NEAR_CODE_PTR ) {
        call = CV_NEARC;
    } else {
        call = CV_FARC;
    }
#elif  _TARGET & _TARG_AXP
    call = CV_AXP;
#else
    call = CV_GENERIC;
#endif
    if( pr->cls != DBG_NIL_TYPE ){
        if( pr->this == DBG_NIL_TYPE ){
            pr->this = LF_TVOID;
        }
        NewTypeString( out );
        proci = ++TypeIdx;
        f.a_mfunction = StartType( out, LFG_MFUNCTION );
        f.a_mfunction->rvtype = pr->ret;
        f.a_mfunction->class_idx = pr->cls;
        f.a_mfunction->thisptr = pr->this;
        f.a_mfunction->call = call;
        f.a_mfunction->res = 0;
        f.a_mfunction->parms = pr->num;
        f.a_mfunction->arglist = arglist;
        f.a_mfunction->thisadjust = 0; /* always zero for watcom */
    } else {
        NewTypeString( out );
        proci = ++TypeIdx;
        f.a_procedure = StartType( out, LFG_PROCEDURE );
        f.a_procedure->rvtype = pr->ret;
        f.a_procedure->call = call;
        f.a_procedure->res = 0;
        f.a_procedure->parms = pr->num;
        f.a_procedure->arglist = arglist;
    }
    EndTypeString( out );
    return( proci );
}
