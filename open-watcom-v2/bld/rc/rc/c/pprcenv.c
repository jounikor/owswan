/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2013-2013 The Open Watcom Contributors. All Rights Reserved.
*
* =========================================================================
*
* Description:  C preprocessor environment processing.
*
****************************************************************************/


#include "global.h"
#include "preproc.h"
#include "rccore.h"


const char * PPENTRY PP_GetEnv( const char *name )
{
    return( RcGetEnv( name ) );
}
