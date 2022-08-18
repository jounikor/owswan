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
* Description:  Build a subroutine or a subroutine call.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "cgauxinf.h"
#include "zoiks.h"
#include "namelist.h"
#include "makeins.h"
#include "convins.h"
#include "data.h"
#include "bldins.h"
#include "makeaddr.h"
#include "bgcall.h"
#include "fpu.h"
#include "dbsyms.h"
#include "blips.h"
#include "redefby.h"
#include "intrface.h"
#include "makeblk.h"
#if _TARGET & ( _TARG_80386 | _TARG_8086 )
#include "x86objd.h"
#include "x86obj.h"
#endif
#include "rgtbl.h"
#include "insutil.h"
#include "namelist.h"
#include "typemap.h"
#include "bldcall.h"
#include "parmreg.h"
#include "generate.h"
#include "parm.h"
#include "trecurse.h"
#include "bldcall.h"
#include "tree.h"
#include "treeprot.h"
#include "feprotos.h"


type_class_def  AddCallBlock( cg_sym_handle sym, type_def *tipe )
/*********************************************************************
    create the initial basic block for routine "sym", and call some
    other initialization routines.
*/
{
    type_class_def      type_class;

    if( BlipsOn ) {
        PGBlip( FEName( sym ) );
    }
    NewProc( FELexLevel( sym ) );
    EnLink( AskForSymLabel( sym, CG_FE ), false );
    CurrProc->label = CurrBlock->label;
    _MarkBlkAttr( CurrBlock, BLK_BIG_LABEL );
    type_class = InitCallState( tipe );
    AddIns( MakeNop() );
    return( type_class );
}


void    BGFiniCall( cn call )
/************************************
    free up a call node
*/
{
    BGDone( call->name );
    CGFree( call->state->parm.table );
    CGFree( call->state );
    CGFree( call );
}


cn      BGInitCall(an node,type_def *tipe,aux_handle aux) {
/******************************************************************
    initialize a call node for a call to routine "node", with type
    "tipe" and aux handle "aux". This also allocates the instruction
    to hold the call and parially initializes it.
*/

    cn                  call;
    type_class_def      type_class;
    name                *mem;
#if _TARGET & ( _TARG_80386 | _TARG_8086 )
    void                *cookie;
#endif

    if( tipe->refno == TY_DEFAULT ) {
        tipe = TypeInteger;
    }
    call = CGAlloc( sizeof( call_node ) );
    call->state = CGAlloc( sizeof( call_state ) );
    call->name = node;
    call->tipe = tipe;
    call->parms = NULL;
    call->ins = NewIns( 3 );
    if( node->format == NF_ADDR && node->class == CL_ADDR_GLOBAL ) {
        call->ins->head.opcode = OP_CALL;
        type_class = CallState( aux, tipe, call->state );
        mem = node->u.n.name;
        mem = (name *)SAllocMemory( mem->v.symbol, mem->v.offset, mem->m.memory_type,
                            mem->n.type_class, mem->n.size );
        node->u.n.name = mem;
    } else {
        call->ins->head.opcode = OP_CALL_INDIRECT;
        type_class = CallState( aux, tipe, call->state );
#if _TARGET & ( _TARG_80386 | _TARG_8086 )
        cookie = FEAuxInfo( aux, VIRT_FUNC_REFERENCE );
        if( cookie != NULL )
            TellObjVirtFuncRef( cookie );
#elif _TARGET & _TARG_PPC
        CurrProc->targ.toc_clobbered = true;
#endif
    }
    call->ins->type_class = type_class;
    return( call );
}


void    BGAddParm( cn call, an parm ) {
/**********************************************
    link a parm into the list of parms for a call node
*/

    pn          new;

    new = CGAlloc( sizeof( parm_node ) );
    new->name = AddrToIns( parm );
    new->name->flags |= FL_ADDR_OK_ACROSS_BLOCKS; /* always taken care of by BGCall*/
    new->next = call->parms;
    call->parms = new;
}


void    BGAutoDecl( cg_sym_handle sym, type_def *tipe ) {
/*************************************************************
    declare an automatic variable with name "sym" and type "tipe". This
    just creates the appropriate back end symbol table entry. (eg: N_TEMP)
*/

    BGDone( MakeAddrName( CG_FE, sym, tipe ) );
}

