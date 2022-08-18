/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Screen output and windowing interface.
*
****************************************************************************/


#include "winaux.h"


#define DEF_TEXT_STYLE              { WHITE, BLACK, FONT_COURIER }
#define DEF_HILIGHT_STYLE           { BRIGHT_WHITE, BLACK, FONT_COURIER }

#ifndef __WIN__

#define WRITE_SCREEN( a, b )        (*(char_info _FAR *)&(a)) = (*(char_info *)(&b))
#define WRITE_SCREEN2( a, b )       (*(char_info _FAR *)&(a)) = (*(char_info _FAR *)(&b))
#define WRITE_SCREEN_DATA( a, b )   (*(char_info *)&(a)) = (*(char_info *)(&b))

#define WINDOW_FROM_ID( x )         (Windows[x])
#define WINDOW_TO_ID( x, d )        (Windows[x] = d)

/*
 * character info
 */
#if defined( __NT__ )
typedef CHAR_INFO char_info;
#define cinfo_char  Char.AsciiChar
#define cinfo_wchar Char.UnicodeChar
#define cinfo_attr  Attributes
#else
typedef struct char_info {
    char            _char;
    unsigned char   _attr;
} char_info;
#define cinfo_char  _char
#define cinfo_attr  _attr
#endif

/*
 * window structure
 */
typedef struct window {
    vi_color        border_color1;
    vi_color        border_color2;
    vi_color        text_color;
    vi_color        background_color;
    winarea         area;
    windim          width;
    windim          height;
    char_info       *text;
    window_id       *overlap;
    window_id       *whooverlapping;
    short           text_lines;
    short           text_cols;
    char            *title;
    char            *borderdata;
    int             vert_scroll_pos;
    short           bordercol;
    window_id       id;
    boolbit         isswapped           : 1;
    boolbit         has_border          : 1;
    boolbit         has_gadgets         : 1;
    boolbit         min_slot            : 1;
    boolbit         has_scroll_gadgets  : 1;
    signed char     accessed;
    signed char     overcnt[1];
} window;

extern window       *Windows[MAX_WINDS + 1];

#define THUMB_START         2
#define NORMAL_ATTR         7
#define MAX_MIN_SLOTS       40
#define WIND_TOP_BORDER     0
#define WIND_BOTTOM_BORDER  1

#ifndef __VIO__
    #define MAKE_ATTR( w, a, b )    (viattr_t)( (a) + (b) * 16 )
#else
    #define MAKE_ATTR( w, a, b )    (viattr_t)( (a) + ((b) & 7) * 16 )
#endif

#ifdef __NT__
extern HANDLE       InputHandle, OutputHandle;
#endif

extern char_info    WindowNormalAttribute;
extern char         WindowBordersNG[];
extern char         WindowBordersG[];
extern char_info    _FAR *Scrn;
extern char_info    _FAR *ClockStart;
extern char_info    _FAR *SpinLoc;
extern window_id    *ScreenImage;
extern bool         MinSlots[MAX_MIN_SLOTS];

#endif

#ifdef __WIN__

/* win/window.c */
extern void     DefaultWindows( RECT *world, RECT *workspace );
extern void     InitWindows( void );
extern void     FiniWindows( void );
extern int      WindowAuxInfo( window_id wid, int type );
extern vi_rc    NewWindow2( window_id *wid, window_info *wi );
extern void     CloseAWindow( window_id wid );
extern void     CloseAChildWindow( window_id wid );
extern bool     InsideWindow( window_id wid, int x, int y );
extern void     InactiveWindow( window_id wid );
extern void     ActiveWindow( window_id wid );
extern void     MoveWindowToFront( window_id wid );
extern void     MoveWindowToFrontDammit( window_id wid, bool );
extern vi_rc    MaximizeCurrentWindow( void );
extern vi_rc    MinimizeCurrentWindow( void );
extern void     FinishWindows( void );

