/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2016 The Open Watcom Contributors. All Rights Reserved.
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


#include "_cgstd.h"
#include "optwif.h"
#include "model.h"
#include "optmain.h"
#include "optutil.h"
#include "optcom.h"
#include "optins.h"
#include "optpull.h"
#include "optpush.h"
#include "optrel.h"


static  ins_entry       *Redirect( ins_entry *, ins_entry * );


static  bool    LineLabel( ins_entry *label )
/*******************************************/
{
#if _TARGET & _TARG_RISC
    if( _LblLine( label ) != 0 ) {
        return( true );
    }
#else
    /* unused parameters */ (void)label;
#endif
    return( false );
}


static  bool    CompressLabels( ins_entry *label )
/************************************************/
{
    ins_entry   *other_label;
    bool        lbl_unique;

  optbegin
    lbl_unique = UniqueLabel( _Label( label ) );
    for( ;; ) {
        if( PrevClass( label ) != OC_LABEL )
            break;
        other_label = PrevIns( label );
        if( lbl_unique && UniqueLabel( _Label( other_label ) ) )
            break;
        if( LineLabel( label ) && LineLabel( other_label ) )
            break;
        AliasLabels( other_label, label );
        if( _Class( label ) == OC_DEAD ) {
            optreturn( false );
        }
    }
    for( ;; ) {
        if( NextClass( label ) != OC_LABEL )
            break;
        other_label = NextIns( label );
        if( lbl_unique && UniqueLabel( _Label( other_label ) ) )
            break;
        if( LineLabel( label ) && LineLabel( other_label ) )
            break;
        AliasLabels( other_label, label );
        if( _Class( label ) == OC_DEAD ) {
            optreturn( false );
        }
    }
    optreturn( true );
}


