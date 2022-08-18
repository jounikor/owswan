/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Process macro definitions.
*
****************************************************************************/


#include <string.h>

#include "dwpriv.h"
#include "dwcliuti.h"
#include "dwutils.h"
#include "dwmem.h"
#include "dwmac.h"

typedef struct dw_parm {
    struct dw_parm      *next;
    size_t              len;
    char                name[1];        /* not nul terminated */
} dw_parm;

struct dw_macro {
    dw_parm             *parms;
    dw_linenum          line;
    size_t              len;
    char                name[1];        /* not nul terminated */
};


void DWENTRY DWMacStartFile( dw_client cli, dw_linenum line, const char *name )
{
    uint_8              buf[1 + 2 * MAX_LEB128];
    uint_8              *end;

    buf[0] = DW_MACINFO_start_file;
    end = ULEB128( buf + 1, line );
    end = ULEB128( end, GetFileNumber( cli, name ) );
    CLIWrite( cli, DW_DEBUG_MACINFO, buf, end - buf );
}


void DWENTRY DWMacEndFile( dw_client cli )
{
    CLIWriteU8( cli, DW_DEBUG_MACINFO, DW_MACINFO_end_file );
}


dw_macro DWENTRY DWMacDef( dw_client cli, dw_linenum line, const char *name )
{
    dw_macro            mac;
    size_t              len;

    _Validate( name != NULL );

    len = strlen( name );
    mac = CLIAlloc( cli, sizeof( struct dw_macro ) - 1 + len );
    mac->parms = NULL;
    mac->line = line;
    mac->len = len;
    memcpy( mac->name, name, len );
    return( mac );
}


void DWENTRY DWMacParam( dw_client cli, dw_macro mac, const char *name )
{
    dw_parm             *parm;
    size_t              len;

    _Validate( mac != NULL && name != NULL );

    len = strlen( name );
    parm = CLIAlloc( cli, sizeof( dw_parm ) - 1 + len );
    parm->next = mac->parms;
    mac->parms = parm;
    parm->len = len;
    memcpy( parm->name, name, len );
}


void DWENTRY DWMacFini( dw_client cli, dw_macro mac, const char *def )
{
    uint_8              buf[1 + MAX_LEB128];
    uint_8              *end;
    dw_parm             *parm;

    _Validate( mac != NULL );

    buf[0] = DW_MACINFO_define;
    end = ULEB128( buf + 1, mac->line );
    CLIWrite( cli, DW_DEBUG_MACINFO, buf, end - buf );
    CLIWrite( cli, DW_DEBUG_MACINFO, mac->name, mac->len );
    parm = mac->parms;
    if( parm != NULL ) {
        CLIWriteU8( cli, DW_DEBUG_MACINFO, '(' );
        /* parms are in the linked list in reverse order */
        for( parm = ReverseChain( parm ); parm->next != NULL; parm = FreeLink( cli, parm ) ) {
            CLIWrite( cli, DW_DEBUG_MACINFO, parm->name, parm->len );
            CLIWriteU8( cli, DW_DEBUG_MACINFO, ',' );
        }
        CLIWrite( cli, DW_DEBUG_MACINFO, parm->name, parm->len );
        CLIWriteU8( cli, DW_DEBUG_MACINFO, ')' );
        CLIFree( cli, parm );
    }
    /* write parms terminator */
    CLIWriteU8( cli, DW_DEBUG_MACINFO, ' ' );
    CLIFree( cli, mac );
    if( def == NULL )
        def = "";
    CLIWriteString( cli, DW_DEBUG_MACINFO, def );
}


void DWENTRY DWMacUnDef( dw_client cli, dw_linenum line, const char *name )
{
    uint_8              buf[1 + MAX_LEB128];
    uint_8              *end;

    _Validate( name != NULL );

    buf[0] = DW_MACINFO_undef;
    end = ULEB128( buf + 1, line );
    CLIWrite( cli, DW_DEBUG_MACINFO, buf, end - buf );
    CLIWriteString( cli, DW_DEBUG_MACINFO, name );
}


void DWENTRY DWMacUse( dw_client cli, dw_linenum line, const char *name )
{
    uint_8              buf[2 + MAX_LEB128];
    uint_8              *end;

    _Validate( name != NULL );

    buf[0] = DW_MACINFO_vendor_ext;
    end = ULEB128( buf + 1, line );
    *end++ = 1;
    CLIWrite( cli, DW_DEBUG_MACINFO, buf, end - buf );
    CLIWriteString( cli, DW_DEBUG_MACINFO, name );
}


void InitDebugMacInfo( dw_client cli )
/* must be called after InitDebugLine */
{
    /* unused parameters */ (void)cli;
}


void FiniDebugMacInfo( dw_client cli )
{
    /* write the terminator */
    CLISectionWriteZeros( cli, DW_DEBUG_MACINFO, 1 );
}
