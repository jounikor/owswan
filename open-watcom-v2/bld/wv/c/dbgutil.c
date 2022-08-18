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
* Description:  Debugger utility routines.
*
****************************************************************************/


#include <ctype.h>
#include <float.h>
#include <limits.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbglit.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "dbgitem.h"
#include "dbgio.h"
#include "ldsupp.h"
#include "dui.h"
#include "strutil.h"
#include "dbgscan.h"
#include "madinter.h"
#include "dbgmad.h"
#include "dbgutil.h"
#include "dbgovl.h"
#include "dbgprog.h"
#include "dipimp.h"
#include "dipinter.h"
#include "dbgset.h"
#include "dbgdot.h"


extern char             *Language;

dig_type_size DefaultSize( default_kind dk )
{
    dig_type_info       ti;
    mad_type_info       mti;

    if( DIPModDefault( CodeAddrMod, dk, &ti ) != DS_OK ) {
        ti.kind = TK_NONE;
        ti.size = 0;
    }
    mti.b.kind = MTK_BASIC;
    if( ti.size == 0 ) {
        GetMADTypeDefaultAt( GetCodeDot(), ( dk == DK_INT ) ? MTK_INTEGER : MTK_ADDRESS, &mti );
        ti.size = BITS2BYTES( mti.b.bits );
        if( mti.b.kind == MTK_ADDRESS ) {
            ti.size -= BITS2BYTES( mti.a.seg.bits );
        }
    }
    if( ti.kind == TK_POINTER && ti.modifier == TM_FAR ) {
        if( mti.b.kind == MTK_BASIC ) {
            /* haven't gotten the info yet */
            GetMADTypeDefaultAt( GetCodeDot(), MTK_ADDRESS, &mti );
        }
        ti.size -= BITS2BYTES( mti.a.seg.bits );
    }
    return( ti.size );
}

static char *DoMadLongConv( char *buff, size_t buff_len, unsigned long value, mad_radix radix, int size )
{
    mad_radix           old_radix;
    mad_type_info       mti;

    old_radix = NewCurrRadix( radix );
    MADTypeInfoForHost( MTK_INTEGER, size, &mti );
    MADTypeToString( radix, &mti, &value, buff, &buff_len );
    NewCurrRadix( old_radix );
    return( buff + buff_len );
}

char *CnvULongHex( unsigned long value, char *buff, size_t buff_len )
{
    return( DoMadLongConv( buff, buff_len, value, 16, sizeof( value ) ) );
}

char    *CnvLongDec( long value, char *buff, size_t buff_len )
{
    return( DoMadLongConv( buff, buff_len, value, 10, SIGNTYPE_SIZE( sizeof( value ) ) ) );
}

char    *CnvULongDec( unsigned long value, char *buff, size_t buff_len )
{
    return( DoMadLongConv( buff, buff_len, value, 10, sizeof( value ) ) );
}

char    *CnvLong( long value, char *buff, size_t buff_len )
{
    return( DoMadLongConv( buff, buff_len, value, CurrRadix, SIGNTYPE_SIZE( sizeof( value ) ) ) );
}

char    *CnvULong( unsigned long value, char *buff, size_t buff_len )
{
    return( DoMadLongConv( buff, buff_len, value, CurrRadix, sizeof( value ) ) );
}

#ifdef DEAD_CODE
void CnvAddrToItem( address *a, item_mach *item, mad_type_info *mti )
{
    //MAD: a bit ugly
    AddrFix( a );
    switch( mti->b.bits - mti->a.seg.bits ) {
    case 16:
        item->sa.offset = a->mach.offset;
        item->sa.segment = a->mach.segment;
        break;
    case 32:
        item->la.offset = a->mach.offset;
        item->la.segment = a->mach.segment;
        break;
    }
}
#endif

char *AddrTypeToString( address *a, mad_type_handle mth, char *buff, size_t buff_len )
{
    mad_type_info       mti;
    item_mach           item;
    mad_type_info       host;

    MADTypeInfo( mth, &mti );
    MADTypeInfoForHost( MTK_ADDRESS, sizeof( address ), &host );
    AddrFix( a );
    MADTypeConvert( &host, a, &mti, &item, 0 );
    MADTypeToString( 16, &mti, &item, buff, &buff_len );
    return( buff + buff_len );
}

