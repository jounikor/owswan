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
* Description:  Functions controlling the abstract type "a_variable".
*               The only reference to the GlobalVarList is through these
*               functions.
*
****************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "wio.h"
#include "setup.h"
#include "guiutil.h"
#include "setupinf.h"
#include "utils.h"
#include "dlggen.h"
#include "dynarray.h"
#include "hash.h"
#include "genvbl.h"

#include "clibext.h"


// If too slow make hash size bigger
#define HASH_SIZE   1021

typedef struct  a_variable {
    char        *name;
    char        *strval;    /* value */
    char        *autoset;
    bool        has_value;
    char        restriction;
    void        (*hook)( vhandle );
} a_variable;

            //only reference to this Structure is through functions.
static a_variable   *GlobalVarList = NULL;
static array_info   GlobalVarArray;
static hash_table   *GlobalVarHash = NULL;


vhandle NextGlobalVar( vhandle var_handle )
/*****************************************/
{
    if( var_handle == NO_VAR )
        return( 0 );
    if( ++var_handle >= GlobalVarArray.num )
        return( NO_VAR );
    return( var_handle );
}

void InitGlobalVarList( void )
/****************************/
/* must be called before any other reference to GlobalVarList is made */
{
    GlobalVarArray.num = 0;
    GlobalVarArray.alloc = 20;
    GlobalVarArray.increment = 20;
    InitArray( (void **)&GlobalVarList, sizeof( a_variable ), &GlobalVarArray );
    GlobalVarHash = HashInit( HASH_SIZE, &stricmp );
}


void VarSetAutoSetCond( vhandle var_handle, const char *cond )
/************************************************************/
{
    GlobalVarList[var_handle].autoset = GUIStrDup( cond, NULL );
}


void VarSetAutoSetRestriction( vhandle var_handle, const char *cond )
/*******************************************************************/
{
    GlobalVarList[var_handle].restriction = cond[0]; // 't' or 'f'
}


bool VarIsRestrictedFalse( vhandle var_handle )
/*********************************************/
{
    return( GlobalVarList[var_handle].restriction == 'f' );
}


bool VarIsRestrictedTrue( vhandle var_handle )
/********************************************/
{
    return( GlobalVarList[var_handle].restriction == 't' );
}


const char *VarGetAutoSetCond( vhandle var_handle )
/*************************************************/
{
    if( GlobalVarList[var_handle].restriction == 't' )
        return( "true" );
    if( GlobalVarList[var_handle].restriction == 'f' )
        return( "false" );
    return( GlobalVarList[var_handle].autoset );
}

vhandle GetVariableByName( const char *vbl_name )
/***********************************************/
{
    vhandle     var_handle;

    if( GlobalVarHash != NULL ) {
        return( HashFind( GlobalVarHash, vbl_name ) );
    } else {
        for( var_handle = 0; var_handle < GlobalVarArray.num; var_handle++ ) {
            if( stricmp( GlobalVarList[var_handle].name, vbl_name ) == 0 ) {
                return( var_handle );
            }
        }
    }
    return( NO_VAR );
}

vhandle GetVariableById( gui_ctl_id id )
/**************************************/
{
    if( id < FIRST_UNUSED_ID || ID2VH( id ) >= GlobalVarArray.num )
        return( NO_VAR );
    return( ID2VH( id ) );
}


const char *VarGetName( vhandle var_handle )
/******************************************/
{
    if( var_handle == NO_VAR )
        return( "" );
    return( GlobalVarList[var_handle].name );
}


gui_ctl_id VarGetId( vhandle var_handle )
/***************************************/
{
    if( var_handle == NO_VAR )
        return( CTL_NULL );
    return( VH2ID( var_handle ) );
}


const char *VarGetStrVal( vhandle var_handle )
/********************************************/
{
    if( var_handle == NO_VAR )
        return( "" );
    if( !GlobalVarList[var_handle].has_value )
        return( "" );
    return( GlobalVarList[var_handle].strval );
}

