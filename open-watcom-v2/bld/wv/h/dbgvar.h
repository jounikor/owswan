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
* Description:  Structures and prototypes for display of program variables.
*
****************************************************************************/


typedef enum node_class {
    NODE_ROOT,
    NODE_FIELD,
    NODE_SUBSCR,
    NODE_INHERIT,
    NODE_POINTS,
} node_class;

typedef enum var_gadget_type {
    VARGADGET_NONE,
    VARGADGET_OPEN,
    VARGADGET_CLOSED,
    VARGADGET_POINTS,
    VARGADGET_UNPOINTS,
    VARGADGET_BADPOINTS,
    VARGADGET_INHERIT_OPEN,
    VARGADGET_INHERIT_CLOSED,
    VARGADGET_LAST
} var_gadget_type;

typedef enum var_type_bits {
    VARNODE_CODE                = 0x01,
    VARNODE_INHERIT             = 0x02,
    VARNODE_COMPILER            = 0x04,
    VARNODE_PRIVATE             = 0x08,
    VARNODE_PROTECTED           = 0x10,
    VARNODE_THIS                = 0x20,
    VARNODE_STATIC              = 0x40
} var_type_bits;

typedef enum var_display_bits {
    VARDISP_HEX                 = 0x01,
    VARDISP_DECIMAL             = 0x02,
    VARDISP_CHARACTER           = 0x04,
    VARDISP_STRING              = 0x08,
    VARDISP_POINTER             = 0x10,
    // free slot here!
    VARDISP_INHERIT_CLOSED      = 0x40,
} var_display_bits;

typedef struct var_node {
    struct var_node     *next;          // link to next element
//      VarFirstNode, VarExpandNode, VarNextNode and VarNextRowNode must always be used
//      when drawing the window. Never reference ->next or ->expand directly
//      unless you're manipulating this data structure
    struct var_node     *path;          // when working on a given node, this is the reverse of 'parent'
    struct var_node     *parent;        // pointer to parent if this is a NODE_SUBSCR or NODE_FIELD
    struct var_node     *expand;        // pointer to array/struct expansion
    struct var_node     *old_expand;    // saved 'expand' when var is closed
    struct scope_state  *s;             // points back to originating scope
    type_handle         *th;            // the type of this node's expression
    char                *value;         // the value of the node (eg: "17")
    long                exprsp_timestamp;// compared against var->exprsp_timestamp so we can tell if this expression is really on ExprSP or not
    long                element;        // subscript for array
    var_gadget_type     gadget;         // what gadget should we display
    node_class          node_type;      // NODE_ROOT, NODE_SUBSCR or NODE_FIELD
    struct type_display *display_type;  // how should we display this node

    boolbit             pushed          : 1;    // has this expression been pushed on ExprSP?
    boolbit             popped          : 1;    // has this expression been popped off ExprSP?
    boolbit             buried          : 1;    // versus on the top of the stack
    boolbit             is_string       : 1;    // is this a natural character string

    boolbit             have_type       : 1;    // is ->th valid?
    boolbit             is_sym_handle   : 1;    // versus a character string
    boolbit             fake_array      : 1;    // not really an array but expanded as such
    boolbit             value_valid     : 1;    // are the gadget/str fields accurate?

    boolbit             gadget_valid    : 1;    // are the gadget/str fields accurate?
    boolbit             standout        : 1;    // draw in standout attribute?
    boolbit             on_top          : 1;    // draw the on_top indicator?
    boolbit             on_top_valid    : 1;    // is the on_top field valid?

    var_type_bits       bits;           // class of var (inherited, etc)
    var_display_bits    display;        // how to display the sucker

//  struct var_node     *inherit;       // inherited from ...
    union {
        char            __expr[1];      // the expression string (for expression window)
        char            __hdl[1];       // the sym handle (for locals, etc window)
    } u;
} var_node;

#define VarNodeExpr( v ) ((v)->u.__expr)
#define VarNodeHdl( v ) ((sym_handle*)((v)->u.__hdl))

