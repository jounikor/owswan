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
* Description:  MIPS MAD disassembly control.
*
****************************************************************************/


#include "mips.h"

imp_mad_state_data      *MADState;

mad_status MADIMPENTRY( Init )(void)
{
    mad_status  ms;

    ms = RegInit();
    if( ms != MS_OK )
        return ( ms );
    ms = DisasmInit();
    if( ms == MS_OK )
        return( ms );
    RegFini();
    return( ms );
}

void MADIMPENTRY( Fini )(void)
{
    DisasmFini();
    RegFini();
}

unsigned MADIMPENTRY( StateSize )( void )
{
    return( sizeof( *MADState ) );
}

void MADIMPENTRY( StateInit )( imp_mad_state_data *new )
{
    memset( new, 0, sizeof( *new ) );
    new->reg_state[CPU_REG_SET] = CT_HEX | CT_SYMBOLIC_NAMES;
    new->disasm_state = DT_PSUEDO_OPS;
}

void MADIMPENTRY( StateSet )( imp_mad_state_data *new )
{
    MADState = new;
}

void MADIMPENTRY( StateCopy )( imp_mad_state_data const *src, imp_mad_state_data *dst )
{
    memcpy( dst, src, sizeof( *dst ) );
}
