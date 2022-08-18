/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2016 The Open Watcom Contributors. All Rights Reserved.
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


#include "drwatcom.h"
#include "srchmsg.h"
#include "priority.rh"
#include "menu.rh"
#include "jdlg.h"
#include "madrtn.h"
#include "madsys1.h"
#include "digcli.h"


/* Local Window callback functions prototypes */
WINEXPORT BOOL CALLBACK ProcPriorityDlg( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
WINEXPORT BOOL CALLBACK ProcListProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

typedef struct {
    DWORD       procid;
    ProcStats   stats;
    HANDLE      hdl;            // used for owned processes only
} ProcPriorityInfo;

static ProcNode         *procList;
static HWND             procDlg;

ProcNode *FindProcess( DWORD process )
{
    ProcNode    *cur;

    for( cur = procList; cur != NULL; cur = cur->next ) {
        if( cur->procid == process ) {
            break;
        }
    }
    return( cur );
}

void GetProcName( DWORD process, char *name )
{
    ProcNode    *cur;

    cur = FindProcess( process );
    if( cur != NULL && cur ->procname != NULL ) {
        strcpy( name, cur->procname );
    } else {
        *name =  '\0';
    }
}


static DWORD getStackPtr( HANDLE threadhdl )
{
    mad_registers   *regs;
    addr_ptr        addr;

    AllocMadRegisters( &regs );
    LoadMADRegisters( regs, threadhdl );
    MADRegSpecialGet( MSR_SP, regs, &addr );
    DeAllocMadRegisters( regs );
    return( addr.offset );
}
/*
 * AddThread
 */

void AddThread( DWORD procid, DWORD threadid, HANDLE threadhdl )
{
    ProcNode    *process;
    ThreadNode  *new;

    process = FindProcess( procid );
    if( process != NULL ) {
        new = FindThread( process, threadid );
        if( new == NULL ) {
            new = MemAlloc( sizeof( ThreadNode ) );
            new->threadid = threadid;
            new->threadhdl = threadhdl;
            new->next = process->thread;
            process->thread = new;
            new->stack = getStackPtr( threadhdl );
        } else {
            if( new->threadhdl == NULL ) {
                new->threadhdl = threadhdl;
                new->stack = getStackPtr( threadhdl );
            }
        }
    }
}

/*
 * AddProcess
 */
void AddProcess( DWORD procid, HANDLE prochdl, DWORD threadid,
                 HANDLE threadhdl )
{
    ProcNode            *process;
    ProcNode            *new;
    ProcStats           stats;
    unsigned            cnt;
    BOOL                noprocinfo;

    process = FindProcess( procid );
    RefreshInfo();
    cnt = 0;
    noprocinfo = FALSE;
    while( !GetProcessInfo( procid, &stats ) ) {
        Sleep( 100 );
        RefreshInfo();
        if( cnt > 100 ) {
            noprocinfo = TRUE;
            break;
        }
    }
    if( process == NULL ) {
#if defined( _M_IX86 )
        CONTEXT context;
#endif

        new = MemAlloc( sizeof( ProcNode ) );
        new->procid = procid;
        new->prochdl = prochdl;
        new->thread = NULL;
        if( noprocinfo ) {
            new->procname[0] = '\0';
        } else {
            strcpy( new->procname, stats.name );
        }
#if defined( _M_IX86 )
        context.ContextFlags = CONTEXT_SEGMENTS | CONTEXT_CONTROL;
        GetThreadContext( threadhdl, &context );
        new->SegCs = context.SegCs;
        new->SegDs = context.SegDs;
#else
        new->SegCs = 1;
        new->SegDs = 1;
#endif
        new->next = procList;
        procList = new;
    }
    AddThread( procid, threadid, threadhdl );
}

/*
 * freeModuleNode
 */

static void freeModuleNode( ModuleNode *node )
{
    if( node != NULL ) {
        if( node->syminfo != NULL ) {
            MemFree( node->syminfo );
        }
        if( node->name != NULL ) {
            MemFree( node->name );
        }
        if( node->objects != NULL ) {
            MemFree( node->objects );
        }
        if( node->fp != NULL ) {
            DIGCli( Close )( node->fp );
        }
        MemFree( node );
    }
}

/*
 * GetFirstModule
 */
ModuleNode *GetFirstModule( ProcNode *procinfo )
{
    if( procinfo == NULL )
        return( NULL );
    return( procinfo->module );
}

/*
 * GetNextModule
 */
ModuleNode *GetNextModule( ModuleNode *modinfo )
{
    if( modinfo == NULL )
        return( NULL );
    return( modinfo->next );
}

/*
 * AddModule
 */
void AddModule( DWORD procid, FILE *fp, DWORD base, char *name )
{
    ProcNode    *process;
    ModuleNode  *new;
    ModuleNode  **cur;

    process = FindProcess( procid );
    if( process != NULL ) {
        for( cur = &process->module; (*cur) != NULL; cur = &(*cur)->next ) {
            if( (*cur)->base > base ) {
                break;
            }
        }
        new = MemAlloc( sizeof( ModuleNode ) );
        new->next = (*cur);
        (*cur) = new;
        new->syminfo = NULL;
        new->base = base;
        new->fp = fp;
        new->name = name;
        new->procnode = process;
        if( !GetModuleSize( fp, &new->size ) ) {
            new->size = -1;
        }
        new->objects = GetModuleObjects( fp, &new->num_objects );
    }
}

/*
 * ModuleFromAddr
 */
ModuleNode *ModuleFromAddr( ProcNode *proc, void *addr )
{
    ModuleNode  *cur;

    if( proc == NULL ) {
        return( NULL );
    }
    for( cur = proc->module; cur != NULL; cur = cur->next ) {
        if( cur->base > (DWORD)addr ) {
            cur = NULL;
            break;
        }
        if( cur->size == -1 ) {
            if( cur->next == NULL || (DWORD)addr < cur->next->base ) {
                break;
            }
        } else {
            if( (DWORD)addr < cur->base + cur->size ) {
                break;
            }
        }
    }
    return( cur );
}

/*
 * RemoveModule
 */
void RemoveModule( DWORD procid, DWORD base )
{
    ProcNode    *process;
    ModuleNode  **cur;
    ModuleNode  *toremove;

    process = FindProcess( procid );
    if( process != NULL ) {
        for( cur = &process->module; *cur != NULL; cur = &(*cur)->next ) {
            if( (*cur)->base == base ) {
                toremove = *cur;
                *cur = (*cur)->next;
                freeModuleNode( toremove );
                break;
            }
        }
    }
}

/*
 * FindThread
 */

ThreadNode *FindThread( ProcNode *procnode, DWORD threadid )
{
    ThreadNode          *thread;

    if( procnode == NULL ) {
        thread = NULL;
    } else {
        for( thread = procnode->thread; thread != NULL; thread = thread->next ) {
            if( thread->threadid == threadid ) {
                break;
            }
        }
    }
    return( thread );
}

/*
 * freeThreadNode
 */
static void freeThreadNode( ThreadNode *node )
{
    if( node == NULL ) {
        CloseHandle( node->threadhdl );
        MemFree( node );
    }
}

/*
 * RemoveThread
 */
void RemoveThread( DWORD processid, DWORD threadid )
{
    ProcNode    *process;
    ThreadNode  **cur;
    ThreadNode  *tmp;

    process = FindProcess( processid );
    if( process != NULL ) {
        for( cur = &process->thread; *cur != NULL; cur = &tmp->next ) {
            tmp = *cur;
            if( tmp->threadid == threadid ) {
                *cur = tmp->next;
                freeThreadNode( tmp );
                break;
            }
        }
        if( process->thread == NULL ) {
            RemoveProcess( processid );
        }
        if( procDlg != NULL ) {
            SendMessage( procDlg, DR_TASK_LIST_CHANGE, 0, 0L );
        }
    }
}

/*
 * freeProcNode
 */
static void freeProcNode( ProcNode *node )
{
    ModuleNode  *mod;

    if( node != NULL ) {
        while( node->module != NULL ) {
            mod = node->module;
            node->module = mod->next;
            freeModuleNode( mod );
        }
        CloseHandle( node->prochdl );
        MemFree( node );
    }
}


/*
 * RemoveProcess
 */
void RemoveProcess( DWORD process )
{
    ProcNode    **proc;
    ProcNode    *tmp;
    ThreadNode  *thread;
    ThreadNode  *next_thread;

    for( proc = &procList; *proc != NULL; proc = &tmp->next ) {
        tmp = *proc;
        if( tmp->procid == process ) {
            *proc = tmp->next;
            break;
        }
    }
    if( tmp != NULL ) {
        for( thread  = tmp->thread; thread != NULL; thread = next_thread ) {
            next_thread = thread->next;
            freeThreadNode( thread );
        }
        freeProcNode( tmp );
        if( procDlg != NULL ) {
            SendMessage( procDlg, DR_TASK_LIST_CHANGE, 0, 0L );
        }
    }
}

/*
 * GetNextOwnedProc
 */
ProcNode *GetNextOwnedProc( ProcNode *cur )
{
    if( cur == NULL ) {
        return( procList );
    } else {
        return( cur->next );
    }
}

/*
 * enableProcChoices
 */
static void enableProcChoices( HWND hwnd, BOOL enable )
{
    EnableWindow( GetDlgItem( hwnd, PROCCTL_KILL ), enable );
    EnableWindow( GetDlgItem( hwnd, PROCCTL_THREAD ), enable );
    EnableWindow( GetDlgItem( hwnd, PROCCTL_VIEWMEM ), enable );
    EnableWindow( GetDlgItem( hwnd, PROCCTL_SET_PRIORITY ), enable );
    EnableWindow( GetDlgItem( hwnd, PROCCTL_ATTATCH ), enable );
#ifndef CHICAGO
    EnableWindow( GetDlgItem( hwnd, PROCCTL_MEM ), enable );
#endif
    if( !enable ) {
        SetDlgItemText( hwnd, PROCCTL_PRIORITY, "" );
        SetDlgItemText( hwnd, PROCCTL_PID, "" );
        SetDlgItemText( hwnd, PROCCTL_PATH, "" );
    }
}

/*
 * getProcId
 */
static LRESULT getProcId( HWND hwnd, DWORD id, WPARAM index )
{
    return( SendDlgItemMessage( hwnd, id, LB_GETITEMDATA, index, 0 ) );
}

/*
 * fillProcInfo
 */
static void fillProcInfo( HWND hwnd, char *buf )
{
    DWORD       procid;
    ProcNode    *proc;
    ProcStats   stats;
    LRESULT     index;
    BOOL        error;

    error = FALSE;
    index = SendDlgItemMessage( hwnd, PROCCTL_TASKLIST, LB_GETCURSEL, 0, 0 );
    if( index == LB_ERR ) {
        error = TRUE;
        SetDlgItemText( hwnd, PROCCTL_PID, "" );
    } else {
        procid = getProcId( hwnd, PROCCTL_TASKLIST, (WPARAM)index );
        sprintf( buf, "Pid: %08lX", procid );
        SetDlgItemText( hwnd, PROCCTL_PID, buf );
        proc = FindProcess( procid );
        if( !GetProcessInfo( procid, &stats ) ) {
            error = TRUE;
        }
    }
    if( !error ) {
        RCsprintf( buf, STR_PRIORITY_X, stats.priority );
        SetDlgItemText( hwnd, PROCCTL_PRIORITY, buf );
        RCsprintf( buf, STR_NAME_X, stats.name );
        SetDlgItemText( hwnd, PROCCTL_PATH, buf);
    } else {
        SetDlgItemText( hwnd, PROCCTL_PATH, "" );
        SetDlgItemText( hwnd, PROCCTL_PRIORITY, "" );
    }
}

/*
 * fillTaskListBox
 */
static void fillTaskListBox( HWND hwnd, char *buf )
{
    HWND                lb;
    BOOL                rc;
    LRESULT             curproc;
    LRESULT             topproc;
    LRESULT             tmp;
    int                 topindex;
    int                 index;
    int                 select;
    int                 itemcnt;
    ProcList            info;
    ProcPlace           place;

    RefreshInfo();
    lb = GetDlgItem( hwnd, PROCCTL_TASKLIST );
    index = (int)SendMessage( lb, LB_GETTOPINDEX, 0, 0L );
    if( index == LB_ERR ) {
        topproc = -1;
        curproc = -1;
    } else {
        topproc = SendMessage( lb, LB_GETITEMDATA, index, 0 );
        index = (int)SendMessage( lb, LB_GETCURSEL, 0, 0L );
        curproc = SendMessage( lb, LB_GETITEMDATA, index, 0 );
    }
    SendMessage( lb, LB_RESETCONTENT, 0, 0L );
    select = -1;
    topindex = -1;
    rc = GetNextProcess( &info, &place, TRUE );
    while( rc ) {
        if( FindProcess( info.pid ) != NULL ) {
            sprintf( buf, "* Pid %08lX (%s)", info.pid, info.name );
        } else {
            sprintf( buf, "  Pid %08lX (%s)", info.pid, info.name );
        }
        index = (int)SendMessage( lb, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)buf );
        SendMessage( lb, LB_SETITEMDATA, index, info.pid );
        if( info.pid == curproc ) {
            select = index;
        }
        if( info.pid == topproc ) {
            topindex = index;
        }
        rc = GetNextProcess( &info, &place, FALSE );
    }
    if( select == -1 ) {
        enableProcChoices( hwnd, FALSE );
    } else {
        SendMessage( lb, LB_SETCURSEL, select, 0L );
    }
    /* if the old top item no longer exists choose the one before it */
    if( topindex == -1 && topproc != -1 ) {
        itemcnt = (int)SendMessage( lb, LB_GETCOUNT, 0, 0L );
        for( topindex = 0; topindex < itemcnt; topindex++ ) {
            tmp = SendMessage( lb, LB_GETITEMDATA, topindex, 0 );
            if( tmp > topproc ) {
                topproc--;
                break;
            }
        }
    }
    if( topproc != -1 ) {
        SendMessage( lb, LB_SETTOPINDEX, topindex, 0L );
    }
}

