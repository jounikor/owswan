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
* Description:  Take the instruction stream from the code generator and
*               writes the instructions into the object file.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "optmain.h"
#include "pccode.h"
#include "system.h"
#include "pcencode.h"
#include "zoiks.h"
#include "zeropage.h"
#include "cfloat.h"
#include "cgaux.h"
#include "p5prof.h"
#include "data.h"
#include "display.h"
#include "rtrtn.h"
#include "utils.h"
#include "x86objd.h"
#include "objout.h"
#include "dbsyms.h"
#include "objprof.h"
#include "x86enc2.h"
#include "encode.h"
#include "object.h"
#include "x86proc.h"
#include "targetin.h"
#include "x87.h"
#include "x86esc.h"
#include "rgtbl.h"
#include "split.h"
#include "namelist.h"
#include "fixindex.h"
#include "x86segs.h"
#include "x86enc.h"
#include "x86split.h"
#include "x86opcod.h"
#include "feprotos.h"


template            Temp;           /* template for oc_entries */
byte                Inst[INSSIZE];  /* template for instructions */
byte                ILen;           /* length of object instruction */

static  byte        ICur;           /* cursor for writing into Inst */
static  byte        IEsc;           /* number of initial bytes that must be */
                                    /* checked for escapes when copied into Temp */

static  hw_reg_set RegTab[] = {
#define REGS 24
    HW_D( HW_AL ),      HW_D( HW_AX ),      HW_D( HW_EAX ),
    HW_D( HW_CL ),      HW_D( HW_CX ),      HW_D( HW_ECX ),
    HW_D( HW_DL ),      HW_D( HW_DX ),      HW_D( HW_EDX ),
    HW_D( HW_BL ),      HW_D( HW_BX ),      HW_D( HW_EBX ),
    HW_D( HW_AH ),      HW_D( HW_SP ),      HW_D( HW_ESP ),
    HW_D( HW_CH ),      HW_D( HW_BP ),      HW_D( HW_EBP ),
    HW_D( HW_DH ),      HW_D( HW_SI ),      HW_D( HW_ESI ),
    HW_D( HW_BH ),      HW_D( HW_DI ),      HW_D( HW_EDI )
};

static  hw_reg_set SegTab[] = {
#define SEGS 6
    #define _SR_(h,f)   HW_D( h ),
    #include "x86sregs.h"
    #undef _SR_
};

/* routines that maintain instruction buffers*/

void    Format( oc_class class )
/*******************************
    Get Temp and Inst ready to accept instructions.  Each instruction is
    formatted into Inst, transferred into Temp (as an opcode_entry) and
    then dumped into the peephole optimizer.
*/
{
    SetFPPatchType( FPP_NONE );
    Temp.hdr.class = class;
    Temp.hdr.objlen = 0;
    Temp.hdr.reclen = offsetof( template, data );
    ICur = 0;
    ILen = 0;
    IEsc = 0;
}

void    ReFormat( oc_class class )
/*********************************
    Change the class of Temp
*/
{
    Temp.hdr.class = class;
}

void    AddByte( byte b )
/************************
    Add a byte to Inst[]
*/
{
    if( b == ESC ) {
        Inst[ICur++] = b;
    }
    Inst[ICur++] = b;
    ILen++;
}

void    AddToTemp( byte b )
/********************************
    Add a byte to the end of Temp
*/
{
    if( b == ESC ) {
        Temp.data[Temp.hdr.reclen++ - offsetof( template, data )] = b;
    }
    Temp.data[Temp.hdr.reclen++ - offsetof( template, data )] = b;
    Temp.hdr.objlen++;
}

void    EmitByte( byte b )
/**************************
    Plop a byte into Inst[]
*/
{
    Inst[ICur++] = b;
}

void    EmitPtr( pointer p )
/*****************************
    Plop a pointer into Inst[]
*/
{
    *(pointer *)(Inst + ICur) = p;
    ICur += sizeof( pointer );
}

void    EmitSegId( segment_id segid )
/************************************
    Plop a segment_id into Inst[]
*/
{
    *(segment_id *)(Inst + ICur) = segid;
    ICur += sizeof( segment_id );
}

void    EmitOffset( offset i )
/***************************************
    Plop an "offset" int Inst[] (a machine word)
*/
{
    *(offset *)(Inst + ICur) = i;
    ICur += sizeof( offset );
}

static  void    TransferIns( void ) {
/************************************
    Transfer an instruction from Inst[] to Temp
*/

    unsigned i;
    unsigned j;

    j = Temp.hdr.reclen - offsetof( template, data );
    for( i = 0; i < IEsc; ++i ) {
        if( Inst[i] == ESC ) {
            Temp.data[j++] = ESC;
            Temp.hdr.reclen++;
        }
        Temp.data[j++] = Inst[i];
    }
    Copy( &Inst[i], &Temp.data[j], ICur - i );
    Temp.hdr.reclen += ICur;
    Temp.hdr.objlen += ILen;
}

void    EjectInst( void )
/**********************************
    Dump the current instruction into the peephole optimizer
*/
{
    TransferIns();
    ICur = 0;
    ILen = 0;
    IEsc = 0;
}

void    Finalize( void )
/*********************************
    Invoked by macro _Emit. Spits Temp into the peephole optimizer
*/
{
    EjectInst();
    FPPatchTypeRef();
    if( Temp.hdr.objlen != 0 ) {
        InputOC( (any_oc *)&Temp );
    }
}


static  void    LayInitial( instruction *ins, gentype gen ) {
/************************************************************
    Do some really magical jiggery pokery based on "gen" to get the
    right opcode int Inst[].  See PCCodeTable if you want, but this is
    not for the faint of heart!
*/

    int         index;
    pccode_def  *table;

    table = PCCodeTable;
    for(;;) {
        if( gen < table->low_gen )
            break;
        if( table->width == 0 )
            return;
        ++ table;
    }
    -- table;
    index = 0;
    for(;;) {
        if( table->opcode_list[index] == ins->head.opcode )
            break;
        if( table->opcode_list[index] == OP_NOP )
            break;
        ++ index;
    }
    index = index * table->width + gen - table->low_gen;
    if( table->flags & NEED_WAIT ) {
        Used87 = true;
#if _TARGET & _TARG_8086
        if( gen == G_FINIT || !_CPULevel( CPU_286 ) || _IsEmulation() ) {
            if( _IsEmulation() ) {
                SetFPPatchType( FPP_NORMAL );
            }
            LayOpbyte( 0x9b );
            _Next;
        }
#endif
    }
    if( table->flags & BYTE_OPCODE ) {
        LayOpbyte( table->gen_opcode_table[index] );
    } else {
        LayOpword( table->gen_opcode_table[index] );
    }
    if( table->flags & BYTE_WORD ) {
        LayW( ins->type_class );
    }
    if( table->flags & SIGN_UNSIGN ) {
        if( ins->type_class == I1
         || ins->type_class == I2
         || ins->type_class == I4 ) {
            if( ins->head.opcode >= OP_MUL && ins->head.opcode <= OP_MOD ) {
                Inst[RMR] |= B_RMR_MUL_SGN;
            } else if( ins->head.opcode == OP_RSHIFT ) {
                Inst[RMR] |= B_RMR_SHR_SAR;
            }
        }
    }
}

static  byte    SegTrans( hw_reg_set regs ) {
/********************************************
    Return the encoding of a segment register name
*/

    byte i;

    HW_COnlyOn( regs, HW_SEGS );
    for( i = 0; i < SEGS; ++i ) {
        if( HW_Equal( regs, SegTab[i] ) ) {
            break;
        }
    }
    if( i >= SEGS ) {
        _Zoiks( ZOIKS_032 );
    }
    return( i );
}

static  byte    RegTrans( hw_reg_set regs ) {
/********************************************
    Return the encoding of a register name
*/

    byte i;

    HW_CTurnOff( regs, HW_SEGS );
    for( i = 0; i < REGS; ++i ) {
        if( HW_Equal( regs, RegTab[i] ) ) {
            break;
        }
    }
    if( i >= REGS ) {
        _Zoiks( ZOIKS_031 );
    }
    i = i / 3;
    return( i );
}