bool VarGetBoolVal( vhandle var_handle )
/**************************************/
{
    return( atoi( VarGetStrVal( var_handle ) ) != 0 );
}

int VarGetIntVal( vhandle var_handle )
/************************************/
{
    return( atoi( VarGetStrVal( var_handle ) ) );
}

bool VarHasValue( vhandle var_handle )
/************************************/
{
    if( var_handle == NO_VAR )
        return( false );
    return( GlobalVarList[var_handle].has_value );
}

void VarSetHook( vhandle var_handle, void (*hook)( vhandle ) )
/************************************************************/
{
    if( var_handle == NO_VAR )
        return;
    GlobalVarList[var_handle].hook = hook;
}

bool GetVariableBoolVal( const char *vbl_name )
/*********************************************/
{
    return( VarGetBoolVal( GetVariableByName( vbl_name ) ) );
}

int GetVariableIntVal( const char *vbl_name )
/*******************************************/
{
    return( VarGetIntVal( GetVariableByName( vbl_name ) ) );
}

const char *GetVariableStrVal( const char *vbl_name )
/***************************************************/
{
    return( VarGetStrVal( GetVariableByName( vbl_name ) ) );
}

static vhandle NewVariable( const char *vbl_name )
/************************************************/
{
    a_variable  *tmp_variable;
    vhandle     var_handle;

    var_handle = GlobalVarArray.num;
    BumpArray( &GlobalVarArray );
    tmp_variable = &GlobalVarList[var_handle];
    tmp_variable->name = GUIStrDup( vbl_name, NULL );
    tmp_variable->has_value = false;
    tmp_variable->autoset = NULL;
    tmp_variable->restriction = 0;
    tmp_variable->hook = NULL;
    tmp_variable->strval = NULL;
    if( GlobalVarHash != NULL ) {
        HashInsert( GlobalVarHash, vbl_name, var_handle );
    }
    return( var_handle );
}

vhandle AddVariable( const char *vbl_name )
/*****************************************/
{
    vhandle var_handle;

    var_handle = GetVariableByName( vbl_name );
    if( var_handle == NO_VAR )
        var_handle = NewVariable( vbl_name );
    return( var_handle );
}

static vhandle DoSetVariable( vhandle var_handle, const char *strval, const char *vbl_name )
/******************************************************************************************/
{
    a_variable  *tmp_variable;

    if( strval == NULL ) {
        strval = "";
    }
    if( var_handle != NO_VAR ) {
        tmp_variable = &GlobalVarList[var_handle];
        if( tmp_variable->has_value ) {
            if( strcmp( tmp_variable->strval, strval ) == 0 ) {
                if( tmp_variable->hook ) {
                    tmp_variable->hook( var_handle );
                }
                return( var_handle );
            }
            GUIMemFree( tmp_variable->strval );   // free the old string
        }
    } else {
        var_handle = NewVariable( vbl_name );
    }
    tmp_variable = &GlobalVarList[var_handle];
    tmp_variable->strval = GUIStrDup( strval, NULL );
    tmp_variable->has_value = true;
    if( tmp_variable->hook ) {
        tmp_variable->hook( var_handle );
    }
    return( var_handle );
}

vhandle SetBoolVariableByName( const char *vbl_name, bool bval )
/**************************************************************/
{
    return( DoSetVariable( GetVariableByName( vbl_name ), bval ? "1" : "0", vbl_name ) );
}

vhandle SetBoolVariableByHandle( vhandle var_handle, bool bval )
/**************************************************************/
{
    if( var_handle == NO_VAR )
        return( NO_VAR );
    return( DoSetVariable( var_handle, bval ? "1" : "0", NULL ) );
}

vhandle SetVariableByName( const char *vbl_name, const char *strval )
/*******************************************************************/
{
    return( DoSetVariable( GetVariableByName( vbl_name ), strval, vbl_name ) );
}