/* win/display.c */
extern void     ScreenPage( int page );
extern void     WindowTitleAOI( window_id wid, const char *title, bool active );
extern void     WindowTitle( window_id wid, const char *title );
extern void     ClearWindow( window_id wid );
extern void     ShiftWindowUpDown( window_id wid, int lines );
extern bool     SetDrawingObjects( HDC hdc, type_style *ts );
extern int      DisplayLineInWindowWithSyntaxStyle( window_id, int, line *, linenum, char *, int, HDC );

/* win/stubs.c */
extern bool     DisplayMouse( bool p1 );
extern void     TurnOffCapsLock( void );
extern vi_rc    HandleMouseEvent( void );
extern void     SwapAllWindows( void );
extern void     SetMouseSpeed( int i );
extern void     GetClockStart( void );
extern void     GetSpinStart( void );
extern void     WindowAuxUpdate( window_id wid, int x, int y );
extern void     DrawBorder( window_id wid );
extern void     PushMouseEventHandler( mouse_callback cb );
extern void     PopMouseEventHandler( void );
extern void     WindowBorderData( window_id wid, const char *c, int x );
extern vi_rc    ResizeWindowRelative( window_id wid, windim p1, windim p2, windim p3, windim p4, bool flags );
extern vi_rc    ResizeWindow( window_id wid, windim p1, windim p2, windim p3, windim p4, bool flags );
extern void     RestoreInterrupts( void );
extern void     WindowSwapFileClose( void );
extern void     FiniMouse( void );
extern void     ScreenFini( void );
extern vi_rc    ResizeCurrentWindowWithKeys( void );
extern vi_rc    MoveCurrentWindowWithKeys( void );
extern drive_type DoGetDriveType( int i );
extern void     ClearScreen( void );
extern vi_rc    ResetWindow( window_id *wid );
extern bool     WindowIsVisible( window_id wid );
extern void     ScreenInit( void );
extern void     SetInterrupts( void );
extern void     ChkExtendedKbd( void );
extern void     InitMouse( void );
extern void     SetBorderGadgets( window_id wid, bool how );
extern vi_rc    GetNewValueDialog( char * );
extern void     DisplayCrossLineInWindow( window_id wid, int line );
extern int      SetCharInWindowWithColor( window_id wid, windim line, windim col, char text, type_style *style );
extern void     DisplayLineWithHilite( window_id wid, int line, char *text, int start, int end, int ignore );
extern void     SetPosToMessageLine( void );
extern void     HideCursor( void );

/* win/editwnd.c */
extern void     PositionVerticalScrollThumb( window_id wid, linenum top, linenum last );
extern void     PositionHorizontalScrollThumb( window_id wid, int left );

/* win/wintica.c */
extern vi_rc    WindowTile( int, int );
extern vi_rc    WindowCascade( void );

/* win/cursor.c */
extern void     SetCursorOnLine( window_id wid, int col, char *str, type_style *style );
extern void     SetGenericWindowCursor( window_id wid, int row, int col );
extern void     ResetEditWindowCursor( window_id wid );
extern void     MyShowCaret( window_id wid );
extern void     MyHideCaret( window_id wid );
extern void     MyKillCaret( window_id wid );
extern void     MyRaiseCaret( window_id wid );

/* win/main.c */
extern void     StartWindows( void );

/* win/utils.c */
extern void     SetGadgetString( char *str );
extern bool     IsGadgetStringChanged( const char *str );

/* win/repcnt.c */
extern void     UpdateRepeatString( char *str );

/* win/clipbrd.c */
extern int      AddLineToClipboard( char *data, int scol, int ecol );
extern int      AddFcbsToClipboard( fcb_list *fcbs );
extern int      GetClipboardSavebuf( savebuf *clip );
extern bool     IsClipboardEmpty( void );

#else

/* adjwin.c */
extern vi_rc    ResizeCurrentWindowWithKeys( void );
extern vi_rc    MoveCurrentWindowWithKeys( void );
extern vi_rc    ResizeCurrentWindowWithMouse( void );
extern vi_rc    MoveCurrentWindowWithMouse( void );

/* winaux.c */
extern int      WindowAuxInfo( window_id, int );
extern void     WindowAuxUpdate( window_id, int, int );