#if _TARGET & _TARG_80386
static  bool    NeedOpndSize( instruction *ins ) {
/*************************************************
        Do we REALLY, REALLY need the operand size override.
*/
    opcnt       i;
    hw_reg_set  result_reg;
    hw_reg_set  full_reg;
    instruction *next;

    switch( ins->head.opcode ) {
    case OP_MOD:
    case OP_DIV:
    case OP_RSHIFT:
        return( true );
    default:
        break;
    }
    if( _OpIsCondition( ins->head.opcode ) )
        return( true );
    if( ins->ins_flags & INS_CC_USED )
        return( true );
    if( ins->result == NULL )
        return( true );
    switch( ins->result->n.class ) {
    case N_TEMP:
        /* it's OK to store a DWORD into a WORD temp because there's always
           two bytes of slack on the stack */
        if( ins->result->t.alias != ins->result )
            return( true );
        break;
    case N_REGISTER:
        /* check that it's OK to trash high word of register */
        next = ins->head.next;
        if( next == NULL )
            return( true );
        result_reg = ins->result->r.reg;
        full_reg = FullReg( result_reg );
        HW_TurnOff( full_reg, result_reg );
        if( HW_Ovlap( full_reg, next->head.live.regs ) )
            return( true );
        break;
    default:
        return( true );
    }
    for( i = ins->num_operands; i-- > 0; ) {
        /* Could be smarter here and allow N_MEMORY,N_INDEXED if
           they're in DGROUP */
        switch( ins->operands[i]->n.class ) {
        case N_TEMP:
        case N_REGISTER:
        case N_CONSTANT:
            break;
        default:
            return( true );
        }
    }
    ins->type_class = U4; /* get correct size of constants */
    return( false );
}
#endif


static  bool    LayOpndSize( instruction *ins, gentype gen ) {
/*************************************************************
    Generate an operand size prefix if the instruction needs it
    (Eg: MOV    AX,DX need it in a USE32 segment)
*/

#if _TARGET & _TARG_8086
    switch( ins->head.opcode ) {
    case OP_PUSH:
        if( ( gen == G_C1 || gen == G_M1 ) && ins->type_class == I4 ) {
            AddToTemp( M_OPND_SIZE );
            return( true );
        }
        break;
    case OP_MOV:
        if( ins->type_class == U4 || ins->type_class == I4 ) {
            if( ( gen == G_MOVMC ) ) {
                AddToTemp( M_OPND_SIZE );
                return( true );
            }
        }
        break;
    default:
        break;
    }
    return( false );
#else
    switch( ins->head.opcode ) {
    case OP_CONVERT: /* these ones handle themselves */
    case OP_CALL:
    case OP_CALL_INDIRECT:
    case OP_SELECT:
        return( false );
        break;
    default:
        switch( gen ) {
        case G_SR:
        case G_RS:
        case G_MS1:
        case G_SM1:
        case G_SEG_SEG:
        case G_SEGR1:
        case G_POW2DIV:
        case G_DIV2:
            return( false );
        default:
            break;
        }
        if( _IsTargetModel( USE_32 ) ) {
            if( ins->type_class == U2 || ins->type_class == I2 ) {
                if( NeedOpndSize( ins ) ) {
                    AddToTemp( M_OPND_SIZE );
                    return( true );
                }
            }
        } else {
            if( ins->type_class == U4
             || ins->type_class == I4
             || ins->type_class == FS ) {
                AddToTemp( M_OPND_SIZE );
                return( true );
            }
        }
        return( false );
    }
#endif
}

static  void    LayST( name *op ) {
/**********************************
    add the ST(i) operand to a floating point instruction
*/

    Inst[RMR] |= FPRegTrans( op->r.reg );
}


static  void    LayMF( name *op ) {
/**********************************
    For a floating point instruction, add the qword ptr stuff
*/

    if( op->n.class != N_CONSTANT ) {
        switch( op->n.type_class ) {
        case FS:
            Inst[KEY] |= MF_FS;
            break;
        case FD:
            Inst[KEY] |= MF_FD;
            break;
        case FL:
            Inst[KEY] |= MF_FL;
            Inst[RMR] |= B_RMR_FMT_FL;
            break;
        case U2:
        case I2:
            Inst[KEY] |= MF_I2;
            break;
        case U4:
        case I4:
            Inst[KEY] |= MF_I4;
            break;
        case U8:
        case I8:
        case XX:
            Inst[KEY] |= MF_I8;
            Inst[RMR] |= B_RMR_FMT_I8;
            break;
        default:
            _Zoiks( ZOIKS_029 );
            break;
        }
    }
}

#if 1
static  void    DoP5RegisterDivide( instruction *ins ) {
/******************************************************/

    int                 st_reg;
    byte                reverse;
    byte                pop;
    byte                dest;
    int                 ins_key;
    label_handle        lbl;
    label_handle        lbl_2;
    any_oc              oc;

    lbl = AskForNewLabel();
    lbl_2 = AskForNewLabel();

    oc.oc_jcond.hdr.class = OC_JCOND | ATTR_SHORT;
    oc.oc_jcond.hdr.reclen = sizeof( oc_jcond );
    oc.oc_jcond.hdr.objlen = OptInsSize( OC_JCOND, OC_DEST_NEAR );
    oc.oc_jcond.ref = NULL;
    oc.oc_jcond.cond = 4;
    oc.oc_jcond.handle = lbl;
    InputOC( &oc );
    st_reg = FPRegTrans( ins->operands[0]->r.reg );
    reverse = false;
    pop = false;
    dest = false;
    switch( G( ins ) ) {
    case G_RRFBIN:
        reverse = true;
        break;
    case G_RNFBIN:
        break;
    case G_RRFBINP:
        reverse = true;
        pop = true;
        dest = true;
        break;
    case G_RNFBINP:
        pop = true;
        dest = true;
        break;
    case G_RRFBIND:
        reverse = true;
        dest = true;
        break;
    case G_RNFBIND:
        dest = true;
        break;
    }
    ins_key = ( pop & 1 ) | ( ( reverse & 1 ) << 1 ) | ( ( dest & 1 ) << 2 );
    ins_key |= st_reg << 3;
    // je ok
    _Code;
    _Next;
    LayOpbyte( 0x50 );                  /* push [e]ax           */
    _Next;
    LayOpbyte( 0xb8 );                  /* mov #cons -> [e]ax   */
    AddWData( ins_key, WD );
    _Emit;
    DoRTCall( RT_FDIV_FPREG, false );   /* call __fdiv_fpr      */
    _Code;
    LayOpbyte( 0x58 );                  /* pop [e]ax            */
    _Emit;
    GenJumpLabel( lbl_2 );
    CodeLabel( lbl, 0 );
    _Code;
    LayInitial( ins, G( ins ) );
    LayOpndSize( ins, G( ins ) );
    LayST( ins->operands[0] );
    _Emit;
    CodeLabel( lbl_2, 0 );
    GenKillLabel( lbl );
    GenKillLabel( lbl_2 );
}