vhandle SetVariableByHandle( vhandle var_handle, const char *strval )
/*******************************************************************/
{
    if( var_handle == NO_VAR )
        return( NO_VAR );
    return( DoSetVariable( var_handle, strval, NULL ) );
}

void SetDefaultGlobalVarList( void )
/**********************************/
{
    char    szBuf[_MAX_PATH];
#if defined( __NT__ )
    char    *last_slash;
  #if !defined( _M_X64 )
    DWORD   version = GetVersion();
  #endif
#endif

    // create global variables for true/false value
    SetBoolVariableByName(      "true",         true );
    SetBoolVariableByName(      "false",        false );

    // create global variables for each default system
#if defined( __DOS__ )
    // 16/32-bit DOS
    SetBoolVariableByName(      "IsDos",        true );
    SetBoolVariableByName(      "IsOS2DosBox",  _osmajor >= 10 );
#else
    SetBoolVariableByName(      "IsDos",        false );
#endif

#if defined( __WINDOWS__ )
    // 16-bit Windows
    SetBoolVariableByName(      "IsWin16",      true );
    SetBoolVariableByName(      "IsOS2DosBox",  _osmajor >= 10 );
#else
    SetBoolVariableByName(      "IsWin16",      false );
#endif

#if defined( __OS2__ )
    // 16/32-bit OS/2
    SetBoolVariableByName(      "IsOS2",        true );
#else
    SetBoolVariableByName(      "IsOS2",        false );
#endif

#if defined( __NT__ )
  #if defined( _M_X64 )
    // 64-bit Windows
    SetBoolVariableByName(      "IsWin32s",     false );
    SetBoolVariableByName(      "IsWin32",      false );
    SetBoolVariableByName(      "IsWin95",      false );
    SetBoolVariableByName(      "IsWin98",      false );
    SetBoolVariableByName(      "IsWinNT",      true );
    SetBoolVariableByName(      "IsWinNT40",    true );
    SetBoolVariableByName(      "IsWin2000",    true );
    SetBoolVariableByName(      "IsWin64",      true );
  #else
    // 32-bit Windows or WOW
    if( version & 0x80000000 ) {
        SetBoolVariableByName(  "IsWin32s",     LOBYTE( LOWORD( version ) ) < 4 );
        SetBoolVariableByName(  "IsWin32",      LOBYTE( LOWORD( version ) ) >= 4 );
        SetBoolVariableByName(  "IsWin95",      LOBYTE( LOWORD( version ) ) >= 4 );
        SetBoolVariableByName(  "IsWin98",      LOBYTE( LOWORD( version ) ) >= 5 );
        SetBoolVariableByName(  "IsWinNT",      false );
        SetBoolVariableByName(  "IsWinNT40",    false );
        SetBoolVariableByName(  "IsWin2000",    false );
    } else {
        SetBoolVariableByName(  "IsWin32s",     false );
        SetBoolVariableByName(  "IsWin32",      true );
        SetBoolVariableByName(  "IsWin95",      false );
        SetBoolVariableByName(  "IsWin98",      false );
        SetBoolVariableByName(  "IsWinNT",      true );
        SetBoolVariableByName(  "IsWinNT40",    LOBYTE( LOWORD( version ) ) >= 4 );
        SetBoolVariableByName(  "IsWin2000",    LOBYTE( LOWORD( version ) ) >= 5 );
    }
    SetBoolVariableByName(      "IsWin64",      false );
  #endif
#else
    SetBoolVariableByName(      "IsWin32s",     false );
    SetBoolVariableByName(      "IsWin32",      false );
    SetBoolVariableByName(      "IsWin95",      false );
    SetBoolVariableByName(      "IsWin98",      false );
    SetBoolVariableByName(      "IsWinNT",      false );
    SetBoolVariableByName(      "IsWinNT40",    false );
    SetBoolVariableByName(      "IsWin2000",    false );
    SetBoolVariableByName(      "IsWin64",      false );
#endif

#if defined( __LINUX__ )
  #if defined( _M_X64 ) && 0
    // 64-bit Linux
    SetBoolVariableByName(      "IsLinux64",    true );
    SetBoolVariableByName(      "IsLinux32",    false );
  #else
    // 32-bit Linux
    SetBoolVariableByName(      "IsLinux64",    false );
    SetBoolVariableByName(      "IsLinux32",    true );
  #endif
#else
    SetBoolVariableByName(      "IsLinux64",    false );
    SetBoolVariableByName(      "IsLinux32",    false );
#endif

#if defined( __AXP__ )
    SetBoolVariableByName(      "IsAlpha",      true );
#else
    SetBoolVariableByName(      "IsAlpha",      false );
#endif

#if defined( __WINDOWS__ ) || defined( __NT__ )
    GetSystemDirectory( szBuf, sizeof( szBuf ) );
#else
    strcpy( szBuf, "C:\\WINDOWS\\SYSTEM" );
#endif
    SetVariableByName( "WinSystemDir", szBuf );
#if defined( __NT__ )
    last_slash = strrchr( szBuf, '\\' );
    strcpy( last_slash, "\\SYSTEM" );
#endif
    SetVariableByName( "WinSystem16Dir", szBuf );
#if defined( __WINDOWS__ ) || defined( __NT__ )
    GetWindowsDirectory( szBuf, sizeof( szBuf ) );
#else
    strcpy( szBuf, "C:\\WINDOWS" );
#endif
    SetVariableByName( "WinDir", szBuf );

#if defined( __NT__ )
    if( !SearchPath( NULL, "CMD.EXE", NULL, sizeof( szBuf ), szBuf, NULL ) ) {
        if( !SearchPath( NULL, "COMMAND.COM", NULL, sizeof( szBuf ), szBuf, NULL ) ) {
#endif
            strcpy( szBuf, "COMMAND.COM" );
#if defined( __NT__ )
        }
    }
#endif
    SetVariableByName( "Command", szBuf );
}


