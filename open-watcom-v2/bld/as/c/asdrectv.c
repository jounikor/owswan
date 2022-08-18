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
* Description:  Assembler directive processing.
*
****************************************************************************/


#include "as.h"

#define MAX_ALIGNMENT   6       // 64 byte alignment

dir_set_flags       AsDirSetOptions;

static bool         autoAlignment;
static bool         getDirOpLine = false; // decides if we'll grab the line
static directive_t  *lastDirective = NULL;


static bool dirHasOperand( directive_t *dir )
//*******************************************
{
    return( dir->num_operands > 0 );
}


static bool dirNumOperandsVerify( dir_opcount actual, dir_opcount wanted )
//************************************************************************
{
    if( actual < wanted ) {
        Error( DIROP_ERR_MISSING, actual );
        return( false );
    }
    if( actual > wanted ) {
        Error( DIROP_ERR_TOO_MANY );
        return( false );
    }
    return( true );
}


#if 0
static bool dirFuncNYI( directive_t *dir, dir_table_enum parm )
//*************************************************************
{
    printf( "Directive '%s' is not yet implemented.\n", dir->dir_sym->name );
    return( true );
}
#endif


static bool dirFuncAlign ( directive_t *dir, dir_table_enum parm )
//****************************************************************
{
    int_32      val;

    /* unused parameters */ (void)parm;

    if( !dirNumOperandsVerify( dir->num_operands, 1 ) ) {
        return( true );
    }
    assert( dir->operand_list->type == DIROP_INTEGER );
    val = NUMBER_INTEGER( dir->operand_list );
    if( val < 0 || val > MAX_ALIGNMENT ) {
        Error( OP_OUT_OF_RANGE, 0 );
        return( true );
    }
    autoAlignment = false;
    CurrAlignment = val;
    return( true );
}


#ifdef _STANDALONE_
static bool dirFuncSwitchSection( directive_t *, dir_table_enum );
static bool dirFuncStorageAlloc( directive_t *, dir_table_enum );

static bool dirFuncBSS( directive_t *dir, dir_table_enum parm )
//*************************************************************
{
    if( !dirHasOperand( dir ) )
        return( dirFuncSwitchSection( dir, parm ) );
    return( dirFuncStorageAlloc( dir, parm ) ); // ".bss tag, bytes"
}


static bool dirFuncErr( directive_t *dir, dir_table_enum parm )
//*************************************************************
{
    char    *str;

    /* unused parameters */ (void)parm;

    if( dirHasOperand( dir ) ) {
        assert( dir->num_operands == 1 );
        assert( dir->operand_list->type == DIROP_LINE );
        str = STRING_CONTENT( dir->operand_list );
        if( *str ) {
            Error( GET_STRING, str );
        }
    }
    return( false );    // so that yyparse will terminate
}
#endif


static bool dirFuncIgnore( directive_t *dir, dir_table_enum parm )
//****************************************************************
// Silently ignore this directive...
{
    /* unused parameters */ (void)dir; (void)parm;

    return( true );
}


static bool dirFuncNop( directive_t *dir, dir_table_enum parm )
//*************************************************************
{
    uint_32     opcode = INS_NOP;

    /* unused parameters */ (void)dir;

    if( parm == DT_NOP_NOP ) {
#ifdef _STANDALONE_
        ObjEmitData( CurrentSection, (char *)&opcode, sizeof( opcode ), true );
#else
        ObjEmitData( (char *)&opcode, sizeof( opcode ), true );
#endif
        return( true );
    }
#ifdef AS_ALPHA
    assert( parm == DT_NOP_FNOP );
    opcode = INS_FNOP;
  #ifdef _STANDALONE_
    ObjEmitData( CurrentSection, (char *)&opcode, sizeof( opcode ), true );
  #else
    ObjEmitData( (char *)&opcode, sizeof( opcode ), true );
  #endif
    return( true );
#else
    assert( false );
    return( true );
#endif
}


#ifdef _STANDALONE_
static bool dirFuncSetLinkage( directive_t *dir, dir_table_enum parm )
//********************************************************************
{
    sym_handle  sym;

    if( !dirNumOperandsVerify( dir->num_operands, 1 ) ) {
        return( true );
    }
    sym = SYMBOL_HANDLE( dir->operand_list );
    assert( sym != NULL );
    assert( SymClass( sym ) == SYM_LABEL );
    switch( parm ) {
    case DT_LNK_GLOBAL:
        if( SymGetLinkage( sym ) == SL_STATIC ) {
            // Too bad the label is already emitted as static :(
            Warning( GLOBL_DECL_OUT_OF_ORDER );
        } else {
            SymSetLinkage( sym, SL_GLOBAL );
        }
        break;
    default:
        assert( false );
    }
    return( true );
}
#endif


