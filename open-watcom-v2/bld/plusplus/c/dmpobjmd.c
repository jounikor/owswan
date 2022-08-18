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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "plusplus.h"
#include "pragdefn.h"
#include "vbuf.h"
#include "vstk.h"
#include "name.h"


typedef enum dump_struct_control {
    DS_BASE     = 0x0001,
    DS_NULL     = 0x0000,
} ds_control;

typedef struct                  // DUMP INFORMATION
{   target_size_t offset;       // - offset of current structure
    VSTK_CTL stack;             // - parentage stack
    VBUF buffer;                // - buffer for printing
    TYPE original;              // - original type
} DUMP_INFO;


static void bufferChr(          // CONCATENATE CHAR TO BUFFER
    DUMP_INFO* di,              // - dump information
    char chr )                  // - to be concatenated
{
    VbufConcChr( &di->buffer, chr );
}


static void bufferStr(          // CONCATENATE STRING TO BUFFER
    DUMP_INFO* di,              // - dump information
    const char *str )           // - to be concatenated
{
    VbufConcStr( &di->buffer, str );
}


static void bufferNmb(          // CONCATENATE NUMBER TO BUFFER
    DUMP_INFO* di,              // - dump information
    unsigned numb )             // - to be concatenated
{
    char buf[16];               // - buffer

    sprintf( buf, "%d", numb );
    VbufConcStr( &di->buffer, buf );
    if( numb >= 10 ) {
        sprintf( buf, "0x%X", numb );
        VbufConcStr( &di->buffer, buf );
    }
}


static void bufferInit(         // INITIALIZE BUFFER (NON-TITLE LINE)
    DUMP_INFO* di )             // - dump information
{
    VbufRewind( &di->buffer );
    bufferStr( di, "    " );
}


static void dumpDirect( BASE_CLASS*, void * );
static void dumpVirtual( BASE_CLASS*, void * );


static void dumpTitle(          // DUMP A TITLE LINE
    DUMP_INFO *di,              // - dump information
    const char *title,          // - title line
    const char *class_name )    // - name of class
{
    VbufRewind( &di->buffer );
    bufferChr( di, '\n' );
    bufferStr( di, title );
    bufferChr( di, ' ' );
    bufferStr( di, class_name );
    MsgDisplayLineVbuf( &di->buffer );
}


static void dumpOffset(         // DUMP OFFSET LINE
    DUMP_INFO* di )             // - dump information
{
    bufferInit( di );
    bufferStr( di, "offset of class: " );
    bufferNmb( di, di->offset );
    MsgDisplayLineVbuf( &di->buffer );
}


static void dumpParentage(      // DUMP PARENTAGE
    DUMP_INFO* di )             // - dump information
{
    char **daughter;            // - daughter class

    VstkIterBeg( &di->stack, daughter ) {
        bufferInit( di );
        bufferStr( di, "base of: " );
        bufferStr( di, *daughter );
        MsgDisplayLineVbuf( &di->buffer );
    }
}


static void dumpBitMemb(        // DUMP A BITFIELD MEMBER
    DUMP_INFO *di,              // - dump information
    const char *kind,           // - kind of field
    const char *name,           // - field name
    target_offset_t offset,     // - field offset
    target_size_t start,        // - field start
    target_size_t size )        // - field size
{
    bufferInit( di );
    bufferStr( di, kind );
    bufferChr( di, ' ' );
    bufferStr( di, name );
    bufferStr( di, ", offset = " );
    bufferNmb( di, offset );
    bufferStr( di, ", bit offset =" );
    bufferNmb( di, start );
    bufferStr( di, ", bit width =" );
    bufferNmb( di, size );
    MsgDisplayLineVbuf( &di->buffer );
}


static void dumpDataMemb(       // DUMP A DATA MEMBER
    DUMP_INFO *di,              // - dump information
    const char *kind,           // - kind of field
    const char *name,           // - field name
    target_offset_t offset,     // - field offset
    target_size_t size )        // - field size
{
    bufferInit( di );
    bufferStr( di, kind );
    bufferChr( di, ' ' );
    bufferStr( di, name );
    bufferStr( di, ", offset = " );
    bufferNmb( di, offset );
    bufferStr( di, ", size = " );
    bufferNmb( di, size );
    MsgDisplayLineVbuf( &di->buffer );
}


static void dumpMember(         // DUMP A MEMBER
    SYMBOL memb,                // - member
    void *_di )                 // - dump information
{
    DUMP_INFO* di = _di;
    target_offset_t offset;     // - offset of symbol
    TYPE type;                  // - type of symbol
    NAME name;                  // - symbol's name

    offset = di->offset + memb->u.member_offset;
    name = memb->name->name;
    type = TypedefModifierRemove( memb->sym_type );
    if( type->id == TYP_BITFIELD ) {
        dumpBitMemb( di
                    , "bit member:"
                    , NameStr( name )
                    , offset
                    , type->u.b.field_start
                    , type->u.b.field_width );
    } else {
        dumpDataMemb( di
                    , "member:"
                    , NameStr( name )
                    , offset
                    , CgMemorySize( type ) );
    }
}

static void dumpSize( DUMP_INFO* di, bool embed, unsigned size )
{
    bufferInit( di );
    bufferStr( di, ( embed ) ? "embedded size: " : "size: " );
    bufferNmb( di, size );
    MsgDisplayLineVbuf( &di->buffer );
}