void FreeGlobalVarList( bool including_real_globals )
/***************************************************/
{
    int i;

    if( GlobalVarList == NULL )
        return;

    if( including_real_globals ) {
        for( i = 0; i < GlobalVarArray.num; i++ ) {
            GUIMemFree( GlobalVarList[i].name );
            GUIMemFree( GlobalVarList[i].strval );
            GUIMemFree( GlobalVarList[i].autoset );
        }
        GlobalVarArray.num = 0;
        GUIMemFree( GlobalVarList );
        if( GlobalVarHash != NULL ) {
            HashFini( GlobalVarHash );
            GlobalVarHash = NULL;
        }
    } else {
        for( i = 0; i < GlobalVarArray.num; ) {
            if( GlobalVarList[i].name[0] != '$' ) {
                GUIMemFree( GlobalVarList[i].name );
                GUIMemFree( GlobalVarList[i].strval );
                GUIMemFree( GlobalVarList[i].autoset );

                GlobalVarArray.num -= 1;
                if( i < GlobalVarArray.num ) {
                    memcpy( &GlobalVarList[i], &GlobalVarList[i + 1], sizeof( a_variable ) * ( GlobalVarArray.num - i ) );
                    // This destroys the concept that a handle to a variable
                    // will always point to the same variable.  Between
                    // script launches, variable ids will change.
                }
                BumpDownArray( &GlobalVarArray );
            } else {
                i++;
            }
        }
        // We have to rebuild the hash table
        if( GlobalVarHash != NULL ) {
            HashFini( GlobalVarHash );
            GlobalVarHash = HashInit( HASH_SIZE, &stricmp );
            for( i = 0; i < GlobalVarArray.num; i++ ) {
                HashInsert( GlobalVarHash, GlobalVarList[i].name, i );
            }
        }
    }
}

