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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "drwatcom.h"
#include "mad.h"
#include "madcli.h"

#define MAX_INSTR_BYTES (8)

#ifdef __NT__
static HANDLE           processHandle;
static ModuleNode       *curModule;

void SetDisasmInfo( HANDLE prochdl, ModuleNode *mod ) {
    processHandle = prochdl;
    curModule = mod;
}
#endif

static mad_disasm_data  *MDData=NULL;

bool FindWatSymbol( address *addr, syminfo *si, bool getsrcinfo )
{
    DWORD       symoff;
    DWORD       line;

    if( !GetSymbolName( addr, si->name, &symoff ) ) {
        return( false );
    }
    si->symoff = symoff;
    if( getsrcinfo ) {
        if( !GetLineNum( addr, si->filename, MAX_FILE_NAME, &line ) )
            return( false );
        si->linenum = line;
    }
    return( true );
}

bool AllocMadDisasmData(void)
{
    unsigned    new;

    new = MADDisasmDataSize();
    MDData = MemAlloc( new );
    if( MDData == NULL ) {
        return( false );
    }
    return( true );
}

void DeAllocMadDisasmData( void )
{
    MemFree( MDData );
}

static unsigned FormatAddr( address *a, char *buffer, unsigned max )
{
    mad_type_info       host_mti;
    mad_type_info       mti;
    unsigned_8          item[16];
    int                 i;
    syminfo     si;
    if( StatShowSymbols && FindWatSymbol( a, &si, false ) ) {
        if( si.symoff == 0L ) {
            sprintf( buffer, "%s ", si.name );
        } else {
            sprintf( buffer, "%s+%lx ", si.name, si.symoff );
        }
        for ( i = strlen( buffer ); i <20; i++ ){
            buffer[i]=' ';
        }
        buffer[i] = '\0';
        return( i );
    }

    MADTypeInfoForHost( MTK_ADDRESS, sizeof( address ), &host_mti );
    MADTypeInfo( MADTypeDefault( MTK_ADDRESS, MAF_FULL, NULL, a ), &mti );
    MADTypeConvert( &host_mti, a, &mti, item, 0 );
    MADTypeToString( 16, &mti, item, buffer, &max );
    return( max );
}

int InstructionBackward( int cnt, address *addr )
{
    int i;
    unsigned ins_size;
    for (i=0;i<cnt;i++){
        if (MADDisasm( MDData, addr, -1 ) != MS_OK){
            break;
        }
        ins_size=MADDisasmInsSize(MDData);
        MADAddrAdd( addr, -(long)ins_size, MAF_FULL);
    }
    return i;
}

int InstructionFoward(int cnt, address *addr)
{
    int i;
    for (i=0;i<cnt;i++){
        if (MADDisasm( MDData, addr, 0  ) != MS_OK){
            break;
        }
    }
    return i;
}

void Disassemble( address *addr, char *buf,int addbytes, unsigned max )
{
    char        *tail;
    address     temp;
    char        *item;
    unsigned    ins_size;
    unsigned    i;

    temp = *addr;
    tail = &buf[ FormatAddr( addr, buf, max ) ];
    *tail++ = ' ';              /* two spaces */
    *tail++ = ' ';
    MADDisasm( MDData, addr, 0 );
    if( addbytes  ) {
        ins_size = MADDisasmInsSize( MDData );
        item = alloca( ins_size );
        MADCli( ReadMem )( temp, ins_size, item );
        for( i = 0; i < ins_size || i < MAX_INSTR_BYTES; i++ ) {
            if( i < ins_size ) {
                sprintf( tail,"%02x", item[i] );
                tail +=2;
            } else if( i < MAX_INSTR_BYTES ) {
                *tail++ = ' ';          /* two spaces */
                *tail++ = ' ';
            }
            *tail++ = ' ';
        }
        *tail++ = ' ';          /* two spaces */
        *tail++ = ' ';
    }
    MADDisasmFormat( MDData, MDP_ALL, 16, tail, max - ( tail - buf ) );
}
