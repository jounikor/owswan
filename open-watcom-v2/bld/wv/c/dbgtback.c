/****************************************************************************
*
*                            Open Watcom Project
*
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


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "dbgstk.h"
#include "dbgtback.h"
#include "mad.h"
#include "dbgexpr.h"
#include "dbgovl.h"
#include "dipimp.h"
#include "dipinter.h"


static unsigned                 OvlLevel;

bool FixOvlRetAddr( address *return_addr )
{
    return_addr->sect_id = 0;
    return_addr->indirect = false;
    if( TransOvlRetAddr( return_addr, OvlLevel ) ) {
        return( true );
    } else {
        AddrFloat( return_addr );
        return( false );
    }
}

bool WalkCallChain( CALL_CHAIN_RTN *walk, void *info )
{
    call_chain_entry    entry;
    int                 levels;
    location_list       ll;
    sym_info            sinfo;
    DIPHDL( sym, rtn );
    mad_status          ms;
    mad_registers       *mr;
    bool                have_sym;
    mad_call_up_data    *mcud;

    InitLC( &entry.lc, true );
    _AllocA( mcud, MADCallUpStackSize() );
    mr = (entry.lc.regs != NULL) ? &entry.lc.regs->mr : NULL;
    if( MADCallUpStackInit( mcud, mr ) != MS_OK )
        return( false );
    OvlLevel = 0;
    levels = 0;
    for( ;; ) {
        if( DeAliasAddrSym( NO_MOD, entry.lc.execution, rtn ) != SR_NONE &&
            DIPSymLocation( rtn, NULL, &ll ) == DS_OK ) {
            entry.start = ll.e[0].u.addr;
            have_sym = true;
        } else {
            entry.start = entry.lc.execution;
            have_sym = false;
        }
        if( !walk( &entry, info ) )
            break;
        if( have_sym && entry.start.mach.offset == entry.lc.execution.mach.offset ) {
            /* at start of routine */
            entry.lc.maybe_have_frame = true;
            entry.lc.have_frame = false;
        } else {
            entry.lc.maybe_have_frame = false;
            entry.lc.have_frame = true;
        }
        ++levels;
        if( have_sym ) {
            if( DIPSymInfo( rtn, NULL, &sinfo ) != DS_OK )
                break;
            if( sinfo.kind != SK_PROCEDURE ) {
                sinfo.ret_addr_offset = (addr_off)-1L;
                sinfo.rtn_far = 0;
            }
        } else {
            sinfo.ret_addr_offset = (addr_off)-1L;
            sinfo.rtn_far = 0;
        }

        mr = (entry.lc.regs != NULL) ? &entry.lc.regs->mr : NULL;
        ms = MADCallUpStackLevel( mcud, &entry.start,
                                sinfo.rtn_far,
                                sinfo.ret_addr_offset,
                                mr,
                                &entry.lc.execution,
                                &entry.lc.frame,
                                &entry.lc.stack,
                                &mr );
        if( mr == NULL )
            entry.lc.regs = NULL;
        if( ms != MS_OK )
            break;
        entry.lc.up_stack_level = true;
        OvlLevel += FixOvlRetAddr( &entry.lc.execution );
    }
    return( levels != 0 );
}


typedef struct {
    address     addr;
    bool        first;
} return_info;

static bool RecordOneLevel( call_chain_entry *entry, void *_info )
{
    return_info *info = _info;

    if( info->first ) {
        info->first = false;
        return( true );
    } else {
        info->addr = entry->lc.execution;
        return( false );
    }
}


address ReturnAddress( void )
{
    return_info ret;

    ret.addr = NilAddr;
    ret.first = true;
    WalkCallChain( RecordOneLevel, &ret );
    return( ret.addr );
}
