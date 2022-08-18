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
* Description:  Macro processing for the preprocessor.
*
****************************************************************************/


#include "_preproc.h"

#define END_OF_MACRO_STR    "Z-<end of macro>"

#define PPT_UNEXPANDABLE_ID     PPT_LAST_TOKEN

typedef struct macro_arg {
    const char      *name;
    MACRO_TOKEN     *arg;
} MACRO_ARG;

typedef struct nested_macros {
    struct nested_macros *next;
    MACRO_ENTRY     *fmentry;
    MACRO_ARG       *macro_parms;
    bool            rescanning;
    bool            substituting_parms;
} NESTED_MACRO;

extern void DumpMTokens( MACRO_TOKEN *mtok );
extern void DumpNestedMacros( void );

NESTED_MACRO *NestedMacros;
int           MacroDepth;
MACRO_TOKEN  *MacroExpansion( MACRO_ENTRY *, bool );
MACRO_TOKEN  *NestedMacroExpansion( MACRO_ENTRY * );

static void FreeTokenList( MACRO_TOKEN *head )
{
    MACRO_TOKEN *next;

    for( ; head != NULL; head = next ) {
        next = head->next;
        PP_Free( head );
    }
}

void DeleteNestedMacro( void )
{
    MACRO_ENTRY  *fmentry;
    NESTED_MACRO *nested;
    MACRO_ARG    *macro_parms;
    MACRO_TOKEN  *mtok;
    int          i;

    nested = NestedMacros;
    if( nested != NULL ) {
        NestedMacros = nested->next;
        macro_parms = nested->macro_parms;
        fmentry = nested->fmentry;
        PP_Free( nested );
        i = fmentry->parmcount - 1;
        if( i > 0 && macro_parms != NULL ) {
            do {
                --i;
                while( (mtok = macro_parms[i].arg) != NULL ) {
                    macro_parms[i].arg = mtok->next;
                    PP_Free( mtok );
                }
            } while( i != 0 );
            PP_Free( macro_parms );
        }
    }
}

MACRO_TOKEN *PPNextToken( void )
{
    MACRO_TOKEN *mtok;

    while( (mtok = PPTokenList) != NULL ) {
        PPTokenList = mtok->next;
        mtok->next = NULL;
        if( !IS_END_OF_MACRO( mtok ) ) {
            PPTokenPtr = mtok->data;
            break;
        }
        DeleteNestedMacro();
        PP_Free( mtok );
    }
    return( mtok );
}

static MACRO_TOKEN *NewToken( ppt_token token, size_t len )
{
    MACRO_TOKEN *mtok;

    mtok = (MACRO_TOKEN *)PP_Malloc( sizeof( MACRO_TOKEN ) + len );
    mtok->next = NULL;
    mtok->token = token;
    return( mtok );
}

static MACRO_TOKEN *BuildAToken( ppt_token token, const char *p )
{
    MACRO_TOKEN *mtok;

    mtok = NewToken( token, strlen( p ) );
    strcpy( mtok->data, p );
    return( mtok );
}

MACRO_TOKEN *NextMToken( void )
{
    MACRO_TOKEN *mtok;
    size_t      len;
    ppt_token   token;

    mtok = PPNextToken();
    if( mtok == NULL ) {
        if( PP_ScanNextToken( &token ) != 0 ) {
            token = PPT_EOF;
            len = 1;
            PPTokenPtr = " ";
        } else {
            len = PPNextTokenPtr - PPTokenPtr;
            if( token == PPT_WHITE_SPACE || token == PPT_COMMENT ) {
                len = 1;
                PPTokenPtr = " ";
            }
        }
        mtok = NewToken( token, len );
        memcpy( mtok->data, PPTokenPtr, len );
        mtok->data[len] = '\0';
        PPTokenPtr = PPNextTokenPtr;
    }
    return( mtok );
}

static MACRO_TOKEN *PPTrimWhiteSpace( MACRO_TOKEN *head )
{
    MACRO_TOKEN *mtok;
    MACRO_TOKEN *prev;

    prev = NULL;
    for( mtok = head; mtok != NULL; mtok = mtok->next ) {
        if( mtok->token != PPT_WHITE_SPACE ) {
            prev = mtok;
        }
    }
    if( prev != NULL ) {
        if( prev->next != NULL ) {
            FreeTokenList( prev->next );
            prev->next = NULL;
        }
    }
    return( head );
}