static  void    DoP5MemoryDivide( instruction *ins ) {
/****************************************************/

    any_oc              oc;
    rt_class            rtindex;
    label_handle        lbl;
    label_handle        lbl_2;
    name                *high;
    name                *low;
#if _TARGET & _TARG_8086
    name                *h;
    name                *l;
#endif
    name                *seg;

    lbl = AskForNewLabel();
    lbl_2 = AskForNewLabel();

    oc.oc_jcond.hdr.class = OC_JCOND | ATTR_SHORT;
    oc.oc_jcond.hdr.reclen = sizeof( oc_jcond );
    oc.oc_jcond.hdr.objlen = OptInsSize( OC_JCOND, OC_DEST_NEAR );
    oc.oc_jcond.ref = NULL;
    oc.oc_jcond.cond = 4;
    oc.oc_jcond.handle = lbl;
    InputOC( &oc );

    seg = NULL;
    if( ins->num_operands > OpcodeNumOperands( ins ) ) {
        seg = ins->operands[ins->num_operands - 1];
    }

    _Code;
    switch( ins->operands[0]->n.type_class ) {
    case FS:
#if _TARGET & _TARG_8086
        high = HighPart( ins->operands[0], U2 );
        low = LowPart( ins->operands[0], U2 );
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( high );
        _Next;
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( low );
#else
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( ins->operands[0] );
#endif
        if( G( ins ) == G_MRFBIN ) {
            rtindex = RT_FDIV_MEM32R;
        } else {
            rtindex = RT_FDIV_MEM32;
        }
        break;
    case FD:
        high = HighPart( ins->operands[0], U4 );
        low = LowPart( ins->operands[0], U4 );
#if _TARGET & _TARG_8086
        h = HighPart( high, U2 );
        l = LowPart( high, U2 );
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( h );
        _Next;
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( l );
        _Next;
        h = HighPart( low, U2 );
        l = LowPart( low, U2 );
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( h );
        _Next;
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( l );
        _Next;
#else
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( high );
        _Next;
        StackDepth += WORD_SIZE;
        if( seg != NULL )
            GenSeg( seg->r.reg );
        LayOpword( 0x30ff );
        LayModRM( low );
        StackDepth -= WORD_SIZE;
#endif
        if( G( ins ) == G_MRFBIN ) {
            rtindex = RT_FDIV_MEM64R;
        } else {
            rtindex = RT_FDIV_MEM64;
        }
        break;
    default:
        rtindex = RT_FDIV_MEM64;
        _Zoiks( ZOIKS_114 );
    }
    _Emit;
    DoRTCall( rtindex, false );
    GenJumpLabel( lbl_2 );
    CodeLabel( lbl, 0 );
    _Code;
    if( seg != NULL ) {
        GenSeg( seg->r.reg );
    }
    LayInitial( ins, G( ins ) );
    LayOpndSize( ins, G( ins ) );
    LayMF( ins->operands[0] );
    LayModRM( ins->operands[0] );
    _Emit;
    CodeLabel( lbl_2, 0 );
    GenKillLabel( lbl );
    GenKillLabel( lbl_2 );
}

static  void    DoP5Divide( instruction *ins ) {
/***********************************************
    Emit a most disgusting sequence of code to
    test for the Pentium FDIV bug and call a
    runtime routine to correct it.
*/
    bool        used_ds = false;

    _Code;
    LayOpbyte( 0x9c );          /* pushf */
    _Emit;
#if _TARGET & _TARG_80386
    StackDepth += WORD_SIZE;
#endif
    _Code;
    if( _IsTargetModel( FLOATING_DS ) ) {
        if( _IsTargetModel( FLOATING_SS ) ) {
            LayOpbyte( 0x1e );  /* push ds    */
            AddByte( 0x50 );    /* push [e]ax */
            _Emit;
            GenLoadDS();
            _Code;
            used_ds = true;
        } else {
            AddToTemp( 0x36 );
        }
    }
#if ( _TARGET & _TARG_8086 )
    LayOpword( 0x06f6 );        /* test byte ptr L1,1 */
#else
    LayOpword( 0x05f6 );        /* test byte ptr L1,1 */
#endif
    ILen += WORD_SIZE;
    DoLblRef( RTLabel( RT_BUGLIST ), AskBackSeg(), 0, OFST );
    AddWData( 1, U1 );
    _Emit;
    if( used_ds ) {
        _Code;
        LayOpbyte( 0x58 );      /* pop [e]ax */
        AddByte( 0x1f );        /* pop ds    */
        _Emit;
    }
    switch( G( ins ) ) {
    case G_RRFBIN:
    case G_RNFBIN:
    case G_RRFBIND:
    case G_RNFBIND:
    case G_RRFBINP:
    case G_RNFBINP:
        // assuming G_RXFBINZ means
        //      X -> R (reverse) and N (normal)
        //      Z -> D (destination is argument)
        //        -> P (argument is popped)
        DoP5RegisterDivide( ins );
        break;
    case G_MRFBIN:
    case G_MNFBIN:
        DoP5MemoryDivide( ins );
    }
    _Code;
    LayOpbyte( 0x9d );          /* popf */
    _Emit;
#if _TARGET & _TARG_80386
    StackDepth -= WORD_SIZE;
#endif
}
#endif


static  void    SetCC(void) {
/******************************
    Generate code to set the condition codes based on an 8087 FCMP instruction
*/

    if( _IsEmulation() ) {
        _Emit;
        _Code;
        SetFPPatchType( FPP_NORMAL );
    }
    if( _CPULevel( CPU_386 ) ) {
        if( _IsEmulation() ) {
            AddByte( 0x9b );        /*  FWAIT - needed for FIDRQQ to work */
        }
        AddByte( 0xdf );            /*  FSTSW  AX       */
        AddByte( 0xe0 );            /*  ..              */
    } else {
        if( !_CPULevel( CPU_386 ) ) {
            AddByte( 0x9b );        /*  FWAIT           */
        }
        if( _CPULevel( CPU_286 ) && !_IsEmulation() ) {
            AddByte( 0xdf );        /*  FSTSW  AX       */
            AddByte( 0xe0 );        /*  ..              */
        } else {
            _Next;                  /*                  */
            LayOpword( 0x38dd );    /*  FSTSW  temp     */
            LayModRM( FPStatWord ); /*  ..              */
            if( _IsEmulation() ) {
                _Emit;              /*                  */
                GFwait();           /*  FWAIT           */
                _Code;              /*                  */
            } else {
                AddByte( 0x9b );    /*  FWAIT           */
                _Next;              /*                  */
            }
            LayOpword( 0x008a );    /*  MOV    AX,temp  */
            LayReg( HW_AX );        /*  ..              */
            LayW( U2 );             /*  ..              */
            LayModRM( FPStatWord ); /*  ..              */
        }
    }
    AddByte( 0x9e );                /*  SAHF            */
}


/* Lay Routines*/

void    LayOpbyte( gen_opcode op )
/**********************************
    Add a one byte opcode to Inst[]
*/
{
    Inst[KEY] = op & 0xff;
    ICur = 1;
    ILen = 1;
    IEsc = 1;
}

void    LayOpword( gen_opcode op )
/*********************************
    Add a 2 byte opcode to Inst[]
*/
{
    Inst[KEY] = op & 0xff;
    Inst[RMR] = (op >> 8) & 0xff;
    ICur = 2;
    ILen = 2;
    IEsc = 2;
}

void    LayW( type_class_def type_class )
/***************************************/
{
    switch( type_class ) {
    case U2:
    case I2:
    case U4:
    case I4:
    case FS:
        Inst[KEY] |= B_KEY_W;       /* turn on the W bit*/
        break;
    default:
        break;
    }
}

static  void    LayRegOp( name *r ) {
/************************************
    Add the register op to the instruction
*/

    Inst[RMR] |= RegTrans( r->r.reg ) << S_RMR_REG;
}

void    LayReg( hw_reg_set r )
/***************************************
    Add the register op to the instruction
*/
{
    Inst[RMR] |= RegTrans( r ) << S_RMR_REG;
}

void    LayRMRegOp( name *r )
/**************************************
    Add the register op to a MOD/RM instruction
*/
{
    Inst[RMR] |= ( RegTrans( r->r.reg ) << S_RMR_RM ) + RMR_MOD_REG;
}

void    LayRegRM( hw_reg_set r )
/**************************************
    Add the register op to a MOD/RM instruction
*/
{
    Inst[RMR] |= ( RegTrans( r ) << S_RMR_RM ) + RMR_MOD_REG;
}

static  void    LayACRegOp( name *r ) {
/*************************************
    Add the other register op to a REG/AX (Accumulator) operation
*/

    Inst[KEY] |= RegTrans( r->r.reg ) << S_KEY_REG;
}

void    LayRegAC( hw_reg_set r )
/*****************************************
    Add the other register op to a REG/AX (Accumulator) operation
*/
{
    Inst[KEY] |= RegTrans( r ) << S_KEY_REG;
}