typedef struct scope_state {
    struct scope_state  *next;                  // link for memory cleanup
    struct scope_state  *outer;                 // a scope surrounding this one
    mappable_addr       scope;                  // the scope handle (for locals window)
    addr_off            scope_len;              //
    unsigned long       scope_unique;
    mod_handle          mod;                    // the module (for filescope variables window)
    var_node            *v;                     // the variable nodes
    long                scope_timestamp;        // LRU timestamp used for garbage collection
    void                *wnd_data;              // window can save stuff here
    boolbit             unmapped        : 1;    // is this scope unmapped (are we in the process of restarting the program)
} scope_state;

typedef struct {
    var_node            *exprsp_cache;          // if ExprSP is still around, what expression is it for
    int                 exprsp_cacherow;        //   - and on what row
    long                exprsp_timestamp;       // incremented when we blow away ExprSP
    scope_state         *s;                     // the current scope for this window
//  var_type_bits       hide;
    int                 name_end_row;
    boolbit             ok_to_cache_exprsp      : 1;    // should we leave ExprSP around?
    boolbit             exprsp_cache_is_error   : 1;    // was the last thing the expression processor gave us an error?
    boolbit             mem_lock                : 1;    // don't garbage collect from this window. We're using it's data structures
    boolbit             members                 : 1;    // show members without this->?
} var_info;

#define VAR_NO_ROW -1

extern bool             VarError;

enum {
    VAR_PIECE_GADGET,
    VAR_PIECE_NAME,
    VAR_PIECE_VALUE,
    VAR_PIECE_LAST
};

typedef struct type_display {
    struct type_display         *next;              // pointer to next in our list
    struct type_display         *fields;            // pointer to a list of fields
    struct type_display         *parent;            // pointer to a fields parent
    struct type_display         *alias;             // pointer to a list of same types
    var_display_bits            display;            // how to display this field
    var_type_bits               hide;               // what to hide
    boolbit                     autoexpand  : 1;    // automatically expand this type?
    boolbit                     on_top      : 1;    // show as the unexpanded value
    boolbit                     has_top     : 1;    // has a subfields to show as unexpanded value
    boolbit                     is_struct   : 1;    // is this field a struct
    boolbit                     is_field    : 1;    // is this field a struct
    boolbit                     dirty       : 1;    // is this field dirty
    char                        name[1];            // MUST BE LAST FIELD
} type_display;

typedef enum {
    #define pick(e,name,wndclass,icon)  e,
    #include "_dbgvar.h"
    #undef pick
    NUM_VAR_TYPE
} var_type;

typedef struct scope_list {
    struct scope_list *next;
    scope_block scope;
} scope_list;

extern type_display     *TypeDisplay;

extern bool             VarDeleteAScope(var_info *i,void*);
extern bool             VarUnMap(var_info *i, void *);
extern bool             VarDeleteAllScopes(var_info *i,void*);
extern bool             VarReMap(var_info *i, void *);

