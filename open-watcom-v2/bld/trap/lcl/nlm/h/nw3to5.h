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


#if defined ( __NW50__ )
    #include "mmu.h"
    #include "process.h"

    #define T_ProcessID                     void

    #define _RunningProcess                 (T_ProcessID *)GetRunningProcess()
    #define FileServerMajorVersionNumber    GetFileServerMajorVersionNumber()
    #define FileServerMinorVersionNumber    GetFileServerMinorVersionNumber()
    #define LoadedListHandle                LONG
    #define ddRTag                          DDSResourceTag

    extern LONG CValidateMappedAddress( void *, void *, LONG );
    #define CValidatePointer(x) \
        CValidateMappedAddress(x,SystemDomain,MMU_PAGE_READ_WRITE_ENABLE)

    extern LONG GetNextLoadedListEntry( LONG );
    extern struct LoadDefinitionStructure * ValidateModuleHandle( LONG );
    #define systemConsoleScreen GetSystemConsoleScreen()
    extern void CYieldWithDelay(void);
    #define CRescheduleLast CYieldWithDelay

#elif defined ( __NW40__ )

    #include "mmu.h"
    #include "process.h"

    #define T_ProcessID                T_PCBStruct
    #define ddRTag                     DDSResourceTag

    #define _RunningProcess            (T_ProcessID *)GetRunningProcess()
//    extern void *CurrentDebuggerDomain;
    extern LONG CValidateMappedAddress( void *, void *, LONG );
    #define CValidatePointer(x) \
      CValidateMappedAddress(x,SystemDomain,MMU_PAGE_READ_WRITE_ENABLE)
    #define systemConsoleScreen GetSystemConsoleScreen()
    #define CRescheduleLast CYieldWithDelay
    extern void CYieldWithDelay(void);
    #define LoadedListHandle LONG
    extern LONG GetNextLoadedListEntry( LONG );
    extern struct LoadDefinitionStructure *ValidateModuleHandle( LONG );

    #define FileServerMajorVersionNumber GetFileServerMajorVersionNumber()
    #define FileServerMinorVersionNumber GetFileServerMinorVersionNumber()

#elif defined ( __NW30__ )
    #define T_ProcessID                 T_ThreadControlStruct

    #define _RunningProcess             (T_ProcessID *)RunningProcess
    extern void                         *CValidatePointer(void *);
    #define GetNextLoadedListEntry( x ) \
    (LoadedListHandle)( ((x)==0) ?LoadedList:((struct LoadDefinitionStructure*)x)->LDLink)
    #define ValidateModuleHandle( x )   (struct LoadDefinitionStructure*)(x)
    #define LoadedListHandle struct LoadDefinitionStructure *
    extern struct LoadDefinitionStructure *LoadedList;
#endif


#if defined ( __NW50__ )
    #define StackFrame T_TSS_StackFrame
    #define FieldGS( x )                        ( (x)->ExceptionGS[0] )
    #define FieldFS( x )                        ( (x)->ExceptionFS[0] )
    #define FieldES( x )                        ( (x)->ExceptionES[0] )
    #define FieldDS( x )                        ( (x)->ExceptionDS[0] )
    #define FieldEDI( x )                       ( (x)->ExceptionEDI )
    #define FieldESI( x )                       ( (x)->ExceptionESI )
    #define FieldEBP( x )                       ( (x)->ExceptionEBP )
    #define FieldESP( x )                       ( (x)->ExceptionESP )
    #define FieldEBX( x )                       ( (x)->ExceptionEBX )
    #define FieldEDX( x )                       ( (x)->ExceptionEDX )
    #define FieldECX( x )                       ( (x)->ExceptionECX )
    #define FieldEAX( x )                       ( (x)->ExceptionEAX )
    #define FieldExceptionNumber( x )           ( (x)->ExceptionNumber )
    #define FieldExceptionDescription(x)        ( (x)->ExceptionDescription )
    #define FieldInfoFlags( x )                 ( (x)->ExceptionFlags )
    #define FieldErrorCode( x )                 ( (x)->ExceptionErrorCode )
    #define FieldEIP( x )                       ( (x)->ExceptionEIP )
    #define FieldCS( x )                        ( (x)->ExceptionCS[0] )
    #define FieldEFLAGS( x )                    ( (x)->ExceptionSystemFlags )
