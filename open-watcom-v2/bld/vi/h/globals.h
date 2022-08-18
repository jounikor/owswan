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
* Description:  Declaration of editor global variables.
*
****************************************************************************/


#ifndef _GLOBALS_INCLUDED
#define _GLOBALS_INCLUDED

/* strings */
extern const char _NEAR   BANNER1[];
extern const char _NEAR   BANNER2[];
extern const char _NEAR   MSG_CHARACTERS[];
extern const char _NEAR   MSG_LINES[];
extern const char _NEAR   MSG_PRESSANYKEY[];
extern const char _NEAR   MSG_DELETEDINTOBUFFER[];
extern const char _NEAR   CONFIG_FILE[];
extern const char _NEAR   MEMORIZE_MODE[];
extern const char _NEAR   SingleBlank[];
extern const char _NEAR   SingleSlash[];
extern const char _NEAR   SingleDQuote[];
extern const char _NEAR   SpinData[];

/* mouse data */
#if defined( __LINUX__ )        /* compatible with the ui lib */
extern unsigned short   MouseRow;
extern unsigned short   MouseCol;
extern unsigned short   MouseStatus;
#else
extern windim           MouseRow;
extern windim           MouseCol;
extern int              MouseStatus;
#endif
extern vi_mouse_event   LastMouseEvent;

/* generic editing globals */
extern long         NextAutoSave;
extern int          HalfPageLines;
extern vi_key       LastEvent;
extern int          SpinCount;
extern char         VideoPage;
extern char         *BndMemory, *EXEName;
extern int          FcbBlocksInUse;
extern char       * _NEAR MatchData[MAX_SEARCH_STRINGS * 2];
extern int          MatchCount;
extern mark         *MarkList;
extern fcb          *FcbThreadHead, *FcbThreadTail;
extern info         *InfoHead, *InfoTail, *CurrentInfo;
extern file         *CurrentFile;
extern fcb          *CurrentFcb;
extern line         *WorkLine;
extern line         *CurrentLine;
extern i_mark       CurrentPos;
extern i_mark       LeftTopPos;
extern int          VirtualColumnDesired;
extern window_id    current_window_id;
extern window_id    message_window_id;
extern window_id    status_window_id;
extern window_id    linenum_current_window_id;
extern window_id    menu_window_id;
extern window_id    repeat_window_id;
extern char         *Comspec;
extern select_rgn   SelRgn;

/* historys */

/* keymap data */
extern int      CurrentKeyMapCount;
extern vi_key   *CurrentKeyMap;
extern key_map  *KeyMaps, *InputKeyMaps;

/* savebuf data */
extern int              CurrentSavebuf;
extern int              SavebufNumber;
extern char             LastSavebuf;
extern savebuf _NEAR    Savebufs[MAX_SAVEBUFS];
extern savebuf _NEAR    SpecialSavebufs[MAX_SPECIAL_SAVEBUFS + 1];
extern savebuf          *WorkSavebuf;

/* undo data */
extern int              MaxUndoStack;
extern undo_stack       *UndoStack;
extern undo_stack       *UndoUndoStack;

/* bound key data */
extern event _NEAR      EventList[];
extern int              MaxKeysBound;
extern vi_key _NEAR     SavebufBound[];

/* directory info */
extern direct_ent * _NEAR   DirFiles[MAX_FILES];
extern list_linenum     DirFileCount;

/* window info */
extern window_info      editw_info, messagew_info, statusw_info;
extern window_info      cmdlinew_info, dirw_info;
extern window_info      setw_info, filelistw_info, setvalw_info;
extern window_info      linenumw_info, filecw_info;
extern window_info      repcntw_info, menubarw_info, menuw_info;
extern window_info      extraw_info, defaultw_info;
extern window_info      activemenu_info, greyedmenu_info, activegreyedmenu_info;

/* misc data */
extern long             SystemRC;
extern char             *CommandBuffer;
extern vi_rc            LastRetCode;
extern vi_rc            LastRC;
extern long             MaxMemFree, MemoryLeft, MaxMemFreeAfterInit;
extern eflags           EditFlags;
extern evars            EditVars;
extern vi_rc            LastError;
extern int              maxdotbuffer;
extern vi_key           *DotBuffer, *DotCmd, *AltDotBuffer;
extern int              DotDigits, DotCount, DotCmdCount, AltDotDigits, AltDotCount;
extern volatile long    ClockTicks;
extern int              RepeatDigits;
extern bool             NoRepeatInfo;
extern char _NEAR       RepeatString[MAX_REPEAT_STRING];
extern int              SourceErrCount;
extern bool             BoundData;
extern bool             RCSActive;

/* file io globals */
extern int              SwapBlocksInUse;
extern int              SwapBlockArraySize, XMemBlockArraySize;
extern char             *ReadBuffer, *WriteBuffer;
extern unsigned char    *XMemBlocks;
extern unsigned char    *SwapBlocks;
extern char             *HomeDirectory, *CurrentDirectory;
extern int              TotalEMSBlocks;
extern int              EMSBlocksInUse;
extern int              TotalXMSBlocks;
extern int              XMSBlocksInUse;

/* windowing globals */
extern char         ScrollBarChar;
extern int          ScrollBarCharColor;

/* parse constants */
extern int          MaxColorTokens, ColorTokensSize;
extern const char _NEAR   CmdLineTokens[];
extern const char _NEAR   SetVarTokens[];
extern const char _NEAR   SetFlagTokens[];
extern const char _NEAR   SetFlagShortTokens[];

/* Toolbar constants */

#endif