static bool optionString( const char *str, const char *option )
//*************************************************************
{
    const char  *s;
    size_t      n;
    char        c;

    if( strncmp( str, option, n = strlen( option ) ) != 0 ) {
        return( false );
    }
    s = str + n;
    while( (c = *s) != '\0' ) {
        // ignore trailing blanks only
        if( c != '\t' && c != ' ' ) return( false );
        ++s;
    }
    return( true );
}


static bool dirFuncSetOption( directive_t *dir, dir_table_enum parm )
//*******************************************************************
{
    char    *str;

    /* unused parameters */ (void)parm;

    if( dirHasOperand( dir ) ) {
        assert( dir->num_operands == 1 );
        assert( dir->operand_list->type == DIROP_LINE );
        str = STRING_CONTENT( dir->operand_list );
        if( optionString( str, "at" ) ) {
            _DirSet( AT );
        } else if( optionString( str, "noat" ) ) {
            _DirUnSet( AT );
        } else if( optionString( str, "macro" ) ) {
            _DirSet( MACRO );
        } else if( optionString( str, "nomacro" ) ) {
            _DirUnSet( MACRO );
        } else if( optionString( str, "reorder" ) ) {
            _DirSet( REORDER );
        } else if( optionString( str, "noreorder" ) ) {
            _DirUnSet( REORDER );
        } else if( optionString( str, "volatile" ) ) {
            // ignore this for now
            // _DirSet( VOLATILE );
        } else if( optionString( str, "novolatile" ) ) {
            // ignore this for now
            // _DirUnSet( VOLATILE );
        } else if( optionString( str, "move" ) ) {
            // ignore this for now
            // _DirSet( MOVE );
        } else if( optionString( str, "nomove" ) ) {
            // ignore this for now
            // _DirUnSet( MOVE );
        } else {
            Error( IMPROPER_SET_DIRECTIVE, str );
        }
    }
    return( true );
}


static bool dirFuncSpace( directive_t *dir, dir_table_enum parm )
//***************************************************************
{
    dir_operand                 *dirop;
    int_32                      count;

    /* unused parameters */ (void)parm;

    if( !dirNumOperandsVerify( dir->num_operands, 1 ) ) {
        return( true );
    }
    dirop = dir->operand_list;
    assert( dirop->type == DIROP_INTEGER );
    count = NUMBER_INTEGER( dirop );
    if( count < 0 ) {
        Error( OP_OUT_OF_RANGE, 0 );
        return( true );
    }
#ifdef _STANDALONE_
    ObjNullPad( CurrentSection, (uint_8)count );
#else
    ObjNullPad( (uint_8)count );
#endif
    return( true );
}


#ifdef _STANDALONE_
static bool dirFuncStorageAlloc( directive_t *dir, dir_table_enum parm )
//**********************************************************************
// e.g.) .comm name, expr
// Emit a label name, then emit expr bytes of data
{
    dir_operand                 *dirop;
    sym_handle                  sym;
    int_32                      expr;
    char                        *buffer;
    owl_section_handle          curr_section;
    reserved_section            as_section = 0;

    if( !dirNumOperandsVerify( dir->num_operands, 2 ) ) {
        return( true );
    }
    dirop = dir->operand_list;
    if( dirop->type != DIROP_SYMBOL ) {
        Error( IMPROPER_DIROP, 0 );
        return( true );
    }
    sym = SYMBOL_HANDLE( dirop );
    dirop = dirop->next;
    if( dirop->type != DIROP_INTEGER ) {
        Error( IMPROPER_DIROP, 1 );
        return( true );
    }
    expr = NUMBER_INTEGER( dirop );
    if( expr <= 0 ) {
        Error( OP_OUT_OF_RANGE, 1 );
        return( true );
    }
    if( ObjLabelDefined( sym ) ) {
        // then such label has already been emitted
        Error( SYM_ALREADY_DEFINED, SymName( sym ) );
        return( true );
    }
    curr_section = CurrentSection;
    switch( parm ) {
    case DT_SEC_DATA:   // .comm => globl
        as_section = AS_SECTION_DATA;
        SymSetLinkage( sym, SL_GLOBAL );
        break;
    case DT_SEC_BSS:    // .lcomm, .bss
        as_section = AS_SECTION_BSS;
        break;
    default:
        assert( false );
    }
    ObjSwitchSection( as_section );     // Have to switch to the right
    ObjEmitLabel( sym );                // section before emitting label.
    buffer = MemAlloc( expr );
    memset( buffer, 0, expr );
    ObjEmitData( CurrentSection, buffer, expr, true ); // Align the data also.
    MemFree( buffer );
    CurrentSection = curr_section;      // Switch back to where you were.
    return( true );
}
#endif