static  void    LinkParmIns( instruction *parm_def, instruction *ins ) {
/**********************************************************************/

    ins->operands[2] = (name *)parm_def; /* link them together*/
    parm_def->operands[2] = (name *)ins;
    ins->ins_flags |= INS_PARAMETER;
    parm_def->ins_flags |= INS_PARAMETER;
}

static  instruction *DoParmDef( name *result, type_class_def type_class )
/***********************************************************************/
{
    instruction *parm_def;

    parm_def = NewIns( 0 );
    parm_def->head.opcode = OP_PARM_DEF;
    parm_def->type_class = type_class;
    parm_def->result = result;
    AddIns( parm_def );
    return( parm_def );
}

#if _TARGET & _TARG_RISC

#if _TARGET & _TARG_AXP
#define BASE_TYPE       U8
#define BASE_SIZE       8
#define BASE_ALIGNMENT  8
#else
#define BASE_TYPE       U4
#define BASE_SIZE       4
#define BASE_ALIGNMENT  4
#endif

static  name    *DoAlphaParmDecl( hw_reg_set reg, cg_sym_handle sym, type_def *tipe, name *t2 ) {
/*********************************************************************************************
    N.B.: This was too weird to be incorporated in routine below.
    If we have a structure being passed by value on the Alpha, we start
    stuffing 8-byte chunks of data into any available parm registers, and
    put the remainder on the stack once those run out. In order to make
    this mess into an actual temp of the correct type, we create something
    which looks like:
        PARMDEF     => Ra
        MOV`     Ra => t2
        PARMDEF     => Rb
        MOV      Rb => t2+8
        ...
        PARMDEF     => t1
        MOV XX   t1 => t2+n
    which give us a real temp in t2, when all is said and done.
*/

    name                *t1;
    name                *reg_name;
    type_length         offset;
    instruction         *ins;
    instruction         *parm_def;
    type_length         len;

    /* unused parameters */ (void)sym;

    offset = 0;
    len = _RoundUp( tipe->length, BASE_SIZE );
    t2->n.size = len;
    for( ;; ) {
        if( HW_CEqual( reg, HW_EMPTY ) ) {
            // put the rest of the structure on the stack
            t1 = AllocTemp( XX );
            t1->v.usage |= USE_IN_ANOTHER_BLOCK|NEEDS_MEMORY|HAS_MEMORY|USE_MEMORY;
            t1->n.size = len - offset;
            t1->t.temp_flags |= STACK_PARM;
            t1->t.location = ParmMem( t1->n.size, BASE_ALIGNMENT, &CurrProc->state );
            parm_def = DoParmDef( t1, XX );
            ins = MakeMove( t1, STempOffset( t2, offset, XX, t1->n.size ), XX );
            LinkParmIns( parm_def, ins );
            AddIns( ins );
            offset += t1->n.size;
        } else {
            reg_name = AllocRegName( reg );
            parm_def = DoParmDef( reg_name, BASE_TYPE );
            ins = MakeMove( reg_name, STempOffset( t2, offset, BASE_TYPE, BASE_SIZE ), BASE_TYPE );
            LinkParmIns( parm_def, ins );
            AddIns( ins );
            offset += BASE_SIZE;
        }
        if( offset >= len )
            break;
        reg = ParmReg( BASE_TYPE, BASE_SIZE, BASE_ALIGNMENT, &CurrProc->state );
    }
    return( t2 );
}
#endif

