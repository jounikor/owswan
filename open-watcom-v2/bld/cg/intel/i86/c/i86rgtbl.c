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
* Description:  Register usage table for i86.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "regset.h"
#include "model.h"
#include "i87data.h"
#include "rgtbl.h"


/* some short forms*/
#define RL_DP   RL_DBL_OR_PTR
#define RL_WS   RL_ANYWORD
#define RL_2B   RL_TWOBYTE
#define RL_IX   RL_INDEX
#define RL_TI   RL_TEMP_INDEX
#define RL_WD   RL_WORD
#define RL_SG   RL_SEG

#define HW_SEG  (HW_DS + HW_ES + HW_SS + HW_CS + HW_FS + HW_GS)
#define HW_D1   (HW_DX + HW_AX);        /* double register on*/
#define HW_D2   (HW_CX + HW_BX);        /* double register tw*/

/*       Note : 32 bit registers are composed of two 16 bit registers*/
/*       Each 16 bit register is used only as a high or a low register,*/
/*       never both. Index register never high.*/
/**/
/*       High                    Low*/
/*       CS,SS,ES,DS,CX,DX       AX,BX,SI,DI*/

/*
 * Note: MAX_RG in "rg.h" must be not less then number of elements
 *       in longest of these tables!
 */

static  hw_reg_set      Reg32Order[] = {
    HW_D_1( HW_AX ),
    HW_D_1( HW_BX ),
    HW_D_1( HW_SI ),
    HW_D_1( HW_DI ),
    HW_D_1( HW_DX ),
    HW_D_1( HW_CX ),
    HW_D_1( HW_SP ),
    HW_D_1( HW_BP ),
    HW_D_1( HW_SEGS ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      Empty[] = {
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      ALReg[] = {
    HW_D_1( HW_AL ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      AHReg[] = {
    HW_D_1( HW_AH ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      AXReg[] = {
    HW_D_1( HW_AX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      CLReg[] = {
    HW_D_1( HW_CL ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      CXReg[] = {
    HW_D_1( HW_CX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      DXReg[] = {
    HW_D_1( HW_DX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      DXAXReg[] = {
    HW_D_2( HW_DX, HW_AX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      CXBXReg[] = {
    HW_D_2( HW_CX, HW_BX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      ESBXReg[] = {
    HW_D_2( HW_ES, HW_BX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      ByteRegs[] = {
    HW_D_1( HW_AL ),
    HW_D_1( HW_AH ),
    HW_D_1( HW_DL ),
    HW_D_1( HW_DH ),
    HW_D_1( HW_BL ),
    HW_D_1( HW_BH ),
    HW_D_1( HW_CL ),
    HW_D_1( HW_CH ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      LowByteRegs[] = {
    HW_D_1( HW_AL ),
    HW_D_1( HW_DL ),
    HW_D_1( HW_BL ),
    HW_D_1( HW_CL ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      WordOrSegReg[] = {
    HW_D_1( HW_AX ),
    HW_D_1( HW_DX ),
    HW_D_1( HW_BX ),
    HW_D_1( HW_CX ),
    HW_D_1( HW_SI ),
    HW_D_1( HW_DI ),
    HW_D_1( HW_BP ),
    HW_D_1( HW_SP ),
    HW_D_1( HW_DS ),
    HW_D_1( HW_ES ),
    HW_D_1( HW_FS ),
    HW_D_1( HW_GS ),
    HW_D_1( HW_CS ),
    HW_D_1( HW_SS ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      WordRegs[] = {
    HW_D_1( HW_AX ),
    HW_D_1( HW_DX ),
    HW_D_1( HW_BX ),
    HW_D_1( HW_CX ),
    HW_D_1( HW_SI ),
    HW_D_1( HW_DI ),
    HW_D_1( HW_BP ),
    HW_D_1( HW_SP ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      TwoByteRegs[] = {
    HW_D_1( HW_AX ),
    HW_D_1( HW_DX ),
    HW_D_1( HW_BX ),
    HW_D_1( HW_CX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      IndexRegs[] = {
    HW_D_1( HW_BX ),
    HW_D_1( HW_SI ),
    HW_D_1( HW_DI ),
    HW_D_1( HW_BP ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      SegRegs[] = {
    HW_D_1( HW_DS ),
    HW_D_1( HW_ES ),
    HW_D_1( HW_FS ),
    HW_D_1( HW_GS ),
    HW_D_1( HW_CS ),
    HW_D_1( HW_SS ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      LongIndexRegs[] = {
    HW_D_2( HW_DS, HW_SI ),
    HW_D_2( HW_DS, HW_DI ),
    HW_D_2( HW_DS, HW_BX ),
    HW_D_2( HW_ES, HW_DI ),
    HW_D_2( HW_ES, HW_SI ),
    HW_D_2( HW_ES, HW_BX ),
    HW_D_2( HW_FS, HW_DI ),
    HW_D_2( HW_FS, HW_SI ),
    HW_D_2( HW_FS, HW_BX ),
    HW_D_2( HW_GS, HW_DI ),
    HW_D_2( HW_GS, HW_SI ),
    HW_D_2( HW_GS, HW_BX ),
    HW_D_2( HW_SS, HW_DI ),
    HW_D_2( HW_SS, HW_SI ),
    HW_D_2( HW_SS, HW_BX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      DoubleRegs[] = {
    HW_D_2( HW_DX, HW_AX ),
    HW_D_2( HW_CX, HW_BX ),
    HW_D_2( HW_CX, HW_AX ),
    HW_D_2( HW_CX, HW_SI ),
    HW_D_2( HW_DX, HW_BX ),
    HW_D_2( HW_DI, HW_AX ),
    HW_D_2( HW_CX, HW_DI ),
    HW_D_2( HW_DX, HW_SI ),
    HW_D_2( HW_DI, HW_BX ),
    HW_D_2( HW_SI, HW_AX ),
    HW_D_2( HW_CX, HW_DX ),
    HW_D_2( HW_DX, HW_DI ),
    HW_D_2( HW_DI, HW_SI ),
    HW_D_2( HW_SI, HW_BX ),
    HW_D_2( HW_BX, HW_AX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      DblPtrRegs[] = {
    HW_D_2( HW_DX, HW_AX ),
    HW_D_2( HW_CX, HW_BX ),
    HW_D_2( HW_CX, HW_AX ),
    HW_D_2( HW_CX, HW_SI ),
    HW_D_2( HW_DX, HW_BX ),
    HW_D_2( HW_DI, HW_AX ),
    HW_D_2( HW_CX, HW_DI ),
    HW_D_2( HW_DX, HW_SI ),
    HW_D_2( HW_DI, HW_BX ),
    HW_D_2( HW_SI, HW_AX ),
    HW_D_2( HW_CX, HW_DX ),
    HW_D_2( HW_DX, HW_DI ),
    HW_D_2( HW_DI, HW_SI ),
    HW_D_2( HW_SI, HW_BX ),
    HW_D_2( HW_BX, HW_AX ),
    HW_D_2( HW_DS, HW_CX ),
    HW_D_2( HW_DS, HW_DX ),
    HW_D_2( HW_DS, HW_DI ),
    HW_D_2( HW_DS, HW_SI ),
    HW_D_2( HW_DS, HW_BX ),
    HW_D_2( HW_DS, HW_AX ),
    HW_D_2( HW_ES, HW_CX ),
    HW_D_2( HW_ES, HW_DX ),
    HW_D_2( HW_ES, HW_DI ),
    HW_D_2( HW_ES, HW_SI ),
    HW_D_2( HW_ES, HW_BX ),
    HW_D_2( HW_ES, HW_AX ),
    HW_D_2( HW_FS, HW_CX ),
    HW_D_2( HW_FS, HW_DX ),
    HW_D_2( HW_FS, HW_DI ),
    HW_D_2( HW_FS, HW_SI ),
    HW_D_2( HW_FS, HW_BX ),
    HW_D_2( HW_FS, HW_AX ),
    HW_D_2( HW_GS, HW_CX ),
    HW_D_2( HW_GS, HW_DX ),
    HW_D_2( HW_GS, HW_DI ),
    HW_D_2( HW_GS, HW_SI ),
    HW_D_2( HW_GS, HW_BX ),
    HW_D_2( HW_GS, HW_AX ),
    HW_D_2( HW_SS, HW_CX ),
    HW_D_2( HW_SS, HW_DX ),
    HW_D_2( HW_SS, HW_DI ),
    HW_D_2( HW_SS, HW_SI ),
    HW_D_2( HW_SS, HW_BX ),
    HW_D_2( HW_SS, HW_AX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      QuadReg[] = {
    HW_D_4( HW_AX, HW_BX, HW_CX, HW_DX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      TempIndex[] = {
    HW_D_1( HW_SI ),
    HW_D_1( HW_DI ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      ST0Reg[] = {
    HW_D_1( HW_ST0 ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      STIReg[] = {
    HW_D_1( HW_ST1 ),
    HW_D_1( HW_ST2 ),
    HW_D_1( HW_ST3 ),
    HW_D_1( HW_ST4 ),
    HW_D_1( HW_ST5 ),
    HW_D_1( HW_ST6 ),
    HW_D_1( HW_ST7 ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      STParmReg[] = {
    HW_D_1( HW_ST1 ),
    HW_D_1( HW_ST2 ),
    HW_D_1( HW_ST3 ),
    HW_D_1( HW_ST4 ),
    HW_D_1( HW_ST5 ),
    HW_D_1( HW_ST6 ),
    HW_D_1( HW_ST7 ),
    HW_D_1( HW_EMPTY ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      EEch[] = {
    HW_D_3( HW_DX, HW_AX, HW_CX ),
    HW_D_3( HW_DX, HW_BX, HW_CX ),
    HW_D_3( HW_DI, HW_AX, HW_CX ),
    HW_D_3( HW_DX, HW_SI, HW_CX ),
    HW_D_3( HW_DI, HW_BX, HW_CX ),
    HW_D_3( HW_SI, HW_AX, HW_CX ),
    HW_D_3( HW_DX, HW_DI, HW_CX ),
    HW_D_3( HW_DI, HW_SI, HW_CX ),
    HW_D_3( HW_SI, HW_BX, HW_CX ),
    HW_D_3( HW_BX, HW_AX, HW_CX ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      DXCLReg[] = {
    HW_D_2( HW_DX, HW_CL ),
    HW_D_1( HW_EMPTY )
};
static  hw_reg_set      AHCLReg[] = {
    HW_D_2( HW_AH, HW_CL ),
    HW_D_1( HW_EMPTY )
};

hw_reg_set *RegSets[] = {
    #define RL(a,b,c,d) a
    #include "rl.h"
    #undef RL
    NULL
};

op_regs RegList[] = {
    #define RG( a,b,c,d,e,f ) {a,b,c,d,e}
    #include "rg.h"
    #undef RG
};

static  reg_set_index   ClassSets[] = {
/*       RL_ indicates that the class is already identified*/
/*       (e.g. I1 already identified as U1*/
/**/
    RL_BYTE,                /* U1*/
    RL_,                    /* I1*/
    RL_WORD,                /* U2*/
    RL_SEG,                 /* I2    (put segs here so their size is OK)*/
    RL_DBL_OR_PTR,          /* U4*/
    RL_,                    /* I4*/
    RL_8,                   /* U8*/
    RL_,                    /* I8*/
    RL_,                    /* CP*/
    RL_,                    /* PT*/
    RL_,                    /* FS*/
    RL_8,                   /* FD*/
    RL_,                    /* FL*/
    RL_                     /* XX*/
};
static  hw_reg_set        *ParmSets[] = {
    ByteRegs,               /* U1*/
    ByteRegs,               /* I1*/
    WordRegs,               /* U2*/
    WordRegs,               /* I2*/
    DoubleRegs,             /* U4*/
    DoubleRegs,             /* I4*/
    QuadReg,                /* U8*/
    QuadReg,                /* I8*/
    DblPtrRegs,             /* CP*/
    DblPtrRegs,             /* PT*/
    DoubleRegs,             /* FS*/
    QuadReg,                /* FD*/
    __FP80BIT(Empty,QuadReg),/* FL*/
    Empty                   /* XX*/
};
static  hw_reg_set        *ParmSets8087[] = {
    ByteRegs,               /* U1*/
    ByteRegs,               /* I1*/
    WordRegs,               /* U2*/
    WordRegs,               /* I2*/
    DoubleRegs,             /* U4*/
    DoubleRegs,             /* I4*/
    QuadReg,                /* U8*/
    QuadReg,                /* I8*/
    DblPtrRegs,             /* CP*/
    DblPtrRegs,             /* PT*/
    STParmReg,              /* FS*/
    STParmReg,              /* FD*/
    STParmReg,              /* FL*/
    Empty                   /* XX*/
};
static  reg_set_index   IsSets[] = {
    RL_BYTE,                /* U1*/
    RL_BYTE,                /* I1*/
    RL_WORD,                /* U2*/
    RL_WORD,                /* I2*/
    RL_DOUBLE,              /* U4*/
    RL_DOUBLE,              /* I4*/
    RL_8,                   /* U8*/
    RL_8,                   /* I8*/
    RL_DBL_OR_PTR,          /* CP*/
    RL_DBL_OR_PTR,          /* PT*/
    RL_DOUBLE,              /* FS*/
    RL_8,                   /* FD*/
    __FP80BIT(RL_,RL_8),    /* FL*/
    RL_                     /* XX*/
};
static  reg_set_index   ReturnSets[] = {
    RL_AL,                  /* U1*/
    RL_AL,                  /* I1*/
    RL_AX,                  /* U2*/
    RL_AX,                  /* I2*/
    RL_DX_AX,               /* U4*/
    RL_DX_AX,               /* I4*/
    RL_8,                   /* U8*/
    RL_8,                   /* I8*/
    RL_DX_AX,               /* CP*/
    RL_DX_AX,               /* PT*/
    RL_DX_AX,               /* FS*/
    RL_8,                   /* FD*/
    __FP80BIT(RL_,RL_8),    /* FL*/
    RL_                     /* XX*/
};
static  reg_set_index   Return8087[] = {
    RL_AL,                  /* U1*/
    RL_AL,                  /* I1*/
    RL_AX,                  /* U2*/
    RL_AX,                  /* I2*/
    RL_DX_AX,               /* U4*/
    RL_DX_AX,               /* I4*/
    RL_8,                   /* U8*/
    RL_8,                   /* I8*/
    RL_DX_AX,               /* CP*/
    RL_DX_AX,               /* PT*/
    RL_ST0,                 /* FS*/
    RL_ST0,                 /* FD*/
    RL_ST0,                 /* FL*/
    RL_                     /* XX*/
};
static  reg_set_index   IndexSets[] = {
    RL_,                    /* U1*/
    RL_,                    /* I1*/
    RL_INDEX,               /* U2*/
    RL_INDEX,               /* I2*/
    RL_,                    /* U4*/
    RL_,                    /* I4*/
    RL_,                    /* U8*/
    RL_,                    /* I8*/
    RL_LONG_INDEX,          /* CP*/
    RL_LONG_INDEX,          /* PT*/
    RL_,                    /* FS*/
    RL_,                    /* FD*/
    RL_,                    /* FL*/
    RL_                     /* XX*/
};

/*       Information for register set intersections*/
/*       if sets are of different classes*/
/*               intersection is empty*/
/*       } else {*/
/*               intersection given in square matrix for class*/
/*       }*/

#define INTERSECT_DEFS \
    pick( ONE_BYTE,   5, OneByteInter ) \
    pick( TWO_BYTE,   9, TwoByteInter ) \
    pick( FOUR_BYTE,  6, FourByteInter ) \
    pick( EIGHT_BYTE, 1, EightByteInter ) \
    pick( FLOATING,   2, FloatingInter ) \
    pick( OTHER,      0, OtherInter )

typedef enum {
    #define pick(e,w,l) e,
        INTERSECT_DEFS
    #undef pick
} intersect_class;


typedef struct reg_class {
    intersect_class class;
    byte            index;
} reg_class;

static  reg_class       IntersectInfo[] = {
    #define RL(a,b,c,d) {c,d}
    #include "rl.h"
    #undef RL
    {OTHER, 0}          /* RL_NUMBER_OF_SETS*/
};

static  byte    Width[] = {
    #define pick(e,w,l) w,
        INTERSECT_DEFS
    #undef pick
};

static  reg_set_index   OneByteInter[] = {
/*   AL      AH      CL      BYTE        LOWBYTE  */
    RL_AL,  RL_,    RL_,    RL_AL,      RL_AL,          /* AL */
    RL_,    RL_AH,  RL_,    RL_AH,      RL_,            /* AH */
    RL_,    RL_,    RL_CL,  RL_CL,      RL_CL,          /* CL */
    RL_AL,  RL_AH,  RL_CL,  RL_BYTE,    RL_LOWBYTE,     /* BYTE */
    RL_AL,  RL_,    RL_CL,  RL_LOWBYTE, RL_LOWBYTE      /* LOWBYTE */
};

/* short forms used in this table*/
/* - RL_WD == RL_WORD*/
/* - RL_IX == RL_INDEX*/
/* - RL_TI == RL_TEMP_INDEX*/
/* - RL_2B == RL_TWOBYTE*/
/* - RL_WS == RL_ANYWORD*/
/* - RL_SG == RL_SEG*/

static  reg_set_index   TwoByteInter[] = {
/* AX   CX     DX     WD     2B     IX     SG     TI     WS           */
RL_AX, RL_,   RL_,   RL_AX, RL_AX, RL_,   RL_,   RL_,   RL_AX,  /* AX */
RL_,   RL_CX, RL_,   RL_CX, RL_CX, RL_,   RL_,   RL_,   RL_CX,  /* CX */
RL_,   RL_,   RL_DX, RL_DX, RL_DX, RL_,   RL_,   RL_,   RL_DX,  /* DX */
RL_AX, RL_CX, RL_DX, RL_WD, RL_2B, RL_IX, RL_,   RL_TI, RL_WD,  /* WD */
RL_AX, RL_CX, RL_CX, RL_2B, RL_2B, RL_,   RL_,   RL_,   RL_2B,  /* 2B */
RL_,   RL_,   RL_,   RL_IX, RL_,   RL_IX, RL_,   RL_TI, RL_IX,  /* IX */
RL_,   RL_,   RL_,   RL_,   RL_,   RL_,   RL_SG, RL_,   RL_SG,  /* SG */
RL_,   RL_,   RL_,   RL_TI, RL_,   RL_TI, RL_,   RL_TI, RL_TI,  /* TI */
RL_AX, RL_CX, RL_DX, RL_WD, RL_2B, RL_IX, RL_SG, RL_TI, RL_WS   /* WS */
};

static  reg_set_index   FourByteInter[] = {
/* DX_AX     ES_BX       CX_BX       LONG_INDEX      DOUBLE      DBL_OR_PTR                   */
RL_DX_AX,   RL_,        RL_,        RL_,            RL_DX_AX,   RL_DX_AX,       /* DX_AX      */
RL_,        RL_ES_BX,   RL_,        RL_ES_BX,       RL_,        RL_ES_BX,       /* ES_BX      */
RL_,        RL_,        RL_CX_BX,   RL_,            RL_CX_BX,   RL_CX_BX,       /* CX_BX      */
RL_,        RL_ES_BX,   RL_,        RL_LONG_INDEX,  RL_,        RL_LONG_INDEX,  /* LONG_INDEX */
RL_DX_AX,   RL_,        RL_CX_BX,   RL_,            RL_DOUBLE,  RL_DOUBLE,      /* DOUBLE     */
RL_DX_AX,   RL_ES_BX,   RL_CX_BX,   RL_LONG_INDEX,  RL_DOUBLE,  RL_DBL_OR_PTR   /* DBL_OR_PTR */
};


static  reg_set_index   EightByteInter[] = {
/*  RL_8            */
    RL_8    /* RL_8 */
};

static  reg_set_index   FloatingInter[] = {
/*  RL_ST0  RL_STI            */
    RL_ST0, RL_,    /* RL_ST0 */
    RL_,    RL_STI  /* RL_STI */
};

static  reg_set_index   OtherInter[] = {
    RL_
};

static  reg_set_index   *InterTable[] = {
    #define pick(e,w,l) l,
        INTERSECT_DEFS
    #undef pick
};

void            InitRegTbl( void )
/********************************/
{
    if( _FPULevel( FPU_87 ) ) {
        HW_CAsgn( STParmReg[Max87Stk], HW_EMPTY );
    }
}


reg_set_index   RegIntersect( reg_set_index s1, reg_set_index s2 )
/****************************************************************/
{
    reg_class           *set1;
    reg_class           *set2;
    intersect_class     class;
    reg_set_index       result;

    set1 = &IntersectInfo[s1];
    set2 = &IntersectInfo[s2];
    class = set1->class;
    if( class == set2->class ) {
        result = InterTable[class][set1->index * Width[class] + set2->index];
    } else if( s1 == RL_NUMBER_OF_SETS ) {
        result = s2;
    } else if( s2 == RL_NUMBER_OF_SETS ) {
        result = s1;
    } else {
        result = RL_;
    }
    return( result );
}


hw_reg_set        *ParmChoices( type_class_def type_class )
/*********************************************************/
{
    hw_reg_set  *list;

    if( _FPULevel( FPU_87 ) ) {
        list = ParmSets8087[type_class];
    } else {
        list = ParmSets[type_class];
    }
    return( list );
}


hw_reg_set      InLineParm( hw_reg_set regs, hw_reg_set used )
/************************************************************/
{
    if( HW_COvlap( regs, HW_FLTS ) ) {
        HW_CAsgn( regs, HW_EMPTY );
        if( HW_COvlap( used, HW_ST4 ) ) {
            /*regs = EMPTY;*/
        } else if( HW_COvlap( used, HW_ST3 ) ) {
            HW_CTurnOn( regs, HW_ST4 );
        } else if( HW_COvlap( used, HW_ST2 ) ) {
            HW_CTurnOn( regs, HW_ST3 );
        } else if( HW_COvlap( used, HW_ST1 ) ) {
            HW_CTurnOn( regs, HW_ST2 );
        } else {
            HW_CTurnOn( regs, HW_ST1 );
        }
    }
    return( regs );
}


hw_reg_set      StructReg( void )
/*******************************/
{
    return( HW_SI );
}


hw_reg_set  ReturnReg( type_class_def type_class, bool use_87 )
/*************************************************************/
{
    hw_reg_set          *list;

    if( _FPULevel( FPU_87 ) && use_87 ) {
        list = RegSets[Return8087[type_class]];
    } else {
        list = RegSets[ReturnSets[type_class]];
    }
    return( *list );
}


reg_set_index   SegIndex( void )
/******************************/
{
    return( RL_SEG );
}


reg_set_index   NoSegments( reg_set_index regs_idx )
/**************************************************/
{
    if( regs_idx == RL_ANYWORD ) {
        regs_idx = RL_WORD;
    }
    return( regs_idx );
}


reg_set_index   IndexIntersect( reg_set_index curr, type_class_def type_class, bool is_temp_index )
/*************************************************************************************************/
{
    if( is_temp_index ) {
        curr = RegIntersect( curr, RL_TEMP_INDEX );
    } else {
        curr = RegIntersect( curr, IndexSets[type_class] );
    }
    return( curr );
}


bool    IsIndexReg( hw_reg_set reg, type_class_def type_class, bool is_temp_index )
/*********************************************************************************/
{
    hw_reg_set  *list;

    if( is_temp_index ) {
        list = RegSets[RL_TEMP_INDEX];
    } else {
        list = RegSets[IndexSets[type_class]];
    }
    for( ; !HW_CEqual( *list, HW_EMPTY ); ++list ) {
        if( HW_Equal( *list, reg ) ) {
            return( true );
        }
    }
    return( false );
}


type_class_def  RegClass( hw_reg_set regs )
/*****************************************/
{
    hw_reg_set          *possible;
    type_class_def      type_class;

    if( HW_COvlap( regs, HW_FLTS ) ) {
        if( HW_CEqual( regs, HW_ST0 ) )
            return( __FP80BIT(FL,FD) );
        for( possible = STIReg; !HW_CEqual( *possible, HW_EMPTY ); ++possible ) {
            if( HW_Equal( regs, *possible ) ) {
                return( __FP80BIT(FL,FD) );
            }
        }
    } else {
        for( type_class = U1; type_class < XX; ++type_class ) {
            for( possible = RegSets[ClassSets[type_class]]; !HW_CEqual( *possible, HW_EMPTY ); ++possible ) {
                if( HW_Equal( *possible, regs ) ) {
                    return( type_class );
                }
            }
        }
    }
    return( XX );
}


bool    IndexRegOk( hw_reg_set reg, bool is_temp_index )
/******************************************************/
{
    hw_reg_set  *list;

    if( RegClass( reg ) != U2 ) {
        list = RegSets[RL_LONG_INDEX];
    } else if( is_temp_index ) {
        list = RegSets[RL_TEMP_INDEX];
    } else {
        list = RegSets[RL_INDEX];
    }
    for( ; !HW_CEqual( *list, HW_EMPTY ); ++list ) {
        if( HW_Equal( *list, reg ) ) {
            return( true );
        }
    }
    return( false );
}


bool    IsSegReg( hw_reg_set regs )
/*********************************/
{
    hw_reg_set  tmp;

    tmp = regs;
    HW_COnlyOn( tmp, HW_SEGS );
    return( HW_Equal( tmp, regs ) );
}


hw_reg_set      Low16Reg( hw_reg_set regs )
/*****************************************/
{
    HW_COnlyOn( regs, HW_AL_BL_CL_DL );
    return( regs );
}


hw_reg_set      High16Reg( hw_reg_set regs )
/******************************************/
{
    HW_COnlyOn( regs, HW_AH_BH_CH_DH );
    return( regs );
}


hw_reg_set      Low32Reg( hw_reg_set regs )
/*****************************************/
{
    hw_reg_set  low;
    hw_reg_set  *order;

    if( HW_CEqual( regs, HW_EMPTY ) )
        return( HW_EMPTY );
    for( order = Reg32Order; ; ++order ) {
        if( HW_Ovlap( *order, regs ) ) {
            break;
        }
    }
    low = regs;
    HW_OnlyOn( low, *order );
    if( HW_Equal( low, regs ) ) {
        HW_CAsgn( low, HW_EMPTY );
    }
    return( low );
}


hw_reg_set      High32Reg( hw_reg_set regs )
/******************************************/
{
    hw_reg_set  high;

    high = Low32Reg( regs );
    if( !HW_CEqual( high, HW_EMPTY ) ) {
        HW_TurnOff( regs, high );
        return( regs );
    }
    return( high );
}


hw_reg_set      HighReg( hw_reg_set regs )
/****************************************/
{
    hw_reg_set  high;

    if( HW_CEqual( regs, HW_ABCD ) ) {
        HW_CAsgn( high, HW_EMPTY );
    } else {
        high = High32Reg( regs );
        if( HW_CEqual( high, HW_EMPTY ) ) {
            high = High16Reg( regs );
            if( HW_Equal( high, regs ) ) {
                HW_CAsgn( high, HW_EMPTY );
            }
        }
    }
    return( high );
}


hw_reg_set      HighOffsetReg( hw_reg_set regs )
/**********************************************/
{
    return( HighReg( regs ) );
}

hw_reg_set      HighTreePart( hw_reg_set regs )
/*********************************************/
{
    return( HighReg( regs ) );
}

hw_reg_set      LowReg( hw_reg_set regs )
/***************************************/
{
    hw_reg_set  low;

    if( HW_CEqual( regs, HW_ABCD ) ) {
        HW_CAsgn( low, HW_EMPTY );
    } else {
        low = Low32Reg( regs );
        if( HW_CEqual( low, HW_EMPTY ) ) {
            low = Low16Reg( regs );
            if( HW_Equal( low, regs ) ) {
                HW_CAsgn( low, HW_EMPTY );
            }
        }
    }
    return( low );
}

hw_reg_set      LowOffsetReg( hw_reg_set regs )
/*********************************************/
{
    return( LowReg( regs ) );
}

hw_reg_set      LowTreePart( hw_reg_set regs )
/********************************************/
{
    return( LowReg( regs ) );
}

hw_reg_set      FullReg( hw_reg_set regs )
/****************************************/
{
    if( HW_COvlap( regs, HW_AX ) ) {
        HW_CTurnOn( regs, HW_AX );
    }
    if( HW_COvlap( regs, HW_BX ) ) {
        HW_CTurnOn( regs, HW_BX );
    }
    if( HW_COvlap( regs, HW_CX ) ) {
        HW_CTurnOn( regs, HW_CX );
    }
    if( HW_COvlap( regs, HW_DX ) ) {
        HW_CTurnOn( regs, HW_DX );
    }
    return( regs );
}


bool    IsRegClass( hw_reg_set regs, type_class_def type_class )
/**************************************************************/
{
    hw_reg_set  *list;

    for( list = RegSets[IsSets[type_class]]; !HW_CEqual( *list, HW_EMPTY ); ++list ) {
        if( HW_Equal( *list, regs ) ) {
            return( true );
        }
    }
    return( false );
}


hw_reg_set      ActualParmReg( hw_reg_set reg )
/*********************************************/
{
    if( HW_COvlap( reg, HW_FLTS ) ) {
        HW_CAsgn( reg, HW_ST0 );
    }
    return( reg );
}


hw_reg_set      FixedRegs( void )
/*******************************/
{
    hw_reg_set  tmp;

    HW_CAsgn( tmp, HW_SP );
    HW_CTurnOn( tmp, HW_BP );
    HW_CTurnOn( tmp, HW_SS );
    HW_CTurnOn( tmp, HW_CS );
    if( _IsntTargetModel( FLOATING_DS ) )
        HW_CTurnOn( tmp, HW_DS );
    if( _IsntTargetModel( FLOATING_ES ) )
        HW_CTurnOn( tmp, HW_ES );
    if( _IsntTargetModel( FLOATING_FS ) )
        HW_CTurnOn( tmp, HW_FS );
    if( _IsntTargetModel( FLOATING_GS ) )
        HW_CTurnOn( tmp, HW_GS );
    return( tmp );
}


bool    IsStackReg( name *sp )
/****************************/
{
    if( sp == NULL )
        return( false );
    if( sp->n.class != N_REGISTER )
        return( false );
    if( !HW_COvlap( sp->r.reg, HW_SP ) )
        return( false );
    return( true );
}


hw_reg_set      StackReg( void )
/******************************/
{
    return( HW_SP );
}


hw_reg_set      DisplayReg( void )
/********************************/
{
    return( HW_BP );
}


int     SizeDisplayReg( void )
/****************************/
{
    return( WORD_SIZE );
}


hw_reg_set      AllCacheRegs( void )
/**********************************/
{
    hw_reg_set  tmp;

    HW_CAsgn( tmp, HW_FLTS );
    HW_CTurnOn( tmp, HW_ABCD );
    HW_CTurnOn( tmp, HW_SI );
    HW_CTurnOn( tmp, HW_DI );
    if( _IsTargetModel( FLOATING_DS ) )
        HW_CTurnOn( tmp, HW_DS );
    if( _IsTargetModel( FLOATING_ES ) )
        HW_CTurnOn( tmp, HW_ES );
    if( _IsTargetModel( FLOATING_FS ) )
        HW_CTurnOn( tmp, HW_FS );
    if( _IsTargetModel( FLOATING_GS ) )
        HW_CTurnOn( tmp, HW_GS );
    return( tmp );
}

hw_reg_set      *IdxRegs( void )
/******************************/
{
    return( IndexRegs );
}


hw_reg_set      FirstReg( reg_set_index regs_idx )
/*************************************************
    The table RTInfo[] uses reg_set_indexes instead of hw_reg_sets since
    they are only one byte long.  This retrieves the first hw_reg_set
    from the reg_set table "index".

    the tables above use RL_ consts rather that hw_reg_sets cause
    it cheaper. This just picks off the first register from a
    register list and returns it.
*/
{
    return( *RegSets[regs_idx] );
}