#define ESCAPE_CHAR     '\\'
#define ESCAPE_A        0x07
#define ESCAPE_B        0x08
#define ESCAPE_F        0x0C
#define ESCAPE_N        0x0A
#define ESCAPE_R        0x0D
#define ESCAPE_T        0x09
#define ESCAPE_V        0x0B

static char *getESCChar( char * const byte, char *ptr )
//*****************************************************
// Interpret the escape sequence, store the value in *byte,
// returns the pointer to the last character in the sequence.
{
    unsigned long   num = 0;
    uint_8          ctr = 0;
    char            *buffer, *endptr;

    assert( *ptr == ESCAPE_CHAR );
    ptr++;
    while( *ptr >= '0' && *ptr <= '7' && ctr < 3) {
        ctr++;
        num = ( num << 3 ) + ( *(ptr++) - '0' );
    }
    if( ctr ) {     // we formed an octal number
        if( num > 0xFF ) Warning( CONST_OUT_OF_RANGE );
        *byte = (char)num;
        return( ptr - 1 );
    }
    if( *ptr == 'x' ) {     // Hex
        ptr++;
        buffer = MemAlloc( strlen( ptr ) + 2 + 1 );
        strcpy( buffer, "0x" );
        strcat( buffer, ptr );
        num = strtoul( buffer, &endptr, 16 );
        if( buffer == endptr ) {    // no hex found
            *byte = *(--ptr);       // *byte gets 'x'
            MemFree( buffer );
            return( ptr );
        }
        ptr += ( endptr - buffer - 3 );
        if( num > 0xFF ) Warning( CONST_OUT_OF_RANGE );
        *byte = (char)num;
        MemFree( buffer );
        return( ptr );
    }
    switch( *ptr ) {
    case 'a':
        *byte = ESCAPE_A; break;
    case 'b':
        *byte = ESCAPE_B; break;
    case 'f':
        *byte = ESCAPE_F; break;
    case 'n':
        *byte = ESCAPE_N; break;
    case 'r':
        *byte = ESCAPE_R; break;
    case 't':
        *byte = ESCAPE_T; break;
    case 'v':
        *byte = ESCAPE_V; break;
    default:
        *byte = *ptr; break;
    }
    return( ptr );
}


static bool dirFuncString( directive_t *dir, dir_table_enum parm )
//****************************************************************
{
    char        *str, *ptr, *byte;
    dir_operand *dirop;
    int         opnum;

    assert( dir->operand_list != NULL );
    opnum = 0;
    for( dirop = dir->operand_list; dirop != NULL; dirop = dirop->next ) {
        assert( dirop->type == DIROP_STRING );
        str = byte = MemAlloc( strlen( STRING_CONTENT( dirop ) ) + 1 );
        for( ptr = STRING_CONTENT( dirop ); *ptr != '\0'; ptr++ ) {
            if( *ptr == ESCAPE_CHAR ) {
                ptr = getESCChar( byte, ptr );
            } else {
                *byte = *ptr;
            }
            byte++;
        }
        if( parm == DT_STR_NULL ) {
            *byte++ = '\0';
        }
#ifdef _STANDALONE_
        ObjEmitData( CurrentSection, str, byte - str, ( opnum == 0 ) );
#else
        ObjEmitData( str, byte - str, ( opnum == 0 ) );
#endif
        MemFree( str );
        opnum++;
    }
    return( true );
}


#ifdef _STANDALONE_
static bool dirFuncSwitchSection( directive_t *dir, dir_table_enum parm )
//***********************************************************************
{
    reserved_section    as_section = 0;

    /* unused parameters */ (void)dir;

    switch( parm ) {
    case DT_SEC_TEXT:
        /* The ones that need autoAlignment are here. */
        autoAlignment = true;
        as_section = AS_SECTION_TEXT;
        break;
    case DT_SEC_DATA:
        /* The ones that need autoAlignment are here. */
        autoAlignment = true;
        as_section = AS_SECTION_DATA;
        break;
    case DT_SEC_BSS:
        as_section = AS_SECTION_BSS;
        break;
    case DT_SEC_PDATA:
        as_section = AS_SECTION_PDATA;
        break;
    case DT_SEC_DEBUG_P:
        as_section = AS_SECTION_DEBUG_P;
        break;
    case DT_SEC_DEBUG_S:
        as_section = AS_SECTION_DEBUG_S;
        break;
    case DT_SEC_DEBUG_T:
        as_section = AS_SECTION_DEBUG_T;
        break;
    case DT_SEC_RDATA:
        as_section = AS_SECTION_RDATA;
        break;
    case DT_SEC_XDATA:
        as_section = AS_SECTION_XDATA;
        break;
    case DT_SEC_YDATA:
        as_section = AS_SECTION_YDATA;
        break;
#ifdef AS_PPC
    case DT_SEC_RELDATA:
        as_section = AS_SECTION_RELDATA;
        break;
    case DT_SEC_TOCD:
        as_section = AS_SECTION_TOCD;
        break;
#endif
    default:
        Error( INTERNAL_UNKNOWN_SECTYPE );
        return( false );
    }
    ObjSwitchSection( as_section );
    return( true );
}
#endif


