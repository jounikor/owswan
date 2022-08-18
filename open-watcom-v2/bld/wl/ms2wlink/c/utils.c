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
* Description:  UTILS : utility routines for the MS linker file translator
*
****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "wio.h"
#include "walloca.h"
#include "ms2wlink.h"

#include "clibext.h"


static const char *PromptText[] = {
    #define SLOTDEF( e, pt, et )  pt,
    SLOT_DEFS
    #undef SLOTDEF
};

static const char *DefExt[] = {
    #define SLOTDEF( e, pt, et )  et,
    SLOT_DEFS
    #undef SLOTDEF
};

static unsigned char DefExtLen[] = {
    #define SLOTDEF( e, pt, et )  sizeof( et ) - 1,
    SLOT_DEFS
    #undef SLOTDEF
};

static bool     WritePrompt;



void UtilsInit( void )
/********************/
// check to see if STDIN is the console. if not, don't write prompt.
{
#if !defined( __UNIX__ )
    _setmode( fileno( stdin ), O_BINARY );
#endif
    WritePrompt = QIsConIn( stdin );
}

void ImplyFormat( format_type typ )
/*********************************/
{
    if( FmtType == FMT_DEFAULT ) {
        FmtType = typ;
    }
}

char *FileName( const char *buff, prompt_slot slot, bool force )
/**************************************************************/
{
    const char      *namptr;
    char            *ptr;
    size_t          cnt;
    size_t          len;

    len = strlen( buff );
    namptr = buff + len;
    for( cnt = 0; cnt < len; ++cnt ) {
        --namptr;
        if( *namptr == '\\' || *namptr == '/' ) {
            break;
        }
    }
    if( *namptr == '\\' || *namptr == '/' ) {
        namptr++;
    }
    cnt = len - ( namptr - buff );
    for( namptr = buff + len; *namptr != '.'; --namptr ) {
        if( cnt-- < 2 ) {
            break;
        }
    }
    if( *namptr != '.' || force ) {
        if( cnt != 0 ) {
            len = cnt;
        }
        ptr = MemAlloc( len + 1 + DefExtLen[slot] + 1 );
        memcpy( ptr, buff, len );
        ptr[len++] = '.';
        strcpy( ptr + len, DefExt[slot] );
    } else {
        ptr = MemAlloc( len + 1 );
        memcpy( ptr, buff, len );
        ptr[len] = '\0';
    }
    return( ptr );
}

void AddCommand( char *msg, prompt_slot slot, bool verbatim )
/*************************************************************/
{
    cmdentry    *cmd;
    cmdentry    *list;

    cmd = MemAlloc( sizeof( cmdentry ) );
    cmd->command = msg;
    cmd->asis = verbatim;
    cmd->next = NULL;
    list = Commands[slot];
    if( list == NULL ) {
        Commands[slot] = cmd;
    } else {                         // always add at the end of the list.
        while( list->next != NULL ) {
            list = list->next;
        }
        list->next = cmd;
    }
}

void Warning( const char *msg, prompt_slot slot )
/*************************************************/
// print a warning to the linker command file in the form of a linker comment.
{
    AddCommand( Msg2Splice( "# ", msg ), slot, true );
}

void AddOption( const char *msg )
/*******************************/
{
    AddCommand( Msg2Splice( "option ", msg ), OPTION_SLOT, true );
}

void AddNumOption( const char *msg, unsigned value )
/****************************************************/
{
    char    buffer[7];
    char    *msg2;

    ultoa( value, buffer, 10 );
    msg2 = Msg3Splice( msg, "=", buffer );
    AddOption( msg2 );
    MemFree( msg2 );
}

void AddStringOption( const char *msg, const char *string, size_t len )
/*********************************************************************/
{
    char    *cmd;
    size_t  msglen;
    char    *tmp;

    msglen = strlen( msg );
    cmd = alloca( len + msglen + 2 );
    memcpy( cmd, msg, msglen );
    tmp = cmd + msglen;
    *tmp++ = '=';
    memcpy( tmp, string, len );
    tmp += len;
    *tmp = '\0';
    AddOption( cmd );
}

void NotSupported( const char *msg )
/*****************************************/
{
    char    *msg2;

    msg2 = Msg2Splice( msg, " option is not supported by WLINK" );
    Warning( msg2, OPTION_SLOT );
    MemFree( msg2 );
}