static  void    LaySROp( name *r ) {
/***********************************
    Add a segment register op
*/

    Inst[RMR] |= SegTrans( r->r.reg ) << S_RMR_SR;
}

void    GenSeg( hw_reg_set regs )
/******************************************
    Generate a segment override if we need one. regs if the full address.
    For example, if regs is DS:BP, we need an override. SS:BP wouldn't
*/
{
    hw_reg_set  segreg;
    int         i;

    segreg = regs;
    HW_COnlyOn( segreg, HW_SEGS );
    if( HW_CEqual( segreg, HW_EMPTY ) )
        return;
    if( HW_COvlap( regs, HW_xBP ) ) {
        if( HW_CEqual( segreg, HW_SS ) )
            return;
        if( HW_CEqual( segreg, HW_DS )
         && _IsntTargetModel(FLOATING_DS|FLOATING_SS) ) {
            return;
        }
    } else {
        if( HW_CEqual( segreg, HW_DS ) ) {
            return;
        }
    }
    /* produce segment override prefix*/
    if( HW_COvlap( regs, HW_FS ) ) {
        AddToTemp( M_SEGFS );
    } else if( HW_COvlap( regs, HW_GS ) ) {
        AddToTemp( M_SEGGS );
    } else {
        AddToTemp( M_SEGOVER | ( SegTrans( regs ) << S_KEY_SR ) );
    }
    if( _IsEmulation() ) {
        for( i = 0; i < SEGS; ++i ) {
            if( HW_Equal( segreg, SegTab[i] ) ) {
                SetFPPatchSegm( i );
                break;
            }
        }
    }
}

type_class_def  OpndSize( hw_reg_set reg )
/***************************************************
    Generate an operand size prefix if we need it and return the
    type_class of the register "reg"
*/
{
    if( HW_COvlap( reg, HW_SEGS ) )
        return( U2 );
#if _TARGET & _TARG_8086
    if( _IsTargetModel( USE_32 ) )
        AddToTemp( M_OPND_SIZE );
    return( U2 );
#else
    if( HW_COvlap( reg, HW_32 ) ) {
        if( _IsntTargetModel( USE_32 ) )
            AddToTemp( M_OPND_SIZE );
        return( U4 );
    } else {
        if( _IsTargetModel( USE_32 ) )
            AddToTemp( M_OPND_SIZE );
        return( U2 );
    }
#endif
}

static  void    PushSeg( hw_reg_set reg ) {
/******************************************
    PUSH segreg
*/

    if( HW_COvlap( reg, HW_FS ) ) {
        LayOpword( M_PUSHFS );
    } else if( HW_COvlap( reg, HW_GS ) ) {
        LayOpword( M_PUSHGS );
    } else {
        LayOpbyte( M_PUSHSEG | ( SegTrans( reg ) << S_KEY_SR ) );
    }
}

static  void    PopSeg( hw_reg_set reg ) {
/*****************************************
    POP segreg
*/

    if( HW_COvlap( reg, HW_FS ) ) {
        LayOpword( M_POPFS );
    } else if( HW_COvlap( reg, HW_GS ) ) {
        LayOpword( M_POPGS );
    } else {
        LayOpbyte( M_POPSEG | ( SegTrans( reg ) << S_KEY_SR ) );
    }
}


void    QuickSave( hw_reg_set reg, opcode_defs op )
/**************************************************
    PUSH/POP    reg     - based on "op"
    assume register is valid for PUSH/POP
*/
{
    _Code;
    if( HW_COvlap( reg, HW_SEGS ) ) {
        if( op == OP_PUSH ) {
            PushSeg( reg );
        } else {
            PopSeg( reg );
        }
    } else {
        if( op == OP_PUSH ) {
            LayOpbyte( M_PUSH | ( RegTrans( reg ) << S_KEY_REG ) );
        } else {
            LayOpbyte( M_POP | ( RegTrans( reg ) << S_KEY_REG ) );
        }
        OpndSize( reg );
    }
    _Emit;
}


void    GenRegXor( hw_reg_set src, hw_reg_set dst )
/**************************************************
    XOR         dst,src
*/
{
    _Code;
    LayOpword( M_XORRR | B_KEY_W );
    OpndSize( src );
    LayReg( src );
    LayRegRM( dst );
    _Emit;
}


void    GenRegNeg( hw_reg_set src )
/**********************************
    NEG         src
*/
{
    _Code;
    LayOpword( M_NEGR | B_KEY_W );
    OpndSize( src );
    LayRegRM( src );
    _Emit;
}


void    GenRegMove( hw_reg_set src, hw_reg_set dst )
/***************************************************
    MOV         dst,src
*/
{
    _Code;
    LayOpword( M_MOVRR | B_KEY_W );
    OpndSize( src );
    LayReg( src );
    LayRegRM( dst );
    _Emit;
}


static  void    AddSWData( opcode_defs op, signed_32 val, type_class_def type_class )
/**************************************************************************
    Add a const (signed_32) to Inst[] with sign extension if the opcode
    allows it
*/
{
    switch( op ) {
    case OP_ADD:
    case OP_EXT_ADD:
    case OP_SUB:
    case OP_EXT_SUB:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_MUL:
    case OP_CMP_EQUAL:
    case OP_CMP_NOT_EQUAL:
    case OP_CMP_GREATER:
    case OP_CMP_LESS_EQUAL:
    case OP_CMP_LESS:
    case OP_CMP_GREATER_EQUAL:
        AddSData( val, type_class );
        break;
    default:
        AddWData( val, type_class );
        break;
    }
}


static  void    AddSWCons( opcode_defs op, name *opnd, type_class_def type_class )
/******************************************************************************
    Add a const (name *) to Inst[] with sign extension if the opcode allows it
*/
{
    if( opnd->c.const_type == CONS_ABSOLUTE ) {
        AddSWData( op, opnd->c.lo.int_value, type_class );
    } else {
        switch( type_class ) {
        case U2:
        case I2:
        case U4:
        case I4:
            DoRelocConst( opnd, type_class );
            break;
        default:
            Zoiks( ZOIKS_045 );
            break;
        }
    }
}


void    AddWData( signed_32 value, type_class_def type_class )
/************************************************************/
{
    AddByte( value );
    if( type_class == U1 || type_class == I1 )
        return;
    value >>= 8;
    AddByte( value );
    if( type_class == U2 || type_class == I2 )
        return;
    value >>= 8;
    AddByte( value );
    value >>= 8;
    AddByte( value );
}

void    AddWCons( name *op, type_class_def type_class )
/************************************************
    Add a WORD constant to Inst[]
*/
{
    if( op->c.const_type == CONS_ABSOLUTE ) {
        AddWData( op->c.lo.int_value, type_class );
    } else  {
        switch( type_class ) {
        case U2:
        case I2:
        case U4:
        case I4:
            DoRelocConst( op, type_class );
            break;
        default:
            Zoiks( ZOIKS_045 );
            break;
        }
    }
}

void    AddSData( signed_32 value, type_class_def type_class )
/*****************************************************************
    Add a constant (signed_32) to Inst[], with possible sign extension
*/
{
    if( ( type_class == U2 || type_class == I2 )
        && ( ( value & 0xff80 ) == 0xff80
          || ( value & 0xff80 ) == 0 ) ) {
        Inst[KEY] |= B_KEY_S;
        AddByte( _IntToByte( value ) );
    } else if( ( type_class == U4 || type_class == I4 )
        && ( ( value & 0xffffff80 ) == 0xffffff80
          || ( value & 0xffffff80 ) == 0 ) ) {
        Inst[KEY] |= B_KEY_S;
        AddByte( _IntToByte( value ) );
    } else {
        AddWData( value, type_class );
    }
}

static  void    AddSCons( name *op, type_class_def type_class )
/**********************************************************
    Add a constant (name *) to Inst[], with possible sign extension
*/
{
    if( op->c.const_type == CONS_ABSOLUTE ) {
        AddSData( op->c.lo.int_value, type_class );
    } else {
        switch( type_class ) {
        case U2:
        case I2:
        case U4:
        case I4:
            DoRelocConst( op, type_class );
            break;
        default:
            Zoiks( ZOIKS_045 );
            break;
        }
    }
}