#ifdef AS_PPC
static bool dirFuncUnsupported( directive_t *dir, dir_table_enum parm )
//*********************************************************************
{
    /* unused parameters */ (void)parm;

    Error( DIRECTIVE_NOT_SUPPORTED, SymName( dir->dir_sym ) );
    return( true );
}
#endif


#ifdef _STANDALONE_
static bool dirFuncUserSection( directive_t *dir, dir_table_enum parm )
//*********************************************************************
{
    dir_operand                 *dirop;
    sym_handle                  sym;
    char                        *str, *s;
    owl_section_type            type = 0;
    owl_section_type            *ptype;
    owl_alignment               align = 0;

    if( dir->num_operands > 2 ) {
        Error( DIROP_ERR_TOO_MANY );
        return( true );
    }
    dirop = dir->operand_list;
    if( dirop->type != DIROP_SYMBOL ) {
        Error( IMPROPER_DIROP, 0 );
        return( true );
    }
    sym = SYMBOL_HANDLE( dirop );
    if( dir->num_operands == 2 ) {
        dirop = dirop->next;
        if( dirop->type != DIROP_STRING ) {
            Error( IMPROPER_DIROP, 1 );
            return( true );
        }
        str = STRING_CONTENT( dirop );
    } else {
        str = NULL;
    }
    if( str ) {
        s = str;
        while( *s ) {
            switch( *s ) {
            case 'c':
                type |= OWL_SEC_ATTR_CODE;
                break;
            case 'd':
                type |= OWL_SEC_ATTR_DATA;
                break;
            case 'u':
                type |= OWL_SEC_ATTR_BSS;
                break;
            case 'i':
                type |= OWL_SEC_ATTR_INFO;
                break;
            case 'n':
                type |= OWL_SEC_ATTR_DISCARDABLE;
                break;
            case 'R':
                type |= OWL_SEC_ATTR_REMOVE;
                break;
            case 'r':
                type |= OWL_SEC_ATTR_PERM_READ;
                break;
            case 'w':
                type |= OWL_SEC_ATTR_PERM_WRITE;
                break;
            case 'x':
                type |= OWL_SEC_ATTR_PERM_EXEC;
                break;
            case 's':
                type |= OWL_SEC_ATTR_PERM_SHARE;
                break;
            case '0': case '1': case '2': case '3': case '4': case '5': case '6':
                align = 1 << (*s - '0');
                break;
            default:
                Error( INVALID_SECT_ATTR, *s );
                break;
            }
            s++;
        }
        ptype = &type;
    } else {
        // default is just read & write
        ptype = NULL;
    }
    if( parm == DT_USERSEC_NEW ) {
        SectionNew( SymName( sym ), ptype, align );
    } else {
        SectionSwitch( SymName( sym ), ptype, align );
    }
    return( true );
}
#endif


static bool assignRelocType( owl_reloc_type *owlrtype, asm_reloc_type artype, dir_table_enum parm )
//*************************************************************************************************
{
    owl_reloc_type reloc_translate[] = {
        OWL_RELOC_ABSOLUTE,         // ASM_RELOC_UNSPECIFIED
        OWL_RELOC_WORD,
        OWL_RELOC_HALF_HI,
        OWL_RELOC_HALF_HA,
        OWL_RELOC_HALF_LO,
    };

    assert( parm == DT_VAL_INT16 || parm == DT_VAL_INT32 );
    if( parm == DT_VAL_INT16 ) {
        reloc_translate[ ASM_RELOC_UNSPECIFIED ] = OWL_RELOC_HALF_LO; //default
        switch( artype ) {
        case ASM_RELOC_HALF_HI:
        case ASM_RELOC_HALF_HA:
        case ASM_RELOC_HALF_LO:
        case ASM_RELOC_UNSPECIFIED:
            *owlrtype = reloc_translate[ artype ];
            break;
        default:
            return( false );
        }
    } else {    // DT_VAL_INT32
        reloc_translate[ ASM_RELOC_UNSPECIFIED ] = OWL_RELOC_WORD; //default
        switch( artype ) {
        case ASM_RELOC_WORD:
        case ASM_RELOC_UNSPECIFIED:
            *owlrtype = reloc_translate[ artype ];
            break;
        default:
            return( false );
        }
    }
    return( true );
}