/*
 * ProcPriorityDlg
 */
BOOL CALLBACK ProcPriorityDlg( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    WORD                cmd;
    ProcNode            *ownedinfo;
    ProcPriorityInfo    *info;
    char                buf[100];
    char                *action;
    DWORD               priority;
    BOOL                ret;
    HANDLE              hdl;

    info = (ProcPriorityInfo *)GET_DLGDATA( hwnd );
    switch( msg ) {
    case WM_INITDIALOG:
        info = (ProcPriorityInfo *)lparam;
        if( !GetProcessInfo( info->procid, &info->stats ) ) {
             RCsprintf( buf, STR_CANT_GET_PROC_INFO, info->procid );
             action = AllocRCString( STR_SET_PRIORITY );
             MessageBox( hwnd, buf, action, MB_OK | MB_ICONEXCLAMATION );
             FreeRCString( action );
             SendMessage( hwnd, WM_CLOSE, 0, 0 );
        }
        SET_DLGDATA( hwnd, lparam );
        sprintf( buf, "Pid = %08lX", info->procid );
        SetDlgItemText( hwnd, PRIORITY_INFO, buf );
        sprintf( buf, "(%s)", info->stats.name );
        SetDlgItemText( hwnd, PRIORITY_PATH, buf );
        ownedinfo = FindProcess( info->procid );
        if( ownedinfo == NULL ) {
            hdl = OpenProcess( PROCESS_QUERY_INFORMATION
                               | PROCESS_SET_INFORMATION, FALSE, info->procid );
            if( hdl == NULL ) {
                 RCsprintf( buf, STR_CANT_GET_PROC_HDL, info->procid );
                 action = AllocRCString( STR_SET_PRIORITY );
                 MessageBox( hwnd, buf, action, MB_OK | MB_ICONEXCLAMATION );
                 FreeRCString( action );
                 SendMessage( hwnd, WM_CLOSE, 0, 0 );
            }
            info->hdl = NULL;
        } else {
            hdl = ownedinfo->prochdl;
            info->hdl = hdl;
        }
        priority = GetPriorityClass( hdl );
        if( info->hdl == NULL ) {
            CloseHandle( hdl );
        }
        switch( priority ) {
        case IDLE_PRIORITY_CLASS:
            CheckDlgButton( hwnd, PRIORITY_IDLE, BST_CHECKED );
            break;
        case NORMAL_PRIORITY_CLASS:
            CheckDlgButton( hwnd, PRIORITY_NORMAL, BST_CHECKED );
            break;
        case HIGH_PRIORITY_CLASS:
            CheckDlgButton( hwnd, PRIORITY_HIGHEST, BST_CHECKED );
            break;
        case REALTIME_PRIORITY_CLASS:
            CheckDlgButton( hwnd, PRIORITY_TIME_CRITICAL, BST_CHECKED );
            break;
        }
        break;
    case WM_COMMAND:
         cmd = LOWORD( wparam );
         switch( cmd ) {
         case IDOK:
             if( info->hdl == NULL ) {
                 hdl = OpenProcess( PROCESS_SET_INFORMATION, FALSE, info->procid );
             } else {
                 hdl = info->hdl;
             }
             if( IsDlgButtonChecked( hwnd, PRIORITY_IDLE ) ) {
                 ret = SetPriorityClass( hdl, IDLE_PRIORITY_CLASS );
             } else if( IsDlgButtonChecked( hwnd, PRIORITY_NORMAL ) ) {
                 ret = SetPriorityClass( hdl, NORMAL_PRIORITY_CLASS );
             } else if( IsDlgButtonChecked( hwnd, PRIORITY_HIGHEST ) ) {
                 ret = SetPriorityClass( hdl, HIGH_PRIORITY_CLASS );
             } else if( IsDlgButtonChecked( hwnd, PRIORITY_TIME_CRITICAL ) ) {
                 ret = SetPriorityClass( hdl, REALTIME_PRIORITY_CLASS );
             }
             if( info->hdl == NULL ) {
                 CloseHandle( hdl );
             }
             if( !ret ) {
                 RCsprintf( buf, STR_CANT_SET_PROC_PRI, info->procid );
                 action = AllocRCString( STR_SET_PRIORITY );
                 MessageBox( hwnd, buf, action, MB_OK | MB_ICONEXCLAMATION );
                 FreeRCString( action );
             }
             SendMessage( hwnd, WM_CLOSE, 0, 0L );
             break;
         case IDCANCEL:
             SendMessage( hwnd, WM_CLOSE, 0, 0L );
             break;
         }
        break;
    case WM_CLOSE:
        EndDialog( hwnd, 0 );
        break;
    default:
        return( FALSE );
    }
    return( TRUE );
}

