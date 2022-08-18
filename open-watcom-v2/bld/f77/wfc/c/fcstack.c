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
* Description:  Stack F-Code processor.
*
****************************************************************************/


#include "ftnstd.h"
#include "global.h"
#include "fcgbls.h"
#include "wf77defs.h"
#include "wf77cg.h"
#include "tmpdefs.h"
#include "ecflags.h"
#include "cpopt.h"
#include "fltcnv.h"
#include "emitobj.h"
#include "fctypes.h"
#include "cbsize.h"
#include "cnvd2s.h"
#include "tcmplx.h"
#include "fccmplx.h"
#include "fcstring.h"
#include "fcstruct.h"
#include "fcsyms.h"
#include "forcstat.h"
#include "rstmgr.h"
#include "fccall.h"
#include "fcstack.h"
#include "fcjmptab.h"
#include "wf77info.h"
#include "cgswitch.h"
#include "cgprotos.h"


extern  void            CmplxAssign(sym_id,cg_type,cg_type);
extern  void            PushCmplxConst(sym_id);
extern  void            PushComplex(sym_id);
extern  void            Cmplx2Scalar( void );


void    InitStack( void ) {
//===================

// Initialize stack.

    StkPtr = &TokenBuff;
}


cg_type ArrayPtrType( sym_id sym ) {
//==================================

    if( sym->u.ns.si.va.u.dim_ext->dim_flags & DIM_EXTENDED ) {
#if _CPU == 8086
        if( CGOpts & CGOPT_M_LARGE ) {
            return( TY_HUGE_POINTER );
        } else { // if( CGOpts & CGOPT_M_MEDIUM ) {
            return( TY_LONG_POINTER );
        }
#elif _CPU == 386
        return( TY_LONG_POINTER );
#endif
    }
    return( TY_POINTER );
}


cg_type SymPtrType( sym_id sym ) {
//================================

// Get type of pointer required to address given symbol.

    sym_id      leader;
    cg_type     p_type;
    signed_32   offset;
    com_eq      *ce_ext;
    unsigned_32 item_size;
    segment_id  leader_segid;
    unsigned_16 flags;

    flags = sym->u.ns.flags;
    if( flags & SY_SUB_PARM ) {
        // subprogram argument
        if( (flags & SY_CLASS) == SY_SUBPROGRAM ) {
            p_type = TY_CODE_PTR;
        } else if( flags & SY_SUBSCRIPTED ) {
            p_type = ArrayPtrType( sym );
        } else {
            p_type = TY_GLOBAL_POINTER;
        }
    } else if( flags & SY_IN_EQUIV ) {
        leader = sym;
        offset = 0;
        for(;;) {
            ce_ext = leader->u.ns.si.va.vi.ec_ext;
            if( ce_ext->ec_flags & LEADER )
                break;
            offset += ce_ext->offset;
            leader = ce_ext->link_eqv;
        }
        if( ce_ext->ec_flags & MEMBER_IN_COMMON ) {
            offset += ce_ext->offset;
            if( GetComBlkSize( ce_ext->com_blk ) <= MaxSegSize ) {
                // common block fits in a segment
                p_type = TY_GLOBAL_POINTER;
            } else {
                item_size = _SymSize( sym );
                if( flags & SY_SUBSCRIPTED ) {
                    item_size *= sym->u.ns.si.va.u.dim_ext->num_elts;
                }
                if( offset + item_size <= MaxSegSize ) {
                    // object fits in first segment of common block
                    // (common block label is at start of first segment)
                    p_type = TY_GLOBAL_POINTER;
                } else {
                    p_type = TY_HUGE_POINTER;
                }
            }
        } else {
            if( ce_ext->high - ce_ext->low <= MaxSegSize ) {
                // equivalence set fits in a segment
                p_type = TY_GLOBAL_POINTER;
            } else {
                item_size = _SymSize( sym );
                if( flags & SY_SUBSCRIPTED ) {
                    item_size *= sym->u.ns.si.va.u.dim_ext->num_elts;
                }
                leader_segid = GetGlobalSegId( ce_ext->offset );
                offset += ce_ext->offset;
                if( ( GetGlobalSegId( offset ) == leader_segid ) &&
                    ( GetGlobalSegId( offset + item_size ) == leader_segid ) ) {
                    // the entire item is in the same segment as the leader
                    p_type = TY_GLOBAL_POINTER;
                } else {
                    p_type = TY_HUGE_POINTER;
                }
            }
        }
    } else if( flags & SY_IN_COMMON ) {
        ce_ext = sym->u.ns.si.va.vi.ec_ext;
        if( GetComBlkSize( ce_ext->com_blk ) <= MaxSegSize ) {
            // common block fits in a segment
            p_type = TY_GLOBAL_POINTER;
        } else {
            item_size = _SymSize( sym );
            if( flags & SY_SUBSCRIPTED ) {
                item_size *= sym->u.ns.si.va.u.dim_ext->num_elts;
            }
            if( ce_ext->com_blk->u.ns.flags & SY_EQUIVED_NAME ) {
                if( ce_ext->offset + item_size <= MaxSegSize ) {
                    // object fits in first segment of common block
                    // (common block label is at start of first segment)
                    p_type = TY_GLOBAL_POINTER;
                } else {
                    p_type = TY_HUGE_POINTER;
                }
            } else {
                // each symbol in common block gets a label at the offset into
                // the common block
                if( GetComOffset( ce_ext->offset ) + item_size <= MaxSegSize ) {
                    // object fits in a segment
                    p_type = TY_GLOBAL_POINTER;
                } else {
                    p_type = TY_HUGE_POINTER;
                }
            }
        }
    } else if( (flags & SY_SUBSCRIPTED) && _Allocatable( sym ) ) {
        p_type = ArrayPtrType( sym );
    } else if( (flags & SY_SUBSCRIPTED) || ( sym->u.ns.u1.s.typ == FT_STRUCTURE ) ) {
        item_size = _SymSize( sym );
        if( flags & SY_SUBSCRIPTED ) {
            item_size *= sym->u.ns.si.va.u.dim_ext->num_elts;
        }
        if( item_size > MaxSegSize ) {
            p_type = TY_HUGE_POINTER;
        } else if( item_size <= DataThreshold ) {
            p_type = TY_LOCAL_POINTER;
        } else {
            p_type = TY_GLOBAL_POINTER;
        }
    } else {
        p_type = TY_LOCAL_POINTER;
    }
    return( p_type );
}


