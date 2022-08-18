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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "plusplus.h"

#include <float.h>

#include "cgfront.h"
#include "cgback.h"
#include "memmgr.h"
#include "codegen.h"
#include "cgbackut.h"
#include "ring.h"
#include "pstk.h"
#include "initdefs.h"
#ifndef NDEBUG
    #include "dbg.h"
    #include "togglesd.h"
    #include "pragdefn.h"
#endif


static PSTK_CTL stack_cond_blks;    // stack: conditional DTOR blocks
static carve_t carveInfo;           // conditional dtor block
static SYMBOL dtor_cond_sym;        // conditional flags, direct DTORing

typedef struct {                // INFO FOR A CONDITION
    unsigned offset;            // - offset of conditional bit
    patch_handle patch_set;     // - patch handle: set
    patch_handle patch_clr;     // - patch handle: clr
    uint_8 mask_set;            // - mask used for setting
    uint_8 mask_clr;            // - mask used for clearing
    unsigned :0;                // - alignment
    SE* posn_last;              // - last significant position
    SE* posn_true;              // - position when flag set
    SE* posn_false;             // - position when flag clr
} COND_STK;

#ifndef NDEBUG
    static void _Dump( COND_STK* cond, const char* msg )
    {
        if( TOGGLEDBG( dump_stab ) ) {
            printf( "COND_STK[%p]: flag(%d) %s\n"
                    "  last(%p) true(%p) false(%p)\n"
                    "  patch_set(%p) patch_clr(%p) mask_set(%x) mask_clr(%x)\n"
                  , cond
                  , cond->offset
                  , msg
                  , cond->posn_last
                  , cond->posn_true
                  , cond->posn_false
                  , cond->patch_set
                  , cond->patch_clr
                  , cond->mask_set
                  , cond->mask_clr );
        }
    }
#else
    #define _Dump( a, b )
#endif


static SE* callBackCurrent(     // SET UP CURRENT POSITION
    COND_STK* info )            // - top entry
{
    SE* posn;                   // - current position

    posn = FstabCurrPosn();
    info->posn_last = posn;
    return( posn );
}


static void callBackTrue(       // CALL-BACK: start of true block
    void* data )                // - COND_STK entry
{
    COND_STK* info = data;      // - COND_STK entry
    SE* posn;                   // - current position

    posn = callBackCurrent( info );
    info->posn_true = posn;
    info->posn_false = posn;
    _Dump( info, "CallBack(true)" );
}


static void callBackFalse(      // CALL-BACK: start of false block
    void* data )                // - COND_STK entry
{
    COND_STK* info = data;      // - COND_STK entry
    SE* posn;                   // - last position

    posn = info->posn_last;
    info->posn_false = callBackCurrent( info );
    FstabSetSvSe( posn );
    _Dump( info, "CallBack(false)" );
}


static void patchMask(          // PATCH A MASK
    patch_handle patch,         // - NULL or handle
    uint_8 mask )               // - mask
{
    if( NULL != patch ) {
        BEPatchInteger( patch, mask );
        BEFiniPatch( patch );
    }
}

static void callBackFini(       // COMPLETE CALL-BACK
    COND_STK* cond )            // - entry to be completed
{
    patchMask( cond->patch_set, cond->mask_set );
    patchMask( cond->patch_clr, cond->mask_clr );
    CarveFree( carveInfo, cond );
}