static  void    GenRegOp( hw_reg_set dst, type_length value, gen_opcode op )
/**************************************************************************/
{
    type_class_def      type_class;

    _Code;
    LayOpword( op | B_KEY_W );
    type_class = OpndSize( dst );
    LayRegRM( dst );
    AddSData( value, type_class );
    _Emit;
}


void    GenRegAdd( hw_reg_set dst, type_length value )
/*****************************************************
    ADD dst,value
*/
{
    GenRegOp( dst, value, M_ADDRC );
}


void    GenRegSub( hw_reg_set dst, type_length value )
/*****************************************************
    SUB         dst,value
*/
{
    GenRegOp( dst, value, M_SUBRC );
}


void    GenRegAnd( hw_reg_set dst, type_length value )
/*****************************************************
    SUB         dst,value
*/
{
    GenRegOp( dst, value, M_ANDRC );
}


static  void    GFld1( void ) {
/******************************
    FLD1
*/

    GCondFwait();
    LayOpword( 0xe8d9 );
    _Emit;
}


void    GFldMorC( name *what )
/***************************************
    FLD         what, where what is either 0,1,or a memory location
*/
{
    if( what->n.class == N_MEMORY || what->n.class == N_TEMP ) {
        GFldM( what );
    } else if( what->c.lo.int_value == 0 ) {
        GFldz();
    } else {
        GFld1();
    }
}

void    GFldM( pointer what )
/**************************************
    FLD         tbyte ptr what
*/
{
    GCondFwait();
    LayOpword( 0x00d9 );
    LayMF( what );
    LayModRM( what );
    _Emit;
}


void    GFstpM( pointer what )
/*****************************
    FSTP        tbyte ptr what
*/
{
    GCondFwait();
    LayOpword( 0x18d9 );
    LayMF( what );
    LayModRM( what );
    _Emit;
}


void    GFstp( int i )
/*********************
    FSTP        ST(i)
*/
{
    GCondFwait();
    LayOpword( 0xd8dd );
    Inst[RMR] |= i;
    _Emit;
}


void    GFxch( int i )
/*********************
    FXCH        ST(i)
*/
{
    GCondFwait();
    LayOpword( 0xc8d9 );
    Inst[RMR] |= i;
    _Emit;
}


void    GFldz( void )
/********************
    FLDZ
*/
{
    GCondFwait();
    LayOpword( 0xeed9 );
    _Emit;
}


void    GFld( int i )
/********************
    FLD         ST(i)
*/
{
    GCondFwait();
    LayOpword( 0xc0d9 );
    Inst[RMR] |= i;
    _Emit;
}


void    GCondFwait( void )
/**********************************
    FWAIT, but only if we need one
*/
{
    _Code;
    Used87 = true;
#if _TARGET & _TARG_8086
    if( !_CPULevel( CPU_286 ) || _IsEmulation() ) {
        if( _IsEmulation() ) {
            SetFPPatchType( FPP_NORMAL );
        }
        LayOpbyte( 0x9b );
        _Next;
    }
#endif
}


void    GFwait( void )
/*********************
    FWAIT
*/
{
    if( _CPULevel( CPU_386 ) )
        return;
    _Code;
    Used87 = true;
    if( _IsEmulation() ) {
        SetFPPatchType( FPP_WAIT );
        LayOpword( 0x9b90 );
    } else {
        LayOpbyte( 0x9b );
    }
   _Emit;
}

void    Gpusha( void )
/*********************
    PUSHA{d}
*/
{
    _Code;
    LayOpbyte( 0x60 );
   _Emit;
}

void    Gpopa( void )
/********************
    POPA{d}
*/
{
    _Code;
    LayOpbyte( 0x61 );
   _Emit;
}

void    Gcld( void )
/*******************
    CLD
*/
{
    _Code;
    LayOpbyte( 0xfc );
   _Emit;
}

void    GenLeave( void )
/***********************
    LEAVE
*/
{
    _Code;
    LayOpbyte( 0xc9 );
    OpndSize( HW_xBP );
    _Emit;
}


void    GenTouchStack( bool sp_might_point_at_something )
/********************************************************
    MOV         [esp],eax
*/
{
#if _TARGET & _TARG_8086
    /* unused parameters */ (void)sp_might_point_at_something;
#else
    if( sp_might_point_at_something || OptForSize == 100 ) {
        QuickSave( HW_xAX, OP_PUSH );
        QuickSave( HW_xAX, OP_POP );
    } else {
        _Code;
        LayOpword( 0x0489 );    /*  mov dword ptr [esp],eax */
        OpndSize( HW_xSP );     /*  ..                      */
        AddByte( 0x24 );        /*  ..                      */
        _Emit;
    }
#endif
}


void    GenEnter( int size, int level )
/**************************************
    ENTER       size,level
*/
{
    _Code;
    LayOpbyte( 0xc8 );
    OpndSize( HW_xBP );
    AddWData( size, U2 );
    AddByte( level );
    _Emit;
}



void    GenPushOffset( byte offset )
/***********************************
    PUSH        word ptr offset[BP]
*/
{
    _Code;
    LayOpword( M_PUSHATBP );
    AddWData( offset, I1 );
    _Emit;
}


void    GenUnkSub( hw_reg_set dst, pointer value )
/*************************************************
    SUB         dst,??? - to be patched later
*/
{
    _Code;
    LayOpword( M_SUBRC | B_KEY_W );
    OpndSize( dst );
    LayRegRM( dst );
    ILen += WORD_SIZE;
    DoAbsPatch( value, WORD_SIZE );
    _Emit;
}


void    GenUnkMov( hw_reg_set dst, pointer value )
/*************************************************
    MOV         dst,??? - to be patched
*/
{
    /* unused parameters */ (void)dst;

    _Code;
    LayOpbyte( 0xb8 );
    OpndSize( dst );
    ILen += WORD_SIZE;
    DoAbsPatch( value, WORD_SIZE );
    _Emit;
}


void    GenUnkEnter( pointer value, int level )
/**********************************************
    ENTER       ??,level - to be patched later
*/
{
    _Code;
    LayOpbyte( 0xc8 );
    DoAbsPatch( value, 2 );
    ILen += 2;
    AddByte( level );
    _Emit;
}

void    GenWindowsProlog( void )
/******************************/
{
    _Code;
    if( _IsTargetModel( SMART_WINDOWS ) ) {
        LayOpbyte( 0x8c );          /*  mov     [e]ax, ss   */
        AddByte( 0xd0 );            /*  ..                  */
    } else {
        LayOpbyte( 0x1e );          /*  push    ds          */
        AddByte( 0x58 );            /*  pop     [e]ax       */
        AddByte( 0x90 );            /*  nop                 */
    }
    if( !_CPULevel( CPU_386 ) ) {
        AddByte( 0x45 );            /*  inc     [e]bp       */
    }
    AddByte( 0x55 );                /*  push    [e]bp       */
    AddByte( 0x89 );                /*  mov     [e]bp,[e]sp */
    AddByte( 0xe5 );                /*  ..                  */
    AddByte( 0x1e );                /*  push    ds          */
    AddByte( 0x8e );                /*  mov     ds,[e]ax    */
    AddByte( 0xd8 );                /*  ..                  */
    _Emit;
}

void    GenCypWindowsProlog( void )
/************************************
    Generate a "cheap" windows prolog
*/
{
    _Code;
    if( !_CPULevel( CPU_386 ) ) {
        LayOpbyte( 0x45 );          /*  inc     [e]bp       */
    }
    AddByte( 0x55 );                /*  push    [e]bp       */
    AddByte( 0x89 );                /*  mov     [e]bp,[e]sp */
    AddByte( 0xe5 );                /*  ..                  */
    _Emit;
}