static bool dirFuncValues( directive_t *dir, dir_table_enum parm )
//****************************************************************
{
    dir_operand         *dirop;
    static int_8        byte;
    static int_16       half;
    static int_32       word;
    static signed_64    quad;
    static float        flt;
    static double       dbl;
    int_32              rep;
    uint_8              prev_alignment = 0;
    int                 opnum;
    void                *target = NULL;
    owl_reloc_type      rtype = 0;
    struct { int size; void *ptr; uint_8 alignment; } data_table[] =
    {
        { 1, &byte,     0 },    // DT_VAL_INT8
        { 8, &dbl,      3 },    // DT_VAL_DOUBLE
        { 4, &flt,      2 },    // DT_VAL_FLOAT
        { 2, &half,     1 },    // DT_VAL_INT16
        { 4, &word,     2 },    // DT_VAL_INT32
        { 8, &quad,     3 },    // DT_VAL_INT64
    };
#define TABLE_IDX( x )      ( ( x ) - DT_VAL_FIRST )

#ifdef _STANDALONE_
    if( OWLTellSectionType( CurrentSection ) & OWL_SEC_ATTR_BSS ) {
        Error( INVALID_BSS_DIRECTIVE, SymName( dir->dir_sym ) );
        return( true );
    }
#endif
    if( autoAlignment ) {
        prev_alignment = CurrAlignment;
        CurrAlignment = data_table[TABLE_IDX( parm )].alignment;
    }

    opnum = 0;
    for( dirop = dir->operand_list; dirop != NULL; dirop = dirop->next ) {
        rep = 1;
        switch( parm ) {
        case DT_VAL_INT8:
            assert( dirop->type == DIROP_INTEGER || dirop->type == DIROP_REP_INT );
            if( dirop->type == DIROP_INTEGER ) {
                byte = (int_8)NUMBER_INTEGER( dirop );
            } else { // repeat
                rep = REPEAT_COUNT( dirop );
                byte = (int_8)REPEAT_INTEGER( dirop );
            }
            break;
        case DT_VAL_INT64:
            assert( dirop->type == DIROP_INTEGER || dirop->type == DIROP_REP_INT );
            if( dirop->type == DIROP_INTEGER ) {
                quad.u._32[I64LO32] = NUMBER_INTEGER( dirop );
            } else { // repeat
                rep = REPEAT_COUNT( dirop );
                quad.u._32[I64LO32] = REPEAT_INTEGER( dirop );
            }
            break;
        case DT_VAL_DOUBLE:
            assert( dirop->type == DIROP_FLOATING || dirop->type == DIROP_REP_FLT );
            if( dirop->type == DIROP_FLOATING ) {
                dbl = NUMBER_FLOAT( dirop );
            } else {
                rep = REPEAT_COUNT( dirop );
                dbl = REPEAT_FLOAT( dirop );
            }
            break;
        case DT_VAL_FLOAT:
            assert( dirop->type == DIROP_FLOATING || dirop->type == DIROP_REP_FLT );
            if( dirop->type == DIROP_FLOATING ) {
                flt = (float)NUMBER_FLOAT( dirop );
            } else {
                rep = REPEAT_COUNT( dirop );
                flt = (float)REPEAT_FLOAT( dirop );
            }
            break;
        case DT_VAL_INT32:
            assert( dirop->type == DIROP_INTEGER || dirop->type == DIROP_REP_INT ||
                    dirop->type == DIROP_SYMBOL || dirop->type == DIROP_NUMLABEL_REF );
            if( dirop->type == DIROP_INTEGER ) {
                word = NUMBER_INTEGER( dirop );
            } else if( dirop->type == DIROP_REP_INT ) {
                rep = REPEAT_COUNT( dirop );
                word = REPEAT_INTEGER( dirop );
            } else {    // reloc
                word = SYMBOL_OFFSET( dirop );
                if( assignRelocType( &rtype, SYMBOL_RELOC_TYPE( dirop ), parm ) ) {
                    if( dirop->type == DIROP_SYMBOL ) {
                        target = SymName( SYMBOL_HANDLE( dirop ) );
                    } else {
                        assert( dirop->type == DIROP_NUMLABEL_REF );
                        target = &SYMBOL_LABEL_REF( dirop );
                    }
                } else {
                    Error( IMPROPER_DIROP, opnum );
                }
            }
            break;
        case DT_VAL_INT16:
            assert( dirop->type == DIROP_INTEGER || dirop->type == DIROP_REP_INT ||
                    dirop->type == DIROP_SYMBOL || dirop->type == DIROP_NUMLABEL_REF );
            if( dirop->type == DIROP_INTEGER ) {
                half = (int_16)NUMBER_INTEGER( dirop );
            } else if( dirop->type == DIROP_REP_INT ) {
                rep = REPEAT_COUNT( dirop );
                half = (int_16)REPEAT_INTEGER( dirop );
            } else {    // reloc
                half = (int_16)SYMBOL_OFFSET( dirop );
                if( assignRelocType( &rtype, SYMBOL_RELOC_TYPE( dirop ), parm ) ) {
                    if( dirop->type == DIROP_SYMBOL ) {
                        target = SymName( SYMBOL_HANDLE( dirop ) );
                    } else {
                        assert( dirop->type == DIROP_NUMLABEL_REF );
                        target = &SYMBOL_LABEL_REF( dirop );
                    }
                } else {
                    Error( IMPROPER_DIROP, opnum );
                }
            }
            break;
        default:
            Error( INTERNAL_UNKNOWN_DT_PARM, parm );
            return( false );
        }
        if( target != NULL ) {
            assert( (parm == DT_VAL_INT32 || parm == DT_VAL_INT16) && rep == 1 );
#ifdef _STANDALONE_
            // align with data
            ObjEmitReloc( CurrentSection, target, rtype, ( opnum == 0 ), (dirop->type == DIROP_SYMBOL) );
#else
            // align with data
            ObjEmitReloc( target, rtype, ( opnum == 0 ), (dirop->type == DIROP_SYMBOL ) );
#endif
            target = NULL;
        }
#ifdef _STANDALONE_
        // only align for the first data operand
        ObjEmitData( CurrentSection, data_table[TABLE_IDX( parm )].ptr, data_table[TABLE_IDX( parm )].size, ( opnum == 0 ) );
#else
        // only align for the first data operand
        ObjEmitData( data_table[TABLE_IDX( parm )].ptr, data_table[TABLE_IDX( parm )].size, ( opnum == 0 ) );
#endif
        for( rep--; rep > 0; rep-- ) {
#ifdef _STANDALONE_
            OWLEmitData( CurrentSection, data_table[TABLE_IDX( parm )].ptr, data_table[TABLE_IDX( parm )].size );
#else
            ObjDirectEmitData( data_table[TABLE_IDX( parm )].ptr, data_table[TABLE_IDX( parm )].size );
#endif
#if 0
            printf( "Size=%d\n", data_table[TABLE_IDX( parm )].size );
            switch( parm ) {
            case DT_VAL_INT8:
                printf( "Out->%d\n", *(int_8 *)(data_table[TABLE_IDX( parm )].ptr) );
                break;
            case DT_VAL_DOUBLE:
                printf( "Out->%lf\n", *(double *)(data_table[TABLE_IDX( parm )].ptr) );
                break;
            case DT_VAL_FLOAT:
                printf( "Out->%f\n", *(float *)(data_table[TABLE_IDX( parm )].ptr) );
                break;
            case DT_VAL_INT16:
                printf( "Out->%d\n", *(int_16 *)(data_table[TABLE_IDX( parm )].ptr) );
                break;
            case DT_VAL_INT32:
                printf( "Out->%d\n", *(int_32 *)(data_table[TABLE_IDX( parm )].ptr) );
                break;
            case DT_VAL_INT64:
                printf( "Out->%d\n", ((signed_64 *)(data_table[TABLE_IDX( parm )].ptr))->_32[0] );
                break;
            default:
                assert( false );
            }
#endif
        }
        opnum++;
    }
    if( autoAlignment ) {
        CurrAlignment = prev_alignment;
    }
    return( true );
}