#elif defined ( __NW40__ )
    #define StackFrame T_TSS_StackFrame
    #define FieldGS( x )                        ( (x)->ExceptionGS[0] )
    #define FieldFS( x )                        ( (x)->ExceptionFS[0] )
    #define FieldES( x )                        ( (x)->ExceptionES[0] )
    #define FieldDS( x )                        ( (x)->ExceptionDS[0] )
    #define FieldEDI( x )                       ( (x)->ExceptionEDI )
    #define FieldESI( x )                       ( (x)->ExceptionESI )
    #define FieldEBP( x )                       ( (x)->ExceptionEBP )
    #define FieldESP( x )                       ( (x)->ExceptionESP )
    #define FieldEBX( x )                       ( (x)->ExceptionEBX )
    #define FieldEDX( x )                       ( (x)->ExceptionEDX )
    #define FieldECX( x )                       ( (x)->ExceptionECX )
    #define FieldEAX( x )                       ( (x)->ExceptionEAX )
    #define FieldExceptionNumber( x )           ( (x)->ExceptionNumber )
    #define FieldExceptionDescription(x)        ( (x)->ExceptionDescription )
    #define FieldInfoFlags( x )                 ( (x)->ExceptionFlags )
    #define FieldErrorCode( x )                 ( (x)->ExceptionErrorCode )
    #define FieldEIP( x )                       ( (x)->ExceptionEIP )
    #define FieldCS( x )                        ( (x)->ExceptionCS[0] )
    #define FieldEFLAGS( x )                    ( (x)->ExceptionSystemFlags )
#elif defined ( __NW30__ )
    #define StackFrame T_StackFrame
    #define FieldGS( x )                        ( (x)->GS[0] )
    #define FieldFS( x )                        ( (x)->FS[0] )
    #define FieldES( x )                        ( (x)->ES[0] )
    #define FieldDS( x )                        ( (x)->DS[0] )
    #define FieldEDI( x )                       ( (x)->EDI )
    #define FieldESI( x )                       ( (x)->ESI )
    #define FieldEBP( x )                       ( (x)->EBP )
    #define FieldESP( x )                       ( (x)->ESP )
    #define FieldEBX( x )                       ( (x)->EBX )
    #define FieldEDX( x )                       ( (x)->EDX )
    #define FieldECX( x )                       ( (x)->ECX )
    #define FieldEAX( x )                       ( (x)->EAX )
    #define FieldExceptionNumber( x )           ( (x)->exceptionNumber )
    #define FieldExceptionDescription(x)        ( (x)->exceptionDescription )
    #define FieldInfoFlags( x )                 ( (x)->infoFlags )
    #define FieldErrorCode( x )                 ( (x)->errorCode )
    #define FieldEIP( x )                       ( (x)->EIP )
    #define FieldCS( x )                        ( (x)->CS[0] )
    #define FieldEFLAGS( x )                    ( (x)->EFLAGS )
#else
#error Stack Frame has not been defined for this version of netware
#endif

extern struct ResourceTagStructure      *BreakTag;
extern struct ResourceTagStructure      *DebugTag;

#if defined ( __NW50__ )
    #define DoReserveBreakpoint() ReserveABreakpointRTag( (LONG)(BreakTag) );
#elif defined ( __NW40__ )
    #define DoReserveBreakpoint() ReserveABreakpointRTag( (LONG)(BreakTag) );
#elif defined ( __NW30__ )
    #define DoReserveBreakpoint() ReserveABreakpointRTag( BreakTag );
#endif
