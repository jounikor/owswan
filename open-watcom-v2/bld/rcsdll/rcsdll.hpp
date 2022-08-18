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
* Description:  SCM interface library classes.
*
****************************************************************************/


#include "rcsapi.h"


#define BUFLEN      512

#define MAX_RCS_STRING_LEN 10 // including nullchar

class userData;

class rcsSystem {
public:
    rcsSystem() {};
    virtual int init( userData * )      { return( 1 ); };
    virtual int fini()                  { return( 1 ); };
    virtual int hasShell()              { return( 0 ); };
    virtual int runShell()              { return( 0 ); };
    virtual int checkout( userData *, rcsstring name, rcsstring pj = 0, rcsstring tgt = 0 );
    virtual int checkin( userData *, rcsstring name, rcsstring pj = 0, rcsstring tgt = 0 );
    rcsstring checkin_name;
    rcsstring checkout_name;
private:
    rcsstring dllName;
};

/* mks 6.2 */
class mksRcsSystem : public rcsSystem
{
public:
    mksRcsSystem() { checkin_name = "mks_ci"; checkout_name = "mks_co"; };
    int hasShell() { return( 1 ); };
    int runShell() {
#if defined( __WINDOWS__ ) || defined( __NT__ )
        DWORD rc;
        rc = WinExec( (LPSTR)"mksrcs.exe", SW_RESTORE );
        return( rc > 31 );
#else
        return( system( "mksrcs.exe" ) == 0 );
#endif
    };
};
extern mksRcsSystem MksRcs;

/* mks 7.X */
class mksSISystem : public rcsSystem
{
public:
#if defined( __NT__ ) || defined( __WINDOWS__ )
    mksSISystem() { dllId = 0; };
#else
    mksSISystem() {};
#endif
    ~mksSISystem();
    int init( userData * );
    int fini();
    int checkout( userData *, rcsstring name, rcsstring pj=0, rcsstring tgt=0 );
    int checkin( userData *, rcsstring name, rcsstring pj=0, rcsstring tgt=0 );
    int hasShell() { return( 1 ); };
    int runShell();
private:
#if defined( __NT__ ) || defined( __WINDOWS__ )
    HINSTANCE dllId;
#endif
};
extern mksSISystem MksSI;

/* PVCS version manager 5.1 */
class pvcsSystem : public rcsSystem
{
public:
    pvcsSystem() { checkin_name = "pvcs_ci"; checkout_name = "pvcs_co"; };
    int hasShell() { return( 1 ); };
    int runShell() {
#if defined( __WINDOWS__ ) || defined( __NT__ )
        DWORD       rc;
        rc = WinExec( (LPSTR)"pvcswinu.exe", SW_RESTORE );
        return( rc > 31 );
#else
        return( system( "pvcswinu.exe" ) == 0 );
#endif
    };
};
extern pvcsSystem Pvcs;

/* generic rcs */
class genericRcs : public rcsSystem
{
public:
    genericRcs() { checkin_name = "gen_ci"; checkout_name = "gen_co"; };
};
extern genericRcs Generic;

/* Watcom internal rcs system */
class wprojRcs : public rcsSystem
{
public:
    wprojRcs() { checkin_name = "wat_ci"; checkout_name = "wat_co";  };
};
extern wprojRcs Wproj;

/* Object Cycle */
class objectCycleSystem : public rcsSystem
{
public:
#if defined( __NT__ ) || defined( __WINDOWS__ )
    objectCycleSystem() { dllId = 0; };
#else
    objectCycleSystem() {};
#endif
    ~objectCycleSystem();
    int init( userData * );
    int fini();
    int checkout( userData *, rcsstring name, rcsstring pj=0, rcsstring tgt=0 );
    int checkin( userData *, rcsstring name, rcsstring pj=0, rcsstring tgt=0 );
    int hasShell() { return( 1 ); };
    int runShell();
private:
#if defined( __NT__ ) || defined( __WINDOWS__ )
    HINSTANCE dllId;
#endif
};
extern objectCycleSystem ObjCycle;

/* Perforce */
class p4System : public rcsSystem
{
public:
    p4System() { checkin_name = "p4_ci"; checkout_name = "p4_co"; };
    int hasShell() { return( 1 ); };
    int runShell() {
#if defined( __WINDOWS__ ) || defined( __NT__ )
        DWORD       rc;
        rc = WinExec( (LPSTR)"p4win.exe", SW_RESTORE );
        return( rc > 31 );
#else
        return( system( "p4win.exe" ) == 0 );
#endif
    };
};
extern p4System Perforce;

/* Git */
class gitSystem : public rcsSystem
{
public:
    gitSystem() { checkin_name = "git_ci"; checkout_name = "git_co"; };
    int hasShell() { return( 1 ); };
    int runShell() {
#if defined( __WINDOWS__ ) || defined( __NT__ )
        DWORD       rc;
        rc = WinExec( (LPSTR)"git.exe", SW_RESTORE );
        return( rc > 31 );
#else
        return( system( "git.exe" ) == 0 );
#endif
    };
};
extern gitSystem Git;

class userData {
public:
    userData( rcshwnd win, rcsstring cfg ) :
        window(win),batcher(NULL),batch_cookie(NULL),msgBox(NULL),msg_cookie(NULL),currentSystem(NULL),cfgDir(cfg),pause(0) {};
    int regBatcher( BatchCallback *fp, void *c )
        { batcher = fp; batch_cookie=c; return( 1 ); };
    int regMessager( MessageBoxCallback *fp, void *c )
        { msgBox = fp; msg_cookie=c; return( 1 ); };
    int setSystem( rcstype rcs_type );
    rcsstring getCfgDir() { return( cfgDir ); };
    rcsSystem *getSystem() { return( currentSystem ); };
    void setPause( int on ) { if( on ) { pause=1; } else { pause = 0; } };
    int getPause() { return( pause ); };

    rcshwnd                     window;
    BatchCallback               *batcher;
    MessageBoxCallback          *msgBox;
    void                        *batch_cookie;
    void                        *msg_cookie;
private:
    rcsSystem                   *currentSystem;
    rcsstring                   cfgDir;
    int                         pause;
};
