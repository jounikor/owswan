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
* Description:  Inform CG which functions modify FPU registers
*
****************************************************************************/

// When compiling "/3s" or "/windows", by default, functions modify CPU
// registers and 80x87 registers. It is not possible to tell the code generator
// that a function does not modify 80x87 registers but the functions
// it calls modifies 80x87 registers. So the best we can do is specify
// that functions do not modify CPU registers.

#if defined( _M_IX86 )
  #if defined( _M_I86 )
    #pragma aux if_rtn "IF@*" __parm [__ax __bx __cx __dx __8087];
    #pragma aux if_va "IF@*" __parm __caller [];
    #pragma aux xf_rtn "IF@*";
    #pragma aux (if_rtn) flt_if_rtn;
    #pragma aux (if_va) flt_if_va;
  #else
    #pragma aux if_rtn "IF@*" __parm __routine [__eax __ebx __ecx __edx __8087];
    #pragma aux if_va  "IF@*" __parm __caller [];
    #pragma aux xf_rtn "IF@*";
    #if defined( __SW_3S )
      #if defined( __FLAT__ )
        #pragma aux (if_rtn) if_rtn __modify [__8087 __gs];
        #pragma aux (if_va)  if_va  __modify [__8087 __gs];
        #pragma aux (xf_rtn) xf_rtn __modify [__8087 __gs];
      #else
        #pragma aux (if_rtn) if_rtn __modify [__8087 __es __fs __gs];
        #pragma aux (if_va)  if_va  __modify [__8087 __es __fs __gs];
        #pragma aux (xf_rtn) xf_rtn __modify [__8087 __es __fs __gs];
      #endif
      #if defined( __FPI__ )
        // When compiling "/3s", by default, functions that return floating-point
        // values do NOT return them using the 80x87 so we must explicitly say
        // they do.
        #pragma aux (if_rtn) flt_if_rtn __value [__8087];
        #pragma aux (if_va) flt_if_va __value [__8087];
      #else
        #pragma aux (if_rtn) flt_if_rtn;
        #pragma aux (if_va) flt_if_va;
      #endif
    #else
      #pragma aux (if_rtn) flt_if_rtn;
      #pragma aux (if_va) flt_if_va;
    #endif
  #endif

  // Define the following if the C library contains alternate definitions
  // of certain intrinsic functions.

  #define __alternate_if__
  #define ALOG  LOG
  #define ALOG10        LOG10
  #define AMOD  FMOD
  #define DMOD  DFMOD
  #define QMOD  QFMOD
  #pragma aux (flt_if_rtn) LOG;         // support routine called IF@LOG, not IF@ALOG
  #pragma aux (flt_if_rtn) LOG10;       // support routine called IF@LOG, not IF@ALOG
  #pragma aux (flt_if_rtn) FMOD;        // support routine called IF@FMOD, not IF@AMOD
  #pragma aux (flt_if_rtn) DFMOD;       // support routine called IF@DFMOD, not IF@DMOD
  #pragma aux (flt_if_rtn) QFMOD;       // support routine called IF@QFMOD, not IF@QMOD

  // UNCOMMENT_TOKEN
  // this can be removed once long double is properly supported
  // by the C library
  #define       __extended_not_implemented__

  #pragma aux (if_rtn) IOR;
  #pragma aux (if_rtn) IAND;
  #pragma aux (if_rtn) NOT;
  #pragma aux (if_rtn) IEOR;
  #pragma aux (if_rtn) ISHL;
  #pragma aux (if_rtn) ISHA;
  #pragma aux (if_rtn) ISHC;
  #pragma aux (if_rtn) BTEST;
  #pragma aux (if_rtn) IBSET;
  #pragma aux (if_rtn) IBCLR;
  #pragma aux (if_rtn) IBCHNG;
  #pragma aux (if_rtn) ISHFT;
  #pragma aux (if_rtn) I1ABS;
  #pragma aux (if_rtn) I1AND;
  #pragma aux (if_rtn) I1BCHNG;
  #pragma aux (if_rtn) I1BCLR;
  #pragma aux (if_rtn) I1BSET;
  #pragma aux (if_rtn) I1BTEST;
  #pragma aux (if_rtn) I1DIM;
  #pragma aux (if_rtn) I1EOR;
  #pragma aux (if_rtn) I1MIN0;
  #pragma aux (if_rtn) I1MAX0;
  #pragma aux (if_rtn) I1MOD;
  #pragma aux (if_rtn) I1NOT;
  #pragma aux (if_rtn) I1OR;
  #pragma aux (if_rtn) I1SHA;
  #pragma aux (if_rtn) I1SHC;
  #pragma aux (if_rtn) I1SHFT;
  #pragma aux (if_rtn) I1SHL;
  #pragma aux (if_rtn) I1SIGN;
  #pragma aux (if_rtn) I2ABS;
  #pragma aux (if_rtn) I2AND;
  #pragma aux (if_rtn) I2BCHNG;
  #pragma aux (if_rtn) I2BCLR;
  #pragma aux (if_rtn) I2BSET;
  #pragma aux (if_rtn) I2BTEST;
  #pragma aux (if_rtn) I2DIM;
  #pragma aux (if_rtn) I2EOR;
  #pragma aux (if_rtn) I2MIN0;
  #pragma aux (if_rtn) I2MAX0;
  #pragma aux (if_rtn) I2MOD;
  #pragma aux (if_rtn) I2NOT;
  #pragma aux (if_rtn) I2OR;
  #pragma aux (if_rtn) I2SHA;
  #pragma aux (if_rtn) I2SHC;
  #pragma aux (if_rtn) I2SHFT;
  #pragma aux (if_rtn) I2SHL;
  #pragma aux (if_rtn) I2SIGN;
  #pragma aux (if_rtn) I1LSHIFT;
  #pragma aux (if_rtn) I1RSHIFT;
  #pragma aux (if_rtn) I2LSHIFT;
  #pragma aux (if_rtn) I2RSHIFT;
  #pragma aux (if_rtn) LSHIFT;
  #pragma aux (if_rtn) RSHIFT;
  #pragma aux (flt_if_rtn) SQRT;
  #pragma aux (flt_if_rtn) DSQRT;
  #pragma aux (flt_if_rtn) QSQRT;
  #pragma aux (if_rtn) CSQRT;
  #pragma aux (if_rtn) CDSQRT;
  #pragma aux (if_rtn) CQSQRT;
  #pragma aux (flt_if_rtn) EXP;
  #pragma aux (flt_if_rtn) DEXP;
  #pragma aux (flt_if_rtn) QEXP;
  #pragma aux (if_rtn) CEXP;
  #pragma aux (if_rtn) CDEXP;
  #pragma aux (if_rtn) CQEXP;
  #pragma aux (flt_if_rtn) ALOG;
  #pragma aux (flt_if_rtn) DLOG;
  #pragma aux (flt_if_rtn) QLOG;
  #pragma aux (if_rtn) CLOG;
  #pragma aux (if_rtn) CDLOG;
  #pragma aux (if_rtn) CQLOG;
  #pragma aux (flt_if_rtn) ALOG10;
  #pragma aux (flt_if_rtn) DLOG10;
  #pragma aux (flt_if_rtn) QLOG10;
  #pragma aux (flt_if_rtn) DASIN;
  #pragma aux (flt_if_rtn) QASIN;
  #pragma aux (flt_if_rtn) DSIN;
  #pragma aux (flt_if_rtn) QSIN;
  #pragma aux (if_rtn) CSIN;
  #pragma aux (if_rtn) CDSIN;
  #pragma aux (if_rtn) CQSIN;
  #pragma aux (flt_if_rtn) COS;
  #pragma aux (flt_if_rtn) DCOS;
  #pragma aux (flt_if_rtn) QCOS;
  #pragma aux (if_rtn) CCOS;
  #pragma aux (if_rtn) CDCOS;
  #pragma aux (if_rtn) CQCOS;
  #pragma aux (flt_if_rtn) ASIN;
  #pragma aux (flt_if_rtn) SIN;
  #pragma aux (flt_if_rtn) ACOS;
  #pragma aux (flt_if_rtn) DACOS;
  #pragma aux (flt_if_rtn) QACOS;
  #pragma aux (flt_if_rtn) ATAN;
  #pragma aux (flt_if_rtn) DATAN;
  #pragma aux (flt_if_rtn) QATAN;
  #pragma aux (flt_if_rtn) ATAN2;
  #pragma aux (flt_if_rtn) DATAN2;
  #pragma aux (flt_if_rtn) QATAN2;
  #pragma aux (flt_if_rtn) SINH;
  #pragma aux (flt_if_rtn) DSINH;
  #pragma aux (flt_if_rtn) QSINH;
  #pragma aux (flt_if_rtn) TAN;
  #pragma aux (flt_if_rtn) DTAN;
  #pragma aux (flt_if_rtn) QTAN;
  #pragma aux (flt_if_rtn) COSH;
  #pragma aux (flt_if_rtn) DCOSH;
  #pragma aux (flt_if_rtn) QCOSH;
  #pragma aux (flt_if_rtn) TANH;
  #pragma aux (flt_if_rtn) DTANH;
  #pragma aux (flt_if_rtn) QTANH;
  #pragma aux (flt_if_rtn) ABS;
  #pragma aux (flt_if_rtn) DABS;
  #pragma aux (flt_if_rtn) QABS;
  #pragma aux (flt_if_rtn) CABS;
  #pragma aux (flt_if_rtn) CDABS;
  #pragma aux (flt_if_rtn) CQABS;
  #pragma aux (if_rtn) IABS;
  #pragma aux (flt_if_rtn) DPROD;
  #pragma aux (flt_if_rtn) QPROD;
  #pragma aux (if_rtn) IDIM;
  #pragma aux (flt_if_rtn) DIM;
  #pragma aux (flt_if_rtn) DDIM;
  #pragma aux (flt_if_rtn) QDIM;
  #pragma aux (if_rtn) ISIGN;
  #pragma aux (flt_if_rtn) SIGN;
  #pragma aux (flt_if_rtn) DSIGN;
  #pragma aux (flt_if_rtn) QSIGN;
  #pragma aux (flt_if_rtn) AIMAG;
  #pragma aux (flt_if_rtn) DIMAG;
  #pragma aux (flt_if_rtn) QIMAG;
  #pragma aux (if_rtn) CONJG;
  #pragma aux (if_rtn) DCONJG;
  #pragma aux (if_rtn) QCONJG;
  #pragma aux (if_rtn) LEN;
  #pragma aux (if_rtn) INDEX;
  #pragma aux (if_rtn) MOD;
  #pragma aux (flt_if_rtn) AMOD;
  #pragma aux (if_rtn) MAX0;
  #pragma aux (flt_if_rtn) DMOD;
  #pragma aux (flt_if_rtn) QMOD;
  #pragma aux (flt_if_rtn) AMAX0;
  #pragma aux (flt_if_rtn) AMAX1;
  #pragma aux (flt_if_rtn) DMAX1;
  #pragma aux (flt_if_rtn) QMAX1;
  #pragma aux (if_rtn) MAX1;
  #pragma aux (if_rtn) MIN0;
  #pragma aux (flt_if_rtn) AMIN1;
  #pragma aux (flt_if_rtn) DMIN1;
  #pragma aux (flt_if_rtn) QMIN1;
  #pragma aux (flt_if_rtn) AMIN0;
  #pragma aux (if_rtn) MIN1;
  #pragma aux (if_rtn) LGE;
  #pragma aux (if_rtn) LGT;
  #pragma aux (if_rtn) LLE;
  #pragma aux (if_rtn) LLT;
  #pragma aux (if_rtn) CHAR;
  #pragma aux (if_rtn) ICHAR;
  #pragma aux (flt_if_rtn) AINT;
  #pragma aux (flt_if_rtn) DINT;
  #pragma aux (flt_if_rtn) QINT;
  #pragma aux (flt_if_rtn) ANINT;
  #pragma aux (flt_if_rtn) DNINT;
  #pragma aux (flt_if_rtn) QNINT;
  #pragma aux (if_rtn) NINT;
  #pragma aux (if_rtn) IDNINT;
  #pragma aux (if_rtn) IQNINT;
  #pragma aux (flt_if_rtn) GAMMA;
  #pragma aux (flt_if_rtn) DGAMMA;
  #pragma aux (flt_if_rtn) QGAMMA;
  #pragma aux (flt_if_rtn) ALGAMA;
  #pragma aux (flt_if_rtn) DLGAMA;
  #pragma aux (flt_if_rtn) QLGAMA;
  #pragma aux (flt_if_rtn) ERF;
  #pragma aux (flt_if_rtn) DERF;
  #pragma aux (flt_if_rtn) QERF;
  #pragma aux (flt_if_rtn) ERFC;
  #pragma aux (flt_if_rtn) DERFC;
  #pragma aux (flt_if_rtn) QERFC;
  #pragma aux (flt_if_rtn) COTAN;
  #pragma aux (flt_if_rtn) DCOTAN;
  #pragma aux (flt_if_rtn) QCOTAN;
  #pragma aux (if_rtn) LENTRIM;
  #pragma aux (xf_rtn) XIOR;
  #pragma aux (xf_rtn) XIAND;
  #pragma aux (xf_rtn) XNOT;
  #pragma aux (xf_rtn) XIEOR;
  #pragma aux (xf_rtn) XISHL;
  #pragma aux (xf_rtn) XISHA;
  #pragma aux (xf_rtn) XISHC;
  #pragma aux (xf_rtn) XBTEST;
  #pragma aux (xf_rtn) XIBSET;
  #pragma aux (xf_rtn) XIBCLR;
  #pragma aux (xf_rtn) XIBCHNG;
  #pragma aux (xf_rtn) XISHFT;
  #pragma aux (xf_rtn) XSQRT;
  #pragma aux (xf_rtn) XDSQRT;
  #pragma aux (xf_rtn) XQSQRT;
  #pragma aux (xf_rtn) XCSQRT;
  #pragma aux (xf_rtn) XCDSQRT;
  #pragma aux (xf_rtn) XCQSQRT;
  #pragma aux (xf_rtn) XEXP;
  #pragma aux (xf_rtn) XDEXP;
  #pragma aux (xf_rtn) XQEXP;
  #pragma aux (xf_rtn) XCEXP;
  #pragma aux (xf_rtn) XCDEXP;
  #pragma aux (xf_rtn) XCQEXP;
  #pragma aux (xf_rtn) XALOG;
  #pragma aux (xf_rtn) XDLOG;
  #pragma aux (xf_rtn) XQLOG;
  #pragma aux (xf_rtn) XCLOG;
  #pragma aux (xf_rtn) XCDLOG;
  #pragma aux (xf_rtn) XCQLOG;
  #pragma aux (xf_rtn) XALOG10;
  #pragma aux (xf_rtn) XDLOG10;
  #pragma aux (xf_rtn) XQLOG10;
  #pragma aux (xf_rtn) XDASIN;
  #pragma aux (xf_rtn) XQASIN;
  #pragma aux (xf_rtn) XDSIN;
  #pragma aux (xf_rtn) XQSIN;
  #pragma aux (xf_rtn) XCSIN;
  #pragma aux (xf_rtn) XCDSIN;
  #pragma aux (xf_rtn) XCQSIN;
  #pragma aux (xf_rtn) XCOS;
  #pragma aux (xf_rtn) XDCOS;
  #pragma aux (xf_rtn) XQCOS;
  #pragma aux (xf_rtn) XCCOS;
  #pragma aux (xf_rtn) XCDCOS;
  #pragma aux (xf_rtn) XCQCOS;
  #pragma aux (xf_rtn) XASIN;
  #pragma aux (xf_rtn) XSIN;
  #pragma aux (xf_rtn) XACOS;
  #pragma aux (xf_rtn) XDACOS;
  #pragma aux (xf_rtn) XQACOS;
  #pragma aux (xf_rtn) XATAN;
  #pragma aux (xf_rtn) XDATAN;
  #pragma aux (xf_rtn) XQATAN;
  #pragma aux (xf_rtn) XATAN2;
  #pragma aux (xf_rtn) XDATAN2;
  #pragma aux (xf_rtn) XQATAN2;
  #pragma aux (xf_rtn) XSINH;
  #pragma aux (xf_rtn) XDSINH;
  #pragma aux (xf_rtn) XQSINH;
  #pragma aux (xf_rtn) XTAN;
  #pragma aux (xf_rtn) XDTAN;
  #pragma aux (xf_rtn) XQTAN;
  #pragma aux (xf_rtn) XCOSH;
  #pragma aux (xf_rtn) XDCOSH;
  #pragma aux (xf_rtn) XQCOSH;
  #pragma aux (xf_rtn) XTANH;
  #pragma aux (xf_rtn) XDTANH;
  #pragma aux (xf_rtn) XQTANH;
  #pragma aux (xf_rtn) XABS;
  #pragma aux (xf_rtn) XDABS;
  #pragma aux (xf_rtn) XQABS;
  #pragma aux (xf_rtn) XCABS;
  #pragma aux (xf_rtn) XCDABS;
  #pragma aux (xf_rtn) XCQABS;
  #pragma aux (xf_rtn) XIABS;
  #pragma aux (xf_rtn) XDPROD;
  #pragma aux (xf_rtn) XQPROD;
  #pragma aux (xf_rtn) XIDIM;
  #pragma aux (xf_rtn) XDIM;
  #pragma aux (xf_rtn) XDDIM;
  #pragma aux (xf_rtn) XQDIM;
  #pragma aux (xf_rtn) XISIGN;
  #pragma aux (xf_rtn) XSIGN;
  #pragma aux (xf_rtn) XDSIGN;
  #pragma aux (xf_rtn) XQSIGN;
  #pragma aux (xf_rtn) XAIMAG;
  #pragma aux (xf_rtn) XDIMAG;
  #pragma aux (xf_rtn) XQIMAG;
  #pragma aux (xf_rtn) XCONJG;
  #pragma aux (xf_rtn) XDCONJG;
  #pragma aux (xf_rtn) XQCONJG;
  #pragma aux (xf_rtn) XLEN;
  #pragma aux (xf_rtn) XINDEX;
  #pragma aux (xf_rtn) XMOD;
  #pragma aux (xf_rtn) XAMOD;
  #pragma aux (xf_rtn) XDMOD;
  #pragma aux (xf_rtn) XQMOD;
  #pragma aux (if_va) XMAX0;
  #pragma aux (flt_if_va) XAMAX0;
  #pragma aux (flt_if_va) XAMAX1;
  #pragma aux (flt_if_va) XDMAX1;
  #pragma aux (flt_if_va) XQMAX1;
  #pragma aux (if_va) XMAX1;
  #pragma aux (if_va) XMIN0;
  #pragma aux (flt_if_va) XAMIN1;
  #pragma aux (flt_if_va) XDMIN1;
  #pragma aux (flt_if_va) XQMIN1;
  #pragma aux (flt_if_va) XAMIN0;
  #pragma aux (if_va) XMIN1;
  #pragma aux (xf_rtn) XLGE;
  #pragma aux (xf_rtn) XLGT;
  #pragma aux (xf_rtn) XLLE;
  #pragma aux (xf_rtn) XLLT;
  #pragma aux (xf_rtn) XCHAR;
  #pragma aux (xf_rtn) XICHAR;
  #pragma aux (xf_rtn) XAINT;
  #pragma aux (xf_rtn) XDINT;
  #pragma aux (xf_rtn) XQINT;
  #pragma aux (xf_rtn) XANINT;
  #pragma aux (xf_rtn) XDNINT;
  #pragma aux (xf_rtn) XQNINT;
  #pragma aux (xf_rtn) XNINT;
  #pragma aux (xf_rtn) XIDNINT;
  #pragma aux (xf_rtn) XIQNINT;
  #pragma aux (xf_rtn) XGAMMA;
  #pragma aux (xf_rtn) XDGAMMA;
  #pragma aux (xf_rtn) XQGAMMA;
  #pragma aux (xf_rtn) XALGAMA;
  #pragma aux (xf_rtn) XDLGAMA;
  #pragma aux (xf_rtn) XQLGAMA;
  #pragma aux (xf_rtn) XERF;
  #pragma aux (xf_rtn) XDERF;
  #pragma aux (xf_rtn) XQERF;
  #pragma aux (xf_rtn) XERFC;
  #pragma aux (xf_rtn) XDERFC;
  #pragma aux (xf_rtn) XQERFC;
  #pragma aux (xf_rtn) XCOTAN;
  #pragma aux (xf_rtn) XDCOTAN;
  #pragma aux (xf_rtn) XQCOTAN;
  #pragma aux (xf_rtn) XLENTRIM;
  #pragma aux (xf_rtn) XI1ABS;
  #pragma aux (xf_rtn) XI1AND;
  #pragma aux (xf_rtn) XI1BCHNG;
  #pragma aux (xf_rtn) XI1BCLR;
  #pragma aux (xf_rtn) XI1BSET;
  #pragma aux (xf_rtn) XI1BTEST;
  #pragma aux (xf_rtn) XI1DIM;
  #pragma aux (xf_rtn) XI1EOR;
  #pragma aux (if_va)  XI1MAX0;
  #pragma aux (if_va)  XI1MIN0;
  #pragma aux (xf_rtn) XI1MOD;
  #pragma aux (xf_rtn) XI1OR;
  #pragma aux (xf_rtn) XI1NOT;
  #pragma aux (xf_rtn) XI1SHA;
  #pragma aux (xf_rtn) XI1SHC;
  #pragma aux (xf_rtn) XI1SHFT;
  #pragma aux (xf_rtn) XI1SHL;
  #pragma aux (xf_rtn) XI1SIGN;
  #pragma aux (xf_rtn) XI2ABS;
  #pragma aux (xf_rtn) XI2AND;
  #pragma aux (xf_rtn) XI2BCHNG;
  #pragma aux (xf_rtn) XI2BCLR;
  #pragma aux (xf_rtn) XI2BSET;
  #pragma aux (xf_rtn) XI2BTEST;
  #pragma aux (xf_rtn) XI2DIM;
  #pragma aux (xf_rtn) XI2EOR;
  #pragma aux (if_va)  XI2MAX0;
  #pragma aux (if_va)  XI2MIN0;
  #pragma aux (xf_rtn) XI2MOD;
  #pragma aux (xf_rtn) XI2OR;
  #pragma aux (xf_rtn) XI2NOT;
  #pragma aux (xf_rtn) XI2SHA;
  #pragma aux (xf_rtn) XI2SHC;
  #pragma aux (xf_rtn) XI2SHFT;
  #pragma aux (xf_rtn) XI2SHL;
  #pragma aux (xf_rtn) XI2SIGN;
  #pragma aux (if_rtn) XI1LSHIFT;
  #pragma aux (if_rtn) XI1RSHIFT;
  #pragma aux (if_rtn) XI2LSHIFT;
  #pragma aux (if_rtn) XI2RSHIFT;
  #pragma aux (if_rtn) XLSHIFT;
  #pragma aux (if_rtn) XRSHIFT;