/* winbrdr.c */
extern void     DrawBorder( window_id );
extern void     SetBorderGadgets( window_id, bool );
extern void     WindowBorderData( window_id, const char *, int );
extern void     SetGadgetString( char *str );
extern bool     IsGadgetStringChanged( const char *str );

/* windisp.c */
extern vi_rc    DisplayLineInWindowWithColor( window_id, int, const char *, type_style *, int );
extern vi_rc    DisplayLineInWindowWithSyntaxStyle( window_id, int, line *, linenum, char *, int, unsigned int );
extern void     DisplayCrossLineInWindow( window_id, int );
extern void     HiliteAColumnRange( linenum, int, int );
extern void     ColorAColumnRange( int, int, int, type_style * );
extern vi_rc    SetCharInWindowWithColor( window_id, windim, windim, char, type_style * );

/* ui/wininit.c */
extern void     StartWindows( void );
extern void     FinishWindows( void );

/* ui/winnew.c */
extern vi_rc        ResetWindow( window_id * );
extern bool         ValidDimension( windim, windim, windim, windim, bool );
extern window       *AllocWindow( window_id, windim, windim, windim, windim, bool, bool, vi_color, vi_color, vi_color, vi_color );
extern void         FreeWindow( window * );
extern vi_rc        NewWindow( window_id *, windim, windim, windim, windim, bool, vi_color, vi_color, type_style * );
extern vi_rc        NewFullWindow( window_id *, bool, vi_color, vi_color, type_style * );
extern vi_rc        NewWindow2( window_id *, window_info * );
extern void         CloseAWindow( window_id );

/* window.c */
extern void     CloseAChildWindow( window_id );

/* winover.c */
extern void         ResetOverlap( window * );
extern bool         TestVisible( window * );
extern void         MarkOverlap( window_id );
extern void         RestoreOverlap( window_id, bool );
extern bool         TestOverlap( window_id );
extern bool         WindowIsVisible( window_id );
extern window_id    WhoIsUnder( windim *, windim * );

/* winscrl.c */
extern void     ShiftWindowUpDown( window_id, int );

/* winsize.c */
extern vi_rc    ResizeWindow( window_id, windim, windim, windim, windim, bool );
extern vi_rc    ResizeWindowRelative( window_id, windim, windim, windim, windim, bool );
extern vi_rc    MinimizeCurrentWindow( void );
extern vi_rc    MaximizeCurrentWindow( void );

/* winswap.c */
extern void     SwapAllWindows( void );
extern void     AccessWindow( window * );
extern void     ReleaseWindow( window *);
extern void     WindowSwapFileClose( void );

/* winthumb.c */
extern void     PositionVerticalScrollThumb( window_id wid, linenum curr, linenum last );
extern void     PositionHorizontalScrollThumb( window_id, int );
extern void     DrawVerticalThumb( window *w, char ch );
extern vi_rc    PositionToNewThumbPosition( window *w, int thumb );

/* ui/wintica.c */
extern vi_rc    WindowTile( int, int );
extern vi_rc    WindowCascade( void );

/* filesel.c (Windows only functions) */
extern vi_rc    SelectFileSave( char * );

/* ui/winshow.c */
extern void     MoveWindowToFront( window_id );
extern void     MoveWindowToFrontDammit( window_id, bool );
extern void     InactiveWindow( window_id );
extern void     ActiveWindow( window_id );
extern void     WindowTitleAOI( window_id wid, const char *title, bool active );
extern void     WindowTitleInactive( window_id, const char * );
extern void     WindowTitle( window_id, const char * );
extern void     ClearWindow( window_id );
extern void     SetGenericWindowCursor( window_id, int, int );
extern void     SetCursorOnLine( window_id, int, char *, type_style * );
extern void     ResetEditWindowCursor( window_id );
extern bool     InsideWindow( window_id, int, int );
extern void     MyShowCaret( window_id );
extern void     MyHideCaret( window_id );
extern void     MyKillCaret( window_id );
extern void     MyRaiseCaret( window_id );

extern viattr_t WindowAttr( window *, vi_color, vi_color );

#endif
