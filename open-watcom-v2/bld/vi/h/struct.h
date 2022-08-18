/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Definition if internal editor structures.
*
****************************************************************************/


#ifndef _STRUCT_INCLUDED
#define _STRUCT_INCLUDED

#include <stdio.h>
#if defined( __DOS__ )
    #include "doschk.h"
#endif

typedef unsigned short  vi_ushort;

typedef struct ss {
    struct ss           *next;
    struct ss           *prev;
} ss;

typedef struct {
    unsigned char       height;
    unsigned char       width;
} cursor_type;

typedef struct {
    int                 posx;
    int                 posy;
    char                *find;
    int                 findlen;
    char                *replace;
    int                 replacelen;
    char                *ext;
    int                 extlen;
    char                *path;
    int                 pathlen;
    boolbit             case_ignore     : 1;
    boolbit             use_regexp      : 1;
    boolbit             search_forward  : 1;
    boolbit             search_wrap     : 1;
    boolbit             prompt          : 1;
    boolbit             selection       : 1;
} fancy_find;

typedef struct {
    int                 max;
    int                 curr;
    char                **data;
} history_data;

typedef struct {
    vi_key              *data;
    boolbit             inuse           : 1;
    boolbit             is_base         : 1;
    boolbit             was_inuse       : 1;
    boolbit             no_input_window : 1;
} key_map;

/* command structure */
typedef struct {
    unsigned char       len;
    char                cmd[256];
} cmd_struct;

/*
 * alias type
 */
typedef struct alias_list {
    struct alias_list   *next;
    struct alias_list   *prev;
    char                *alias;
    char                *expand;
} alias_list;

/*
 * line type
 */
typedef long linenum;

typedef struct i_mark {
    linenum             line;
    int                 column;
} i_mark;

/*
 * file stack
 */
typedef struct file_stack {
    i_mark              p;
    char                fname[1];
} file_stack;

/*
 * directory entry
 */
typedef struct {
    unsigned            sec     : 5;
    unsigned            min     : 6;
    unsigned            hour    : 5;
} time_struct;

typedef struct {
    unsigned            day     : 5;
    unsigned            month   : 4;
    unsigned            year    : 7;
} date_struct;

typedef struct {
    long                fsize;
#ifndef __UNIX__
    time_struct         time;
    date_struct         date;
#else
    unsigned long       time;
    unsigned short      st_mode;
#endif
    char                attr;
    char                name[1];
} direct_ent;

/*
 * window dimension
 */
typedef short           windim;

typedef struct {
    windim      x1, y1, x2, y2;
} winarea;

/*
 * window id
 */
#ifdef __WIN__
    #include "winhdr.h"
    #ifdef HAS_HWND
        typedef HWND            window_id;
        #define NO_WINDOW       ((window_id)NULL)
    #elif defined( __WINDOWS_386__ )
        typedef unsigned short  window_id;
        #define NO_WINDOW       ((window_id)0)
    #else
        typedef const void      _NEAR *window_id;
        #define NO_WINDOW       ((window_id)NULL)
    #endif
    #ifndef NDEBUG
        #define BAD_ID(id)      ((id) == NO_WINDOW || !IsWindow( id ))
    #else
        #define BAD_ID(id)      ((id) == NO_WINDOW)
    #endif
#else
    typedef unsigned char       window_id;
    #define MAX_WINDS           UCHAR_MAX
    #define NO_WINDOW           ((window_id)MAX_WINDS)
    #define BAD_ID(id)          ((id) == NO_WINDOW)
#endif

/*
 * info for a single text file line
 */
typedef struct linedata {
    unsigned    mark        : 5;    // first mark on the line
    unsigned    globmatch   : 1;    // global command matched this line
    unsigned    nolinedata  : 1;    // no data associated with this line (WorkLine has the data instead)
    unsigned    hidden      : 1;    // line is hidden (NYI)
    unsigned    hilite      : 1;    // line need hiliting
} linedata;