static MACRO_ARG *PPCollectParms( MACRO_ENTRY *fmentry )
{
    int         bracket;
    MACRO_TOKEN *mtok;
    MACRO_TOKEN *head;
    MACRO_TOKEN *tail;
    int         parm_cnt;
    MACRO_ARG   *macro_parms;

    macro_parms = NULL;
    if( fmentry->parmcount != 0 ) { /* if () expected */
        if( fmentry->parmcount > 1 ) {
            macro_parms = (MACRO_ARG *)PP_Malloc( (fmentry->parmcount - 1) * sizeof( MACRO_ARG ) );
        }
        parm_cnt = 0;
        mtok = PPNextToken();
        if( mtok == NULL && PPTokenPtr[0] == '(' ) {
            // already positioned at the '('
        } else {
            for( ;; ) {
                if( mtok == NULL )
                    mtok = NextMToken();
                if( mtok->token != PPT_WHITE_SPACE )
                    break;
                PP_Free( mtok );
                mtok = NULL;
            }
            PP_Free( mtok );    // this should be '(', throw it away
        }
        bracket = 0;
        head = NULL;
        tail = NULL;
        for( ;; ) {
            for( ;; ) {
                mtok = NextMToken();
                if( mtok->token != PPT_WHITE_SPACE )
                    break;
                if( head != NULL )
                    break;
                PP_Free( mtok );
            }
            if( mtok->token == PPT_EOF )
                break;
            if( mtok->token == PPT_LEFT_PAREN ) {
                ++bracket;
            } else if( mtok->token == PPT_RIGHT_PAREN ) {
                if( bracket == 0 )
                    break;
                --bracket;
            } else if( mtok->token == PPT_COMMA && bracket == 0 ) {
                if( parm_cnt < fmentry->parmcount - 1 ) {
                    macro_parms[parm_cnt].arg = PPTrimWhiteSpace( head );
                }
                PP_Free( mtok );
                ++parm_cnt;
                head = NULL;
                tail = NULL;
                continue;
            }
            if( head == NULL )
                head = mtok;
            if( tail != NULL )
                tail->next = mtok;
            tail = mtok;
        }
        PP_Free( mtok );
        if( parm_cnt < fmentry->parmcount - 1 ) {
            macro_parms[parm_cnt].arg = PPTrimWhiteSpace( head );
            ++parm_cnt;
        } else {
            for( ; head != NULL; head = mtok ) {
                mtok = head->next;
                PP_Free( head );
            }
        }
#if 0
        if( parm_cnt < fmentry->parmcount - 1 ) {
            CErr( ERR_TOO_FEW_MACRO_PARMS, fmentry->name );
        } else if( parm_cnt > fmentry->parmcount - 1 ) {
            CErr( ERR_TOO_MANY_MACRO_PARMS, fmentry->name );
        }
#endif
        for( ; parm_cnt < fmentry->parmcount - 1; ++parm_cnt ) {
            macro_parms[parm_cnt].arg = NULL;
        }
    }
    return( macro_parms );
}

void DumpMTokens( MACRO_TOKEN *mtok )
{
    for( ; mtok != NULL; mtok = mtok->next ) {
        printf( "%s\n", mtok->data );
    }
    fflush( stdout );
}

void DumpNestedMacros( void )
{
    NESTED_MACRO *nested;

    for( nested = NestedMacros; nested != NULL; nested = nested->next ) {
        printf( "%s\n", nested->fmentry->name );
    }
    fflush( stdout );
}


static MACRO_TOKEN *AppendToken( MACRO_TOKEN *head, ppt_token token, const char *data )
{
    MACRO_TOKEN *tail;
    MACRO_TOKEN *new;

    new = BuildAToken( token, data );
    if( head == NULL ) {
        head = new;
    } else {
        tail = head;
        while( tail->next != NULL )
            tail = tail->next;
        tail->next = new;
    }
    return( head );
}

static bool MacroBeingExpanded( MACRO_ENTRY *fmentry )
{
    NESTED_MACRO *nested;

    for( nested = NestedMacros; nested != NULL; nested = nested->next ) {
        if( nested->fmentry == fmentry )
            return( true );
        if( !nested->rescanning ) {
            break;
        }
    }
    return( false );
}

