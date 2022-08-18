/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2011-2021 The Open Watcom Contributors. All Rights Reserved.
*
* =========================================================================
*
* Description:  rsi get far pointer absolute address.
*
****************************************************************************/


#include "rsi1632.h"

long rsi_abs_address( void FarPtr p )
{
    descriptor   g;

    if( rsi_get_descriptor( _FP_SEG( p ), &g ) ) {
        return( GDT32BASE( g ) + _FP_OFF( p ) );
    }
    return( -1 );
}
