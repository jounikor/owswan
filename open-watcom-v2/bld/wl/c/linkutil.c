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
* Description:  Utility routines for wlink.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <sys/types.h>
#include "linkstd.h"
#include "walloca.h"
#include "pcobj.h"
#include "newmem.h"
#include "alloc.h"
#include "msg.h"
#include "wlnkmsg.h"
#include "fileio.h"
#include "ideentry.h"
#include "ring.h"
#include "overlays.h"
#include "strtab.h"
#include "loadfile.h"
#include "permdata.h"
#include "objio.h"
#include "mapio.h"
#include "pathlist.h"

#include "clibext.h"


typedef void ring_walk_fn( void *);

typedef struct {
    mods_walk_fn   *cbfn;
} mods_walk_data;

typedef struct {
    class_walk_fn   *cbfn;
} class_walk_data;

/* Default File Extension array, see ldefext.h */

static const char       *DefExt[] = {
    #define pick(enum,text) text,
    #include "ldefext.h"
    #undef pick
};

static unsigned char    DefExtLen[] = {
    #define pick(enum,text) sizeof( text ) - 1,
    #include "ldefext.h"
    #undef pick
};

void WriteNulls( f_handle file, size_t len, const char *name )
/************************************************************/
/* copy nulls for uninitialized data */
{
    static unsigned NullArray[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    for( ; len > sizeof( NullArray ); len -= sizeof( NullArray ) ) {
        QWrite( file, NullArray, sizeof( NullArray ), name );
    }
    if( len > 0 ) {
        QWrite( file, NullArray, len, name );
    }
}

void CheckErr( void )
/**************************/
{
    if( LinkState & ( LS_LINK_ERROR | LS_STOP_WORKING ) ) {
        WriteLibsUsed();
        Suicide();
    }
}

void CheckStop( void )
/***************************/
{
    if( LinkState & LS_STOP_WORKING ) {
        Suicide();
    }
}

void LnkFatal( const char *msg )
/******************************/
{
    LnkMsg( FTL+MSG_INTERNAL, "s", msg );
}

bool TestBit( byte *array, unsigned num )
/***********************************************/
/* return true if the specified bit is on */
{
    byte        mask;

    mask = 1 << (num % 8);
    num /= 8;
    return( (*( array + num ) & mask) != 0 );
}

void ClearBit( byte *array, unsigned num )
/************************************************/
/* make sure a bit is turned off */
{
    byte        mask;

    mask = 1 << (num % 8);
    num /= 8;
    array += num;
    *array &= ~mask;
}

char *ChkStrDup( const char *str )
/********************************/
{
    size_t      len;
    char        *copy;

    len = strlen( str ) + 1;
    _ChkAlloc( copy, len );
    memcpy( copy, str, len );
    return( copy );
}

char *ChkToString( const void *mem, size_t len )
/**********************************************/
{
    char        *str;

    _ChkAlloc( str, len + 1 );
    memcpy( str, mem, len );
    str[len] = '\0';
    return( str );
}

static void WalkModsList( mod_entry *list, mods_walk_fn *cbfn )
/*************************************************************/
{
    for( ; list != NULL; list = list->n.next_mod ) {
        cbfn( list );
    }
}

static void WalkSectModsList( section *sect, void *mods_walk_cb )
/***************************************************************/
{
    CurrSect = sect;
    WalkModsList( sect->mods, ((mods_walk_data *)mods_walk_cb)->cbfn );
}

void WalkAllSects( void (*rtn)( section * ) )
/*******************************************/
{
    rtn( Root );
#ifdef _EXE
    if( FmtData.type & MK_OVERLAYS ) {
        WalkAreas( Root->areas, rtn );
    }
#endif
}

void ParmWalkAllSects( void (*rtn)( section *, void * ), void *parm )
/*******************************************************************/
{
    rtn( Root, parm );
#ifdef _EXE
    if( FmtData.type & MK_OVERLAYS ) {
        ParmWalkAreas( Root->areas, rtn, parm );
    }
#endif
}

void WalkMods( mods_walk_fn *cbfn )
/*********************************/
{
    mods_walk_data      mods_walk_cb;

    mods_walk_cb.cbfn = cbfn;
    ParmWalkAllSects( WalkSectModsList, &mods_walk_cb );
    CurrSect = Root;
    WalkModsList( LibModules, cbfn );
}

static void WalkClass( class_entry *class, class_walk_fn *cbfn )
/**************************************************************/
{
    RingWalk( class->segs, (ring_walk_fn *)cbfn );
}

void SectWalkClass( section *sect, class_walk_fn *cbfn )
/******************************************************/
{
    class_entry         *class;

    CurrSect = sect;
    for( class = sect->classlist; class != NULL; class = class->next_class ) {
        WalkClass( class, cbfn );
    }
}

static void _SectWalkClass( section *sect, void *class_walk_cb )
/**************************************************************/
{
    class_entry     *class;

    CurrSect = sect;
    for( class = sect->classlist; class != NULL; class = class->next_class ) {
        WalkClass( class, ((class_walk_data *)class_walk_cb)->cbfn );
    }
}

void WalkLeaders( class_walk_fn *cbfn )
/*************************************/
{
    class_walk_data     class_walk_cb;

    class_walk_cb.cbfn = cbfn;
    ParmWalkAllSects( _SectWalkClass, &class_walk_cb );
}

seg_leader *FindSegment( section *sect, const char *name )
/********************************************************/
/* NOTE: this doesn't work for overlays!
 *
 * sect != NULL then it works as FindFirstSegment
 * sect == NULL then it works as FindNextSegment
 *
 */
{
    static seg_leader   *seg = NULL;
    static class_entry  *class = NULL;

    if( sect != NULL ) {
        class = sect->classlist;
        seg = NULL;
    }
    for( ; class != NULL; class = class->next_class ) {
        while( (seg = RingStep( class->segs, seg )) != NULL ) {
            if( stricmp( seg->segname.u.ptr, name ) == 0 ) {
                return( seg );
            }
        }
    }
    return( seg );
}

void LinkList( void *in_head, void *newnode )
/*******************************************/
/* Link a new node into a linked list (new node goes at the end of the list) */
{
    node    **owner;

    owner = in_head;
    ((node *)newnode)->next = NULL;
    while( *owner != NULL ) {
        owner = (node **)&(*owner)->next;
    }
    *owner = newnode;
}

void FreeList( void *_curr )
/**************************/
/* Free a list of nodes. */
{
    node        *curr;
    node        *next;

    for( curr = _curr; curr != NULL; curr = next ) {
        next = curr->next;
        _LnkFree( curr );
    }
}

obj_name_list *AddNameTable( const char *name, size_t len, bool is_mod, obj_name_list **owner )
/*********************************************************************************************/
{
    obj_name_list   *imp;
    unsigned_32     off;
    unsigned_16     index;

    index = 1;
    off = 1;
    for( imp = *owner; imp != NULL; imp = imp->next ) {
        if( len == imp->len && memcmp( imp->name.u.ptr, name, len ) == 0 )
            break;
        off += imp->len + 1;
        ++index;
        owner = &imp->next;
    }
    if( imp == NULL ) {
        _PermAlloc( imp, sizeof( obj_name_list ) );
        imp->next = NULL;
        imp->len = len;
        imp->name.u.ptr = AddSymbolStringTable( &PermStrings, name, len );
        imp->num = is_mod ? index : off;
        *owner = imp;
    }
    return( imp );
}

unsigned_16 log2_16( unsigned_16 value )
/**************************************/
// This calculates the binary log of value, truncating decimals.
{
    unsigned_16 log;

    if( value == 0 ) {
        return( 0 );
    }
    log = 15;
    for( ; ; ) {
        if( value & 0x8000 ) {  // done if high bit on
            break;
        }
        value <<= 1;            // shift left and decrease possible log.
        log--;
    }
    return( log );
}

unsigned_16 log2_32( unsigned_32 value )
/**************************************/
// This calculates the binary log of a 32-bit value, truncating decimals.
{
    unsigned_16 log;

    if( value == 0 ) {
        return( 0 );
    }
    log = 31;
    for( ; ; ) {
        if( value & 0x80000000 ) {  // done if high bit on
            break;
        }
        value <<= 1;            // shift left and decrease possible log.
        log--;
    }
    return( log );
}

const char *GetBaseName( const char *namestart, size_t len, size_t *lenp )
/************************************************************************/
/* parse name as a filename, "removing" the path and the extension */
/* returns a pointer to the "base" of the filename, and a length without
 * the extension */
{
    const char  *dotpoint;
    const char  *string;
    char        ch;

    if( len == 0 )
        len = strlen( namestart );
    // ignore path & extension in module name.
    dotpoint = NULL;
    for( string = namestart; len-- > 0; string++ ) {
        ch = *string;
        if( ch == '.' ) {
            dotpoint = string;
            continue;
        }
        if( IS_PATH_SEP( ch ) ) {
            namestart = string + 1;
            dotpoint = NULL;
        }
    }
    if( dotpoint != NULL ) {
        *lenp = dotpoint - namestart;
    } else {
        *lenp = string - namestart;
    }
    return( namestart );
}

#define MAXDEPTH        ( sizeof( size_t ) * 8 )

void VMemQSort( virt_mem base, size_t n, size_t width,
                        void (*swapfn)( virt_mem, virt_mem ),
                        int (*cmpfn)( virt_mem, virt_mem ) )
/***********************************************************/
// qsort stolen from clib, and suitably modified since we need to be able
// to swap parallel arrays.
{
    virt_mem        p1;
    virt_mem        p2;
    virt_mem        mid;
    int             comparison;
    size_t          last_non_equal_count;
    size_t          i;
    size_t          count;
    size_t          sp;
    virt_mem        base_stack[MAXDEPTH];
    size_t          n_stack[MAXDEPTH];

    sp = 0;
    for( ; ; ) {
        while( n > 1 ) {
            p1 = base + width;
            if( n == 2 ) {
                if( cmpfn( base, p1 ) > 0 ) {
                    swapfn( base, p1 );
                }
                break;
            } else {
                /* store mid element at base for pivot */
                /* this will speed up sorting of a sorted list */
                mid = base + ( n >> 1 ) * width;
                swapfn( base, mid );
                p2 = base;
                count = 0;
                last_non_equal_count = 0;
                for( i = 1; i < n; ++i ) {
                    comparison = cmpfn( p1, base );
                    if( comparison <= 0 ) {
                        p2 += width;
                        count++;
                        if( i != count ) {              /* p1 != p2 */
                            swapfn( p1, p2 );
                        }
                    }
                    if( comparison != 0 )
                        last_non_equal_count = count;
                    p1 += width;
                }
                /* special check to see if all values compared are equal */
                if( ( count == n - 1 ) && ( last_non_equal_count == 0 ) )
                    break;
                if( count != 0 ) {  /* store pivot in right spot */
                    swapfn( base, p2 );
                }
#if 0
                qsort( base, count, size, cmp );
                qsort( p2 + size, n - count - 1, size, cmp );
#endif
                n = n - count - 1;          /* calc. size of right part */

                /* The pivot is at p2. It is in its final position.
                   There are count items to the left of the pivot.
                   There are n items to the right of the pivot.
                */

                if( count != last_non_equal_count ) {   /* 18-jul-90 */
                    /*
                       There are last_non_equal_count+1 items to the left
                       of the pivot that still need to be checked.
                       There are (count - (last_non_equal_count+1)) items
                       immediately to the left of the pivot that are
                       equal to the pivot. They are in their final position.
                    */
                    count = last_non_equal_count + 1;
                }
                if( count < n ) {           /* if left part is shorter */
                    base_stack[sp] = p2 + width;  /* - stack right part */
                    n_stack[sp] = n;
                    n = count;
                } else {                    /* right part is shorter */
                    base_stack[sp] = base;  /* - stack left part */
                    n_stack[sp] = count;
                    base = p2 + width;
                }
                ++sp;
            }
        }
        if( sp == 0 )
            break;
        --sp;
        base = base_stack[sp];
        n    = n_stack[sp];
    }
}

static void *SpawnStack;

int Spawn( void (*fn)( void ) )
/*****************************/
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
/******************/
{
    if( SpawnStack != NULL ) {
        longjmp( SpawnStack, 1 );
    }
}

void InitEnvVars( void )
/**********************/
{
    const char  *path_list;
    size_t      len;
    char        *p;

    if( ExePath == NULL ) {
#if defined( __QNX__ )
        path_list = "/usr/watcom";
#else
        path_list = GetEnvString( "PATH" );
#endif
        if( path_list != NULL && *path_list != '\0' ) {
            len = strlen( path_list );
            _ChkAlloc( ExePath, len + 1 );
            p = ExePath;
            do {
                if( p != ExePath )
                    *p++ = PATH_LIST_SEP;
                path_list = GetPathElement( path_list, NULL, &p );
            } while( *path_list != '\0' );
            *p = '\0';
        } else {
            _ChkAlloc( ExePath, 1 );
            *ExePath = '\0';
        }
    }
    if( LibPath == NULL ) {
        path_list = GetEnvString( "LIB" );
        if( path_list != NULL && *path_list != '\0' ) {
            len = strlen( path_list );
            _ChkAlloc( LibPath, len + 1 );
            p = LibPath;
            do {
                if( p != LibPath )
                    *p++ = PATH_LIST_SEP;
                path_list = GetPathElement( path_list, NULL, &p );
            } while( *path_list != '\0' );
            *p = '\0';
        } else {
            _ChkAlloc( LibPath, 1 );
            *LibPath = '\0';
        }
    }
}

void FiniEnvVars( void )
/**********************/
{
    if( ExePath != NULL ) {
        _LnkFree( ExePath );
        ExePath = NULL;
    }
    if( LibPath != NULL ) {
        _LnkFree( LibPath );
        LibPath = NULL;
    }
}

f_handle FindPath( const char *name, char *fullname )
/***************************************************/
{
    const char  *path_list;
    f_handle    file;
    char        fullpath[PATH_MAX];


    strcpy( fullpath, name );
    file = QObjOpen( fullpath );
    if( file == NIL_FHANDLE && ExePath != NULL ) {
        for( path_list = ExePath; *path_list != '\0'; ) {
            strcpy( MakePath( fullpath, &path_list ), name );
            file = QObjOpen( fullpath );
            if( file != NIL_FHANDLE ) {
                break;
            }
        }
    }
    if( file != NIL_FHANDLE && fullname != NULL ) {
        strcpy( fullname, fullpath );
    }
    return( file );
}

group_entry *FindGroup( segment seg )
/***********************************/
{
    group_entry *group;

    for( group = Groups; group != NULL; group = group->next_group ) {
        if( group->grp_addr.seg == seg ) {
            break;
        }
    }
    return( group );
}

offset FindLinearAddr( targ_addr *addr )
/**************************************/
{
    group_entry *group;

    group = FindGroup( addr->seg );
    if( group != NULL ) {
        return( addr->off + ( group->linear - group->grp_addr.off ) );
    }
    return( addr->off );
}

offset FindLinearAddr2( targ_addr *addr )
/***************************************/
{
    group_entry *group;

    group = FindGroup( addr->seg );
    if( group != NULL ) {
        return( addr->off + group->linear + FmtData.base );
    }
    return( addr->off );
}

file_list *AllocNewFile( member_list *member )
/********************************************/
{
    file_list       *new_entry;

    _PermAlloc( new_entry, sizeof( file_list ) );
    new_entry->next_file = NULL;
    new_entry->flags = DBIFlag;
    new_entry->strtab = NULL;
    new_entry->u.member = member;
    if( member != NULL ) {
        new_entry->flags |= STAT_HAS_MEMBER;
    }
    return( new_entry );
}

char *FileName( const char *buff, size_t len, file_defext etype, bool force )
/***************************************************************************/
{
    const char  *namptr;
    const char  *namstart;
    char        *ptr;
    size_t      cnt;
    size_t      namelen;
    char        c;
    size_t      extlen;


    for( namptr = buff + len; namptr != buff; --namptr ) {
        c = namptr[-1];
        if( IS_PATH_SEP( c ) ) {
            break;
        }
    }
    namstart = namptr;
    cnt = len - ( namptr - buff );
    if( cnt == 0 ) {
        DUPSTR_STACK( ptr, buff, len );
        LnkMsg( LOC+LINE+FTL+MSG_INV_FILENAME, "s", ptr );
    }
    namelen = cnt;
    namptr = buff + len - 1;
    while( --cnt != 0 && *namptr != '.' ) {
        namptr--;
    }
    if( force || *namptr != '.' ) {
        if( force && etype == E_MAP ) {         // op map goes in current dir.
            buff = namstart;
            len = namelen;
        }
        if( cnt != 0 ) {
            len = namptr - buff;
        }
        extlen = DefExtLen[etype];
        _ChkAlloc( ptr, len + 1 + extlen + 1 );
        memcpy( ptr, buff, len );
        if( extlen > 0 ) {
            ptr[len++] = '.';
            memcpy( ptr + len, DefExt[etype], extlen );
            len += extlen;
        }
        ptr[len] = '\0';
    } else {
        ptr = ChkToString( buff, len );
    }
    return( ptr );
}

static int stricmp_wrapper( const void *s1, const void *s2 )
/**********************************************************/
{
    return( stricmp( s1, s2 ) );
}

section *NewSection( void )
/*************************/
{
    section             *sect;

    _ChkAlloc( sect, sizeof( section ) );
    sect->next_sect = NULL;
    sect->classlist = NULL;
    sect->orderlist = NULL;
    sect->areas = NULL;
    sect->files = NULL;
    sect->modFilesHashed = CreateHTable( 256, StringiHashFunc, stricmp_wrapper, ChkLAlloc, LFree );
    sect->mods = NULL;
    sect->reloclist = NULL;
    SET_ADDR_UNDEFINED( sect->sect_addr );
    sect->ovlref = 0;
    sect->parent = NULL;
    sect->relocs = 0;
    sect->size = 0;
    sect->outfile = NULL;
    sect->u.dist_mods = NULL;
    sect->dbg_info = NULL;
    return( sect );
}