static int Expandable( MACRO_ENTRY *me, MACRO_TOKEN *mtok, bool macro_parm )
{
    int         lparen;
    ppt_token   token;

    if( me->parmcount == PP_SPECIAL_MACRO ) {
        return( 1 );
    }
    if( me->parmcount == 0 ) { /* if () not expected */
        if( macro_parm ) {
            if( MacroBeingExpanded( me ) ) {
                return( 0 );
            }
        }
        return( 1 );
    }
    for( ; mtok != NULL; mtok = mtok->next ) {
        if( mtok->token != PPT_WHITE_SPACE && mtok->token != PPT_NULL ) {
            break;
        }
    }
    if( mtok != NULL ) {
        if( mtok->token == PPT_LEFT_PAREN ) {
            if( MacroDepth == 1 && !macro_parm )
                return( 1 );
            lparen = 0;
            while( (mtok = mtok->next) != NULL ) {
                if( mtok->token == PPT_LEFT_PAREN ) {
                    ++lparen;
                } else if( mtok->token == PPT_RIGHT_PAREN ) {
                    if( lparen == 0 )
                        return( 1 );
                    --lparen;
                }
            }
        }
    } else if( !macro_parm ) {
        for( ;; ) {
            if( PP_ScanNextToken( &token ) != 0 ) {
                return( 0 );
            }
            if( token != PPT_WHITE_SPACE && token != PPT_COMMENT ) {
                break;
            }
        }
        if( token == PPT_LEFT_PAREN ) {
            return( 1 );
        } else if( PPFlags & PPFLAG_PREPROCESSING ) {
            return( 2 );
        }
    }
    return( 0 );
}

static MACRO_TOKEN *ExpandNestedMacros( MACRO_TOKEN *head, bool rescanning )
{
    MACRO_TOKEN *mtok;
    MACRO_TOKEN *toklist;
    MACRO_TOKEN *prev_tok;
    MACRO_TOKEN *old_tokenlist;
    MACRO_ENTRY *me;
    int         i;

    ++MacroDepth;
    prev_tok = NULL;
    for( mtok = head; mtok != NULL; ) {
        toklist = NULL;
        if( mtok->token == PPT_ID ) {
            // if macro and not being expanded, then expand it
            // only tokens available for expansion are those in mtok list
            me = PP_MacroLookup( mtok->data, strlen( mtok->data ) );
            if( me != NULL ) {
                if( rescanning ) {
                    if( MacroBeingExpanded( me ) ) {
                        mtok->token = PPT_UNEXPANDABLE_ID;
                    } else {
                        toklist = mtok;
                        while( toklist->next != NULL ) {
                            toklist = toklist->next;
                        }
                        toklist->next = PPTokenList;
                        i = Expandable( me, mtok->next, false );
                        switch( i ) {
                        case 0:         // macro is currently not expandable
                            mtok->token = PPT_TEMP_ID;
                            toklist->next = NULL;
                            toklist = NULL;
                            break;
                        case 1:         // macro is expandable
                            PPTokenList = mtok->next;
                            if( head == mtok ) {
                                head = NULL;
                                prev_tok = NULL;
                            }
                            PP_Free( mtok );
                            toklist = MacroExpansion( me, rescanning );
                            mtok = PPTokenList;
                            PPTokenList = NULL;
                            break;
                        case 2:         // we skipped over some white space
                            mtok->token = PPT_UNEXPANDABLE_ID;
                            toklist->next = NULL;
                            toklist = BuildAToken( PPT_WHITE_SPACE, " " );
                            toklist->next = mtok->next;
                            mtok->next = toklist;
                            toklist = NULL;
                            break;
                        }
                    }
                } else {        // expanding a macro parm
                    if( Expandable( me, mtok->next, true ) ) {
                        old_tokenlist = PPTokenList;
                        PPTokenList = mtok->next;
                        if( head == mtok ) {
                            head = NULL;
                            prev_tok = NULL;
                        }
                        PP_Free( mtok );
                        toklist = NestedMacroExpansion( me );
                        mtok = PPTokenList;
                        PPTokenList = old_tokenlist;
                    } else {
                        prev_tok = mtok;
                        mtok = mtok->next;      // advance onto next token
                    }
                }
            } else {
                mtok->token = PPT_SAVED_ID;     // avoid rechecking this ID
                prev_tok = mtok;
                mtok = mtok->next;      // advance onto next token
            }
        } else if( mtok->token == PPT_NULL ) {
            toklist = mtok->next;
            rescanning = NestedMacros->rescanning;
            DeleteNestedMacro();
            PP_Free( mtok );
            mtok = toklist;
            toklist = NULL;
        } else {                        // advance onto next token
            prev_tok = mtok;
            mtok = mtok->next;
        }
        if( toklist != NULL ) {         // new tokens to insert
            if( prev_tok == NULL ) {
                head = toklist;
            } else {
                prev_tok->next = toklist;
            }
            if( mtok != NULL ) {
                while( toklist->next != NULL )
                    toklist = toklist->next;
                toklist->next = mtok;
            }
            if( prev_tok == NULL ) {
                mtok = head;
            } else {
                mtok = prev_tok->next;
            }
        } else {
            // either no change, or tokens were deleted
            if( prev_tok == NULL ) {
                head = mtok;
            } else {
                prev_tok->next = mtok;
            }
        }
    }
    for( mtok = head; mtok != NULL; mtok = mtok->next ) {
        // change a temporarily unexpandable ID into an ID because it
        // could become expandable in a later rescanning phase
        if( mtok->token == PPT_TEMP_ID ) {
            mtok->token = PPT_ID;
        }
    }
    --MacroDepth;
    return( head );
}

