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
* Description:  Routines to handle keyboard input.
*
****************************************************************************/


#include <stdlib.h>
#include <windows.h>
#include "fmedit.def"
#include "object.def"
#include "state.def"
#include "currobj.def"
#include "mouse.def"
#include "keybd.def"


static bool IgnoreKbd( int keycode )
/**********************************/
{
    keycode = keycode;
    return( false );
}

static void MoveCurrObj( LPPOINT pt )
/***********************************/
{
    OBJPTR  curr;

    for( curr = GetEditCurrObject(); curr != NULL; curr = GetNextEditCurrObject( curr ) ) {
        Move( curr, pt, true );
    }
}

static bool FilterMoveKeys( int keycode, LPPOINT pt )
/***************************************************/
{
    bool    ret;

    ret = true;
    switch( keycode ) {
    case VK_LEFT:
        pt->x = -GetHorizontalInc();
        pt->y = 0;
        break;
    case VK_UP:
        pt->x = 0;
        pt->y = -GetVerticalInc();
        break;
    case VK_RIGHT:
        pt->x = GetHorizontalInc();
        pt->y = 0;
        break;
    case VK_DOWN:
        pt->x = 0;
        pt->y = GetVerticalInc();
        break;
    default:
        ret = false;
        break;
    }
    return( ret );
}

static void DoKbdMove( LPPOINT pt )
/*********************************/
{
    LIST    *list;

    list = GetCurrObjectList();
    SetShowEatoms( false );
    BeginMoveOperation( list );
    ListFree( list );
    MoveCurrObj( pt );
    FinishMoveOperation( false );
    SetShowEatoms( true );
}

static void SetKbdMoveGrid( void )
/********************************/
{
    OBJPTR      curr;
    int         hinc;
    int         vinc;
    POINT       pt;

    hinc = 0;
    vinc = 0;
    for( curr = GetEditCurrObject(); curr != NULL; curr = GetNextEditCurrObject( curr ) ) {
        if( ResizeIncrements( curr, &pt ) ) {
            if( hinc < pt.x )
                hinc = pt.x;
            if( vinc < pt.y ) {
                vinc = pt.y;
            }
        }
    }
    if( hinc > 0 && vinc > 0 ) {
        SetResizeGrid( hinc, vinc );
    }
}

static bool CheckKbdMove( int keycode )
/*************************************/
{
    POINT   pt;

    if( FilterMoveKeys( keycode, &pt ) ) {
        SetState( KBD_MOVING );
        SetKbdMoveGrid();
        DoKbdMove( &pt );
        return( true );
    } else {
        return( false );
    }
}

static bool ContinueKbdMove( int keycode )
/****************************************/
{
    POINT   pt;

    if( FilterMoveKeys( keycode, &pt ) ) {
        DoKbdMove( &pt );
        return( true );
    } else {
        return( false );
    }
}

static bool EndKbdMove( int keycode )
/***********************************/
{
    POINT   dummy;

    if( FilterMoveKeys( keycode, &dummy ) ) {
        SetDefState();
        return( true );
    } else {
        return( false );
    }
}

static bool (*KeyDownActions[])( int keycode ) = {
    #define pick(id,curs,kdown,kup,mpres,mmove,mrel) kdown,
    #include "_state.h"
    #undef pick
};

static bool (*KeyUpActions[])( int keycode ) = {
    #define pick(id,curs,kdown,kup,mpres,mmove,mrel) kup,
    #include "_state.h"
    #undef pick
};

bool ProcessKeyDown( int keycode )
/********************************/
{
    return( KeyDownActions[GetState()]( keycode ) );
}

bool ProcessKeyUp( int keycode )
/******************************/
{
    return( KeyUpActions[GetState()]( keycode ) );
}