char *AddrToIOString( address *a, char *buff, size_t buff_len )
{
    return( AddrTypeToString( a, MADTypeDefault( MAS_IO | MTK_ADDRESS, MAF_FULL, &DbgRegs->mr, a ), buff, buff_len ) );
}

char *AddrToString( address *a, mad_address_format af, char *buff, size_t buff_len )
{
    return( AddrTypeToString( a, MADTypeDefault( MTK_ADDRESS, af, &DbgRegs->mr, a ), buff, buff_len ) );
}

size_t QualifiedSymName( sym_handle *sh, char *name, size_t max, bool uniq )
{
    mod_handle  mod;
    size_t      name_len;
    size_t      len;
    size_t      rc;
    sym_info    sinfo;

    DIPSymInfo( sh, NULL, &sinfo );
    if( sinfo.is_global ) {
        len = 0;
    } else {
        mod = DIPSymMod( sh );
        name_len = DIPModName( mod, name, max );
        if( name_len < max ) {
            if( name != NULL ) {
                name += name_len;
                *name++ = '@';
                max -= name_len + 1;
            }
        }
        len = name_len + 1;
    }
    if( uniq && DIPSymName( sh, NULL, SNT_DEMANGLED, NULL, 0 ) != 0 ) {
        if( name != NULL ) {
            *name++ = '`';
            max--;
        }
        len++;
        name_len = DIPSymName( sh, NULL, SNT_OBJECT, name, max );
        len += name_len;
        if( name != NULL ) {
            name += name_len;
            *name++ = '`';
            *name = NULLCHAR;
            max -= name_len + 1;
        }
        len++;
    } else {
        rc = DIPSymName( sh, NULL, SNT_SCOPED, name, max );
        if( rc == 0 ) {
            rc = DIPSymName( sh, NULL, SNT_SOURCE, name, max );
        }
        len += rc;
    }
    return( len );
}

/*
 * CnvAddr -- address to string convertion gut routine
 */

char *CnvAddr( address addr, cnvaddr_option cao, bool uniq,
                        char *p, size_t max )
{
    char                off_buff[40];
    size_t              str_width, name_width, off_width;
    addr_off            sym_offset;
    search_result       sr;
    location_list       ll;
    DIPHDL( sym, sym );

    off_width = 0;
    AddrFloat( &addr );
    sr = DeAliasAddrSym( NO_MOD, addr, sym );
    switch( sr ) {
    case SR_NONE:
        return( NULL );
    case SR_CLOSEST:
        if( cao == CAO_NO_PLUS )
            return( NULL );
        DIPSymLocation( sym, NULL, &ll );
        sym_offset = addr.mach.offset - ll.e[0].u.addr.mach.offset;
        if( cao == CAO_NORMAL_PLUS ) {
            char    *prfx;
            prfx = AddHexSpec( off_buff );
            off_width = CnvULongHex( sym_offset, prfx, sizeof( off_buff ) - ( prfx - off_buff ) ) - off_buff + 1;
        }
        break;
    case SR_EXACT:
        break;
    }
    name_width = QualifiedSymName( sym, NULL, 0, uniq );
    str_width = name_width + off_width;
    if( max == 0 )
        max = str_width;
    if( str_width > max ) { /* won't fit */
        if( off_width != 0 )
            return( NULL );
        QualifiedSymName( sym, p, max - 1, uniq );
        p += max - 1;
        *p++ = '>';
        *p = NULLCHAR;
        return( p );
    }
    QualifiedSymName( sym, p, name_width + 1, uniq );
    p += name_width;
    if( off_width != 0 ) {
        --off_width;
        *p++ = '+';
        memcpy( p, off_buff, off_width );
        p += off_width;
        *p = NULLCHAR;
    }
    return( p );
}

char *CnvNearestAddr( address addr, char *buff, size_t buff_len )
{
    char        *p;

    p = CnvAddr( addr, CAO_OMIT_PLUS, false, buff, buff_len );
    if( p == NULL ) {
        p = AddrToString( &addr, MAF_FULL, buff, buff_len );
    }
    return( p );
}