void    XPush( cg_name cgname ) {
//===============================

// Push a CG-name on the stack.

    *(cg_name *)StkPtr = cgname;
    StkPtr = (char *)StkPtr + sizeof( cg_name );
}


cg_name SymIndex( sym_id sym, cg_name i ) {
//=========================================

// Get address of symbol plus an index.
// Merges offset of symbols in common or equivalence with index so that
// we don't get two run-time calls for huge pointer arithmetic.

    sym_id      leader;
    cg_name     addr;
    signed_32   offset;
    com_eq      *ce_ext;
    cg_type     p_type;
    bool        data_reference;

    data_reference = true;
    if( (sym->u.ns.flags & SY_CLASS) == SY_SUBPROGRAM ) {
        if( (sym->u.ns.flags & SY_SUBPROG_TYPE) == SY_STMT_FUNC ) {
            addr = CGFEName( sym, F77ToCGType( sym ) );
        } else {
            addr = CGFEName( sym, TY_CODE_PTR );
            if( sym->u.ns.flags & SY_SUB_PARM ) {
                addr = CGUnary( O_POINTS, addr, TY_CODE_PTR );
            }
            data_reference = false;
        }
    } else if( sym->u.ns.flags & SY_PS_ENTRY ) {
        // it's the shadow symbol for function return value
        if( CommonEntry == NULL ) {
            if( sym->u.ns.u1.s.typ == FT_CHAR ) {
                if( Options & OPT_DESCRIPTOR ) {
                    addr = CGFEName( ReturnValue, F77ToCGType( sym ) );
                    addr = CGUnary( O_POINTS, addr, TY_POINTER );
                } else {
                    addr = SubAltSCB( sym->u.ns.si.ms.sym );
                }
            } else {
                addr = CGFEName( ReturnValue, F77ToCGType( sym ) );
            }
        } else {
            if( (sym->u.ns.u1.s.typ == FT_CHAR) && (Options & OPT_DESCRIPTOR) == 0 ) {
                addr = SubAltSCB( CommonEntry );
            } else {
                addr = CGUnary( O_POINTS, CGFEName( ReturnValue, TY_POINTER ),
                                TY_POINTER );
            }
        }
    } else if( sym->u.ns.flags & SY_SUB_PARM ) {
        // subprogram argument
        if( sym->u.ns.flags & SY_SUBSCRIPTED ) {
            p_type = ArrayPtrType( sym );
            if( sym->u.ns.u1.s.typ == FT_CHAR ) {
                addr = CGUnary( O_POINTS, CGFEName( sym, p_type ), p_type );
                if( (sym->u.ns.flags & SY_VALUE_PARM) == 0 ) {
                    if( Options & OPT_DESCRIPTOR ) {
                        addr = SCBPointer( addr );
                    }
                }
            } else {
                addr = CGUnary( O_POINTS, CGFEName( sym, p_type ), p_type );
            }
        } else {
            p_type = TY_POINTER;
            if( sym->u.ns.u1.s.typ == FT_CHAR ) {
                if( SCBRequired( sym ) ) {
                    addr = VarAltSCB( sym );
                } else {
                    addr = CGUnary( O_POINTS, CGFEName( sym, p_type ), p_type );
                }
            } else if( sym->u.ns.flags & SY_VALUE_PARM ) {
                p_type = F77ToCGType( sym );
                if( TypeCmplx( sym->u.ns.u1.s.typ ) ) {
                    p_type = CmplxBaseType( p_type );
                    addr = CGFEName( sym, p_type );
                    XPush( CGUnary( O_POINTS,
                                    CGFEName( FindArgShadow( sym ), p_type ),
                                    p_type ) );
                    addr = CGUnary( O_POINTS, addr, p_type );
                } else {
                    addr = CGFEName( sym, p_type );
                }
            } else {
                addr = CGUnary( O_POINTS, CGFEName( sym, p_type ), p_type );
            }
        }
    } else if( sym->u.ns.flags & SY_IN_EQUIV ) {
        leader = sym;
        offset = 0;
        for(;;) {
            if( leader->u.ns.si.va.vi.ec_ext->ec_flags & LEADER )
                break;
            offset += leader->u.ns.si.va.vi.ec_ext->offset;
            leader = leader->u.ns.si.va.vi.ec_ext->link_eqv;
        }
        if( leader->u.ns.si.va.vi.ec_ext->ec_flags & MEMBER_IN_COMMON ) {
            addr = CGFEName( leader->u.ns.si.va.vi.ec_ext->com_blk,
                             F77ToCGType( sym ) );
            offset += leader->u.ns.si.va.vi.ec_ext->offset;
        } else {
            sym_id      shadow;

            shadow = FindEqSetShadow( leader );
            if( shadow != NULL ) {
                addr = CGFEName( shadow, shadow->u.ns.si.ms.u.cg_typ );
                offset -= leader->u.ns.si.va.vi.ec_ext->low;
            } else if( (leader->u.ns.u1.s.typ == FT_CHAR) &&
                       (leader->u.ns.flags & SY_SUBSCRIPTED) == 0 ) {
                addr = CGBackName( leader->u.ns.si.va.u.bck_hdl, F77ToCGType( sym ) );
            } else {
                addr = CGFEName( leader, F77ToCGType( sym ) );
            }
        }
        if( i != NULL ) {
            i = CGBinary( O_PLUS, i, CGInteger( offset, TY_INT_4 ), TY_INT_4 );
        } else {
            i = CGInteger( offset, TY_INT_4 );
        }
        addr = CGBinary( O_PLUS, addr, i, SymPtrType( sym ) );
        if( (sym->u.ns.u1.s.typ == FT_CHAR) && (sym->u.ns.flags & SY_SUBSCRIPTED) == 0 ) {
            // tell code generator where storage pointed to by SCB is located
            addr = CGBinary( O_COMMA, addr,
                             CGFEName( sym, F77ToCGType( sym ) ), TY_DEFAULT );
        }
        i = NULL;
    } else if( ( sym->u.ns.u1.s.typ == FT_CHAR ) &&
               ( (sym->u.ns.flags & SY_SUBSCRIPTED) == 0 ) ) {
        // character variable, address of scb
        addr = CGFEName( sym, F77ToCGType( sym ) );
    } else if( sym->u.ns.flags & SY_IN_COMMON ) {
        ce_ext = sym->u.ns.si.va.vi.ec_ext;
        if( i != NULL ) {
            i = CGBinary( O_PLUS, i, CGInteger( ce_ext->offset, TY_INT_4 ),
                          TY_INT_4 );
        } else {
            i = CGInteger( ce_ext->offset, TY_INT_4 );
        }
        addr = CGBinary( O_PLUS, CGFEName( ce_ext->com_blk, F77ToCGType( sym ) ),
                         i, SymPtrType( sym ) );
        i = NULL;
    } else {
        addr = CGFEName( sym, F77ToCGType( sym ) );
        if( (sym->u.ns.flags & SY_SUBSCRIPTED) && _Allocatable( sym ) ) {
            addr = CGUnary( O_POINTS, addr, ArrayPtrType( sym ) );
        }
    }
    if( i != NULL ) {
        addr = CGBinary( O_PLUS, addr, i, SymPtrType( sym ) );
    }
    if( (OZOpts & OZOPT_O_VOLATILE) && data_reference &&
        ( ( sym->u.ns.u1.s.typ >= FT_REAL ) && ( sym->u.ns.u1.s.typ <= FT_XCOMPLEX ) ) ) {
        addr = CGVolatile( addr );
    } else if( sym->u.ns.u1.s.xflags & SY_VOLATILE ) {
        addr = CGVolatile( addr );
    }
    return( addr );
}