static void dumpStruct(         // DUMP A STRUCTURE
    TYPE type,                  // - structure type
    DUMP_INFO* di,              // - dump information
    char* title,                // - title for dump
    ds_control control )        // - control word
{
    CLASSINFO* info;            // - class information
    NAME *parent;               // - where parent ptr is stored

    /* unused parameters */ (void)control;

    type = ClassType( type );
    info = type->u.c.info;
    parent = VstkPush( &di->stack );
    *parent = info->name;
    dumpTitle( di, title, NameStr( info->name ) );
    if( type != di->original ) {
        dumpSize( di, true, info->vsize );
        dumpOffset( di );
        dumpParentage( di );
    } else {
        dumpSize( di, false, info->size );
    }
    if( info->has_vbptr ) {
        dumpDataMemb( di
                    , "[virtual"
                    , "base pointer]"
                    , info->vb_offset + di->offset
                    , CgMemorySize( TypePtrToVoid() ) );
    }
    if( info->has_vfptr ) {
        dumpDataMemb( di
                    , "[virtual"
                    , "functions pointer]"
                    , info->vf_offset + di->offset
                    , CgMemorySize( TypePtrToVoid() ) );
    }
    ScopeWalkDataMembers( type->u.c.scope, dumpMember, di );
    if( type == di->original ) {
        ScopeWalkVirtualBases( type->u.c.scope, dumpVirtual, di );
    }
    ScopeWalkDirectBases( type->u.c.scope, dumpDirect, di );
    VstkPop( &di->stack );
}


static void dumpBase(           // DUMP BASE
    BASE_CLASS* base,           // - base information
    DUMP_INFO* di,              // - dump information
    char* title )               // - title
{
    di->offset += base->delta;
    dumpStruct( base->type, di, title, DS_BASE );
    di->offset -= base->delta;
}


static void dumpVirtual(        // DUMP VIRTUAL BASE
    BASE_CLASS* vbase,          // - virtual base
    void * _di )             // - dump information
{
    DUMP_INFO* di = _di;
    dumpBase( vbase, di, "Virtual Base:" );
}


static void dumpDirect(         // DUMP DIRECT BASE
    BASE_CLASS* dbase,          // - direct base
    void * _di )             // - dump information
{
    DUMP_INFO* di = _di;
    dumpBase( dbase, di, "Direct Base:" );
}


void DumpObjectModelClass(      // DUMP OBJECT MODEL: CLASS
    TYPE type )                 // - structure type
{
    DUMP_INFO di;               // - dump information

    if( ! type->u.c.info->corrupted ) {
        CompFlags.log_note_msgs = true;
        di.original = type;
        di.offset = 0;
        VbufInit( &di.buffer );
        VstkOpen( &di.stack, sizeof( char* ), 16 );
        dumpStruct( type, &di, "Object Model for:", DS_NULL );
        VbufFree( &di.buffer );
        VstkClose( &di.stack );
        CompFlags.log_note_msgs = false;
    }
}


void DumpObjectModelEnum(       // DUMP OBJECT MODEL: ENUM
    TYPE type )                 // - enum type
{
    SYMBOL sym;                 // - current symbol
    TYPE base;                  // - base type
    VBUF buffer;                // - printing buffer
    char buf[16];               // - buffer
    int numb;                   // - a numeric value
    const char *name;           // - name to be printed
    bool sign;                  // - true ==> signed enum
    unsigned mask;              // - used to mask to true size
    unsigned val;               // - value as unsigned

    CompFlags.log_note_msgs = true;
    base = TypedefModifierRemoveOnly( type );
    sym = base->u.t.sym;
    VbufInit( &buffer );
    VbufConcStr( &buffer, "Object Model for: " );
    if( NULL == sym->name->name || NameStr( sym->name->name )[0] == NAME_INTERNAL_PREFIX1 ) {
        VbufConcStr( &buffer, "anonymous enum type" );
    } else {
        VbufConcStr( &buffer, NameStr( sym->name->name ) );
    }
    name = "???";
    sign = false;
    switch( TypedefModifierRemove( base->of ) -> id ) {
    case TYP_CHAR :
    case TYP_UCHAR :
        name = "unsigned char";
        break;
    case TYP_SCHAR :
        name = "signed char";
        sign = true;
        break;
    case TYP_SSHORT :
        name = "signed short";
        sign = true;
        break;
    case TYP_USHORT :
        name = "unsigned short";
        break;
    case TYP_SINT :
        name = "signed int";
        sign = true;
        break;
    case TYP_UINT :
        name = "unsigned int";
        break;
    case TYP_SLONG :
        name = "signed long";
        sign = true;
        break;
    case TYP_ULONG :
        name = "unsigned long";
        break;
    case TYP_SLONG64 :
        name = "__int64";
        sign = true;
        break;
    case TYP_ULONG64 :
        name = "unsigned __int64";
        break;
    DbgDefault( "DumpObjectModelEnum -- bad underlying type" );
    }
    VbufConcStr( &buffer, ", base type is " );
    VbufConcStr( &buffer, name );
    MsgDisplayLineVbuf( &buffer );
    mask = CgMemorySize( base );
    if( mask == sizeof( unsigned ) ) {
        mask = ~0U;
    } else {
        mask = ( 1 << ( mask * 8 ) ) - 1;
    }
    for( sym = sym->thread; SymIsEnumeration( sym ); sym = sym->thread ) {
        VbufRewind( &buffer );
        VbufConcStr( &buffer, "    " );
        VbufConcStr( &buffer, NameStr( sym->name->name ) );
        VbufConcStr( &buffer, " = " );
        numb = sym->u.sval;
        if( sign ) {
            sprintf( buf, "%d", numb );
        } else {
            sprintf( buf, "%u", numb );
        }
        VbufConcStr( &buffer, buf );
        val = mask & numb;
        if( val >= 10 ) {
            sprintf( buf, " /0x%X", val );
            VbufConcStr( &buffer, buf );
        }
        MsgDisplayLineVbuf( &buffer );
    }
    VbufFree( &buffer );
    CompFlags.log_note_msgs = false;
}