static MACRO_TOKEN *Glue2Tokens( MACRO_TOKEN *first, MACRO_TOKEN *second )
{
    MACRO_TOKEN *mtok;
    size_t      len;

    len = strlen( first->data );
    if( second != NULL )
        len += strlen( second->data );
    mtok = NewToken( first->token, len );
    if( mtok->token == PPT_SAVED_ID || mtok->token == PPT_UNEXPANDABLE_ID ) {
        mtok->token = PPT_ID;
    }
    strcpy( mtok->data, first->data );
    if( second != NULL )
        strcat( mtok->data, second->data );
    return( mtok );
}

static MACRO_TOKEN *GlueTokens( MACRO_TOKEN *head )
{
    MACRO_TOKEN *mtok;
    MACRO_TOKEN *prev;
    MACRO_TOKEN *next;
    MACRO_TOKEN *new;

    prev = NULL;
    for( mtok = head; mtok != NULL; ) {
        if( mtok->token != PPT_WHITE_SPACE ) {
            next = mtok->next;
            if( next == NULL )
                break;
            if( next->token == PPT_WHITE_SPACE )
                next = next->next;
            if( next == NULL )
                break;
            if( next->token == PPT_SHARP_SHARP ) {
                next = next->next;
                // glue mtok->token with next->token to make one token
                // create new token
                new = Glue2Tokens( mtok, next );
                if( prev == NULL ) {
                    head = new;
                } else {
                    prev->next = new;
                }
                for( ; new->next != NULL; new = new->next ) {
                    prev = new;
                }
                if( next != NULL ) {
                    new->next = next->next;
                }
                do {
                    next = mtok->next;
                    PP_Free( mtok );
                    mtok = next;
                } while( mtok != new->next );
                mtok = new;
                continue;       /* to catch consecutive ##'s */
            }
        }
        prev = mtok;
        mtok = mtok->next;
    }
    return( head );
}


static MACRO_TOKEN *BuildMTokenList( MACRO_ENTRY *me, MACRO_ARG *macro_parms )
{
    MACRO_TOKEN     *mtok;
    MACRO_TOKEN     *head;
    MACRO_TOKEN     *tail;
    const char      *p;
    const char      *p1;
    const char      *p2;
    size_t          len;
    unsigned char   i;
    ppt_token       token;
    ppt_token       prev_token;

    p = me->replacement_list;
    for( i = 1; i < me->parmcount; i++ ) {
        macro_parms[i - 1].name = p;
        while( *p != '\0' )
            p++;
        p++;
    }
    head = NULL;
    tail = NULL;
    if( p == NULL )
        return( NULL );
    prev_token = PPT_NULL;
    for( p1 = p; *p1 != '\0'; p1 = p2 ) {
        p2 = PP_ScanToken( p1, &token );
        if( token == PPT_WHITE_SPACE && prev_token == PPT_SHARP_SHARP ) {
            continue;
        }
        len = p2 - p1;
        if( token == PPT_ID ) {
            for( i = 1; i < me->parmcount; i++ ) {
                p = macro_parms[i - 1].name;
                if( memcmp( p1, p, len ) == 0 && p[len] == '\0' ) {
                    token = PPT_MACRO_PARM;
                    i--;
                    len = 0;
                    break;
                }
            }
        }
        prev_token = token;
        mtok = NewToken( token, len );
        if( token == PPT_MACRO_PARM ) {
            mtok->data[0] = i;
        } else {
            memcpy( mtok->data, p1, len );
            mtok->data[len] = '\0';
        }
        if( head == NULL )
            head = mtok;
        if( tail != NULL )
            tail->next = mtok;
        tail = mtok;
    }
    return( head );
}

