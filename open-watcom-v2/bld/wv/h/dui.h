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
* Description:  DUI (Debugger User Interface) prototypes.
*
****************************************************************************/


typedef unsigned    dui_res_id;

extern void     DUIUpdate( update_flags flags );
extern void     DUIStatusText( const char *text );
extern void     DUIMsgBox( const char *text );
extern bool     DUIDlgTxt( const char *text );
extern void     DUIInfoBox( const char *text );
extern void     DUIStop( void );
extern void     DUIFini( void );
extern void     DUIInitHistory( void );
extern void     DUIFiniHistory( void );
extern bool     DUIClose( void );
extern void     DUIInit( void );
extern void     DUIFreshAll( void );
extern bool     DUIStopRefresh( bool ok );
extern void     DUIShow( void );
extern void     DUIWndUser( void );
extern void     DUIWndDebug( void );
extern void     DUIShowLogWindow( void );
extern void     DUIRedrawRegisters( void );
extern int      DUIGetMonitorType( void );
extern int      DUIScreenSizeX( void );
extern int      DUIScreenSizeY( void );
extern void     DUIRedrawSources( void );
extern void     DUIErrorBox( const char *buff );
extern void     DUIArrowCursor( void );
extern char     *DUILoadString( dui_res_id id );
extern void     DUIFreeString( void * );
extern bool     DUIAskIfAsynchOk( void );
extern void     DUIFlushKeys( void );
extern void     DUIPlayDead( bool );
extern void     DUISysStart( void );
extern void     DUISysEnd( bool pause );
extern void     DUIRingBell( void );
extern void     DUIProcPendingPaint(void);
extern bool     DUIInfoRelease( void );
extern void     DUIEnterCriticalSection( void );
extern void     DUIExitCriticalSection( void );
extern void     DUIInitLiterals( void );
extern void     DUIFiniLiterals( void );
extern bool     DUIIsDBCS( void );
extern size_t   DUIEnvLkup( const char *name, char *buff, size_t buff_len );
extern void     DUIDirty( void );
extern void     DUISrcOrAsmInspect( address );
extern void     DUIAddrInspect( address );
extern bool     DUICopyCancelled( void * );
extern void     DUICopySize( void *, unsigned long );
extern void     DUICopyCopied( void *, unsigned long );
extern bool     DUIGetSourceLine( cue_handle *cueh, char *buff, size_t len );
extern void     DUIRemoveBreak( brkp *bp );
extern bool     DUIDisambiguate( const ambig_info *ambig, int num_items, int *choice );
extern bool     DUIImageLoaded( image_entry *image, bool load, bool already_stopping, bool *force_stop );
extern unsigned DUIDlgAsyncRun( void );
extern void     DUISetNumLines( int num );
extern void     DUISetNumColumns( int num );
extern void     DUIInitRunThreadInfo( void );
extern void     DUIScreenOptInit( void );
extern bool     DUIScreenOption( const char *start, unsigned len, int pass );
extern unsigned DUIConfigScreen( void );
extern void     DUIProcWindow( void );
extern bool     DUIDlgGivenAddr( const char *title, address *value );
extern void     DUIFingOpen( void );
extern void     DUIFingClose( void );