static void callBackEnd(        // CALL-BACK: end of condition block
    void* data )                // - COND_STK entry
{
    COND_STK* cond = data;      // - COND_STK entry
    SE* posn;                   // - current position

    _Dump( cond, "CallBack(END)" );
    posn = FstabCurrPosn();
#if 0
    if( posn == cond->posn_true
     && posn == cond->posn_false ) {
        cond->mask_set = 0;
        cond->mask_clr = NOT_BITARR_MASK( 0 );
        BlkPosnTempBegSet( posn );
    } else {
#else
    {
#endif
        SE* test = FstabTestFlag( cond->offset
                                , cond->posn_last
                                , posn );
        FstabAdd( test );
        BlkPosnTempBegSet( test );
    }
    callBackFini( cond );
}


static void callBackNewCtorBeg( // CALL-BACK: start of new ctor
    void* data )                // - COND_STK entry
{
    COND_STK* cond = data;      // - COND_STK entry
    SE* posn;                   // - current position
    SE* se;                     // - new test_flag entry

    posn = callBackCurrent( cond );
    DbgVerify( NULL != posn, "callBackNewCtorBeg -- no delete SE" );
    se = FstabTestFlag( cond->offset, posn, FstabPrevious( posn ) );
    cond->posn_true = se;
    FstabAdd( se );
    BlkPosnTempBegSet( se );
}


static void callBackNewCtorEnd( // CALL-BACK: end of new ctor
    void* data )                // - COND_STK entry
{
    COND_STK* cond = data;      // - COND_STK entry

    _Dump( cond, "CallBack(END-NEW_CTOR)" );
    if( cond->posn_true == FstabActualPosn() ) {
        FstabRemove();
        cond->mask_set = 0;
        cond->mask_clr = NOT_BITARR_MASK( 0 );
    }
    callBackFini( cond );
};


void CondInfoPush(              // PUSH COND_INFO STACK
    FN_CTL* fctl )              // - function control
{
    COND_STK* stk = CarveAlloc( carveInfo );
    stk->offset = FnCtlCondFlagNext( fctl );
    stk->patch_set = NULL;
    stk->patch_clr = NULL;
    stk->mask_set = 0;
    stk->mask_clr = NOT_BITARR_MASK( 0 );
    stk->posn_last = 0;
    stk->posn_true = 0;
    stk->posn_false = 0;
    PstkPush( &stack_cond_blks, stk );
    _Dump( stk, "PUSH" );
}


void CondInfoPop(               // POP COND_INFO STACK
    void )
{
#ifndef NDEBUG
    COND_STK* stk = PstkPopElement( &stack_cond_blks );
    _Dump( stk, "POP" );
#else
    PstkPopElement( &stack_cond_blks );
#endif
}


void CondInfoSetup(             // SETUP UP CONDITIONAL INFORMATION
    unsigned index,             // - index of flag
    COND_INFO* cond,            // - conditional information
    FN_CTL* fctl )              // - function information
{
    target_offset_t flag_offset;    // - offset within flags vector

    /* unused parameters */ (void)fctl;

    flag_offset = BITARR_OFFS( index );
    cond->mask = BITARR_MASK( index );
    cond->sym = FstabRw();
    if( cond->sym == NULL ) {
        cond->sym = dtor_cond_sym;
        cond->offset = flag_offset;
    } else {
        cond->offset = flag_offset + CgbkInfo.size_rw_base;
    }
}


static cg_name condSet(         // SET/RESET FLAG
    unsigned index,             // - index of flag
    bool set_flag,              // - true ==> set the flag; false ==> clear
    FN_CTL* fctl )              // - function information
{
    cg_name op_flg;             // - expression for flag setting
    cg_name op_mask;            // - mask operand
    COND_INFO cond;             // - conditional information

    CondInfoSetup( index, &cond, fctl );
    op_flg = CgSymbolPlusOffset( cond.sym, cond.offset );
    if( set_flag ) {
        op_mask = CGInteger( cond.mask, TY_UINT_1 );
        op_flg = CGLVPreGets( O_OR, op_flg, op_mask, TY_UINT_1 );
    } else {
        op_mask = CGInteger( NOT_BITARR_MASK( cond.mask ), TY_UINT_1 );
        op_flg = CGLVPreGets( O_AND, op_flg, op_mask, TY_UINT_1 );
    }
    return( op_flg );
}


void CondInfoSetFlag(           // SET FLAG FOR CONDITIONAL DTOR BLOCK
    FN_CTL* fctl,               // - function control
    bool set_flag )             // - true ==> set the flag; false ==> clear
{
    COND_STK* stk;              // - conditional entry
    cg_name op_flg;             // - expression for flag setting
    cg_name op_mask;            // - mask operand
    COND_INFO cond;             // - conditional information
    patch_handle patch;         // - handle for patch
    cg_op opcode;               // - opcode for set/clr

    stk = PstkTopElement( &stack_cond_blks );
    CondInfoSetup( stk->offset, &cond, fctl );
    patch = BEPatch();
    op_mask = CGPatchNode( patch, TY_UINT_1 );
    if( set_flag ) {
        stk->mask_set = cond.mask;
        stk->patch_set = patch;
        opcode = O_OR;
    } else {
        stk->mask_clr = NOT_BITARR_MASK( cond.mask );
        stk->patch_clr = patch;
        opcode = O_AND;
    }
    op_flg = CgSymbolPlusOffset( cond.sym, cond.offset );
    op_flg = CGLVPreGets( opcode, op_flg, op_mask, TY_UINT_1 );
    CgExprPush( op_flg, TY_POINTER );
}


void CondInfoSetCtorTest(       // SET/RESET FLAG FOR CTOR-TEST
    FN_CTL* fctl,               // - function control
    bool set_flag )             // - true ==> set the flag; false ==> clear
{
    CGDone( condSet( FnCtlCondFlagCtor( fctl ), set_flag, fctl ) );
}


void CondInfoDirectFlags(       // SET FOR DIRECT-FLAGS PROCESSING
    unsigned flag_bytes )       // - # bytes of flags required
{
    if( flag_bytes > 0 ) {
        dtor_cond_sym = CgVarTemp( flag_bytes );
    } else {
        dtor_cond_sym = NULL;
    }
}


static void condInfoCallBack(   // SET A CALL-BACK
    void (*rtn)( void* ),       // - call-back routine
    bool on_left )              // - true ==> call-back on left
{
    cg_name expr;               // - top expression
    cg_type type;               // - top type
    COND_STK* stk;              // - stack ptr

    stk = PstkTopElement( &stack_cond_blks );
    expr = CgExprPopType( &type );
    if( on_left ) {
        expr = CgCallBackLeft( expr, rtn, stk, type );
    } else {
        expr = CgCallBackRight( expr, rtn, stk, type );
    }
    CgExprPush( expr, type );
}


void CondInfoTrue(              // SET UP CALL-BACK FOR IC_COND_TRUE
    void )
{
    condInfoCallBack( &callBackTrue, true );
}


void CondInfoFalse(             // SET UP CALL-BACK FOR IC_COND_FALSE
    void )
{
    condInfoCallBack( &callBackFalse, true );
}


void CondInfoEnd(               // SET UP CALL-BACK FOR IC_COND_END
    void )
{
    condInfoCallBack( &callBackEnd, false );
}


void CondInfoNewCtorBeg(        // CTOR OF NEW'ED OBJECT: START
    FN_CTL* fctl )              // - function information
{
    CondInfoPush( fctl );
    CondInfoSetFlag( fctl, true );
    condInfoCallBack( &callBackNewCtorBeg, true );
}


void CondInfoNewCtorEnd(        // CTOR OF NEW'ED OBJECT: END
    FN_CTL* fctl )              // - function information
{
    CondInfoSetFlag( fctl, false );
    condInfoCallBack( &callBackNewCtorEnd, false );
    CondInfoPop();
}


const char *CallbackName( void *f )
{
#ifndef NDEBUG
    cg_callback rtn;

    rtn = *(cg_callback *)f;
    if( rtn == callBackTrue )
        return( "callBackTrue" );
    if( rtn == callBackFalse )
        return( "callBackFalse" );
    if( rtn == callBackEnd )
        return( "callBackEnd" );
    if( rtn == callBackNewCtorBeg )
        return( "callBackNewCtorBeg" );
    if( rtn == callBackNewCtorEnd )
        return( "callBackNewCtorEnd" );
#else
    /* unused parameters */ (void)f;
#endif
    return( NULL );
}


// MODULE INITIALIZATION


static void init(               // CGBKCOND INITIALIZATION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    PstkOpen( &stack_cond_blks );
    carveInfo = CarveCreate( sizeof( COND_STK ), 32 );
}


static void fini(               // CGBKCOND COMPLETION
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    PstkClose( &stack_cond_blks );
    CarveDestroy( carveInfo );
}

INITDEFN( conditional_blocks, init, fini );