/*
 * StrAddr --
 */

char *StrAddr( address *addr, char *buff, size_t buff_len )
{
    char        *p;

    p = CnvAddr( *addr, CAO_NORMAL_PLUS, false, buff, buff_len );
    if( p == NULL ) {
        p = AddrToString( addr, MAF_FULL, buff, buff_len );
    }
    return( p );
}


char *UniqStrAddr( address *addr, char *buff, size_t buff_len )
{
    char        *p;

    p = CnvAddr( *addr, CAO_NORMAL_PLUS, true, buff, buff_len );
    if( p == NULL ) {
        p = AddrToString( addr, MAF_FULL, buff, buff_len );
    }
    return( p );
}


/*
 * LineAddr
 */

char *LineAddr( address  *addr, char *buff, size_t buff_len )
{
    mod_handle          mod;
    char                *end;
    DIPHDL( cue, cueh_line );

    AddrFloat( addr );
    if( DeAliasAddrMod( *addr, &mod ) == SR_NONE )
        return( NULL );
    if( DeAliasAddrCue( mod, *addr, cueh_line ) == SR_NONE )
        return( NULL );
    end = buff + buff_len;
    buff += DIPModName( mod, buff, buff_len - 1 );
    *buff++ = '@';
    buff = CnvULongDec( DIPCueLine( cueh_line ), buff, end - buff );
    *buff = NULLCHAR;
    return( buff );
}


/*
 * RingBell - ring a bell, if required
 */

void RingBell( void )
{
    if( _IsOn( SW_BELL ) ) {
        DUIRingBell();
    }
}


/*
 * Warn - output a warning message and ring the bell
 */

void Warn( char *p )
{
    DUIMsgBox( p );
    DUIFlushKeys(); /* clear any pending keystrokes */
    RingBell();
}



/*
 * Rtrm  -- right trim a string
 */

static char *Rtrm( char *p )
{
    char        *op;

    op = p;
    p += strlen( p );
    do {
        --p;
    } while( *p == ' ' && p >= op );
    ++p;
    *p = NULLCHAR;
    return( p );
}

/*
 * AllocCmdList -- allocate a command list
 */

cmd_list *AllocCmdList( const char *start, size_t len )
{
    cmd_list *cmds;

    cmds = DbgMustAlloc( sizeof( cmd_list ) + len );
    cmds->use = 1;
    cmds->buff[len] = NULLCHAR;
    memcpy( cmds->buff, start, len );
    return( cmds );
}


/*
 * FreeCmdList -- decrement use count, if zero then free list
 */

void FreeCmdList( cmd_list *cmds )
{
    if( cmds == NULL )
        return;
    cmds->use--;
    if( cmds->use == 0 ) {
        _Free( cmds );
    }
}

/* Lock List -- increment use count */

void LockCmdList( cmd_list *cmds )
{
    if( cmds == NULL )
        return;
    cmds->use++;
}


/*
 * TypeInpStack -- set new type on input stack
 */

void TypeInpStack( input_type set )
{

    if( InpStack != NULL ) {
        InpStack->type |= set;
    }
}


void ClearInpStack( input_type clear )
{

    if( InpStack != NULL ) {
        InpStack->type &= ~clear;
    }
}

input_type SetInpStack( input_type new )
{
    input_type  old;

    old = 0;
    if( InpStack != NULL ) {
        old = InpStack->type;
        InpStack->type = new;
    }
    return( old );
}


/*
 * PopInpStack -- remove a level from the input stack
 */

void PopInpStack( void )
{
    input_stack *old;
    char        buff[TXT_LEN];

    old = InpStack;
    if( old == NULL )
        return;
    if( old->lang != NULL ) {
        StrCopy( old->lang, buff );
        _Free( old->lang );
        old->lang = NULL; /* in case NewLang gets an error */
        NewLang( buff );
    }
    old->rtn( old->handle, INP_RTN_FINI );
    ReScan( old->scan );
    InpStack = old->link;
    _Free( old );
}


/*
 * PushInpStack -- add a new level to the input stack
 */