name    *DoParmDecl( cg_sym_handle sym, type_def *tipe, hw_reg_set reg ) {
/******************************************************************************
    Declare a parameter of type "tipe" for the current routine. This
    routine determines whether the parameter is coming in in a register
    or on the stack and generates a temporary to hold the parm (temp) and
        PARMDEF   => x
        MOV     x => temp
    and links these two instructions together. (x may be a register or
    a temporary that is pre-allocated to a location on the stack)
    in the current basic block. If the parm is coming in on the stack,
    it sets the location field of the temp to be the same as the location it
    arrives in on the stack, so if we do assign the
    parm to memory, it gets its incoming location on the stack.
*/

    instruction         *ins;
    instruction         *parm_def;
    name                *temp;
    name                *parm_name;
    type_class_def      type_class;
    type_def            *ptipe;
    bool                no_temp;

    ptipe = QParmType( AskForLblSym( CurrProc->label ), sym, tipe );
    type_class = TypeClass( ptipe );
    if( sym == NULL ) {
        temp = AllocTemp( TypeClass( tipe ) );
    } else {
        temp = SAllocUserTemp( sym, TypeClass( tipe ), tipe->length );
        BGDone( MakeAddrName( CG_FE, sym, tipe ) );
    }
    temp->v.usage |= USE_IN_ANOTHER_BLOCK;

    no_temp = ( type_class == XX );
#if _TARGET & ( _TARG_80386 | _TARG_8086 )
    // The arguments of an interrupt routine coming in on the stack are used
    // for input and output; routine epilog pops them into registers. Handle
    // them specially to stop the optimizer from eliminating the writes (don't
    // create temps normally used with simple types).
    if( _RoutineIsInterrupt( CurrProc->state.attr ) && HW_CEqual( reg, HW_EMPTY ) ) {
        no_temp = true;
        temp->v.usage |= USE_ADDRESS;
    }
#endif
#if _TARGET & _TARG_RISC
    if( type_class == XX ) {
        return( DoAlphaParmDecl( reg, sym, tipe, temp ) );
    }
#endif
    if( HW_CEqual( reg, HW_EMPTY ) ) {
        if( no_temp ) {
            parm_name = temp;
            parm_name->t.location = ParmMem( tipe->length, ParmAlignment( ptipe ), &CurrProc->state );
        } else {
            parm_name = AllocTemp( type_class );
            parm_name->t.location = ParmMem( ptipe->length, ParmAlignment( ptipe ), &CurrProc->state );
        }
        parm_name->t.temp_flags |= STACK_PARM;
     // parm_name->n.size = tipe->length;
        parm_name->n.size = ptipe->length;
        parm_name->v.usage |= NEEDS_MEMORY | HAS_MEMORY | USE_MEMORY;
    } else {
        parm_name = AllocRegName( ActualParmReg( reg ) );
#if _TARGET & _TARG_80386
        if( CurrProc->state.attr & ROUTINE_STACK_RESERVE ) {
            // Just make sure we are taking up some space
            ParmMem( ptipe->length, ParmAlignment( ptipe ), &CurrProc->state );
        }
#endif
    }
    if( _IsModel( DBG_LOCALS ) ){  // d1+ or d2
        if( sym != NULL ) {
            DbgParmLoc( parm_name, sym );
        }
    }
    parm_def = DoParmDef( parm_name, type_class );
    if( type_class != XX ) {
        ins = MakeConvert( parm_name, temp, TypeClass( tipe ), type_class );
        LinkParmIns( parm_def, ins );
        AddIns( ins );
        TRDeclareParm( ins );
        if( temp->n.class == N_TEMP && parm_name->n.class == N_TEMP ) {
            temp->t.location = parm_name->t.location;
#if (_TARGET & _TARG_PPC) || (_TARGET & _TARG_MIPS)
        } else {
            // for PowerPC varargs routines, ensure that taking the address
            // of a parm coming in in a register will force that parm into the
            // correct home location in the caller's frame (yes - it sucks)
            // For MIPS, ensure that taking the address of a parm passed in
            // register will always force it to the right home location,
            // varargs or not. All this is done basically so that crappy code
            // that doesn't use stdarg.h properly would work - we have some
            // in our own clib ;-)
            temp->t.location = CurrProc->state.parm.offset - ptipe->length;
#endif
        }
    }
    MaxStack = 0;
    return( temp );
}


void    BGParmDecl( cg_sym_handle sym, type_def *tipe ) {
/************************************************************/

    hw_reg_set          reg;
    type_def            *t;

    t = QParmType( AskForLblSym( CurrProc->label ), sym, tipe );
    reg = ParmReg( TypeClass( t ), t->length, ParmAlignment( tipe ), &CurrProc->state );
    DoParmDecl( sym, tipe, reg );
}