static MACRO_TOKEN *DuplicateList( MACRO_TOKEN *list )
{
    MACRO_TOKEN *mtok;
    MACRO_TOKEN *head;
    MACRO_TOKEN *tail;

    head = NULL;
    tail = NULL;
    for( ; list != NULL; list = list->next ) {
        mtok = BuildAToken( list->token, list->data );
        if( head == NULL )
            head = mtok;
        if( tail != NULL )
            tail->next = mtok;
        tail = mtok;
    }
    return( head );
}

static size_t MakeString( MACRO_TOKEN *list, char *p )
{
    size_t      len;
    char        *p2;
    bool        output;

    len = 0;
    output = ( p != NULL );
    if( output )
        *p++ = '\"';
    ++len;
    for( ; list != NULL; list = list->next ) {
        for( p2 = list->data; *p2 != '\0'; ++p2 ) {
            if( *p2 == '\"' || *p2 == '\\' ) {
                if( list->token == PPT_LITERAL ) {
                    if( output )
                        *p++ = '\\';
                    ++len;
                }
            }
            if( output )
                *p++ = *p2;
            ++len;
        }
    }
    if( output )
        *p++ = '\"';
    ++len;
    if( output )
        *p = '\0';
    return( len );
}

static MACRO_TOKEN *BuildString( MACRO_TOKEN *list )
{
    MACRO_TOKEN *mtok;
    size_t      len;

    len = MakeString( list, NULL );
    mtok = NewToken( PPT_LITERAL, len );
    MakeString( list, mtok->data );
    return( mtok );
}

static bool SharpSharp( MACRO_TOKEN *mtok )
{
    for( ; mtok != NULL; mtok = mtok->next ) {
        if( mtok->token == PPT_SHARP_SHARP )
            return( true );
        if( mtok->token != PPT_WHITE_SPACE ) {
            break;
        }
    }
    return( false );
}

static MACRO_TOKEN *SubstituteParms( MACRO_TOKEN *head, MACRO_ARG *macro_parms )
{
    MACRO_TOKEN *mtok;
    MACRO_TOKEN *mtok2;
    MACRO_TOKEN *list;
    MACRO_TOKEN *toklist;
    MACRO_TOKEN *prev_tok;
    ppt_token   prev_token;

    prev_tok = NULL;
    prev_token = PPT_NULL;
    for( mtok = head; mtok != NULL; mtok = mtok->next ) {
        list = NULL;
        if( mtok->token == PPT_SHARP ) {
            // replace this and next token (macro parm) with a string
            for( mtok2 = mtok; (mtok2 = mtok2->next) != NULL; ) {
                if( mtok2->token != PPT_WHITE_SPACE ) {
                    break;
                }
            }
            if( mtok2 != NULL ) {
                if( mtok2->token == PPT_MACRO_PARM ) {
                    list = BuildString( macro_parms[(unsigned char)mtok2->data[0]].arg );
                    toklist = mtok->next;
                    mtok->next = mtok2->next;
                    mtok2->next = NULL;
                    FreeTokenList( toklist );
                }
            }
        } else if( mtok->token == PPT_MACRO_PARM ) {
            // replace this ID with a copy of the tokens from
            // macro_parms[mtok->data[0]].arg
            list = DuplicateList( macro_parms[(unsigned char)mtok->data[0]].arg );
            if( prev_token != PPT_SHARP_SHARP && !SharpSharp( mtok->next ) ) {
                list = ExpandNestedMacros( list, false );
            }
            if( list == NULL ) {
                list = mtok;
                mtok = mtok->next;
                PP_Free( list );
                list = NULL;
                if( prev_tok != NULL ) {
                    prev_tok->next = mtok;
                }
            }
        }
        if( list != NULL ) {
            if( prev_tok == NULL ) {
                head = list;
            } else {
                prev_tok->next = list;
            }
            while( list->next != NULL )
                list = list->next;
            list->next = mtok->next;
            PP_Free( mtok );
            mtok = list;
        }
        if( mtok == NULL )
            break;
        prev_tok = mtok;
        if( mtok->token != PPT_WHITE_SPACE ) {
            prev_token = mtok->token;
        }
    }
    return( head );
}