#else
  #define IOR           _IF_IOR
  #define IAND          _IF_IAND
  #define NOT           _IF_NOT
  #define IEOR          _IF_IEOR
  #define ISHL          _IF_ISHL
  #define ISHA          _IF_ISHA
  #define ISHC          _IF_ISHC
  #define BTEST         _IF_BTEST
  #define IBSET         _IF_IBSET
  #define IBCLR         _IF_IBCLR
  #define IBCHNG        _IF_IBCHNG
  #define ISHFT         _IF_ISHFT
  #define I1ABS         _IF_I1ABS
  #define I1AND         _IF_I1AND
  #define I1BCHNG       _IF_I1BCHNG
  #define I1BCLR        _IF_I1BCLR
  #define I1BSET        _IF_I1BSET
  #define I1BTEST       _IF_I1BTEST
  #define I1DIM         _IF_I1DIM
  #define I1EOR         _IF_I1EOR
  #define I1MIN0        _IF_I1MIN0
  #define I1MAX0        _IF_I1MAX0
  #define I1MOD         _IF_I1MOD
  #define I1NOT         _IF_I1NOT
  #define I1OR          _IF_I1OR
  #define I1SHA         _IF_I1SHA
  #define I1SHC         _IF_I1SHC
  #define I1SHFT        _IF_I1SHFT
  #define I1SHL         _IF_I1SHL
  #define I1SIGN        _IF_I1SIGN
  #define I2ABS         _IF_I2ABS
  #define I2AND         _IF_I2AND
  #define I2BCHNG       _IF_I2BCHNG
  #define I2BCLR        _IF_I2BCLR
  #define I2BSET        _IF_I2BSET
  #define I2BTEST       _IF_I2BTEST
  #define I2DIM         _IF_I2DIM
  #define I2EOR         _IF_I2EOR
  #define I2MIN0        _IF_I2MIN0
  #define I2MAX0        _IF_I2MAX0
  #define I2MOD         _IF_I2MOD
  #define I2NOT         _IF_I2NOT
  #define I2OR          _IF_I2OR
  #define I2SHA         _IF_I2SHA
  #define I2SHC         _IF_I2SHC
  #define I2SHFT        _IF_I2SHFT
  #define I2SHL         _IF_I2SHL
  #define I2SIGN        _IF_I2SIGN
  #define I1LSHIFT      _IF_I1LSHIFT
  #define I1RSHIFT      _IF_I1RSHIFT
  #define I2LSHIFT      _IF_I2LSHIFT
  #define I2RSHIFT      _IF_I2RSHIFT
  #define LSHIFT        _IF_LSHIFT
  #define RSHIFT        _IF_RSHIFT
  #define SQRT          _IF_SQRT
  #define DSQRT         _IF_DSQRT
  #define QSQRT         _IF_QSQRT
  #define CSQRT         _IF_CSQRT
  #define CDSQRT        _IF_CDSQRT
  #define CQSQRT        _IF_CQSQRT
  #define EXP           _IF_EXP
  #define DEXP          _IF_DEXP
  #define QEXP          _IF_QEXP
  #define CEXP          _IF_CEXP
  #define CDEXP         _IF_CDEXP
  #define CQEXP         _IF_CQEXP
  #define ALOG          _IF_ALOG
  #define DLOG          _IF_DLOG
  #define QLOG          _IF_QLOG
  #define CLOG          _IF_CLOG
  #define CDLOG         _IF_CDLOG
  #define CQLOG         _IF_CQLOG
  #define ALOG10        _IF_ALOG10
  #define DLOG10        _IF_DLOG10
  #define QLOG10        _IF_QLOG10
  #define DASIN         _IF_DASIN
  #define QASIN         _IF_QASIN
  #define DSIN          _IF_DSIN
  #define QSIN          _IF_QSIN
  #define CSIN          _IF_CSIN
  #define CDSIN         _IF_CDSIN
  #define CQSIN         _IF_CQSIN
  #define COS           _IF_COS
  #define DCOS          _IF_DCOS
  #define QCOS          _IF_QCOS
  #define CCOS          _IF_CCOS
  #define CDCOS         _IF_CDCOS
  #define CQCOS         _IF_CQCOS
  #define ASIN          _IF_ASIN
  #define SIN           _IF_SIN
  #define ACOS          _IF_ACOS
  #define DACOS         _IF_DACOS
  #define QACOS         _IF_QACOS
  #define ATAN          _IF_ATAN
  #define DATAN         _IF_DATAN
  #define QATAN         _IF_QATAN
  #define ATAN2         _IF_ATAN2
  #define DATAN2        _IF_DATAN2
  #define QATAN2        _IF_QATAN2
  #define SINH          _IF_SINH
  #define DSINH         _IF_DSINH
  #define QSINH         _IF_QSINH
  #define TAN           _IF_TAN
  #define DTAN          _IF_DTAN
  #define QTAN          _IF_QTAN
  #define COSH          _IF_COSH
  #define DCOSH         _IF_DCOSH
  #define QCOSH         _IF_QCOSH
  #define TANH          _IF_TANH
  #define DTANH         _IF_DTANH
  #define QTANH         _IF_QTANH
  #define ABS           _IF_ABS
  #define DABS          _IF_DABS
  #define QABS          _IF_QABS
  #define CABS          _IF_CABS
  #define CDABS         _IF_CDABS
  #define CQABS         _IF_CQABS
  #define IABS          _IF_IABS
  #define DPROD         _IF_DPROD
  #define QPROD         _IF_QPROD
  #define IDIM          _IF_IDIM
  #define DIM           _IF_DIM
  #define DDIM          _IF_DDIM
  #define QDIM          _IF_QDIM
  #define ISIGN         _IF_ISIGN
  #define SIGN          _IF_SIGN
  #define DSIGN         _IF_DSIGN
  #define QSIGN         _IF_QSIGN
  #define AIMAG         _IF_AIMAG
  #define DIMAG         _IF_DIMAG
  #define QIMAG         _IF_QIMAG
  #define CONJG         _IF_CONJG
  #define DCONJG        _IF_DCONJG
  #define QCONJG        _IF_QCONJG
  #define LEN           _IF_LEN
  #define INDEX         _IF_INDEX
  #define MOD           _IF_MOD
  #define AMOD          _IF_AMOD
  #define MAX0          _IF_MAX0
  #define DMOD          _IF_DMOD
  #define QMOD          _IF_QMOD
  #define AMAX0         _IF_AMAX0
  #define AMAX1         _IF_AMAX1
  #define DMAX1         _IF_DMAX1
  #define QMAX1         _IF_QMAX1
  #define MAX1          _IF_MAX1
  #define MIN0          _IF_MIN0
  #define AMIN1         _IF_AMIN1
  #define DMIN1         _IF_DMIN1
  #define QMIN1         _IF_QMIN1
  #define AMIN0         _IF_AMIN0
  #define MIN1          _IF_MIN1
  #define LGE           _IF_LGE
  #define LGT           _IF_LGT
  #define LLE           _IF_LLE
  #define LLT           _IF_LLT
  #define CHAR          _IF_CHAR
  #define ICHAR         _IF_ICHAR
  #define AINT          _IF_AINT
  #define DINT          _IF_DINT
  #define QINT          _IF_QINT
  #define ANINT         _IF_ANINT
  #define DNINT         _IF_DNINT
  #define QNINT         _IF_QNINT
  #define NINT          _IF_NINT
  #define IDNINT        _IF_IDNINT
  #define IQNINT        _IF_IQNINT
  #define GAMMA         _IF_GAMMA
  #define DGAMMA        _IF_DGAMMA
  #define QGAMMA        _IF_QGAMMA
  #define ALGAMA        _IF_ALGAMA
  #define DLGAMA        _IF_DLGAMA
  #define QLGAMA        _IF_QLGAMA
  #define ERF           _IF_ERF
  #define DERF          _IF_DERF
  #define QERF          _IF_QERF
  #define ERFC          _IF_ERFC
  #define DERFC         _IF_DERFC
  #define QERFC         _IF_QERFC
  #define COTAN         _IF_COTAN
  #define DCOTAN        _IF_DCOTAN
  #define QCOTAN        _IF_QCOTAN
  #define LENTRIM       _IF_LENTRIM
  #define XIOR          _IF_XIOR
  #define XIAND         _IF_XIAND
  #define XNOT          _IF_XNOT
  #define XIEOR         _IF_XIEOR
  #define XISHL         _IF_XISHL
  #define XISHA         _IF_XISHA
  #define XISHC         _IF_XISHC
  #define XBTEST        _IF_XBTEST
  #define XIBSET        _IF_XIBSET
  #define XIBCLR        _IF_XIBCLR
  #define XIBCHNG       _IF_XIBCHNG
  #define XISHFT        _IF_XISHFT
  #define XSQRT         _IF_XSQRT
  #define XDSQRT        _IF_XDSQRT
  #define XQSQRT        _IF_XQSQRT
  #define XCSQRT        _IF_XCSQRT
  #define XCDSQRT       _IF_XCDSQRT
  #define XCQSQRT       _IF_XCQSQRT
  #define XEXP          _IF_XEXP
  #define XDEXP         _IF_XDEXP
  #define XQEXP         _IF_XQEXP
  #define XCEXP         _IF_XCEXP
  #define XCDEXP        _IF_XCDEXP
  #define XCQEXP        _IF_XCQEXP
  #define XALOG         _IF_XALOG
  #define XDLOG         _IF_XDLOG
  #define XQLOG         _IF_XQLOG
  #define XCLOG         _IF_XCLOG
  #define XCDLOG        _IF_XCDLOG
  #define XCQLOG        _IF_XCQLOG
  #define XALOG10       _IF_XALOG10
  #define XDLOG10       _IF_XDLOG10
  #define XQLOG10       _IF_XQLOG10
  #define XDASIN        _IF_XDASIN
  #define XQASIN        _IF_XQASIN
  #define XDSIN         _IF_XDSIN
  #define XQSIN         _IF_XQSIN
  #define XCSIN         _IF_XCSIN
  #define XCDSIN        _IF_XCDSIN
  #define XCQSIN        _IF_XCQSIN
  #define XCOS          _IF_XCOS
  #define XDCOS         _IF_XDCOS
  #define XQCOS         _IF_XQCOS
  #define XCCOS         _IF_XCCOS
  #define XCDCOS        _IF_XCDCOS
  #define XCQCOS        _IF_XCQCOS
  #define XASIN         _IF_XASIN
  #define XSIN          _IF_XSIN
  #define XACOS         _IF_XACOS
  #define XDACOS        _IF_XDACOS
  #define XQACOS        _IF_XQACOS
  #define XATAN         _IF_XATAN
  #define XDATAN        _IF_XDATAN
  #define XQATAN        _IF_XQATAN
  #define XATAN2        _IF_XATAN2
  #define XDATAN2       _IF_XDATAN2
  #define XQATAN2       _IF_XQATAN2
  #define XSINH         _IF_XSINH
  #define XDSINH        _IF_XDSINH
  #define XQSINH        _IF_XQSINH
  #define XTAN          _IF_XTAN
  #define XDTAN         _IF_XDTAN
  #define XQTAN         _IF_XQTAN
  #define XCOSH         _IF_XCOSH
  #define XDCOSH        _IF_XDCOSH
  #define XQCOSH        _IF_XQCOSH
  #define XTANH         _IF_XTANH
  #define XDTANH        _IF_XDTANH
  #define XQTANH        _IF_XQTANH
  #define XABS          _IF_XABS
  #define XDABS         _IF_XDABS
  #define XQABS         _IF_XQABS
  #define XCABS         _IF_XCABS
  #define XCDABS        _IF_XCDABS
  #define XCQABS        _IF_XCQABS
  #define XIABS         _IF_XIABS
  #define XDPROD        _IF_XDPROD
  #define XQPROD        _IF_XQPROD
  #define XIDIM         _IF_XIDIM
  #define XDIM          _IF_XDIM
  #define XDDIM         _IF_XDDIM
  #define XQDIM         _IF_XQDIM
  #define XISIGN        _IF_XISIGN
  #define XSIGN         _IF_XSIGN
  #define XDSIGN        _IF_XDSIGN
  #define XQSIGN        _IF_XQSIGN
  #define XAIMAG        _IF_XAIMAG
  #define XDIMAG        _IF_XDIMAG
  #define XQIMAG        _IF_XQIMAG
  #define XCONJG        _IF_XCONJG
  #define XDCONJG       _IF_XDCONJG
  #define XQCONJG       _IF_XQCONJG
  #define XLEN          _IF_XLEN
  #define XINDEX        _IF_XINDEX
  #define XMOD          _IF_XMOD
  #define XAMOD         _IF_XAMOD
  #define XDMOD         _IF_XDMOD
  #define XQMOD         _IF_XQMOD
  #define XMAX0         _IF_XMAX0
  #define XAMAX0        _IF_XAMAX0
  #define XAMAX1        _IF_XAMAX1
  #define XDMAX1        _IF_XDMAX1
  #define XQMAX1        _IF_XQMAX1
  #define XMAX1         _IF_XMAX1
  #define XMIN0         _IF_XMIN0
  #define XAMIN1        _IF_XAMIN1
  #define XDMIN1        _IF_XDMIN1
  #define XQMIN1        _IF_XQMIN1
  #define XAMIN0        _IF_XAMIN0
  #define XMIN1         _IF_XMIN1
  #define XLGE          _IF_XLGE
  #define XLGT          _IF_XLGT
  #define XLLE          _IF_XLLE
  #define XLLT          _IF_XLLT
  #define XCHAR         _IF_XCHAR
  #define XICHAR        _IF_XICHAR
  #define XAINT         _IF_XAINT
  #define XDINT         _IF_XDINT
  #define XQINT         _IF_XQINT
  #define XANINT        _IF_XANINT
  #define XDNINT        _IF_XDNINT
  #define XQNINT        _IF_XQNINT
  #define XNINT         _IF_XNINT
  #define XIDNINT       _IF_XIDNINT
  #define XIQNINT       _IF_XIQNINT
  #define XGAMMA        _IF_XGAMMA
  #define XDGAMMA       _IF_XDGAMMA
  #define XQGAMMA       _IF_XQGAMMA
  #define XALGAMA       _IF_XALGAMA
  #define XDLGAMA       _IF_XDLGAMA
  #define XQLGAMA       _IF_XQLGAMA
  #define XERF          _IF_XERF
  #define XDERF         _IF_XDERF
  #define XQERF         _IF_XQERF
  #define XERFC         _IF_XERFC
  #define XDERFC        _IF_XDERFC
  #define XQERFC        _IF_XQERFC
  #define XCOTAN        _IF_XCOTAN
  #define XDCOTAN       _IF_XDCOTAN
  #define XQCOTAN       _IF_XQCOTAN
  #define XLENTRIM      _IF_XLENTRIM
  #define XI1ABS        _IF_XI1ABS
  #define XI1AND        _IF_XI1AND
  #define XI1BCHNG      _IF_XI1BCHNG
  #define XI1BCLR       _IF_XI1BCLR
  #define XI1BSET       _IF_XI1BSET
  #define XI1BTEST      _IF_XI1BTEST
  #define XI1DIM        _IF_XI1DIM
  #define XI1EOR        _IF_XI1EOR
  #define XI1MAX0       _IF_XI1MAX0
  #define XI1MIN0       _IF_XI1MIN0
  #define XI1MOD        _IF_XI1MOD
  #define XI1OR         _IF_XI1OR
  #define XI1NOT        _IF_XI1NOT
  #define XI1SHA        _IF_XI1SHA
  #define XI1SHC        _IF_XI1SHC
  #define XI1SHFT       _IF_XI1SHFT
  #define XI1SHL        _IF_XI1SHL
  #define XI1SIGN       _IF_XI1SIGN
  #define XI2ABS        _IF_XI2ABS
  #define XI2AND        _IF_XI2AND
  #define XI2BCHNG      _IF_XI2BCHNG
  #define XI2BCLR       _IF_XI2BCLR
  #define XI2BSET       _IF_XI2BSET
  #define XI2BTEST      _IF_XI2BTEST
  #define XI2DIM        _IF_XI2DIM
  #define XI2EOR        _IF_XI2EOR
  #define XI2MAX0       _IF_XI2MAX0
  #define XI2MIN0       _IF_XI2MIN0
  #define XI2MOD        _IF_XI2MOD
  #define XI2OR         _IF_XI2OR
  #define XI2NOT        _IF_XI2NOT
  #define XI2SHA        _IF_XI2SHA
  #define XI2SHC        _IF_XI2SHC
  #define XI2SHFT       _IF_XI2SHFT
  #define XI2SHL        _IF_XI2SHL
  #define XI2SIGN       _IF_XI2SIGN
  #define XI1LSHIFT     _IF_XI1LSHIFT
  #define XI1RSHIFT     _IF_XI1RSHIFT
  #define XI2LSHIFT     _IF_XI2LSHIFT
  #define XI2RSHIFT     _IF_XI2RSHIFT
  #define XLSHIFT       _IF_XLSHIFT
  #define XRSHIFT       _IF_XRSHIFT