cg_name SymAddr( sym_id sym ) {
//=============================

    return( SymIndex( sym, NULL ) );
}


void    FCPush( void ) {
//================

// Process PUSH F-Code.

    sym_id      sym;

    sym = GetPtr();
    if( TypeCmplx( sym->u.ns.u1.s.typ ) ) {
        PushComplex( sym );
    } else {
        XPush( SymAddr( sym ) );
    }
}


cg_name SymValue( sym_id sym ) {
//==============================

// Generate value of a symbol.

    return( CGUnary( O_POINTS, SymAddr( sym ), F77ToCGType( sym ) ) );
}


void    DXPush( intstar4 val ) {
//==============================

// Push a constant on the stack for DATA statement expressions.

    *(intstar4 *)StkPtr = val;
    StkPtr = (char *)StkPtr + sizeof( intstar4 );
}


void    SymPush( sym_id val ) {
//=============================

// Push a symbol table entry on the stack.

    *(sym_id *)StkPtr = val;
    StkPtr = (char *)StkPtr + sizeof( sym_id );
}


cg_name XPop( void ) {
//==============

// Pop a CG-name from the stack.

    StkPtr = (char *)StkPtr - sizeof( cg_name );
    return( *(cg_name *)StkPtr );
}


cg_name XPopValue( cg_type typ ) {
//================================

// Pop a CG-name from the stack (its value).

    cg_name     opn;

    opn = XPop();
    if( TypePointer( CGType( opn ) ) ) {
        opn = CGUnary( O_POINTS, opn, typ );
    }
    return( opn );
}