extern int              VarRowTotal(var_info *i);
extern void             VarAllNodesInvalid(var_info *i);
extern void             VarKillExprSPCache(var_info *i);
extern bool             VarErrState(void);
extern var_node         *VarAdd1(var_info *i,const void *name,unsigned int len,bool expand,bool is_sym_handle);
extern mad_radix        VarNewDisplayRadix( var_display_bits display );
extern mad_radix        VarNewCurrRadix(var_node *v);
extern int              VarFindRootRow(var_info *i,var_node *v,int row);
extern bool             VarExpandable(type_kind node_class);
extern bool             VarParentIsArray( var_node *v );
extern var_node         *VarFindRow(var_info *i,int row);
extern void             VarAddNodeToScope(var_info *i,var_node *v,const char *buff);
extern void             VarExpandRow(var_info *i,var_node *v,int row);
extern void             VarExpandRowNoCollapse(var_info *i,var_node *v,int row);
extern bool             VarPrintText(var_info *i,char *buff,void (*rtn)(void ),int len);
extern void             VarBuildName(var_info *i,var_node *v,bool just_end_bit);
extern bool             VarGetStackClass(type_kind *node_class);
extern var_node         *VarFindRowNode(var_info *i,int row);
extern bool             VarOldErrState(void);
extern void             VarDoneRow(var_info *i);
extern void             VarDoAssign(var_info *i, var_node *v, const char *value );
extern var_node         *VarFindRoot(var_info *i,int row,int *skipped);
extern bool             VarIsPointer(type_kind node_class);
extern void             VarDeExpand(var_node *v);
extern void             VarDelete(var_info *i,var_node *v);
extern bool             VarExpand(var_info *i,var_node *v,long start,long end);
extern void             VarOkToCache(var_info *i,bool ok);
extern char             *VarGetValue(var_info *i,var_node *v);
extern var_node         *VarNextRowNode(var_info *i,var_node *v);
extern void             VarSetGadget(var_node *v,var_gadget_type gadget);
extern void             VarSetOnTop(var_node *v,bool);
extern void             VarFiniInfo(var_info *i);
extern var_gadget_type  VarGetGadget(var_node *v);
extern bool             VarGetOnTop( var_node *v );
extern void             VarNodeInvalid(var_node *v);
extern var_node         *VarExpandNode( var_node *v );
extern var_node         *VarFirstExpandNode( var_info *i, var_node *v );
extern var_node         *VarFirstNode(var_info *i);
extern void             VarInitInfo(var_info *i);
extern bool             VarInfoRefresh(var_type vtype,var_info *i,address *addr);
extern void             VarSetValue( var_node *v, const char *value );

extern void             VarDisplaySetHex(var_node*v);
extern void             VarDisplaySetArrayHex(var_node*v);
extern void             VarDisplaySetDecimal(var_node*v);
extern void             VarDisplaySetArrayDec(var_node*v);
extern void             VarDisplaySetString( var_node *v );
extern void             VarDisplaySetPointer( var_node *v );
extern void             VarDisplaySetChar( var_node *v );
extern bool             VarDisplayIsHex(var_node*v);
extern bool             VarDisplayIsDecimal(var_node*v);
extern bool             VarDisplayIsString( var_node *v );
extern bool             VarDisplayIsPointer( var_node *v );
extern bool             VarDisplayIsChar( var_node *v );
extern void             VarBreakOnWrite(var_info *i,var_node*v);
extern void             VarAddWatch( var_info *i, var_node *v );
extern void             VarInspectMemory( void );
extern void             VarInspectPointer( void );
extern void             VarInspectCode( void );
extern bool             VarIsLValue( void );

extern bool             VarDisplayShowMembers( var_info *i );
extern void             VarDisplaySetMembers( var_info *i, bool on );
extern void             VarDisplaySetBits( var_node *v );
extern type_display     *VarDisplayFindParent( type_display *curr );
extern void             VarDisplayOnTop( var_node *v, bool on );
extern bool             VarDisplayedOnTop( var_node *v );
extern void             VarDisplayUpdate( var_info *i );
extern char             *VarDisplayType( var_node *v, char *buff, size_t buff_len );

extern type_display     *VarDisplayAddStruct( const char *name );
extern type_display     *VarDisplayAddField( type_display *parent, const char *name );
extern void             VarDisplayAlias( type_display *type, type_display *to );
extern void             VarDisplayDirty( type_display *curr );
extern void             VarDisplayInit( void );
extern void             VarDisplayFini( void );
extern void             VarDisplayFlipHide( var_node *v, var_type_bits bit );
extern bool             VarDisplayIsHidden( var_node *v, var_type_bits bit );
extern void             VarDisplaySetHidden( var_node *v, var_type_bits bit, bool on );
extern bool             VarDisplayIsStruct( var_node *v );
extern void             VarGetDepths( var_info *i, var_node *v, int *pdepth, int *pinherit );
extern var_node         *VarNextVisibleSibling( var_info *i, var_node *v );
extern void             VarBaseName( var_node *v );

extern scope_state      *NewScope( var_info *i, scope_block *scope, mod_handle mod, bool *new );
extern bool             SameScope( scope_block *scope, scope_state *s );