typedef vi_ushort   linedata_t;

typedef struct line {
    struct line *next, *prev;   // links for other lines
    union {
        linedata        ld;
        linedata_t      ld_word;
    } u;
    short       len;            // length of line
    char        data[1];        // actual string for line
} line;

typedef struct {
    line    *head, *tail;
} line_list;

/*
 * info for a file control block in a file
 */
struct file;
typedef struct fcb {
    struct fcb  *next, *prev;               // links fcbs in a file
    struct fcb  *thread_next, *thread_prev; // links all fcbs created
    struct file *f;                         // file associated with fcb
    line_list   lines;                      // linked list of lines
    linenum     start_line, end_line;       // starting/ending line number
    long        offset;                     // offset in swap file
    long        last_swap;                  // time fcb was last swapped
#if defined( __DOS__ )
    union {
        long    addr;
        xhandle handle;
    } xblock;                               // address of fcb in extended memory
#endif
    short       byte_cnt;                   // number of bytes in lines
    boolbit     swapped             : 1;    // fcb is swapped
    boolbit     in_memory           : 1;    // fcb is in memory
    boolbit     on_display          : 1;    // lines in fcb are displayed
    boolbit     non_swappable       : 1;    // fcb is not swappable
    boolbit     dead                : 1;    // fcb is dead (obsolete)
    boolbit     was_on_display      : 1;    // fcb was on display (used to save
                                            // display state when switching files)
    boolbit     in_extended_memory  : 1;    // fcb is in extended memory
    boolbit     in_xms_memory       : 1;    // fcb is in XMS memory
    boolbit     in_ems_memory       : 1;    // fcb is in EMS memory
    boolbit     nullfcb             : 1;    // fcb is a special one that has no
                                            // lines associated with it
    boolbit     globalmatch         : 1;    // a global command matched at least
                                            // one line in this fcb
} fcb;
#define FCB_SIZE sizeof( fcb )

typedef struct {
    fcb         *head, *tail;
} fcb_list;

/*
 * info for an entire file
 */
typedef struct file {
    char        *name;                  // file name
    char        *home;                  // home directory of file
    fcb_list    fcbs;                   // linked list of fcbs
    long        curr_pos;               // current offset in file on disk
    long        size;                   // size of file in bytes
    unsigned    modified        : 1;    // file has been modified
    unsigned    bytes_pending   : 1;    // there are still bytes to be read off the disk for the file
    unsigned    viewonly        : 1;    // file is view only
    unsigned    read_only       : 1;    // file is read only
    unsigned    check_readonly  : 1;    // file needs its read-only status checked against the file on disk
    unsigned    dup_count       : 4;    // number of duplicate views on the file that have been opened
    unsigned    been_autosaved  : 1;    // file has been autosaved
    unsigned    need_autosave   : 1;    // file needs to be autosaved
    unsigned    is_stdio        : 1;    // file is a "stdio" file (reads from stdin and writes to stdout)
    unsigned    needs_display   : 1;    // file needs to be displayed
    unsigned    write_crlf      : 1;    // check file system when we write it out
    unsigned    fill15          : 1;
    unsigned    fill16          : 1;
    int         handle;                 // file handle (if entire file is not read, will be an open file handle)
#ifdef __UNIX__
    short       attr;
#endif
    char        as_name[TMP_NAME_LEN];  // name that file was auto-saved as
} file;
#define FILE_SIZE sizeof( file )

/*
 * mark setting
 */
typedef struct {
    i_mark              p;              // line number and column number that mark is on
    unsigned            next    : 5;    // pointer to next mark on the same line
    unsigned            inuse   : 1;    // mark is being used
} mark;
#define MARK_SIZE sizeof( mark )