void    GenWindowsEpilog( void )
/******************************/
{
    _Code;
    LayOpbyte( 0x1f );              /*  pop     ds          */
    AddByte( 0x5d );                /*  pop     [e]bp       */
    if( !_CPULevel( CPU_386 ) ) {
        AddByte( 0x4d );            /*  dec     [e]bp       */
    }
    _Emit;
}

void    GenCypWindowsEpilog( void )
/************************************
    Generate a "cheap" windows epilog
*/
{
    _Code;
    LayOpbyte( 0x5d );              /*  pop     [e]bp       */
    if( !_CPULevel( CPU_386 ) ) {
        AddByte( 0x4d );            /*  dec     [e]bp       */
    }
    _Emit;
}


void    GenRdosdevProlog( void )
/******************************/
{
    _Code;
    LayOpbyte( 0x1e );              /*  push    ds        */
    _Emit;

    _Code;
    LayOpbyte( 0x6 );               /*  push    es        */
    _Emit;

    _Code;
    LayOpbyte( 0xf );               /*  push    fs        */
    AddByte( 0xa0 );
    _Emit;

    _Code;
    LayOpbyte( 0xf );               /*  push    gs        */
    AddByte( 0xa8 );
    _Emit;

    _Code;
    LayOpbyte( 0x68 );              /*  push    DGROUP    */
    ILen += WORD_SIZE;
    DoSegRef( AskBackSeg() );
    AddByte( 0x0 );
    AddByte( 0x0 );
    AddByte( 0x1F );                /*  pop     ds        */
    _Emit;
}

void    GenRdosdevEpilog( void )
/******************************/
{
    _Code;
    LayOpbyte( 0xf );               /*  pop     gs      */
    AddByte( 0xa9 );
    _Emit;

    _Code;
    LayOpbyte( 0xf );               /*  pop     fs      */
    AddByte( 0xa1 );
    _Emit;

    _Code;
    LayOpbyte( 0x7 );               /*  pop     es      */
    _Emit;

    _Code;
    LayOpbyte( 0x1f );              /*  pop     ds      */
    _Emit;
}

void    GenLoadDS( void )
/***********************/
{
    _Code;
    LayOpbyte( 0xb8 );              /*  mov     [e]ax,DGROUP */
    OpndSize( HW_xAX );
    ILen += WORD_SIZE;
    DoSegRef( AskBackSeg() );
    AddByte( 0x8e );                /*  mov     ds,[e]ax   */
    AddByte( 0xd8 );                /*  ..                 */
    _Emit;
}


static  void    OutputFP( gen_opcode op ) {
/******************************************
*/

    GCondFwait();
    LayOpword( op );
}

static  void    MathFunc( instruction *ins ) {
/*********************************************
    Generate inline code for a math function instruction
*/

    switch( ins->head.opcode ) {
    case OP_EXP:
        OutputFP( 0xE8D9 ); _Emit;  /*   FLD1           */
        OutputFP( 0xEAD9 ); _Emit;  /*   FLDL2E         */
        OutputFP( 0xCAD8 ); _Emit;  /*   FMUL ST,ST(2)  */
        OutputFP( 0xD2DD ); _Emit;  /*   FST  ST(2)     */
        OutputFP( 0xF8D9 ); _Emit;  /*   FPREM          */
        OutputFP( 0xF0D9 ); _Emit;  /*   F2XM1          */
        OutputFP( 0xC1DE ); _Emit;  /*   FADDP ST(1),ST */
        OutputFP( 0xFDD9 ); _Emit;  /*   FSCALE         */
        OutputFP( 0xD9DD );         /*   FSTP ST(1)     */
        break;
    case OP_LOG:
        OutputFP( 0xEDD9 ); _Emit;  /*   FLDLN2         */
        OutputFP( 0xC9D9 ); _Emit;  /*   FXCH    ST(1)  */
        OutputFP( 0xF1D9 );         /*   FYL2X          */
        break;
    case OP_LOG10:
        OutputFP( 0xECD9 ); _Emit;  /*   FLDLG2         */
        OutputFP( 0xC9D9 ); _Emit;  /*   FXCH    ST(1)  */
        OutputFP( 0xF1D9 );         /*   FYL2X          */
        break;
    case OP_COS:
        OutputFP( 0xffD9 );         /*   FCOS           */
        break;
    case OP_SIN:
        OutputFP( 0xfeD9 );         /*   FSIN           */
        break;
    case OP_TAN:
        OutputFP( 0xF2D9 ); _Emit;  /*   FPTAN          */
        OutputFP( 0xD8DD );         /*   FSTP    ST(0)  */
        break;
    case OP_ATAN:
        OutputFP( 0xE8D9 ); _Emit;  /*   FLD1           */
        OutputFP( 0xF3D9 );         /*   FPATAN         */
        break;
    case OP_SQRT:
        OutputFP( 0xFAD9 );         /*   FSQRT          */
        break;
    case OP_FABS:
        OutputFP( 0xE1D9 );         /*   FABS           */
        break;
    default:
        break;
    }
}

static  void    CallMathFunc( instruction *ins ) {
/*************************************************
    Call a runtime routine for a math function instructions
*/
    rt_class    rtindex;

    rtindex = LookupRoutine( ins );
    DoCall( RTLabel( rtindex ), true, _IsTargetModel( BIG_CODE ), false );
}

static void     GenUnkLea( pointer value )
/****************************************/
{
    LayOpword( M_LEA );
    OpndSize( HW_xSP );
    LayReg( HW_xSP );
    Inst[RMR] |= DISPW;
    ILen += WORD_SIZE;
    DoAbsPatch( value, WORD_SIZE );
    Inst[RMR] |= DoIndex( HW_xBP );
}