static  void    LinkParms( instruction *call_ins, pn *owner ) {
/**************************************************************
    call TRAddParm for each parm in order and delete the parm nodes
*/
    pn          next;
    pn          parm;

    for( parm = *owner; parm != NULL; parm = next ) {
        next = parm->next;
        // because of FPPushParms and delayed stuff
        // if( parm->ins == NULL ) _Zoiks( ZOIKS_XXX );
        TRAddParm( call_ins, parm->ins );
        CGFree( parm );
    }
    *owner = NULL;
}

void    AddCallIns( instruction *ins, cn call ) {
/********************************************************
    stick the call instruction into the current basic block
*/

    name                *call_name;
    fe_attr             attr;
    type_class_def      addr_type_class;
    name                *temp;
    instruction         *new_ins;

    PreCall( call );
    if( ins->head.opcode == OP_CALL ) {
        call_name = call->name->u.n.name;
        attr = 0;
        if( call_name->m.memory_type == CG_FE ) {
            attr = FEAttr( call_name->v.symbol );
#if _TARGET & _TARG_RISC
            // in case the inline assembly code references a local variable
            if( FindAuxInfoSym( call_name->v.symbol, CALL_BYTES ) != NULL ) {
                CurrProc->targ.base_is_fp = true;
            }
#endif
        }
        // don't do this for far16 functions since they are handled
        // in a weird manner by Far16Parms and will not call data labels
#if _TARGET & (_TARG_80386 | _TARG_8086)
        if( (attr & FE_PROC) == 0 && (ins->flags.call_flags & CALL_FAR16) == 0 ) {
#else
        if( (attr & FE_PROC) == 0 ) {
#endif
            // indirect since calling data labels directly
            // screws up the back end
            addr_type_class = WD;
#if _TARGET & (_TARG_80386|_TARG_8086)
            if( *(call_class *)FindAuxInfo( call_name, CALL_CLASS ) & FAR_CALL ) {
                addr_type_class = CP;
            }
#endif
            temp = AllocTemp( addr_type_class );
            new_ins = MakeUnary( OP_LA, call_name, temp, addr_type_class );
            AddIns( new_ins );
            call_name = temp;
            ins->head.opcode = OP_CALL_INDIRECT;
            ins->num_operands = 3;
        }
        ins->operands[CALL_OP_ADDR] = call_name;
    } else {
        ins->operands[CALL_OP_ADDR] = GenIns( call->name );
    }
    AddIns( ins );
    TNZapParms();
    PostCall( call );
}


void    ReverseParmNodeList( pn *owner ) {
/*************************************************
    reverse a linked list of parm_nodes.
*/

    pn  parm;
    pn  next;

    parm = *owner;
    *owner = NULL;
    for( ; parm != NULL; parm = next ) {
        next = parm->next;
        parm->next = *owner;
        *owner = parm;
    }
}


void            PushParms( pn parm, call_state *state ) {
/***************************************************************
    Run through the list of parameters, generating pushes
    for the ones that are being passed on the stack. Unhook them
    from the list, but leave the others (register parms) alone.
    Add pointers to push instructions to the call instruction.
*/

    instruction         *ins;
    instruction         *push_ins;
    an                  addr;

    for( ; parm != NULL; parm = parm->next ) {
        if( parm->ins == NULL ) {
            if( HW_CEqual( parm->regs, HW_EMPTY ) ) {
                addr = parm->name;
                if( addr->format != NF_INS ) {
                    _Zoiks( ZOIKS_043 );
                }
                ins = addr->u.i.ins;
                PushInSameBlock( ins );
                if( ins->head.opcode == OP_MOV && !IsVolatile( ins->operands[0] ) && (addr->flags & FL_VOLATILE) == 0 ) {
                    push_ins = PushOneParm( ins, ins->operands[0], ins->type_class, parm->offset, state );
                    // ins->result = ins->operands[0]; -- was this useful? BBB
                    FreeIns( ins );
                } else {
                    ins->result = BGNewTemp( addr->tipe );
                    FPNotStack( ins->result );
                    if( addr->flags & FL_ADDR_CROSSED_BLOCKS ) {
                        ins->result->v.usage |= USE_IN_ANOTHER_BLOCK;
                    }
                    push_ins = PushOneParm( ins, ins->result, ins->result->n.type_class, parm->offset, state );
                }
                addr->format = NF_ADDR; /* so instruction doesn't get freed! */
                BGDone( addr );
                parm->ins = push_ins;
            }
        }
    }
}

#if _TARGET & _TARG_80386
void    ReserveStack( call_state *state, instruction *prev, type_length len ) {
/**************************************************************************************
    grab len bytes off the stack - doesn't matter what goes in there
    as long as the space is allocated.  Guaranteed that len is a multiple
    of 4 bytes in size.
*/

    name        *reg;
    instruction *ins;

    CurrProc->targ.never_sp_frame = true;
    reg = AllocRegName( HW_xAX );
    switch( len ) {
    case 8:
        ins = MakeUnary( OP_PUSH, reg, NULL, WD );
        SuffixIns( prev, ins );
        /* fall through */
    case 4:
        ins = MakeUnary( OP_PUSH, reg, NULL, WD );
        SuffixIns( prev, ins );
        break;
    default:
        _Zoiks( ZOIKS_133 );
    }
    state->parm.offset += len;
    if( state->parm.offset > MaxStack ) {
        MaxStack = state->parm.offset;
    }
}
#endif

void    ParmIns( pn parm, call_state *state ) {
/******************************************************
    generate the move instructions for parameters that are passed in registers.
    This should be all that's left on the list by now. The rest have all
    been pushed on the stack.
*/

    name                *reg;
    name                *curr;
    instruction         *ins;
    an                  addr;

    for( ; parm != NULL; parm = parm->next ) {
        if( parm->ins == NULL ) {
            addr = parm->name;
            if( addr->format != NF_INS ) {
                _Zoiks( ZOIKS_043 );
            }
            reg = AllocRegName( ActualParmReg( parm->regs ) );
            ins = addr->u.i.ins;
            ins->result = BGNewTemp( addr->tipe );
            if( addr->flags & FL_ADDR_CROSSED_BLOCKS ) {
                ins->result->v.usage |= USE_IN_ANOTHER_BLOCK;
            }
            curr = ins->result;
#if _TARGET & _TARG_AXP
            ins = MakeMove( curr, reg, TypeClass( addr->tipe ) );
            AddIns( ins );
#else
            if( addr->tipe->length == reg->n.size ) {
                ins = MakeMove( curr, reg, TypeClass( addr->tipe ) );
                AddIns( ins );
            } else if( !CvtOk( TypeClass( addr->tipe ), reg->n.type_class ) ) {
                ins = NULL;
                FEMessage( MSG_BAD_PARM_REGISTER, (pointer)(pointer_uint)parm->num );
  #if _TARGET & ( _TARG_8086 | _TARG_80386 )
            } else if( HW_CEqual( reg->r.reg, HW_ABCD ) ) {
                ins = NULL;
                FEMessage( MSG_BAD_PARM_REGISTER, (pointer)(pointer_uint)parm->num );
  #endif
            } else {
                ins = MakeConvert( curr, reg, reg->n.type_class, TypeClass( addr->tipe ) );
                AddIns( ins );
            }
#endif
#if _TARGET & _TARG_80386
            if( state->attr & ROUTINE_STACK_RESERVE ) {
                // this is for the stupid OS/2 _Optlink calling convention
                ReserveStack( state, ins, addr->tipe->length );
            }
#else
            /* unused parameters */ (void)state;
#endif
            addr->format = NF_ADDR; /* so instruction doesn't get freed! */
            BGDone( addr );
            parm->ins = ins;
        }
    }
}


void    BGZapBase( name *base, type_def *tipe ) {
/********************************************************
    for FORTRAN. Generate a NOP with result of base[temp], so that we
    know that a pass by reference argument can be modified by the
    call. The NOP instruction right after the call.
*/

    instruction *ins;

    if( base == NULL )
        return;
    if( _IsntModel( FORTRAN_ALIASING ) )
        return;
    if( (tipe->attr & TYPE_POINTER) == 0 )
        return;
    ins = MakeNop();
    if( DummyIndex == NULL )
        DummyIndex = AllocTemp( WD );
    ins->result = ScaleIndex( DummyIndex, base, 0, XX, tipe->length, 0, X_FAKE_BASE );
    ins->flags.nop_flags |= NOP_ZAP_INFO;
    AddIns( ins );
}


void    BGReturn( an retval, type_def *tipe ) {
/******************************************************
    generate the instructions to return the value 'retval', then
    go off and call generate to optimize the whole routine and
    spew it out into the object file.
*/

    instruction         *ins;
    instruction         *last_ins;
    instruction         *ret_ins;
    name                *name;
    type_class_def      tipe_type_class;
    type_class_def      type_class;

    ins = MakeNop();
    last_ins = NULL;
    if( retval != NULL ) {
        tipe_type_class = TypeClass( tipe );
        type_class = ReturnTypeClass( tipe, CurrProc->state.attr );
        UpdateReturn( &CurrProc->state, tipe, type_class, FEAuxInfo( AskForLblSym(CurrProc->label), AUX_LOOKUP ) );
        if( _IsModel( DBG_LOCALS ) ){  // d1+ or d2
            DbgRetLoc();
        }
        if( type_class == XX ) {
            name = StReturn( retval, tipe, &last_ins );
            AddIns( MakeMove( GenIns( retval ), name, tipe_type_class ) );
        } else {
            if( HW_CEqual( CurrProc->state.return_reg, HW_EMPTY ) ) {
                name = StReturn( retval, tipe, &last_ins );
            } else {
                name = AllocRegName( CurrProc->state.return_reg );
            }
#if _TARGET & _TARG_AXP
            if( tipe_type_class == U4 && type_class == I8 ) {
                ret_ins = MakeConvert( GenIns( retval ), name, I8, I4 );
            } else {
#endif
                if( tipe->length == name->n.size ) {
                    ret_ins = MakeMove( GenIns( retval ), name, name->n.type_class );
                } else {
                    ret_ins = MakeConvert( GenIns( retval ), name, name->n.type_class, tipe_type_class );
                }
                // BBB - we can get a situation where we are returning
                // a float in eax (when compiling -3s) and we don't want
                // to do a convert - ack.
                // ret_ins = MakeConvert( GenIns( retval ), name, name->n.type_class, class );
#if _TARGET & _TARG_AXP
            }
#endif
            AddIns( ret_ins );
        }
        BGDone( retval );
    } else {
        ins->zap = &AllocRegName( CurrProc->state.return_reg )->r;
        ins->flags.nop_flags |= NOP_ZAP_INFO;
    }
    AddIns( ins );
    if( last_ins != NULL )
        AddIns( last_ins );
    GenBlock( BLK_RETURN, 0 );
    if( AddrList != NULL ) {
        _Zoiks( ZOIKS_003 );
    }
    Generate( true );
    TargetModel = SaveTargetModel;
}

#if _TARGET & _TARG_RISC

static pn   BustUpStruct( pn parm, type_class_def from, type_class_def using_type_class )
/***************************************************************************************/
{
    pn                  curr;
    pn                  last;
    type_length         len;
    type_length         offset;
    type_length         size;
    name                *temp;
    instruction         *ins;

    curr = NULL;
    size = TypeClassSize[using_type_class];
    len = _RoundUp( parm->name->tipe->length, size );
    temp = AllocTemp( from );
    temp->n.size = len;
    last = parm->next;
    parm->name->u.i.ins->result = temp;
    for( offset = len - size; offset >= 0; offset -= size ) {
        // create a parm node for this part of the struct
        curr = CGAlloc( sizeof( parm_node ) );
        ins = MakeMove( STempOffset( temp, offset, using_type_class, size ), NULL, using_type_class );
        AddIns( ins );
        curr->next = last;
        curr->name = InsName( ins, TypeOfTypeClass( using_type_class ) );
        curr->name->flags = parm->name->flags;
        curr->alignment = 4;
        last = curr;
    }
    // only needed for first parm (PPC hack - doubles skip registers)
    curr->alignment = parm->alignment;
    return( curr );
}

static void SplitStructParms( pn *parm_list, call_state *state )
/***************************************************************
    Split up any structures being passed as parms into
    smaller, independant chunks (system dependant).
*/
{
    pn                  parm;
    pn                  *last_parm;
    an                  name;
    type_class_def      using_type_class;
    type_class_def      type_class;

#if _TARGET & _TARG_PPC
    if( _IsTargetModel( CG_OS2_CC ) )
        return;
    using_type_class = U4;
#elif _TARGET & _TARG_AXP
    /* unused parameters */ (void)state;

    using_type_class = U8;
#elif _TARGET & _TARG_MIPS
    /* unused parameters */ (void)state;

    using_type_class = U4;
#else
    #error Unknown RISC CPU
#endif
    last_parm = parm_list;
    for( parm = *last_parm; parm != NULL; parm = parm->next ) {
        name = parm->name;
        parm->alignment = ParmAlignment( name->tipe );
        type_class = TypeClass( name->tipe );
#if _TARGET & _TARG_PPC
        if( type_class == XX || ( type_class == FD ) && (state->attr & ROUTINE_HAS_VARARGS) ) {
#else
        if( type_class == XX ) {
#endif
            if( ( type_class == FD ) || ( name->tipe->length > 7 ) ) {
                parm->alignment = 8;
            }
            *last_parm = BustUpStruct( parm, type_class, using_type_class );
            name->format = NF_ADDR; /* so instruction doesn't get freed! */
            BGDone( name );
            CGFree( parm );
            parm = *last_parm;
        }
        last_parm = &parm->next;
    }
}
#endif

bool        AssgnParms( cn call, bool in_line ) {
/********************************************************
    Decide what registers the parms should go in.
    Assign registers to first parm first, etc. Also, assign a congolmeration
    of all the parameter registers to one of the operands of the call
    so that the dataflow knows that the registers are live between the
    move into them and the call.
*/


    pn                  parm;
    call_state          *state;
    bool                push_no_pop;
    int                 parms;
    instruction         *call_ins;
    type_class_def      parm_type_class;
    type_class_def      reg_type_class;


    push_no_pop = false;
    parm = call->parms;
    state = call->state;
    call_ins = call->ins;
    parms = 0;
#if _TARGET & _TARG_RISC
    SplitStructParms( &call->parms, state );
    parm = call->parms;
#endif
    for( ; parm != NULL; parm = parm->next ) {
        if( in_line ) {
            parm->regs = ParmInLineReg( &state->parm );
            if( HW_CEqual( parm->regs, HW_EMPTY ) ) {
                if( !HW_CEqual( *(state->parm.curr_entry), HW_EMPTY ) ) {
                    FEMessage( MSG_ERROR, "More parameters than registers in pragma" );
                } else {
                    parm->offset = ParmMem( parm->name->tipe->length, ParmAlignment( parm->name->tipe ), state );
                    push_no_pop = true;
                }
            } else {
                parm_type_class = TypeClass( parm->name->tipe );
                reg_type_class  = AllocRegName( parm->regs )->n.type_class;
                if( parm_type_class != FD || reg_type_class != U8 ) {
                    if( !CvtOk( parm_type_class, reg_type_class ) ) {
                        FEMessage( MSG_BAD_PARM_REGISTER, (pointer)(pointer_uint)( parms + 1 ) );
                    }
                }
            }
        } else {
#if ( _TARGET & _TARG_PPC ) == 0
            parm->alignment = 1;
#endif
            parm->regs = ParmReg( TypeClass( parm->name->tipe ),
                                   parm->name->tipe->length,
                                   parm->alignment,
                                   state );
            if( HW_CEqual( parm->regs, HW_EMPTY ) ) {
                parm->offset = ParmMem( parm->name->tipe->length, parm->alignment, state );
                push_no_pop = true;
            }
        }
        ++parms;
        parm->num = parms;
        parm->ins = NULL;
    }
    ReverseParmNodeList( &call->parms );
    PushParms( call->parms, state );
    FPPushParms( call->parms, state );
    ParmIns( call->parms, state );
    ReverseParmNodeList( &call->parms );
    LinkParms( call_ins, &call->parms );
    if( call_ins->head.opcode == OP_CALL ) {
        /* special case for call so we ignore address in dataflow */
        call_ins->num_operands = 2;
        call_ins->operands[CALL_OP_USED] = AllocRegName( state->parm.used );
    } else {
        call_ins->operands[CALL_OP_USED] = AllocRegName( state->parm.used );
    }
    call_ins->operands[CALL_OP_POPS] = AllocS32Const( state->parm.offset );
    call_ins->zap = &AllocRegName( CallZap( state ) )->r;
    return( push_no_pop );
}