static  bool    UnTangle1( ins_entry *jmp, ins_entry **instr )
/************************************************************/
{
    ins_entry   *c_jmp;
    oc_class    cl;

    if( jmp == NULL )
        return( false );
    cl = _Class( jmp );
    if( (cl != OC_JCOND) && (cl != OC_JMP) )
        return( false );
    if( _Label( jmp ) == _Label( *instr ) ) {
        /* jump next*/
        *instr = DelInstr( jmp );
        return( true );
    }
    if( cl != OC_JMP )
        return( false );
    c_jmp = PrevIns( jmp );
    if( c_jmp == NULL )
        return( false );
    if( _Class( c_jmp ) != OC_JCOND )
        return( false );
#if( OPTIONS & SEGMENTED )
    if( _Label( c_jmp ) == _Label( *instr ) && ( _Attr( jmp ) & ATTR_FAR ) == 0 ) {
#else
    if( _Label( c_jmp ) == _Label( *instr ) ) {
#endif
        /* conditional jump around a jump*/
        _JmpCond( c_jmp ) = ReverseCondition( _JmpCond( c_jmp ) );
        cl = _ClassInfo( jmp );
        _ClassInfo( jmp ) = OC_DEAD_JMP;    /* to stop dead code removal*/
        ChgLblRef( c_jmp, _Label( jmp ) );
        _ClassInfo( jmp ) = cl;
        *instr = DelInstr( jmp );
        return( true );
    } else if( _Label( c_jmp ) == _Label( jmp ) ) {
        /* conditional jump followed by a jump to the same label*/
        DelInstr( c_jmp );
        return( true );
    }
    return( false );
}


static  bool    UnTangle2( ins_entry *jmp, ins_entry **instr )
/************************************************************/
{
    oc_class    cl;
    ins_entry   *ins;

    if( _IsModel( NO_OPTIMIZATION ) )
        return( false );
    if( jmp == NULL )
        return( false );
    cl = _Class( jmp );
    if( cl == OC_RET ) {
        /* label followed by a return*/
        return( RetAftrLbl( jmp ) );
    }
    if( cl != OC_JMP )
        return( false );
#if( OPTIONS & SEGMENTED )
    if( _Attr( jmp ) & ATTR_FAR )
        return( false );
#endif
    if( _Label( *instr )->ins == NULL )
        return( false );
    if( _Label( jmp )->redirect == _Label( *instr ) )
        return( false );
    if( _Label( jmp ) == _Label( *instr ) )
        return( false );
    /* jump to jump*/
    *instr = Redirect( *instr, jmp );
    if( *instr == NULL )
        return( false );
    cl = PrevClass( *instr );
    if( !_TransferClass( cl ) )
        return( true );
    /* dead code*/
    ins = *instr;
    for(;;) {
        if( ins == NULL )
            break;
        if( _Class( ins ) == OC_LABEL )
            break;
        if( _Class( ins ) == OC_INFO ) {
            ins = ins->ins.next;
        } else {
            ins = DelInstr( ins );
        }
    }
    *instr = ins;
    return( true );
}

ins_entry       *Untangle( ins_entry *instr )
/*******************************************/
{
    ins_entry   *jmp;
    bool        change;

  optbegin
    for( ;; ) {
        if( instr == NULL )
            break;
        if( _Class( instr ) != OC_LABEL )
            break;
        if( !CompressLabels( instr ) )
            break;
        change = false;
        jmp = PrevIns( instr );
        change |= UnTangle1( jmp, &instr );
        if( instr == NULL )
            break;
        if( _Class( instr ) != OC_LABEL )
            break;
        if( !CompressLabels( instr ) )
            break;
        jmp = NextIns( instr );
        change |= UnTangle2( jmp, &instr );
        if( !change ) {
            break;
        }
    }
    optreturn( instr );
}


static  ins_entry       *Redirect( ins_entry *l_ins, ins_entry *j_ins )
/*********************************************************************/
// Redirect all refs to l_ins to the target of j_ins
{
    ins_entry       *ref;
    label_handle    new;
    ins_entry       *next;
    ins_entry       *new_ins;

  optbegin
    new = _Label( j_ins );
    new_ins = new->ins;
    if( UniqueLabel( _Label( l_ins ) ) && UniqueLabel( new ) ) {
        optreturn( NULL );
    }
    for( ref = _Label( l_ins )->refs; ref != NULL; ref = next ) {
        next = _LblRef( ref );
        ChgLblRef( ref, new );
    }
    if( new_ins != NULL && _Class( new_ins ) != OC_DEAD ) {
        Untangle( new_ins );
    }
    if( _Class( l_ins ) == OC_DEAD ) {
        /* got deleted*/
        optreturn( ValidIns( l_ins ) );
    }
    if( new_ins == NULL
     || _Class( new_ins ) == OC_DEAD
     || ( _Attr( l_ins ) & ATTR_SHORT )
     || _TstStatus( _Label( l_ins ), REDIRECTION ) ) {
         optreturn( NextIns( l_ins ) );
    } else {
         optreturn(  AliasLabels( l_ins, new->ins ) );
    }
}


void    OptPush( void )
/*********************/
{
    ins_entry   *ins;

  optbegin
    ins = LastIns;
    for( ;; ) {
        InsDelete = false;
        switch( _Class( ins ) ) {
        case OC_INFO:
            MultiLineNums( ins );
            break;
        case OC_LABEL:
            Untangle( ins );
            if( !InsDelete ) {
                TraceCommon( ins );
            }
            break;
        case OC_CALL:
            if( _IsntModel( NO_OPTIMIZATION ) ) {
                CallRet( ins );
            }
            break;
        case OC_JMP:
            Untangle( PrevIns( ins ) );
            if( !InsDelete ) {
                ComCode( ins );
                JmpRet( ins );
            }
            break;
        case OC_RET:
            if( _IsntModel( NO_OPTIMIZATION ) ) {
                RetAftrLbl( ins );
                RetAftrCall( ins );
            }
            if( !InsDelete ) {
                if( _Attr( ins ) & ATTR_NORET ) {
                    ComTail( NoRetList, ins );
                } else {
                    ComTail( RetList, ins );
                }
            }
            break;
        }
        if( !InsDelete )
            break;      /* nothing happened*/
        FreePendingDeletes();
        /* find the last interesting queue entry and try some more*/
        ins = LastIns;
        for(;;) {
            if( ins == NULL )
                optreturnvoid;
            if( _Class( ins ) != OC_INFO )
                break;
            ins = ins->ins.prev;
        }
    }
  optend
}


void    OptPull( void )
/*********************/
{
    oc_class    ins_class;

  optbegin
    for( ;; ) {
        InsDelete = false;
        ins_class = _Class( FirstIns );
        switch( ins_class ) {
        case OC_LABEL:
            Untangle( FirstIns );
            if( _IsntModel( NO_OPTIMIZATION ) && !InsDelete ) {
                CloneCode( _Label( FirstIns ) );
            }
            break;
        case OC_CALL:
            if( _IsntModel( NO_OPTIMIZATION ) ) {
                CallRet( FirstIns );
            }
            break;
        case OC_JMP:
            if( _IsntModel( NO_OPTIMIZATION ) ) {
                IsolatedCode( FirstIns );
                if( !InsDelete ) {
                    StraightenCode( FirstIns );
                    JmpRet( FirstIns );
                    if( !InsDelete ) {
                        CheckStraightenCode( NextIns( FirstIns ) );
                        if( !InsDelete ) {
                            CloneCode( _Label( FirstIns ) );
                        }
                    }
                }
            }
            break;
        case OC_JMPI:
            if( _IsntModel( NO_OPTIMIZATION ) ) {
                IsolatedCode( FirstIns );
                if( !InsDelete ) {
                    CheckStraightenCode( NextIns( FirstIns ) );
                }
            }
            break;
        case OC_JCOND:
            Untangle( NextIns( FirstIns ) );
            break;
        case OC_RET:
            if( _IsntModel( NO_OPTIMIZATION ) ) {
                IsolatedCode( FirstIns );
                if( !InsDelete ) {
                    CheckStraightenCode( NextIns( FirstIns ) );
                }
            }
            break;
        }
        FreePendingDeletes();
        if( FirstIns == NULL )
            optreturnvoid;
        if( !InsDelete ) {
            break;
        }
    }
    switch( ins_class ) {
    case OC_LREF:
    case OC_CALL:
        _SetStatus( _Label( FirstIns ), KEEPLABEL );
        break;
    case OC_JCOND:
    case OC_JMP:
#if( OPTIONS & SHORT_JUMPS )
        if( (_Attr( FirstIns ) & ATTR_FAR) == 0 ) {
            SetBranches();
        }
#endif
        _SetStatus( _Label( FirstIns ), KEEPLABEL );
        break;
    }
  optend
}