void    FCPop( void ) {
//===============

// Process POP F-Code.

    sym_id      sym;
    cg_name     dst;
    unsigned_16 typ_info;
    cg_type     dst_typ;
    cg_type     src_typ;
    sym_id      fd;

    sym = GetPtr();
    typ_info = GetU16();
    dst_typ = GetType1( typ_info );
    src_typ = GetType2( typ_info );
    if( ( dst_typ == TY_COMPLEX ) || ( dst_typ == TY_DCOMPLEX ) || ( dst_typ == TY_XCOMPLEX ) ) {
        CmplxAssign( sym, dst_typ, src_typ );
    } else {
        dst = NULL;
        if( (sym->u.ns.flags & SY_CLASS) == SY_SUBPROGRAM ) {
            // it's a statement function
            if( (OZOpts & OZOPT_O_INLINE) == 0 ) {
                dst = SymAddr( sym );
            }
        } else {
            fd = NULL;
            if( sym->u.ns.u1.s.typ == FT_STRUCTURE ) {
                if( GetU16() ) {
                    // target is a sub-field
                    dst = XPop();
                    if( dst_typ == TY_USER_DEFINED ) {
                        // sub-field is a structure or an array element
                        fd = GetPtr();
                    }
                } else {
                    dst = SymAddr( sym );
                }
            } else if( sym->u.ns.flags & SY_SUBSCRIPTED ) {
                // target is an array element
                dst = XPop();
            } else {
                dst = SymAddr( sym );
            }
            if( dst_typ == TY_USER_DEFINED ) {
                if( fd == NULL ) {
                    dst_typ = sym->u.ns.xt.record->cg_typ;
                } else {
                    dst_typ = fd->u.fd.xt.record->cg_typ;
                }
                XPush( CGAssign( dst, CGUnary( O_POINTS, XPop(), dst_typ ), dst_typ ) );
                return;
            }
        }
        if( (src_typ == TY_COMPLEX) || (src_typ == TY_DCOMPLEX) || (src_typ == TY_XCOMPLEX) ) {
            Cmplx2Scalar();
            src_typ = CmplxBaseType( src_typ );
        }
        if( ( (sym->u.ns.flags & SY_CLASS) == SY_SUBPROGRAM ) && (OZOpts & OZOPT_O_INLINE) )
            return;
        XPush( CGAssign( dst, XPopValue( src_typ ), dst_typ ) );
    }
}