#define DOF_NUM     ( DOF_INT | DOF_FLT )
#define DOF_REF     ( DOF_SYM | DOF_NUMREF )
#define DOF_REP     ( DOF_REP_INT | DOF_REP_FLT )

static dir_table asm_directives[] = {
//  { name,         func,                   parm,           flags }
    { ".address",   dirFuncValues,          DT_VAL_INT32,   DOF_INT | DOF_REF },
    { ".align",     dirFuncAlign,           DT_NOPARM,      DOF_INT },
    { ".ascii",     dirFuncString,          DT_STR_NONULL,  DOF_STR },
    { ".asciz",     dirFuncString,          DT_STR_NULL,    DOF_STR },
    { ".asciiz",    dirFuncString,          DT_STR_NULL,    DOF_STR },
#ifdef _STANDALONE_
    { ".bss",       dirFuncBSS,             DT_SEC_BSS,     DOF_INT | DOF_REF | DOF_NONE },
#endif
    { ".byte",      dirFuncValues,          DT_VAL_INT8,    DOF_INT | DOF_REP_INT },
#ifdef _STANDALONE_
    { ".comm",      dirFuncStorageAlloc,    DT_SEC_DATA,    DOF_INT | DOF_REF },
    { ".data",      dirFuncSwitchSection,   DT_SEC_DATA,    DOF_NONE },
    { ".debug$P",   dirFuncSwitchSection,   DT_SEC_DEBUG_P, DOF_NONE },
    { ".debug$S",   dirFuncSwitchSection,   DT_SEC_DEBUG_S, DOF_NONE },
    { ".debug$T",   dirFuncSwitchSection,   DT_SEC_DEBUG_T, DOF_NONE },
#endif
    { ".double",    dirFuncValues,          DT_VAL_DOUBLE,  DOF_FLT | DOF_REP_FLT },
#ifdef _STANDALONE_
    { ".err",       dirFuncErr,             DT_NOPARM,      DOF_LINE },
#endif
    { ".even",      dirFuncIgnore,          DT_NOPARM,      DOF_NONE },
#ifdef _STANDALONE_
    { ".extern",    dirFuncSetLinkage,      DT_LNK_GLOBAL,  DOF_REF },
#endif
    { ".float",     dirFuncValues,          DT_VAL_FLOAT,   DOF_FLT | DOF_REP_FLT },
#ifdef _STANDALONE_
    { ".globl",     dirFuncSetLinkage,      DT_LNK_GLOBAL,  DOF_REF },
#endif
    { ".half",      dirFuncValues,          DT_VAL_INT16,   DOF_INT | DOF_REP_INT | DOF_REF },
#ifdef _STANDALONE_
    { ".ident",     dirFuncIgnore,          DT_NOPARM,      DOF_LINE },
    { ".lcomm",     dirFuncStorageAlloc,    DT_SEC_BSS,     DOF_INT | DOF_REF },
#endif
    { ".long",      dirFuncValues,          DT_VAL_INT32,   DOF_INT | DOF_REP_INT | DOF_REF },
#ifdef _STANDALONE_
    { ".new_section", dirFuncUserSection,   DT_USERSEC_NEW, DOF_REF | DOF_STR },
#endif
    { "nop",        dirFuncNop,             DT_NOP_NOP,     DOF_NONE },
#ifdef _STANDALONE_
    { ".pdata",     dirFuncSwitchSection,   DT_SEC_PDATA,   DOF_NONE },
    { ".rdata",     dirFuncSwitchSection,   DT_SEC_RDATA,   DOF_NONE },
    { ".xdata",     dirFuncSwitchSection,   DT_SEC_XDATA,   DOF_NONE },
    { ".ydata",     dirFuncSwitchSection,   DT_SEC_YDATA,   DOF_NONE },
#ifdef AS_PPC
    { ".reldata",   dirFuncSwitchSection,   DT_SEC_RELDATA, DOF_NONE },
    { ".tocd",      dirFuncSwitchSection,   DT_SEC_TOCD,    DOF_NONE },
#endif
    { ".section",   dirFuncUserSection,     DT_NOPARM,      DOF_REF | DOF_STR },
#endif
    { ".set",       dirFuncSetOption,       DT_NOPARM,      DOF_LINE },
    { ".short",     dirFuncValues,          DT_VAL_INT16,   DOF_INT | DOF_REP_INT | DOF_REF },
    { ".space",     dirFuncSpace,           DT_NOPARM,      DOF_INT },
    { ".string",    dirFuncString,          DT_STR_NULL,    DOF_STR },
#ifdef _STANDALONE_
    { ".text",      dirFuncSwitchSection,   DT_SEC_TEXT,    DOF_NONE },
#endif
    { ".value",     dirFuncValues,          DT_VAL_INT16,   DOF_INT | DOF_REP_INT | DOF_REF },
#ifdef _STANDALONE_
    { ".version",   dirFuncIgnore,          DT_NOPARM,      DOF_LINE },
#endif
#if defined( AS_ALPHA )
    { "unop",       dirFuncNop,             DT_NOP_NOP,     DOF_NONE },
    { "fnop",       dirFuncNop,             DT_NOP_FNOP,    DOF_NONE },
// The .quad directive is disabled because 64-bit integers are not parsed
// properly
//    { ".quad",      dirFuncValues,          DT_VAL_INT64,   INT | RINT },
    { ".s_floating",dirFuncValues,          DT_VAL_FLOAT,   DOF_FLT | DOF_REP_FLT },
    { ".t_floating",dirFuncValues,          DT_VAL_DOUBLE,  DOF_FLT | DOF_REP_FLT },
    { ".word",      dirFuncValues,          DT_VAL_INT16,   DOF_INT | DOF_REP_INT | DOF_REF },
#elif defined( AS_PPC )
    { ".word",      dirFuncValues,          DT_VAL_INT32,   DOF_INT | DOF_REP_INT | DOF_REF },
    { ".little_endian", dirFuncIgnore,      DT_NOPARM,      DOF_LINE },
    { ".big_endian",    dirFuncUnsupported, DT_NOPARM,      DOF_LINE },
#elif defined( AS_MIPS )
    { ".word",      dirFuncValues,          DT_VAL_INT32,   DOF_INT | DOF_REP_INT | DOF_REF },
#endif
};