void NotNecessary( const char *msg )
/*****************************************/
{
    char    *msg2;

    msg2 = Msg2Splice( msg, " option is not necessary when using WLINK" );
    Warning( msg2, OPTION_SLOT );
    MemFree( msg2 );
}

void NotRecognized( const char *msg )
/******************************************/
{
    char    *msg2;

    msg2 = Msg3Splice( "/", msg, " : unrecognized option name; ignored" );
    Warning( msg2, OPTION_SLOT );
    MemFree( msg2 );
}

char *Msg2Splice( const char *msg1, const char *msg2 )
/***********************************************************/
// splice 2 messages together
{
    size_t  len1;
    size_t  len2;
    char    *both;

    len1 = strlen( msg1 );
    len2 = strlen( msg2 );
    both = MemAlloc( len1 + len2 + 1 );
    memcpy( both, msg1, len1 );
    memcpy( both + len1, msg2, len2 );
    *(both + len1 + len2) = '\0';
    return( both );
}

char *Msg3Splice( const char *msg1, const char *msg2, const char *msg3 )
/*****************************************************************************/
{
    size_t  len1;
    size_t  len2;
    size_t  len3;
    char    *all;

    len1 = strlen( msg1 );
    len2 = strlen( msg2 );
    len3 = strlen( msg3 );
    all = MemAlloc( len1 + len2 + len3 + 1 );
    memcpy( all, msg1, len1 );
    memcpy( all + len1, msg2, len2 );
    memcpy( all + len1 + len2, msg3, len3 );
    *(all + len1 + len2 + len3) = '\0';
    return( all );
}

char *FindNotAsIs( prompt_slot slot )
/***********************************/
// search through the given slot for a command which isn't marked "asis"
// since comments are "asis", this can be used to determine if a filename is
// specified, and what that filename is.
{
    cmdentry    *cmd;

    for( cmd = Commands[slot]; cmd != NULL; cmd = cmd->next ) {
        if( !cmd->asis ) {
            return( cmd->command );
        }
    }
    return( NULL );
}

char *FindObjectName( void )
/**************************/
{
    char    *msg;

    msg = FindNotAsIs( OBJECT_SLOT );
    if( msg == NULL ) {
        msg = FindNotAsIs( OVERLAY_SLOT );
        if( msg == NULL ) {
            ErrorExit( "no object files specified" );
        }
    }
    return( msg );
}

static void PromptStart( const char *msg, prompt_slot slot )
/**********************************************************/
{
    const char  *text;

    text = PromptText[slot];
    QWrite( stderr, text, strlen( text ), "console" );
    QWrite( stderr, "[", 1, "console" );
    if( msg != NULL ) {
        QWrite( stderr, msg, strlen( msg ), "console" );
    }
}

void OutPutPrompt( prompt_slot slot )
/***********************************/
{
    char    *msg;

    if( !WritePrompt )
        return;
    msg = NULL;
    switch( slot ) {
    case RUN_SLOT:
        msg = FileName( FindObjectName(), slot, true );
        PromptStart( msg, slot );
        MemFree( msg );
        break;
    case MAP_SLOT:
        if( MapOption ) {
            msg = FindNotAsIs( RUN_SLOT );
            if( msg == NULL ) {
                msg = FindObjectName();
            }
            msg = FileName( msg, slot, true );
            PromptStart( msg, slot );
            MemFree( msg );
            break;
        }
        // note: fall down
    case DEF_SLOT:
        msg = "nul";
        // note: fall down
    default:
        PromptStart( msg, slot );
        QWrite( stderr, DefExt[slot], 4, "console" );
        break;
    }
    QWrite( stderr, "]: ", 3, "console" );
}

// spawn/suicide support.

static void *SpawnStack;

int Spawn( void (*fn)( void ) )
/************************************/
{
    void    *save_env;
    jmp_buf env;
    int     status;

    save_env = SpawnStack;
    SpawnStack = env;
    status = setjmp( env );
    if( status == 0 ) {
        (*fn)();
    }
    SpawnStack = save_env;  /* unwind */
    return( status );
}


void Suicide( void )
/*************************/
{
    if( SpawnStack != NULL ) {
        longjmp( SpawnStack, 1 );
    }
}