cg_name GetTypedValue( void ) {
//=======================

// Pop a CG-name from the stack (its value).

    cg_name     opn;
    cg_type     typ;

    opn = XPop();
    typ = GetType( GetU16() );
    if( TypePointer( CGType( opn ) ) ) {
        opn = CGUnary( O_POINTS, opn, typ );
    }
    return( opn );
}


cg_name         StkElement( int idx ) {
//=====================================

// Get idx'th stack element.

    return(  *(cg_name * )((char *)StkPtr - idx * sizeof( cg_name )) );
}


void            PopStkElements( int num ) {
//=========================================

// Pop stack elements from the stack.

    StkPtr = (char *)StkPtr - num * sizeof( cg_name );
}


intstar4        DXPop( void ) {
//=======================

// Pop a constant from the stack for DATA statement expressions.

    StkPtr = (char *)StkPtr - sizeof( intstar4 );
    return( *(intstar4 *)StkPtr );
}


sym_id          SymPop( void ) {
//========================

// Pop a symbol table entry from the stack.

    StkPtr = (char *)StkPtr - sizeof( sym_id );
    return( *(sym_id *)StkPtr );
}


cg_name IntegerConstant( ftn_type *value, size_t size )
//=====================================================
{
    if( size == sizeof( intstar1 ) ) {
        return( CGInteger( value->intstar1, TY_INT_1 ) );
    } else if( size == sizeof( intstar2 ) ) {
        return( CGInteger( value->intstar2, TY_INT_2 ) );
    } else {
        return( CGInteger( value->intstar4, TY_INT_4 ) );
    }
}


void    FCPushConst( void ) {
//=====================

// Process PUSH_CONST F-Code.

    sym_id      sym;
    char        fmt_buff[CONVERSION_BUFFER+1];

    sym = GetPtr();
    switch( sym->u.cn.typ ) {
    case FT_INTEGER_1 :
    case FT_INTEGER_2 :
    case FT_INTEGER :
        XPush( IntegerConstant( &sym->u.cn.value, sym->u.cn.size ) );
        break;
    case FT_LOGICAL_1 :
    case FT_LOGICAL :
        XPush( CGInteger( sym->u.cn.value.logstar4, TY_UINT_1 ) );
        break;
    case FT_REAL :
        CnvS2S( &sym->u.cn.value.single, fmt_buff );
        XPush( CGFloat( fmt_buff, TY_SINGLE ) );
        break;
    case FT_DOUBLE :
        CnvD2S( &sym->u.cn.value.dble, fmt_buff );
        XPush( CGFloat( fmt_buff, TY_DOUBLE ) );
        break;
    case FT_TRUE_EXTENDED :
        CnvX2S( &sym->u.cn.value.extended, fmt_buff );
        XPush( CGFloat( fmt_buff, TY_LONGDOUBLE ) );
        break;
    case FT_COMPLEX :
        PushCmplxConst( sym );
        break;
    case FT_DCOMPLEX :
        PushCmplxConst( sym );
        break;
    case FT_TRUE_XCOMPLEX :
        PushCmplxConst( sym );
        break;
    }
}


void    FCPushLit( void ) {
//===================

// Process PUSH_LIT F-Code.

    sym_id      sym;

    sym = GetPtr();
    if( sym->u.lt.flags & (LT_SCB_REQUIRED | LT_SCB_TMP_REFERENCE) ) {
        XPush( CGBackName( ConstBack( sym ), TY_CHAR ) );
    }
}
