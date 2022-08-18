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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "cvars.h"
#include "pragdefn.h"
#include "pdefn2.h"
#include "caux.h"


static  int     Offset;

hw_reg_set PragRegName( const char * buffer )
    {
        hw_reg_set      name;

        buffer = buffer; /* don't need it. */
        HW_CAsgn( name, HW_EMPTY );
        if( CurToken != T_CONSTANT || Constant > 15 ) return( name );
        return( RegBits[ Constant ] );
    }


void PragmaInit( void )
{
    Offset = 0;
}


void PragmaFini( void )
/*********************/
{
}


static call_class PragLinkage()
{
    call_class  linkage;

    linkage = 0;
    if( PragRecog( "os" ) || PragRecog( "OS" ) ) {
        linkage = LINKAGE_OS;
    } else if( PragRecog( "osfunc" ) ) {
        linkage = LINKAGE_OSFUNC;
    } else if( PragRecog( "osentry" ) ) {
        linkage = LINKAGE_OSENTRY;
    } else if( PragRecog( "clink" ) ) {
        linkage = LINKAGE_CLINK;
    }
    return( linkage );
}


static void PragInitCurrInfo( call_class linkage )
{
    *CurrInfo = *CurrAlias;
    if( linkage != 0 ) {
        if( linkage == LINKAGE_CLINK ) {
            CurrInfo->linkage = &DefaultLinkage;
        } else {
            CurrInfo->linkage = &OSLinkage;
        }
        CurrInfo->class &= ~LINKAGES;
        CurrInfo->class |= linkage;
    }
}


void PragAux()
    {
        call_class      linkage;
        struct {
            unsigned f_equal  : 1;
            unsigned f_parm   : 1;
            unsigned f_linkage: 1;
            unsigned f_value  : 1;
            unsigned f_modify : 1;
            unsigned f_offset : 1;
        } have;


        if( CheckForOrigin() ) return;
        if( !GetPragmaAuxAliasInfo() ) return;
        CurrEntry = NULL;
        if( CurToken != T_ID ) return;
        linkage = PragLinkage();
        SetCurrInfo( Buffer );
        NextToken();
        PragInitCurrInfo( linkage );
        have.f_offset = 0;
        have.f_equal = 0;
        have.f_parm = 0;
        have.f_linkage = 0;
        have.f_value = 0;
        have.f_modify = 0;
        for( ;; ) {
            if( !have.f_equal && CurToken == T_EQUAL ) {
                // GetByteSeq();
                have.f_equal = 1;
            } else if( !have.f_parm && PragRecog( "parm" ) ) {
                GetParmInfo();
                have.f_parm = 1;
            } else if( !have.f_linkage && PragRecog( "registers" ) ) {
                GetLinkInfo();
                have.f_linkage = 1;
            } else if( !have.f_value && PragRecog( "value" ) ) {
                GetRetInfo();
                have.f_value = 1;
            } else if( !have.f_modify && PragRecog( "modify" ) ) {
                GetSaveInfo();
                have.f_modify = 1;
            } else if( !have.f_offset && PragRecog( "offset" ) ) {
                GetOffsetInfo();
                have.f_offset = 1;
            } else {
                break;
            }
        }
        PragmaAuxEnding();
    }

static int TryForReg( hw_reg_set *field, char *name )
    {
        if( HW_CEqual( *field, HW_EMPTY ) && PragRecog( name ) ) {
            *field = PragRegName("");
            NextToken();
            return( 1 );
        }
        return( 0 );
    }


static void NotEmpty( hw_reg_set *link, hw_reg_set *curr )
    {
        if( HW_CEqual( *link, HW_EMPTY ) ) {
            HW_Asgn( *link, *curr );
        }
    }


static void GetLinkInfo()
    {
        linkage_regs    *link;

        link = (linkage_regs *)CMemAlloc( sizeof( *link ) );
        memset( link, 0, sizeof( *link ) ); /* assume equivalent to HW_EMPTY */
        for( ;; ) {
            if( TryForReg( &link->gp, "gp" ) ) continue;
            if( TryForReg( &link->sp, "sp" ) ) continue;
            if( TryForReg( &link->ln, "ln" ) ) continue;
            if( TryForReg( &link->ra, "ra" ) ) continue;
            if( TryForReg( &link->pr, "pr" )) continue;
            if( TryForReg( &link->sa, "sa" ) ) continue;
            break;
        }
        NotEmpty( &link->gp, &CurrInfo->linkage->gp );
        NotEmpty( &link->sp, &CurrInfo->linkage->sp );
        NotEmpty( &link->ln, &CurrInfo->linkage->ln );
        NotEmpty( &link->ra, &CurrInfo->linkage->ra );
        NotEmpty( &link->pr, &CurrInfo->linkage->pr);
        NotEmpty( &link->sa, &CurrInfo->linkage->sa );
        CurrInfo->linkage = link;
    }


static void GetParmInfo()
    {
        if( PragRegSet() != T_NULL ) {
            PragManyRegSets();
        }
    }


static int CheckForOrigin()
    {
        if( !PragRecog( "origin" ) ) return( false );
        if( CurToken != T_CONSTANT ) return( false );
        Offset = Constant;
        NextToken();
        return( true );
    }


static void GetOffsetInfo()
    {
        if( CurToken != T_CONSTANT ) return;
        CurrEntry->offset = Offset;
        Offset += Constant;
        NextToken();
    }


static void GetSaveInfo()
    {
        hw_reg_set      reg;

        reg = PragRegList();
        HW_TurnOff( CurrInfo->save, reg );
    }


static void GetRetInfo()
    {
        CurrInfo->returns = PragRegList();
    }
