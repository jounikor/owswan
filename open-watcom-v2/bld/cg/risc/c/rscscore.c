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


#include "_cgstd.h"
#include "coderep.h"
#include "score.h"
#include "data.h"
#include "namelist.h"


#if 0
bool    MultiIns( instruction *ins )
/**********************************/
{
    /* unused parameters */ (void)ins;

    return( false );
}
#endif

void    ScInitRegs( score *scoreboard )
/*************************************/
{
    /* unused parameters */ (void)scoreboard;
}


void    AddRegs( void )
/*********************/
{
}


void    ScoreSegments( score *scoreboard )
/****************************************/
{
    /* unused parameters */ (void)scoreboard;
}


bool    ScAddOk( hw_reg_set reg1, hw_reg_set reg2 )
/*************************************************/
{
    /* unused parameters */ (void)reg1; (void)reg2;

    return( true );
}


bool    ScConvert( instruction *ins )
/***********************************/
{
    /* unused parameters */ (void)ins;

    return( false );
}


bool    CanReplace( instruction *ins )
/************************************/
{
    /* unused parameters */ (void)ins;

    return( true );
}

bool    ScRealRegister( name *reg )
/********************************************
    Return "true" if "reg" is a real machine register and not some
    monstrosity like R1:R0:R2 used for calls.
*/
{
    return( reg->n.type_class != XX );
}