static bool dirValidate( directive_t *dir, dir_table *table_entry )
//*****************************************************************
// Check if the operands types are as expected
{
    static const dirop_flags flags[] = {    // corresponds to dirop_type
        DOF_INT,
        DOF_FLT,
        DOF_SYM,
        DOF_NUMREF,
        DOF_LINE,
        DOF_STR,
        DOF_REP_INT,
        DOF_REP_FLT,
        DOF_ERROR,
    };
    int                 opnum;
    dir_operand         *dirop;

    if( dir->operand_list == NULL && (table_entry->flags & DOF_NONE) != DOF_NONE ) {
        Error( DIROP_ERR_MISSING, 0 );
        return( false );
    }
    opnum = 0;
    for( dirop = dir->operand_list; dirop != NULL; dirop = dirop->next ) {
        dirop_flags     flag;

        flag = flags[dirop->type];
        if( (table_entry->flags & flag) != flag ) {
            Error( IMPROPER_DIROP, opnum );
            return( false );
        }
        opnum++;
    }
    return( true );
}


static directive_t *dirAlloc( void )
//**********************************
{
    return( MemAlloc( sizeof( directive_t ) ) );
}


static void dirFree( directive_t *directive )
//*******************************************
{
    MemFree( directive );
}


void DirInit( void )
//******************
{
    dir_table   *curr;
    sym_handle  sym;
    int         i, n;

    n = ArraySize( asm_directives );
    for( i = 0; i < n; i++ ) {
        curr = &asm_directives[i];
        sym = SymAdd( curr->name, SYM_DIRECTIVE );
        SymSetLink( sym, curr );
    }
    autoAlignment = true;
    AsDirSetOptions = NONE;
    _DirSet( AT );
    _DirSet( MACRO );
    _DirSet( REORDER );
}