typedef struct range {
    i_mark              start;
    i_mark              end;
/*
 * ACK! Here we have a lovely little wart on the operator/movement
 * system. Some commands want to highlight a region of text after they
 * have moved; to support this we add these marks which tell us where
 * to begin and end highlighting, as well as a flag to decide whether to
 * actually do the highlighting. Puke.
 */
    i_mark              hi_start;
    i_mark              hi_end;
    boolbit             highlight   : 1;
    boolbit             line_based  : 1;
/*
 * Double ACK! Some times we need to treat a range differently depending
 * on whether an operator or a move is using it. This tells us if we should
 * include the last character or not (compare "d/foo" to "/foo"). Puke.
 */
    boolbit             fix_range   : 1;
    boolbit             selected    : 1;
} range;

typedef vi_rc (*insert_rtn)( void );
typedef vi_rc (*move_rtn)( range *, long count );
typedef vi_rc (*op_rtn)( range * );
typedef vi_rc (*misc_rtn)( long count );
typedef vi_rc (*old_rtn)( void );
typedef vi_rc (*alias_rtn)( void *, void * );

typedef union command_rtn {
    vi_rc               (*dummy)(); /* Must go first to avoid parameter type mismatches */
    insert_rtn          ins;
    move_rtn            move;
    op_rtn              op;
    misc_rtn            misc;
    old_rtn             old;
    alias_rtn           alias;
} command_rtn;

typedef struct {
    unsigned            type                    : 3;    // type of event (list in const.h)
    unsigned            keep_selection          : 1;    // keep selection after this event?
    unsigned            is_number               : 1;    // is event a digit? (repeat count)
    unsigned            fix_range               : 1;    // see comment in doMove (editmain.c)
    unsigned            is_op_alias             : 1;    // event is an aliased operator
    unsigned            keep_selection_maybe    : 1;    // keep selection if already selecting
    unsigned            modifies                : 1;    // does this event modify the file?
    unsigned            spare                   : 7;
} event_bits;

typedef struct {
    command_rtn         alt_rtn;
    insert_rtn          ins;
    command_rtn         rtn;
    event_bits          b;
    event_bits          alt_b;
} event;

/* used to cast a base event (as in keys.h) to a char */
#define EVENT_CHAR( e ) ((char) e)

/*
 * structure to define all the goop needed to display text
 */
typedef struct {
    vi_color            foreground;
    vi_color            background;
    font_type           font;
} type_style;

/*
 * window init info
 */
typedef struct {
    vi_color            border_color1;
    vi_color            border_color2;
    type_style          text_style;
    type_style          hilight_style;
    winarea             area;
    bool                has_border;
} window_info;

/*
 * undo info
 */
typedef struct {
    linenum             start, end;
} undo_insert;

typedef fcb_list undo_delete;

typedef struct {
    i_mark              p;
    linenum             top;
    long                time_stamp;
    short               depth;
} undo_start;

typedef union {
   undo_start           sdata;
   undo_insert          del_range;
   undo_delete          fcbs;
} undo_data;

typedef struct undo {
    struct undo         *next;
    char                type;
    undo_data           data;
} undo;
#define UNDO_SIZE sizeof( undo )

typedef struct undo_stack {
    int                 current;        // current depth of undo stack
    int                 OpenUndo;       // number of "StartUndoGroups" open
                                        // on stack
    undo                **stack;        // stack data
    bool                rolled;         // stack has been rolled (i.e., data
                                        // has rolled off the end)
} undo_stack;

typedef struct select_rgn {
    i_mark              start;
    i_mark              end;
    int                 start_col_v;
    boolbit             selected    : 1;
    boolbit             lines       : 1;
    boolbit             dragging    : 1;
} select_rgn;

/*
 * all specific info for a file being edited
 */
typedef struct fs_info {
    char                *TagFileName;
    char                *GrepDefault;
    int                 TabAmount;
    int                 HardTab;
    int                 ShiftWidth;
    lang_t              Language;
    bool                PPKeywordOnly;
    bool                CMode;
    bool                ReadEntireFile;
    bool                ReadOnlyCheck;
    bool                IgnoreCtrlZ;
    bool                CRLFAutoDetect;
    bool                WriteCRLF;
    bool                EightBits;
    bool                RealTabs;
    bool                AutoIndent;
    bool                IgnoreTagCase;
    bool                TagPrompt;
    bool                ShowMatch;
} fs_info;

