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
* Description:  Interface for communicating betwen user applications and
*               the Open Watcom debugger.
*
****************************************************************************/


#ifndef __WATCOM_H_ENTERDB__
#define __WATCOM_H_ENTERDB__

#ifdef __cplusplus
extern "C" {
#endif

/*
        entry points:

        void CheckEnterDebugger
        - enter debugger if present

        void CheckEnterDebuggerWithMessage
        - enter the debugger and print "msg", if present

        void EnterDebugger( void )
        - unconditional hard coded breakpoint

        void EnterDebuggerWithMessage( char __far *msg )
        - unconditional hard coded breakpoint - if the debugger is there,
          print "msg"

        void DebuggerSetThreadId( unsigned long threadid, char *name )
        - tell the debugger to give the thread a name
        - if you don't like using alloca and sprintf, use
          CheckEnterDebuggerWithMessage and DEBUGGER_THREADID_FORMAT.

        char DebuggerPresent( void )
        - is the Watcom Debugger present

        char DebuggerBreakOnCatch( void )
        - does the debugger want to break on a caught exception

        char DebuggerBreakOnThrow( void )
        - does the debugger want to break on a thrown exception

        void DebuggerInitPresent()
        - initialize for all the calls that check if the debugger's present

        void DebuggerSetCharVariableTrue( char var )
        - tell the debugger to set "var" to 1 if it's present

        void DebuggerExecuteCommand( char cmd )
        - tell the debugger to run a command

        DebuggerBreakAfterReturnWithMessage( num, msg ) \
        - like CheckEnterDebuggerWithMessage, only it delays until after
          num returns have happened

        DebuggerBreakWithMessageAndUnwind( num, msg ) \
        - like CheckEnterDebuggerWithMessage, plus cause the debugger to
          display the app with the stack unwound 'num' levels

 */

#define DEBUG_PRESENT_NAME __WD_Present
#define DEBUG_BREAK_ON_CATCH_NAME __WD_Break_On_Catch
#define DEBUG_BREAK_ON_THROW_NAME __WD_Break_On_Throw

#define DEBUG_PRESENT_STR "__WD_Present"
#define DEBUG_BREAK_ON_CATCH_STR "__WD_Break_On_Catch"
#define DEBUG_BREAK_ON_THROW_STR "__WD_Break_On_Throw"

#ifndef _WCRTDATA
# define _WCRTDATA /* nothing */
#endif

_WCRTDATA extern char volatile DEBUG_PRESENT_NAME;
_WCRTDATA extern char volatile DEBUG_BREAK_ON_THROW_NAME;
_WCRTDATA extern char volatile DEBUG_BREAK_ON_CATCH_NAME;

#if defined( __WATCOMC__ ) && defined( _M_IX86 )

    extern void EnterDebugger( void );
    #pragma aux EnterDebugger = "int 3"

    extern void EnterDebuggerWithMessage( const char __far * );
    #pragma aux EnterDebuggerWithMessage \
        __parm __caller [] = \
            "int 3" \
            "jmp short L1" \
            'W' 'V' 'I' 'D' 'E' 'O' \
            "L1:"

#elif defined( _MSC_VER )  &&  _MSC_VER >= 1100

    #define EnterDebugger()                 _asm { int 3 }
    #define EnterDebuggerWithMessage( s )   EnterDebugger()

#elif defined(__AXP__)

    extern void EnterDebugger( void );
    #pragma aux EnterDebugger = "call_pal 0x80"

    #if 0 //NYI: can't pass parm to inline yet
    extern void EnterDebuggerWithMessage( const char * );
    #pragma aux EnterDebuggerWithMessage =      \
                    "call_pal   0x80"           \
                    ".long      0xc3e00002"     \
                    ".byte      0x57,0x56,0x49,0x44,0x45,0x4f,0,0"
    #else
    #define EnterDebuggerWithMessage( s )       EnterDebugger()
    #endif

#elif defined(__PPC__) || !defined( __WATCOMC__ ) || defined( MAC )
    /*
        This should be replaced when we have in-line assembly support
        in the compiler.
    */
    #define EnterDebugger()     \
            { volatile int i = 0; volatile int j = 0; i /= j; }

    #define EnterDebuggerWithMessage( s )       EnterDebugger()

#elif defined(__MIPS__)

    extern void EnterDebugger( void );
    #pragma aux EnterDebugger = "break"

    extern void EnterDebuggerWithMessage( const char * );
    #pragma aux EnterDebuggerWithMessage =      \
                   "break"                      \
                   "beq $0,$0,1f"               \
                   "nop"                        \
                   ".byte 0x57,0x56,0x49,0x44"  \
                   ".byte 0x45,0x4f,0,0"        \
                   "1:"

#else
   #error enterdb.h not configured for CPU
#endif

#define CheckEnterDebugger() \
    { \
        if( DEBUG_PRESENT_NAME ) EnterDebugger(); \
    }

#define CheckEnterDebuggerWithMessage( msg ) \
    { \
        if( DEBUG_PRESENT_NAME ) EnterDebuggerWithMessage( msg ); \
    }

#ifdef __NT__
    // Why? #define PassDebuggerAMessage OutputDebugString
    #define PassDebuggerAMessage CheckEnterDebuggerWithMessage
#else
    #define PassDebuggerAMessage CheckEnterDebuggerWithMessage
#endif

#define DebuggerPresent() DEBUG_PRESENT_NAME
#define DebuggerBreakOnCatch() DEBUG_BREAK_ON_CATCH_NAME
#define DebuggerBreakOnThrow() DEBUG_BREAK_ON_THROW_NAME

#define EnterDebuggerWithSignature EnterDebuggerWithMessage

#define DEBUGGER_COMMAND(c)   DEBUGGER_ ## c ## _COMMAND

#define DEBUGGER_THREADID_COMMAND "!THREADID "
#define DEBUGGER_THREADID_FORMAT DEBUGGER_THREADID_COMMAND "0x%8.8x=%s"
#define DebuggerSetThreadId( id, name ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_THREADID_COMMAND )+2+8+1+strlen( name )+1 ); \
        sprintf( __buff, DEBUGGER_THREADID_FORMAT, id, name ); \
        PassDebuggerAMessage( __buff ); \
    }