static MACRO_TOKEN *BuildSpecialToken( MACRO_ENTRY *me )
{
    MACRO_TOKEN *head;
    char        *p;
    const char  *filename;
    ppt_token   token;
    char        buffer[200];

    switch( me->name[2] ) {
    case 'L':                           /* __LINE__ */
        sprintf( buffer, "%d", PPLineNumber );
        p = buffer;
        token = PPT_NUMBER;
        break;
    case 'F':                           /* __FILE__ */
        p = buffer;
        *p++ = '\"';
        for( filename = PP_File->filename; *filename != '\0'; ++filename ) {
            if( *filename == '\\' )
                *p++ = '\\';
            *p++ = *filename;
        }
        *p++ = '\"';
        *p = '\0';
        p = buffer;
        token = PPT_LITERAL;
        break;
    case 'D':                           /* __DATE__ */
        p = PP__DATE__;
        token = PPT_LITERAL;
        break;
    case 'T':                           /* __TIME__ */
        p = PP__TIME__;
        token = PPT_LITERAL;
        break;
    case 'S':                           /* __STDC__ */
        p = "1";
        token = PPT_NUMBER;
        break;
    default:
        p = "";
        token = PPT_NULL;
        break;
    }
    head = BuildAToken( token, p );
    return( head );
}

MACRO_TOKEN *MacroExpansion( MACRO_ENTRY *me, bool rescanning )
{
    MACRO_ARG   *macro_parms;
    MACRO_TOKEN *head;
    MACRO_TOKEN *mtok;
    NESTED_MACRO *nested;

    nested = (NESTED_MACRO *)PP_Malloc( sizeof( NESTED_MACRO ) );
    nested->fmentry = me;
    nested->rescanning = rescanning;
    nested->substituting_parms = false;
    nested->macro_parms = NULL;
    if( me->parmcount == PP_SPECIAL_MACRO ) {
        head = BuildSpecialToken( me );
        nested->next = NestedMacros;
        NestedMacros = nested;
    } else {
        macro_parms = PPCollectParms( me );
        nested->next = NestedMacros;
        NestedMacros = nested;
        nested->macro_parms = macro_parms;
        head = BuildMTokenList( me, macro_parms );
        if( macro_parms != NULL ) {
            nested->substituting_parms = true;
            head = SubstituteParms( head, macro_parms );
            nested->substituting_parms = false;
        }
        head = GlueTokens( head );
        for( mtok = head; mtok != NULL; mtok = mtok->next ) {
            if( mtok->token == PPT_ID ) {
                for( nested = NestedMacros; nested != NULL; nested = nested->next ) {
                    if( strcmp( nested->fmentry->name, mtok->data ) == 0 ) {
                        if( !nested->substituting_parms ) {
                            // change token so it won't be considered a
                            // candidate as a macro
                            mtok->token = PPT_SAVED_ID;
                            break;
                        }
                    }
                }
            }
        }
    }
    return( AppendToken( head, PPT_NULL, END_OF_MACRO_STR ) );
}

MACRO_TOKEN *NestedMacroExpansion( MACRO_ENTRY *me )
{
    return( ExpandNestedMacros( MacroExpansion( me, false ), true ) );
}

void DoMacroExpansion( MACRO_ENTRY *me )
{
    MacroDepth = 0;
    PPTokenList = NestedMacroExpansion( me );
    // GetMacroToken will feed back tokens from the TokenList
    // when the PPTokenList is exhausted, then revert back to normal scanning
}

void PPMacroVarInit( void )
/*************************/
{
    NestedMacros = NULL;
    MacroDepth = 0;
}

void PPMacroVarFini( void )
/*************************/
{
}