typedef struct ss_flags_c {
    boolbit             inCComment        : 1;
    boolbit             inCPPComment      : 1;
    boolbit             inString          : 1;
    boolbit             inPreprocessor    : 1;
    boolbit             inErrorDir        : 1;
    boolbit             inIfDir           : 1;
    boolbit             inPragmaDir       : 1;
    boolbit             inDeclspec        : 1;
    boolbit             inDeclspec2       : 1;
} ss_flags_c;

typedef struct ss_flags_f {
    boolbit             inString          : 1;
} ss_flags_f;

typedef struct ss_flags_h {
    boolbit             inHTMLComment     : 1;
    boolbit             inHTMLKeyword     : 1;
    boolbit             inAltHTMLKeyword  : 1;
    boolbit             inString          : 1;
} ss_flags_h;

typedef struct ss_flags_g {
    boolbit             inGMLComment      : 1;
    boolbit             inGMLKeyword      : 1;
    boolbit             inAltGMLKeyword   : 1;
    boolbit             inString          : 1;
} ss_flags_g;

typedef struct ss_flags_m {
    boolbit             inPreproc         : 1;
    boolbit             inInlineFile      : 1;
    boolbit             inMacro           : 1;
} ss_flags_m;

typedef struct ss_flags_p {
    boolbit             inString          : 1;
    boolbit             beforeRegExp      : 1;
    boolbit             doubleRegExp      : 1;
} ss_flags_p;

typedef union ss_flags {
    ss_flags_c          c;
    ss_flags_f          f;
    ss_flags_h          h;
    ss_flags_g          g;
    ss_flags_m          m;
    ss_flags_p          p;
} ss_flags;

typedef enum syntax_element {
    SE_TEXT,            // always first
    SE_WHITESPACE,      // don't mess with order (fonts in .cfg parallel #s)
    SE_SELECTION,
    SE_EOFTEXT,
    SE_KEYWORD,
    SE_OCTAL,
    SE_HEX,
    SE_INTEGER,
    SE_CHAR,
    SE_PREPROCESSOR,
    SE_SYMBOL,
    SE_INVALIDTEXT,
    SE_IDENTIFIER,
    SE_JUMPLABEL,
    SE_COMMENT,
    SE_FLOAT,
    SE_STRING,
    SE_VARIABLE,
    SE_REGEXP,
    SE_UNUSED,          // hidden, use to temporarily setup a style
    SE_UNPARSED,        // hidden, basically used to flag problems
    SE_MAXSIZE          // max style array size
} syntax_element;

#define SE_NUMTYPES     SE_UNUSED

typedef struct ss_block {
    short               end;
    short               len;
#ifdef __WIN__
    // offset of start of following block
    int                 offset;
#endif
    syntax_element      type;
} ss_block;

typedef struct dc_line {
    ss_block            *ss;
    int                 start_col;
    char                *text;
    size_t              textlen;
    ss_flags            flags;
    // Windows & text mode
    boolbit             display : 1;    // line needs to be redisplayed
    // Windows only
    boolbit             valid   : 1;    // cache reflects what is on screen
} dc_line;

/*
 * all info for a file being edited
 */
typedef struct info {
    struct info         *next;
    struct info         *prev;
    file                *CurrentFile;
    i_mark              CurrentPos;
    i_mark              LeftTopPos;
    undo_stack          *UndoStack;
    undo_stack          *UndoUndoStack;
    int                 CurrentUndoItem;
    int                 CurrentUndoUndoItem;
    mark                *MarkList;
    int                 VirtualColumnDesired;
    select_rgn          SelRgn;
    boolbit             IsColumnRegion      : 1;
    boolbit             linenumflag         : 1;
    window_id           linenum_current_window_id;
    window_id           current_window_id;
    vi_ushort           DuplicateID;
    dc_line             *dclines;
    int                 dc_size;
    fs_info             fsi;
#ifdef __WIN__
    long                VScrollBarScale;
    int                 HScrollBarScale;
#endif
} info;