void PushInpStack( inp_data_handle handle, inp_rtn_func *rtn, bool save_lang )
{
    input_stack *new;
    char        *lang;

    _Alloc( new, sizeof( input_stack ) );
    if( new == NULL ) {
        rtn( handle, INP_RTN_FINI ); /* clean up handle */
        Error( ERR_NONE, LIT_ENG( ERR_NO_MEMORY ) );
    }
    if( save_lang ) {
        lang = DupStr( Language );
        if( lang == NULL ) {
            Error( ERR_NONE, LIT_ENG( ERR_NO_MEMORY ) );
        }
        new->lang = lang;
    } else {
        new->lang = NULL;
    }
    new->handle = handle;
    new->rtn = rtn;
    new->type = INP_NORMAL;
    new->scan = ScanPos();
    new->link = InpStack;
    InpStack = new;
    if( !new->rtn( handle, INP_RTN_INIT ) ) {
        PopInpStack();
    }
}

void CopyInpFlags( void )
{
    if( InpStack == NULL )
        return;
    if( InpStack->link == NULL )
        return;
    InpStack->type |= InpStack->link->type & (INP_HOOK+INP_BREAK_POINT);
}


static bool DoneCmdList( inp_data_handle _cmds, inp_rtn_action action )
{
    cmd_list    *cmds = _cmds;

    switch( action ) {
    case INP_RTN_INIT:
        ReScan( cmds->buff );
        return( true );
    case INP_RTN_EOL:
        return( false );
    case INP_RTN_FINI:
        FreeCmdList( cmds );
        return( true );
    }
    return( false ); // silence compiler
}


/*
 * PushCmdList -- push a command list onto the input stack
 */

void PushCmdList( cmd_list *cmds )
{
    cmds->use++;
    PushInpStack( cmds, DoneCmdList, false );
}


#ifdef DEADCODE
static bool DoneCmdText( inp_data_handle cmds, inp_rtn_action action )
{
    switch( action ) {
    case INP_RTN_INIT:
        ReScan( cmds );
        return( true );
    case INP_RTN_EOL:
        return( false );
    case INP_RTN_FINI:
        return( true );
    }
    return( false ); // silence compiler
}
#endif


/*
 * PushCmdText -- push a command string
 */

#ifdef DEADCODE
void PushCmdText( char *cmds )
{
    PushInpStack( cmds, DoneCmdText, false );
}
#endif


/*
 * PurgeInpStack -- clean up input stack
 */

bool PurgeInpStack( void )
{
    while( InpStack != NULL ) {
        if( InpStack->type & INP_STOP_PURGE )
            return( false );
        PopInpStack();
    }
    return( true );
}

static bool DoneNull( inp_data_handle buff, inp_rtn_action action )
{
    switch( action ) {
    case INP_RTN_INIT:
        ReScan( buff );
        return( true );
    case INP_RTN_EOL:
        return( false );
    case INP_RTN_FINI:
        return( true );
    }
    return( false ); // silence compiler
}

void FreezeInpStack( void )
{
    PushInpStack( LIT_ENG( Empty ), DoneNull, false );
    TypeInpStack( INP_NEW_LANG | INP_HOLD | INP_STOP_PURGE );
}

void UnAsm( address addr, char *buff, size_t buff_len )
{
    mad_disasm_data     *dd;

    _AllocA( dd, MADDisasmDataSize() );

    MADDisasm( dd, &addr, 0 );
    MADDisasmFormat( dd, MDP_ALL, CurrRadix, buff, buff_len );
    Rtrm( buff );
}

const char *ModImageName( mod_handle handle )
{
    image_entry *image;

    image = ImageEntry( handle );
    if( image == NULL || image == ImagePrimary() ) {
        return( LIT_ENG( Empty ) );
    } else {
        return( SkipPathInfo( image->image_name, OP_REMOTE ) );
    }
}

static walk_result RegWalkList( const mad_reg_set_data *data, void *pdata )
{
    *((const mad_reg_set_data **)pdata) = data;
    return( WR_STOP );
}

void RegFindData( mad_type_kind kind, const mad_reg_set_data **pdata )
{
    *pdata = NULL;
    MADRegSetWalk( kind, RegWalkList, (void *)pdata );
}