void DirSetNextScanState( sym_handle sym )
//****************************************
// Call this to set up what to scan for the next token.
// Necessary because some directives take the whole line as a token.
{
    dir_table   *table_entry;

    table_entry = SymGetLink( sym );
    if ( table_entry->flags & DOF_LINE ) {
        getDirOpLine = true;
    }
}


bool DirGetNextScanState( void )
//******************************
// Call this to check what to scan for the next token.
// Returns true if we want the whole line as a token next.
// Necessary because some directives take the whole line as a token.
{
    if( getDirOpLine ) {
        getDirOpLine = false;
        return( true );
    }
    return( getDirOpLine );
}


directive_t *DirCreate( sym_handle sym )
//**************************************
{
    lastDirective = dirAlloc();
    lastDirective->dir_sym = sym;
    lastDirective->num_operands = 0;
    lastDirective->operand_list = NULL;
    lastDirective->operand_tail = NULL;
    return( lastDirective );
}


void DirAddOperand( directive_t *dir, dir_operand *dirop )
//********************************************************
{
    if( dirop == NULL )
        return;
    dir->num_operands++;
    if( dir->operand_tail ) {
        dir->operand_tail->next = dirop;
        dir->operand_tail = dirop;
    } else {
        dir->operand_list = dir->operand_tail = dirop;
    }
}


void DirDestroy( directive_t *directive )
//***************************************
{
    dir_operand *dirop, *dirop_next;

    if( !directive )
        return;

    for( dirop = directive->operand_list; dirop != NULL; dirop = dirop_next ) {
        dirop_next = dirop->next;
        DirOpDestroy( dirop );
    }
    dirFree( directive );
    lastDirective = NULL;
}


bool DirParse( directive_t *directive )
//*************************************
{
    dir_table   *table_entry;

    table_entry = SymGetLink( directive->dir_sym );
    _DBGMSG2( "Got directive '%s'\n", table_entry->name );
    if ( dirValidate( directive, table_entry ) ) {
        // Return false only if you want to abort yyparse
        return( table_entry->func( directive, table_entry->parm ) );
    }
    return( true );
}


void DirFini( void )
//******************
{
    DirDestroy( lastDirective );
}
