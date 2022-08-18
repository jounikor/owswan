/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2013-2013 The Open Watcom Contributors. All Rights Reserved.
*
* =========================================================================
*
* Description:  Environment processing.
*
****************************************************************************/


#include "_preproc.h"

const char * PPENTRY PP_GetEnv( const char *name )
{
    return( getenv( name ) );
}