/*
 * AddRunningErrMsg
 */
static void AddRunningErrMsg( void *_info )
{
    ProcAttatchInfo   *info = _info;
    char        buf[100];
    ProcStats   stats;

    if( GetProcessInfo( info->info.pid, &stats ) ) {
        RCsprintf( buf, STR_CANT_ADD_PROCESS, info->info.pid, stats.name );
    } else {
        RCsprintf( buf, STR_CANT_ADD_PROCESS, info->info.pid, "" );
    }
    MessageBox( MainHwnd, buf, AppName, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND );
}

#define ACTION_BUFSIZE          50

/*
 * ProcListProc
 */
BOOL CALLBACK ProcListProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    char                buf[200];
    char                action[ ACTION_BUFSIZE ];
    HWND                lb;
    WORD                cmd;
    LRESULT             index;
    DWORD               procid;
    ProcNode            *procinfo;
    DWORD               rc;
    HANDLE              hdl;

    lparam = lparam;
    switch( msg ) {
    case WM_INITDIALOG:
        procDlg = hwnd;
        fillTaskListBox( hwnd, buf );
        SendDlgItemMessage( hwnd, PROCCTL_TASKLIST, LB_SETCURSEL, 0, 0L );
        index = SendDlgItemMessage( hwnd, PROCCTL_TASKLIST, LB_GETCURSEL, 0, 0L );
        fillProcInfo( hwnd, buf );
        if( index != LB_ERR ) {
            enableProcChoices( hwnd, TRUE );
        }
        break;
    case DR_TASK_LIST_CHANGE:
        RefreshInfo();
        fillTaskListBox( hwnd, buf );
        if( ThreadDlg != NULL ) {
            SendMessage( ThreadDlg, DR_TASK_LIST_CHANGE, 0, 0L );
        }
        break;
    case WM_COMMAND:
        cmd = LOWORD( wparam );
        if( cmd == PROCCTL_THREAD || cmd == PROCCTL_KILL
           || cmd == PROCCTL_VIEWMEM || cmd == PROCCTL_SET_PRIORITY
           || cmd == PROCCTL_ATTATCH || cmd == PROCCTL_MEM ) {
            lb = GetDlgItem( hwnd, PROCCTL_TASKLIST );
            index = SendMessage( lb, LB_GETCURSEL, 0, 0L );
            if( index == LB_ERR ) {
                RCMessageBox( hwnd, STR_NO_SELECTED_PROCESS, AppName, MB_OK | MB_ICONEXCLAMATION );
                break;
            }
            procid = getProcId( hwnd, PROCCTL_TASKLIST, (WPARAM)index );
        }
        switch( cmd ) {
#ifndef CHICAGO
        case PROCCTL_MEM:
            DoMemDlg( hwnd, procid );
            break;
#endif
        case PROCCTL_SET_PRIORITY:
            {
                ProcPriorityInfo        prinfo;

                prinfo.procid = procid;
                JDialogBoxParam( Instance, "PROC_PRIORITY", hwnd, ProcPriorityDlg, (LPARAM)&prinfo );
                fillProcInfo( hwnd, buf );
            }
            break;
        case PROCCTL_ATTATCH:
            {
                if( FindProcess( procid ) != NULL ) {
                    RCsprintf( buf, STR_ALREADY_WATCHING, AppName, procid );
                    CopyRCString( STR_WATCH_PROCESS, action, ACTION_BUFSIZE );
                    MessageBox( hwnd, buf, action, MB_ICONEXCLAMATION | MB_OK );
                    break;
                }
                CallProcCtl( MENU_ADD_RUNNING, &procid, AddRunningErrMsg );
                fillTaskListBox( hwnd, buf );
            }
            break;
        case PROCCTL_THREAD:
            JDialogBoxParam( Instance, "THREAD_CTL", hwnd, ThreadCtlProc, procid );
            break;
        case PROCCTL_KILL:
            procinfo = FindProcess( procid );
            if( procinfo == NULL ) {
                hdl = OpenProcess( PROCESS_TERMINATE, FALSE, procid );
            } else {
                hdl = procinfo->prochdl;
            }
            if( hdl == NULL ) {
                RCsprintf( buf, STR_PROC_NOT_TERMINATED, procid );
                CopyRCString( STR_KILL_PROCESS, action, ACTION_BUFSIZE );
                MessageBox( hwnd, buf, action, MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            if( GetRetCode( hwnd, RETCD_PROCESS, procid, &rc ) ) {
                if( !TerminateProcess( hdl, rc ) ) {
                    RCsprintf( buf, STR_CANT_KILL_PROCESS, procid );
                    CopyRCString( STR_KILL_PROCESS, action, ACTION_BUFSIZE );
                    MessageBox( hwnd, buf, action, MB_OK | MB_ICONEXCLAMATION );
                } else {
                    /* wait awhile so the registry is updated and the
                       process is properly removed from the list box
                       THIS IS A COMPLETE KLUDGE        */
                    Sleep( 1000 );
                }
                if( procinfo == NULL ) {
                    CloseHandle( hdl );
                }
                fillTaskListBox( hwnd, buf );
            }
            break;
        case PROCCTL_VIEWMEM:
            procinfo = FindProcess( procid );
            if( procinfo == NULL ) {
                hdl = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, procid );
            } else {
                DuplicateHandle(
                                GetCurrentProcess(),
                                procinfo->prochdl,
                                GetCurrentProcess(),
                                &hdl,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS );
            }
            if( hdl == NULL ) {
                RCsprintf( buf, STR_CANT_GET_PROC_HDL, procid );
                CopyRCString( STR_VIEW_MEMORY, action, ACTION_BUFSIZE );
                MessageBox( hwnd, buf, action, MB_ICONEXCLAMATION | MB_OK );
                break;
            }
            WalkMemory( hwnd, hdl, procid );
            break;
        case PROCCTL_REFRESH:
            fillTaskListBox( hwnd, buf );
            fillProcInfo( hwnd, buf );
            break;
        case IDOK:
            SendMessage( hwnd, WM_CLOSE, 0, 0L );
            break;
        case PROCCTL_TASKLIST:
            if( HIWORD( wparam ) == LBN_SELCHANGE ) {
                enableProcChoices( hwnd, TRUE );
                fillProcInfo( hwnd, buf );
            }
            break;
        }
        break;
    case WM_CLOSE:
        EndDialog( hwnd, 0 );
        break;
    case WM_DESTROY:
        procDlg = NULL;
        break;
    default:
        return( FALSE );
    }
    return( TRUE );
}

/*
 * DisplayProcList
 */
void DisplayProcList( void )
{
    RefreshInfo();
    JDialogBox( Instance, "PROCCTL", MainHwnd, ProcListProc );
}