#define DEBUGGER_SETTRUE_COMMAND "!SETTRUE "
#define DEBUGGER_SETTRUE_FORMAT DEBUGGER_SETTRUE_COMMAND "0x%8.8x"
#define DebuggerSetCharVariableTrue( var ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_SETTRUE_COMMAND )+2+8+1 ); \
        sprintf( __buff, DEBUGGER_SETTRUE_FORMAT, (void*)&var ); \
        PassDebuggerAMessage( __buff ); \
    }

#define DEBUGGER_EXECUTE_COMMAND "!EXECUTE "
#define DEBUGGER_EXECUTE_FORMAT DEBUGGER_EXECUTE_COMMAND "%s"
#define DebuggerExecuteCommand( cmd ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_EXECUTE_COMMAND )+strlen( cmd )+1 ); \
        sprintf( __buff, DEBUGGER_EXECUTE_FORMAT, cmd ); \
        PassDebuggerAMessage( __buff ); \
    }

#define DEBUGGER_MESSAGE_COMMAND "!MESSAGE "
#define DEBUGGER_MESSAGE_FORMAT DEBUGGER_MESSAGE_COMMAND "%s"
#define DebuggerMessage( cmd ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_MESSAGE_COMMAND )+strlen( cmd )+1 ); \
        sprintf( __buff, DEBUGGER_MESSAGE_FORMAT, cmd ); \
        PassDebuggerAMessage( __buff ); \
    }

#define DEBUGGER_LOOKUP_COMMAND "!SYMLOOKUP "
#define DEBUGGER_LOOKUP_FORMAT DEBUGGER_LOOKUP_COMMAND "0x%8.8x,0x%8.8x,0x%4.4x"
#define DebuggerSymbolLookup( addr, buff, buff_len ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_LOOKUP_COMMAND )+2+8+1+2+8+1+2+4+1 ); \
        sprintf( __buff, DEBUGGER_LOOKUP_FORMAT, addr, buff, buff_len ); \
        PassDebuggerAMessage( __buff ); \
    }


#define DEBUGGER_BREAKRETURN_COMMAND "!BREAKRETURN "
#define DEBUGGER_BREAKRETURN_FORMAT DEBUGGER_BREAKRETURN_COMMAND "%3d"
#define DebuggerBreakAfterReturnWithMessage( after_num_returns, msg ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_BREAKRETURN_COMMAND )+3+1 ); \
        sprintf( __buff, DEBUGGER_BREAKRETURN_FORMAT, after_num_returns ); \
        PassDebuggerAMessage( __buff ); \
        CheckEnterDebuggerWithMessage( msg ); \
    }

#define DEBUGGER_BREAKUNWIND_COMMAND "!BREAKUNWIND "
#define DEBUGGER_BREAKUNWIND_FORMAT DEBUGGER_BREAKUNWIND_COMMAND "%3d"
#define DebuggerBreakWithMessageAndUnwind( levels, msg ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_BREAKUNWIND_COMMAND )+3+1 ); \
        sprintf( __buff, DEBUGGER_BREAKUNWIND_FORMAT, levels ); \
        PassDebuggerAMessage( __buff ); \
        CheckEnterDebuggerWithMessage( msg ); \
    }

#define DEBUGGER_LOADMODULE_COMMAND "!LOADMODULE "
#define DEBUGGER_LOADMODULE_FORMAT DEBUGGER_LOADMODULE_COMMAND "0x%4.4x:0x%8.8x,%s"
#define DebuggerLoadUserModule( modname, segment, offset ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_LOADMODULE_COMMAND )+2+4+1+2+8+1+strlen( modname )+1 ); \
        sprintf( __buff, DEBUGGER_LOADMODULE_FORMAT, segment, offset, modname ); \
        PassDebuggerAMessage( __buff ); \
    }

#define DEBUGGER_UNLOADMODULE_COMMAND "!UNLOADMODULE "
#define DEBUGGER_UNLOADMODULE_FORMAT DEBUGGER_UNLOADMODULE_COMMAND "%s"
#define DebuggerUnloadUserModule( modname ) \
    { \
        char *__buff = (char*)alloca( sizeof( DEBUGGER_UNLOADMODULE_COMMAND )+strlen( modname )+1 ); \
        sprintf( __buff, DEBUGGER_UNLOADMODULE_FORMAT, modname ); \
        PassDebuggerAMessage( __buff ); \
    }

#define DebuggerInitPresent() DebuggerSetCharVariableTrue( DEBUG_PRESENT_NAME )

#ifdef __cplusplus
};
#endif


#endif