void    GenObjCode( instruction *ins ) {
/***************************************
    Generate object code for the instruction "ins" based on gen_table->generate
*/

    gentype     gen;
    name        *result;
    name        *left = NULL;
    name        *right = NULL;
    opcnt       i;
    bool        opnd_size;

    gen = G( ins );
    if( gen != G_NO ) {
#if 1
        // fixme - should be _IsTargetModel
        if( _IsTargetModel( P5_DIVIDE_CHECK ) ) {
            if( ins->head.opcode == OP_DIV && _IsFloating( ins->type_class ) ) {
                DoP5Divide( ins );
                return;
            }
        }
#endif
        result = ins->result;
        if( ins->num_operands != 0 ) {
            left = ins->operands[0];
            if( ins->num_operands != 1 ) {
                right = ins->operands[1];
            }
        }

        if( gen == G_MFSTRND ) {
            _Code;
#if ( _TARGET & _TARG_8086 )
            AddByte(0x6a);      /*  push    0x00            */
            AddByte(0x00);      /*  ..                      */
            AddByte(0x68);      /*  push    0x0c3f          */
            AddByte(0x3f);      /*  ..                      */
            AddByte(0x0c);      /*  ..                      */
            AddByte(0x55);      /*  push    bp              */
            AddByte(0x89);      /*  mov     bp,sp           */
            AddByte(0xe5);      /*  ..                      */
            AddByte(0xd9);      /*  fnstcw  word ptr [bp+4] */
            AddByte(0x7e);      /*  ..                      */
            AddByte(0x04);      /*  ..                      */
            AddByte(0xd9);      /*  fldcw   word ptr [bp+2] */
            AddByte(0x6e);      /*  ..                      */
            AddByte(0x02);      /*  ..                      */
            AddByte(0x5d);      /*  pop     bp              */
#else
            AddByte(0x68);      /*  push    0x00000c3f      */
            AddByte(0x3f);      /*  ..                      */
            AddByte(0x0c);      /*  ..                      */
            AddByte(0x00);      /*  ..                      */
            AddByte(0x00);      /*  ..                      */
            AddByte(0xd9);      /*  fnstcw  word ptr [esp+2]*/
            AddByte(0x7c);      /*  ..                      */
            AddByte(0x24);      /*  ..                      */
            AddByte(0x02);      /*  ..                      */
            AddByte(0xd9);      /*  fldcw   word ptr [esp]  */
            AddByte(0x2c);      /*  ..                      */
            AddByte(0x24);      /*  ..                      */
#endif
            _Emit;

#if ( _TARGET & _TARG_8086 )
            AdjustStackDepthDirect( 2 * WORD_SIZE );
#else
            AdjustStackDepthDirect( WORD_SIZE );
#endif
        }

        _Code;
        LayInitial( ins, gen );
        if( ins->num_operands > OpcodeNumOperands( ins ) ) {
            i = ins->num_operands - 1;
            if( ins->operands[i]->n.class == N_REGISTER ) {
                GenSeg( ins->operands[i]->r.reg );
            } else {
                _Zoiks( ZOIKS_027 );
            }
        }
        opnd_size = LayOpndSize( ins, gen );
        switch( gen ) {
        case G_RC:
            LayRMRegOp( left );
            AddSWCons( ins->head.opcode, right, ins->type_class );
            break;
        case G_AC:
            ICur--;
            ILen--;
            IEsc--;
            AddWCons( right, ins->type_class );
            break;
        case G_MC:
            LayModRM( left );
            AddSWCons( ins->head.opcode, right, ins->type_class );
            break;
        case G_RR2:
            LayRegOp( right );
            LayRMRegOp( left );
            break;
        case G_0FRR2:
            LayOpword( M_386MUL );
            LayRegOp( left );
            LayRMRegOp( right );
            AddToTemp( M_SECONDARY );
            break;
        case G_0FRM2:
            LayOpword( M_386MUL );
            LayModRM( right );
            LayRegOp( left );
            AddToTemp( M_SECONDARY );
            break;
        case G_PUSHFS:
            LayOpbyte( M_PUSHI );
            AddSCons( IntEquivalent( left ), U4 );
            break;
        case G_RM2:
            LayModRM( right );
            LayRegOp( left );
            break;
        case G_MR2:
            LayModRM( left );
            LayRegOp( right );
            break;
        case G_WORDR1:
            if( ins->head.opcode == OP_POP ) {
                LayACRegOp( result );
                break;
            }
            LayACRegOp( left );
            if( OpcodeNumOperands( ins ) != 1 && right->c.lo.int_value == 2 ) {
                /* never address*/
                TransferIns();
                if( opnd_size ) {
                    AddToTemp( M_OPND_SIZE );
                }
            }
            break;
        case G_R1:
            LayRMRegOp( left );
            if( ins->num_operands != 1 && right->c.lo.int_value == 2 ) {
                TransferIns();
                if( opnd_size ) {
                    AddToTemp( M_OPND_SIZE );
                }
            }
            break;
        case G_R1SHIFT:
        case G_RCLSHIFT:
            LayRMRegOp( left );
            break;
        case G_M1:
        case G_1SHIFT:
        case G_CLSHIFT:
            LayModRM( left );
            break;
        case G_RNSHIFT:
            LayRMRegOp( left );
            AddByte( right->c.lo.int_value ); /* never address */
            break;
        case G_NSHIFT:
            LayModRM( left );
            AddByte( right->c.lo.int_value ); /* never address */
            break;
        case G_R2:
            LayRMRegOp( right );
            break;
        case G_M2:
            LayModRM( right );
            break;
        case G_RR1:
            LayRegOp( left );
            LayRMRegOp( result );
            break;
        case G_RM1:
            LayRegOp( result );
            LayModRM( left );
            break;
        case G_MR1:
            LayRegOp( left );
            LayModRM( result );
            break;
        case G_LEA:
            LayRegOp( result );
            if( left->n.class == N_REGISTER ) {  /* add/sub transformed to LEA */
                LayLeaRegOp( ins );
            } else {
                LayModRM( left );
            }
            break;
        case G_LDSES:
            LayRegOp( result );
            LayModRM( left );
            if( HW_COvlap( result->r.reg, HW_SS ) ) {
                AddToTemp( M_SECONDARY );   /* load SS */
                break;
            }
            if( HW_COvlap( result->r.reg, HW_FS_GS ) ) {
                Inst[KEY] += B_KEY_FSGS;    /* load FS or GS */
                AddToTemp( M_SECONDARY );
            } else {
                Inst[KEY] += B_KEY_DSES;    /* load DS or ES */
            }
            if( HW_COvlap( result->r.reg, HW_DS_GS ) ) {
                Inst[KEY] += 1;             /* indicate load to DS and GS */
            }
            break;
        case G_MS1:
            LaySROp( left );
            LayModRM( result );
            break;
        case G_SM1:
            LaySROp( result );
            LayModRM( left );
            break;
        case G_RS:
            LaySROp( left );
            LayRMRegOp( result );
            break;
        case G_SR:
            LaySROp( result );
            LayRMRegOp( left );
            break;
        case G_MOVMC:
            LayW( ins->type_class );
            LayModRM( result );
            AddWCons( left, ins->type_class );
            break;
        case G_MADDR:
            LayModRM( result );
            DoMAddr( left );
            break;
        case G_TEST:
            LayW( ins->type_class );
            LayRegOp( left );
            LayRMRegOp( left );
            break;
        case G_SEGR1:
            {
                uint    ins_loc;
                byte    extra_bits;
                byte    reg_index;

                ins_loc = KEY;
                if( ins->head.opcode == OP_POP ) {
                    extra_bits = B_KEY_POPSEG;
                    reg_index = SegTrans( result->r.reg );
                } else {
                    extra_bits = 0;
                    reg_index = SegTrans( left->r.reg );
                }
                if( reg_index > 3 ) {
                    ins_loc += 1;
                    Inst[KEY] = 0x0f;
                    AddByte( 0x80 );
                }
                Inst[ins_loc] |= extra_bits;
                Inst[ins_loc] |= reg_index << S_KEY_SR;
            }
            break;
        case G_MOVAM:
            LayW( ins->type_class );
            DoMDisp( left, false );
            break;
        case G_MOVMA:
            LayW( ins->type_class );
            DoMDisp( result, false );
            break;
        case G_MOVRC:
            if( ins->type_class == U2 || ins->type_class == I2
             || ins->type_class == U4 || ins->type_class == I4
             || ins->type_class == FS ) {
                Inst[KEY] |= B_KEY_AW;
            }
            LayACRegOp( result );
            AddWCons( left, ins->type_class );
            break;
        case G_RADDR:
            LayACRegOp( result );
            DoMAddr( left );
            break;
        case G_REPOP:
            DoRepOp( ins );
            break;
        case G_LOADADDR:
            if( AskIsFrameIndex( left ) ) {
                GenUnkLea( NextFramePatch() );
                break;
            }
#if _TARGET & _TARG_80386
            if( ( left->n.class == N_TEMP ) && TmpLoc( DeAlias( left ), left ) == 0 ) {
                /* turn "LEA <reg>,[ESP+0]" into "MOV <reg>,ESP"
                 * or "LEA <reg>,[EBP+0]" into "MOV <reg>,EBP"
                 */
                LayOpword( M_MOVRR | B_KEY_W );
                if( BaseIsSP( left ) ) {
                    LayReg( HW_xSP );
                } else {
                    LayReg( HW_xBP );
                }
                LayRMRegOp( result );
                break;
            }
#endif
            LayOpword( M_LEA );
            LayRegOp( result );
            LayModRM( left );
            break;
        case G_4SHIFT:
            Do4Shift( ins );
            break;
        case G_4RSHIFT:
            Do4RShift( ins );
            break;
        case G_CXSHIFT:
            Do4CXShift( ins, &Do4Shift );
            break;
        case G_RCXSHIFT:
            Do4CXShift( ins, &Do4RShift );
            break;
        case G_4NEG:
            Gen4Neg( ins );
            break;
        case G_4RNEG:
            Gen4RNeg( ins );
            break;
        case G_ENTER:
            /*   do nothing, prolog should already be generated*/
            break;
        case G_CALL:
            GenCall( ins );
            AdjustStackDepth( ins );
            if( _IsTargetModel( NEW_P5_PROFILING ) ) {
                label_handle    lbl;
                segment_id      segid;

                segid = GenProfileData( "", &lbl, &CurrProc->targ.routine_profile_data );
                _Code;
                LayOpword( 0xc4f7 );                /* test [e]sp, offset L1  */
                ILen += WORD_SIZE;                  /* ..                     */
                DoLblRef( lbl, segid, 0, OFST );    /* ..                     */
                _Emit;
            }
            return;
        case G_ICALL:
            if( ins->operands[CALL_OP_ADDR]->n.class == N_REGISTER ) {
                GenRCall( ins );
            } else {
                GenICall( ins );
            }
            AdjustStackDepth( ins );
            if( _IsTargetModel( NEW_P5_PROFILING ) ) {
                label_handle    lbl;
                label_handle    junk;
                segment_id      segid;
                char            c[2];

                segid = GenProfileData( "", &lbl, &CurrProc->targ.routine_profile_data );
                GenProfileData( "", &junk, &CurrProc->targ.routine_profile_data );
                c[0] = PROFILE_FLAG_END_GROUP;
                c[1] = 0;
                GenProfileData( c, &junk, &CurrProc->targ.routine_profile_data );
                _Code;
                LayOpword( 0xc5f7 );                /* test [e]bp, offset L1  */
                ILen += WORD_SIZE;                  /* ..                     */
                DoLblRef( lbl, segid, 0, OFST );    /* ..                     */
                _Emit;
            }
            return;
        case G_RJMP:
            GenRJmp( ins );
            AdjustStackDepth( ins );
            return;
        case G_MJMP:
            GenMJmp( ins );
            break;
        case G_SIGNEX:
            switch( ins->type_class ) {
#if _TARGET & _TARG_80386
            case U8:
            case I8:
                LayOpbyte( M_CWD );
                if( _IsntTargetModel( USE_32 ) )
                    AddToTemp( M_OPND_SIZE );
                break;
#endif
            case U4:
            case I4:
#if _TARGET & _TARG_8086
                LayOpbyte( M_CWD );
#elif _TARGET & _TARG_80386
                switch( ins->base_type_class ) {
                case U1:
                case I1:
                    if( _IsTargetModel( USE_32 ) )
                        AddToTemp( M_OPND_SIZE );
                    AddToTemp( M_CBW );
                    break;
                default:
                    break;
                }
                if( HW_CEqual( ins->result->r.reg, HW_DX_AX ) ) {
                    LayOpbyte( M_CWD );
                    if( _IsTargetModel( USE_32 ) ) {
                        AddToTemp( M_OPND_SIZE );
                    }
                } else {
                    LayOpbyte( M_CBW );
                    if( _IsntTargetModel( USE_32 ) ) {
                        AddToTemp( M_OPND_SIZE ); /* CWDE */
                    }
                }
#endif
                break;
            case U2:
            case I2:
                LayOpbyte( M_CBW );
                if( _IsTargetModel( USE_32 ) )
                    AddToTemp( M_OPND_SIZE );
                break;
            default:
                break;
            }
            break;
        case G_C1:
            AddSCons( left, ins->type_class );
            break;
        case G_186RMUL:
            LayOpword( 0xc069 );
            LayRegOp( result );
            LayRMRegOp( left );
            AddSCons( right, ins->type_class );
            break;
        case G_186MUL:
            LayOpword( 0x0069 );
            LayRegOp( result );
            LayModRM( left );
            AddSCons( right, ins->type_class );
            break;
        case G_MRFBIN:
        case G_MNFBIN:
        case G_MFLD:
            LayMF( left );
            LayModRM( left );
            break;
        case G_RFLD:
        case G_RRFBIN:
        case G_RRFBINP:
        case G_RRFBIND:
        case G_RNFBIN:
        case G_RNFBINP:
        case G_RNFBIND:
            LayST( left );
            break;
        case G_RFST:
        case G_RFSTNP:
            LayST( result );
            break;
        case G_MFST:
        case G_MFSTNP:
        case G_MFSTRND:
            LayMF( result );
            LayModRM( result );
            break;
        case G_FCHS:
        case G_FLD1:
        case G_FLDZ:
        case G_FINIT:
            break;
        case G_IFUNC:
            CallMathFunc( ins );
            break;
        case G_FMATH:
            MathFunc( ins );
            break;
        case G_FSINCOS:
            GCondFwait();
            LayOpword( 0xFBD9 );
            break;
        case G_FCHOP:
            DoCall( RTLabel( RT_CHOP ), true, _IsTargetModel( BIG_CODE ), false );
            break;
        case G_FTST:
        case G_FCOMPP:
            Used87 = true;
            SetCC();
            break;
        case G_FWAIT:
            Used87 = true;
            if( _IsEmulation() ) {
                SetFPPatchType( FPP_WAIT );
                LayOpword( 0x9b90 );
            } else {
                LayOpbyte( 0x9b );
            }
            break;
        case G_FXCH:
            Used87 = true;
            GCondFwait();
            LayOpword( 0xC8D9 );    /* fxch st(1) */
            LayST( result );
            break;
        case G_MCOMP:
            LayMF( left );
            LayModRM( left );
            SetCC();
            break;
        case G_RCOMP:
            LayST( left );
            SetCC();
            break;
        case G_DEBUG:
            EmitDbgInfo( ins );
            break;
        case G_SEG_SEG:
            PushSeg( left->r.reg );
            _Next;
            PopSeg( result->r.reg );
            break;
        case G_POW2DIV:
            Pow2Div( ins );
            break;
        case G_DIV2:
            By2Div( ins );
            break;
#if _TARGET & _TARG_8086
        case G_POW2DIV_286:
            Pow2Div286( ins );
            break;
#endif
        case G_MOVSX:
        case G_MOVZX:
            if( gen == G_MOVSX ) {
                LayOpword( M_MOVSX );
            } else {
                LayOpword( M_MOVZX );
            }
#if _TARGET & _TARG_80386
            if( ins->operands[0]->n.size == 2 ) {
                Inst[KEY] |= B_KEY_W;
            } else { /* base is byte */
                switch( ins->type_class ) {
                case U2:
                case I2:
                    if( _IsTargetModel( USE_32 ) )
                        AddToTemp( M_OPND_SIZE );
                    break;
                case U4:
                case I4:
                    if( _IsntTargetModel( USE_32 ) )
                        AddToTemp( M_OPND_SIZE );
                    break;
                default:
                    break;
                }
            }
#endif
            LayModRM( left );
            LayRegOp( result );
            AddToTemp( M_SECONDARY );
            break;
        case G_UNKNOWN:
            _Zoiks( ZOIKS_097 );
            break;
        default:
            _Zoiks( ZOIKS_028 );
            break;
        }
        _Emit;
        if( gen == G_MFSTRND ) {
#if ( _TARGET & _TARG_8086 )
            AdjustStackDepthDirect( -2 * WORD_SIZE );
#else
            AdjustStackDepthDirect( -WORD_SIZE );
#endif

            _Code;
#if ( _TARGET & _TARG_8086 )
            AddByte(0x55);      /*  push    bp              */
            AddByte(0x89);      /*  mov     bp,sp           */
            AddByte(0xe5);      /*  ..                      */
            AddByte(0xd9);      /*  fldcw   word ptr [bp+4] */
            AddByte(0x6e);      /*  ..                      */
            AddByte(0x04);      /*  ..                      */
            AddByte(0x5d);      /*  pop     bp              */
            AddByte(0x83);      /*  add     sp,4            */
            AddByte(0xc4);      /*  ..                      */
            AddByte(0x04);      /*  ..                      */
#else
            AddByte(0xd9);      /*  fldcw   word ptr [esp+2]*/
            AddByte(0x6c);      /*  ..                      */
            AddByte(0x24);      /*  ..                      */
            AddByte(0x02);      /*  ..                      */
            AddByte(0x8d);      /*  lea     esp,[esp+4]     */
            AddByte(0x64);      /*  ..                      */
            AddByte(0x24);      /*  ..                      */
            AddByte(0x04);      /*  ..                      */
#endif
            _Emit;
        }
    }
    AdjustStackDepth( ins );
    if( _OpIsCondition( ins->head.opcode ) ) {
        EndBlockProfiling();
        GenCondJump( ins );
    }
}