/*
 * save buffer (for yanking/moving text)
 */
typedef struct savebuf {
    union {
        char            *data;
        fcb_list        fcbs;
    } u;
    char                type;
} savebuf;
#define SAVEBUF_SIZE sizeof( savebuf )

/*
 * color settings
 */
typedef struct {
    unsigned char       red;
    unsigned char       green;
    unsigned char       blue;
} rgb;

/*
 * video attributes (internal)
 */
typedef unsigned short  viattr_t;

typedef struct {
    int                 _offs;
    char                _char;
} hichar;

typedef int             list_linenum;

/*
 * SelectItem data
 */
typedef struct {
    window_info         *wi;                // info describing window to create
    char                *title;             // title of window
    char                **list;             // lines to display
    list_linenum        maxlist;            // number of lines in list
    char                *result;            // where to copy the data for the picked line
    list_linenum        num;                // number of the picked line
    int                 *allowrl;           // allow cursor right/left (for menu bar)
    hichar              *hi_list;           // chars to highlight
    const vi_key        *retevents;         // events that simulate pressing enter
    vi_key              event;              // event that caused a return
    list_linenum        cln;                // current line to display
    window_id           event_wid;          // alternate window to accept events in (like the options window after fgrep...)
    boolbit             show_lineno : 1;    // show lines in top-right corner
    boolbit             is_menu     : 1;    // is a menu we are showing
} selectitem;

typedef vi_rc checkres_fn( const char *, char *, int * );

/*
 * SelectLineInFile data structure
 */
typedef struct {
    file                *f;                     // file with data for lines
    char                **vals;                 // values associated with each line
    size_t              valoff;                 // offset to display values beside line data
    window_info         *wi;                    // info describing window to create
    list_linenum        sl;                     // selected line
    char                *title;                 // title of window
    checkres_fn         *checkres;              // check if selected change is valid
    int                 *allowrl;               // allow cursor right/left (for menu bar)
    hichar              *hi_list;               // chars to highlight
    const vi_key        *retevents;             // events that simulate pressing enter
    vi_key              event;                  // event that caused a return
    list_linenum        cln;                    // current line to display
    window_id           event_wid;              // alternate window to accept events in (like the options window after fgrep...)
    boolbit             show_lineno        : 1; // show lines in top-right corner
    boolbit             is_menu            : 1; // select list is a menu
    boolbit             has_scroll_gadgets : 1; // list has scroll gadgets
} selflinedata;

/*
 * special file (used to process bound data)
 */
typedef struct {
    unsigned            maxlines;
    unsigned            currline;
    unsigned char       length;
} gfa;

typedef struct {
    struct line         *cline;
    struct fcb          *cfcb;
} gfb;

typedef struct {
    union {
        FILE            *fp;
        int             handle;
        char            *pos;
        struct file     *cfile;
    } data;
    union {
        gfa             a;
        gfb             b;
    } gf;
    gftype type;
} GENERIC_FILE;

#define INITVARS

typedef struct {
    /*
     * set booleans are here
     */
    #define PICK(a,b,c,d,e)     boolbit     c : 1;
    #include "setb.h"
    #undef PICK
    /*
     * internal booleans are here
     */
    #define PICK(a,b)           boolbit     a : 1;
    #include "setbi.h"
    #undef PICK
} eflags;               // don't forget to give default in globals.c

typedef struct {
    /*
     * set variables are here
     */
    #define PICK(a,b,c,d,e,f)   c d;
    #include "setnb.h"
    #undef PICK
    /*
     * internal variables are here
     */
    #define PICK(a,b,c)         a b;
    #include "setnbi.h"
    #undef PICK
} evars;               // don't forget to give default in globals.c

#undef INITVARS

#endif