#endif

extern single   AINT( single arg );
extern single   XAINT( single *arg );
extern single   ANINT( single arg );
extern single   XANINT( single *arg );
extern scomplex CCOS( single rp, single ip );
extern scomplex XCCOS( scomplex *arg );
extern dcomplex CDCOS( double rp, double ip );
extern dcomplex XCDCOS( dcomplex *arg );
extern dcomplex CDEXP( double rp, double ip );
extern dcomplex XCDEXP( dcomplex *arg );
extern dcomplex CDLOG( double rp, double ip );
extern dcomplex XCDLOG( dcomplex *arg );
extern dcomplex CDSIN( double rp, double ip );
extern dcomplex XCDSIN( dcomplex *arg );
extern dcomplex CDSQRT( double rp, double ip );
extern dcomplex XCDSQRT( dcomplex *arg );
extern scomplex CEXP( single rp, single ip );
extern scomplex XCEXP( scomplex *arg );
extern scomplex CLOG( single rp, single ip );
extern scomplex XCLOG( scomplex *arg );
extern scomplex CSIN( single rp, single ip );
extern scomplex XCSIN( scomplex *arg );
extern scomplex CSQRT( single rp, single ip );
extern scomplex XCSQRT( scomplex *arg );
extern xcomplex CQCOS( extended rp, extended ip );
extern xcomplex XCQCOS( xcomplex *arg );
extern xcomplex CQEXP( extended rp, extended ip );
extern xcomplex XCQEXP( xcomplex *arg );
extern xcomplex CQLOG( extended rp, extended ip );
extern xcomplex XCQLOG( xcomplex *arg );
extern xcomplex CQSIN( extended rp, extended ip );
extern xcomplex XCQSIN( xcomplex *arg );
extern xcomplex CQSQRT( extended rp, extended ip );
extern xcomplex _CQSQRT( xcomplex *arg );
extern xcomplex _CQSQRT( xcomplex *arg );
extern double   DDIM( double arg1, double arg2 );
extern double   XDDIM( double *arg1, double *arg2 );
extern extended QDIM( extended arg1, extended arg2 );
extern extended XQDIM( extended *arg1, extended *arg2 );
extern extended QDIM( extended arg1, extended arg2 );
extern extended XQDIM( extended *arg1, extended *arg2 );
extern single   DIM( single arg1, single arg2 );
extern single   XDIM( single *arg1, single *arg2 );
extern double   DINT( double arg );
extern double   XDINT( double *arg );
extern extended QINT( extended arg );
extern extended XQINT( extended *arg );
extern double   DNINT( double arg );
extern double   XDNINT( double *arg );
extern extended QNINT( extended arg );
extern extended XQNINT( extended *arg );
extern double   DSIGN( double arg1, double arg2 );
extern double   XDSIGN( double *arg1, double *arg2 );
extern extended QSIGN( extended arg1, extended arg2 );
extern extended XQSIGN( extended *arg1, extended *arg2 );
extern intstar1 I1DIM( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1DIM( intstar1 *arg1, intstar1 *arg2 );
extern intstar2 I2DIM( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2DIM( intstar2 *arg1, intstar2 *arg2 );
extern intstar4 IDIM( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIDIM( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 IDNINT( double arg );
extern intstar4 XIDNINT( double *arg );
extern intstar4 IQNINT( extended arg );
extern intstar4 XIQNINT( extended *arg );
extern intstar4 XINDEX( char PGM *s, char PGM *t, uint slen, uint tlen );
extern intstar4 INDEX( string PGM *arg1, string PGM *arg2 );
extern intstar4 ISIGN( intstar4 arg1, intstar4 arg2 );
extern intstar4 XISIGN( intstar4 *arg1, intstar4 *arg2 );
extern logstar4 LGE( string PGM *arg1, string PGM *arg2 );
extern logstar4 LGT( string PGM *arg1, string PGM *arg2 );
extern logstar4 LLE( string *arg1, string *arg2 );
extern logstar4 LLT( string PGM *arg1, string PGM *arg2 );
extern intstar4 NINT( single arg );
extern intstar4 XNINT( single *arg );
extern intstar1 I1SIGN( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1SIGN( intstar1 *arg1, intstar1 *arg2 );
extern intstar2 I2SIGN( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2SIGN( intstar2 *arg1, intstar2 *arg2 );
extern single   SIGN( single arg1, single arg2 );
extern single   XSIGN( single *arg1, single *arg2 );
extern intstar1 I1SHA( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1SHA( intstar1 *arg1, intstar1 *arg2 );
extern intstar1 I1SHC( unsigned_8 arg1, intstar1 arg2 );
extern intstar1 XI1SHC( unsigned_8 *arg1, intstar1 *arg2 );
extern intstar1 I1SHFT( unsigned_8 arg1, intstar1 arg2 );
extern intstar1 XI1SHFT( unsigned_8 *arg1, intstar1 *arg2 );
extern intstar1 I1SHL( unsigned_8 arg1, intstar1 arg2 );
extern intstar1 XI1SHL( unsigned_8 *arg1, intstar1 *arg2 );
extern intstar2 I2SHA( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2SHA( intstar2 *arg1, intstar2 *arg2 );
extern intstar2 I2SHC( unsigned_16 arg1, intstar2 arg2 );
extern intstar2 XI2SHC( unsigned_16 *arg1, intstar2 *arg2 );
extern intstar2 I2SHFT( unsigned_16 arg1, intstar2 arg2 );
extern intstar2 XI2SHFT( unsigned_16 *arg1, intstar2 *arg2 );
extern intstar2 I2SHL( unsigned_16 arg1, intstar2 arg2 );
extern intstar2 XI2SHL( unsigned_16 *arg1, intstar2 *arg2 );
extern intstar4 ISHL( unsigned_32 arg1, intstar4 arg2 );
extern intstar4 XISHL( unsigned_32 *arg1, intstar4 *arg2 );
extern intstar4 ISHA( intstar4 arg1, intstar4 arg2 );
extern intstar4 XISHA( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 ISHC( unsigned_32 arg1, intstar4 arg2 );
extern intstar4 XISHC( unsigned_32 *arg1, intstar4 *arg2 );
extern intstar4 ISHFT( unsigned_32 arg1, intstar4 arg2 );
extern intstar4 XISHFT( unsigned_32 *arg1, intstar4 *arg2 );
extern intstar1 I1LSHIFT( intstar1 arg1, unsigned_8 arg2 );
extern intstar1 XI1LSHIFT( intstar1 *arg1, unsigned_8 *arg2 );
extern intstar1 I1RSHIFT( intstar1 arg1, unsigned_8 arg2 );
extern intstar1 XI1RSHIFT( intstar1 *arg1, unsigned_8 *arg2 );
extern intstar2 I2LSHIFT( intstar2 arg1, unsigned_16 arg2 );
extern intstar2 XI2LSHIFT( intstar2 *arg1, unsigned_16 *arg2 );
extern intstar2 I2RSHIFT( intstar2 arg1, unsigned_16 arg2 );
extern intstar2 XI2RSHIFT( intstar2 *arg1, unsigned_16 *arg2 );
extern intstar4 LSHIFT( intstar4 arg1, unsigned_32 arg2 );
extern intstar4 XLSHIFT( intstar4 *arg1, unsigned_32 *arg2 );
extern intstar4 RSHIFT( intstar4 arg1, unsigned_32 arg2 );
extern intstar4 XRSHIFT( intstar4 *arg1, unsigned_32 *arg2 );
extern intstar1 I1BCHNG( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1BCHNG( intstar1 *arg1, intstar1 *arg2 );
extern intstar2 I2BCHNG( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2BCHNG( intstar2 *arg1, intstar2 *arg2 );
extern intstar4 IBCHNG( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIBCHNG( intstar4 *arg1, intstar4 *arg2 );
extern single   GAMMA( single arg );
extern single   XGAMMA( single *arg );
extern double   DGAMMA( double arg );
extern double   XDGAMMA( double *arg );
extern extended QGAMMA( extended arg );
extern extended XQGAMMA( extended *arg );
extern single   ALGAMA( single arg );
extern single   XALGAMA( single *arg );
extern single   ALGAMA( single arg );
extern single   XALGAMA( single *arg );
extern double   DLGAMA( double arg );
extern double   XDLGAMA( double *arg );
extern extended QLGAMA( extended arg );
extern extended XQLGAMA( extended *arg );
extern single   ERF( single arg );
extern single   XERF( single *arg );
extern single   ERFC( single arg );
extern single   XERFC( single *arg );
extern double   DERF( double arg );
extern double   XDERF( double *arg );
extern double   DERFC( double arg );
extern double   XDERFC( double *arg );
extern extended QERF( extended arg );
extern extended XQERF( extended *arg );
extern extended QERFC( extended arg );
extern extended XQERFC( extended *arg );
extern single   CABS( single rp, single ip );
extern single   XCABS( scomplex *arg );
extern double   CDABS( double rp, double ip );
extern double   XCDABS( dcomplex *arg );
extern extended CQABS( extended rp, extended ip );
extern extended XCQABS( xcomplex *arg );
extern intstar4 LEN( string PGM *arg );
extern intstar4 XLEN( char PGM *arg, int len );
extern intstar4 XLENTRIM( char PGM *str, int len );
extern intstar4 LENTRIM( string PGM *str );
extern intstar1 I1ABS( intstar1 arg );
extern intstar1 XI1ABS( intstar1 *arg );
extern intstar2 I2ABS( intstar2 arg );
extern intstar2 XI2ABS( intstar2 *arg );
extern intstar4 IABS( intstar4 arg );
extern intstar4 XIABS( intstar4 *arg );
extern intstar1 I1MOD( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1MOD( intstar1 *arg1, intstar1 *arg2 );
extern intstar2 I2MOD( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2MOD( intstar2 *arg1, intstar2 *arg2 );
extern intstar4 MOD( intstar4 arg1, intstar4 arg2 );
extern intstar4 XMOD( intstar4 *arg1, intstar4 *arg2 );
extern void     CHAR( intstar4 arg, string PGM *dest );
extern intstar4 ICHAR( string PGM *arg );
extern single   AIMAG( single rp, single ip );
extern single   XAIMAG( scomplex *arg );
extern double   DIMAG( double rp, double ip );
extern double   XDIMAG( dcomplex *arg );
extern extended QIMAG( extended rp, extended ip );
extern extended XQIMAG( xcomplex *arg );
extern scomplex CONJG( single rp, single ip );
extern scomplex XCONJG( scomplex *arg );
extern dcomplex DCONJG( double rp, double ip );
extern dcomplex XDCONJG( dcomplex *arg );
extern xcomplex QCONJG( extended rp, extended ip );
extern xcomplex XQCONJG( xcomplex *arg );
extern double   DPROD( single arg1, single arg2 );
extern double   XDPROD( single *arg1, single *arg2 );
extern extended QPROD( double arg1, double arg2 );
extern extended XQPROD( double *arg1, double *arg2 );
extern logstar4 BTEST( intstar4 arg1, intstar4 arg2 );
extern logstar4 XBTEST( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 IBSET( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIBSET( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 IBCLR( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIBCLR( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 IOR( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIOR( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 IAND( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIAND( intstar4 *arg1, intstar4 *arg2 );
extern intstar4 NOT( intstar4 arg1 );
extern intstar4 XNOT( intstar4 *arg1 );
extern intstar4 IEOR( intstar4 arg1, intstar4 arg2 );
extern intstar4 XIEOR( intstar4 *arg1, intstar4 *arg2 );
extern intstar1 I1AND( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1AND( intstar1 *arg1, intstar1 *arg2 );
extern intstar1 I1BCLR( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1BCLR( intstar1 *arg1, intstar1 *arg2 );
extern intstar1 I1BSET( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1BSET( intstar1 *arg1, intstar1 *arg2 );
extern logstar4 I1BTEST( intstar1 arg1, intstar1 arg2 );
extern logstar4 XI1BTEST( intstar1 *arg1, intstar1 *arg2 );
extern intstar1 I1EOR( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1EOR( intstar1 *arg1, intstar1 *arg2 );
extern intstar1 I1NOT( intstar1 arg1 );
extern intstar1 XI1NOT( intstar1 *arg1 );
extern intstar1 I1OR( intstar1 arg1, intstar1 arg2 );
extern intstar1 XI1OR( intstar1 *arg1, intstar1 *arg2 );
extern intstar2 I2AND( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2AND( intstar2 *arg1, intstar2 *arg2 );
extern intstar2 I2BCLR( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2BCLR( intstar2 *arg1, intstar2 *arg2 );
extern intstar2 I2BSET( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2BSET( intstar2 *arg1, intstar2 *arg2 );
extern logstar4 I2BTEST( intstar2 arg1, intstar2 arg2 );
extern logstar4 XI2BTEST( intstar2 *arg1, intstar2 *arg2 );
extern intstar2 I2EOR( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2EOR( intstar2 *arg1, intstar2 *arg2 );
extern intstar2 I2NOT( intstar2 arg1 );
extern intstar2 XI2NOT( intstar2 *arg1 );
extern intstar2 I2OR( intstar2 arg1, intstar2 arg2 );
extern intstar2 XI2OR( intstar2 *arg1, intstar2 *arg2 );
extern single   ATAN( single arg );
extern single   XATAN( single *arg );
extern double   DATAN( double arg );
extern double   XDATAN( double *arg );
extern single   ALOG( single arg );
extern single   XALOG( single *arg );
extern double   DLOG( double arg );
extern double   XDLOG( double *arg );
extern extended QLOG( extended arg );
extern extended XQLOG( extended *arg );
extern single   COS( single arg );
extern single   XCOS( single *arg );
extern double   DCOS( double arg );
extern double   XDCOS( double *arg );
extern single   EXP( single arg );
extern single   XEXP( single *arg );
extern double   DEXP( double arg );
extern double   XDEXP( double *arg );
extern extended QEXP( extended arg );
extern extended XQEXP( extended *arg );
extern single   ALOG10( single arg );
extern single   XALOG10( single *arg );
extern double   DLOG10( double arg );
extern double   XDLOG10( double *arg );
extern extended QLOG10( extended arg );
extern extended XQLOG10( extended *arg );
extern single   SIN( single arg );
extern single   XSIN( single *arg );
extern double   DSIN( double arg );
extern double   XDSIN( double *arg );
extern single   SQRT( single arg );
extern single   XSQRT( single *arg );
extern double   DSQRT( double arg );
extern double   XDSQRT( double *arg );
extern extended QSQRT( extended arg );
extern extended XQSQRT( extended *arg );
extern single   TAN( single arg );
extern single   XTAN( single *arg );
extern double   DTAN( double arg );
extern double   XDTAN( double *arg );
extern single   ATAN2( single arg1, single arg2 );
extern single   XATAN2( single *arg1, single *arg2 );
extern double   DATAN2( double arg1, double arg2 );
extern double   XDATAN2( double *arg1, double *arg2 );
extern double   DACOS( double arg );
extern double   XDACOS( double *arg );
extern double   DASIN( double arg );
extern double   XDASIN( double *arg );
extern double   DCOSH( double arg );
extern double   XDCOSH( double *arg );
extern double   DSINH( double arg );
extern double   XDSINH( double *arg );
extern double   DTANH( double arg );
extern double   XDTANH( double *arg );
extern single   COTAN( single arg );
extern single   XCOTAN( single *arg );
extern double   DCOTAN( double arg );
extern double   XDCOTAN( double *arg );
extern single   SINH( single arg );
extern single   XSINH( single *arg );
extern single   COSH( single arg );
extern single   XCOSH( single *arg );
extern single   TANH( single arg );
extern single   XTANH( single *arg );
extern single   ACOS( single arg );
extern single   XACOS( single *arg );
extern single   ASIN( single arg );
extern single   XASIN( single *arg );
extern extended QACOS( extended arg );
extern extended XQACOS( extended *arg );
extern extended QASIN( extended arg );
extern extended XQASIN( extended *arg );
extern extended QATAN( extended arg );
extern extended XQATAN( extended *arg );
extern extended QATAN2( extended arg1, extended arg2 );
extern extended XQATAN2( extended *arg1, extended *arg2 );
extern extended QCOS( extended arg );
extern extended XQCOS( extended *arg );
extern extended QCOSH( extended arg );
extern extended XQCOSH( extended *arg );
extern extended QCOTAN( extended arg );
extern extended XQCOTAN( extended *arg );
extern extended QSIN( extended arg );
extern extended XQSIN( extended *arg );
extern extended QSINH( extended arg );
extern extended XQSINH( extended *arg );
extern extended QTAN( extended arg );
extern extended XQTAN( extended *arg );
extern extended QTANH( extended arg );
extern extended XQTANH( extended *arg );
extern single   AMOD( single arg1, single arg2 );
extern single   XAMOD( single *arg1, single *arg2 );
extern double   DMOD( double arg1, double arg2 );
extern double   XDMOD( double *arg1, double *arg2 );
extern extended QMOD( extended arg1, extended arg2 );
extern extended XQMOD( extended *arg1, extended *arg2 );
extern single   ABS( single arg );
extern single   XABS( single *arg );
extern double   DABS( double arg );
extern double   XDABS( double *arg );
extern extended QABS( extended arg );
extern extended XQABS( extended *arg );
extern intstar1 XI1MAX0( int num_args, ... );
extern intstar1 XI1MIN0( int num_args, ... );
extern intstar2 XI2MAX0( int num_args, ... );
extern intstar2 XI2MIN0( int num_args, ... );
extern single   XAMAX0( int num_args, ... );
extern single   XAMAX1( int num_args, ... );
extern single   XAMIN0( int num_args, ... );
extern single   XAMIN1( int num_args, ... );
extern double   XDMAX1( int num_args, ... );
extern double   XDMIN1( int num_args, ... );
extern extended XQMAX1( int num_args, ... );
extern extended XQMIN1( int num_args, ... );
extern intstar4 XMAX0( int num_args, ... );
extern intstar4 XMAX1( int num_args, ... );
extern intstar4 XMIN0( int num_args, ... );
extern intstar4 XMIN1( int num_args, ... );
