/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2011-2013 The Open Watcom Contributors. All Rights Reserved.
* Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
* Copyright (c) 1987-1992 Rational Systems, Incorporated. All Rights Reserved.
*
* =========================================================================
*
* Description:  loader interface
*
****************************************************************************/

typedef ACTION_RETURN   CDECL_FAR16 loader_init_fn(void);
typedef ACTION_RETURN   CDECL_FAR16 loader_load_fn(FDORNAME filename,ULONG,TSF32 FarPtr tspv,LONG FarPtr main_cookie,char FarPtr cmdline);
typedef ACTION_RETURN   CDECL_FAR16 loader_rel_fn(Fptr32 FarPtr fptrp, LONG current_cookie);
typedef ACTION_RETURN   CDECL_FAR16 loader_unrel_fn(Fptr32 FarPtr fptrp, LONG current_cookie);
typedef ACTION_RETURN   CDECL_FAR16 loader_unload_fn(LONG FarPtr main_cookie);
typedef ACTION_RETURN   CDECL_FAR16 loader_freemap_fn(void);   // ???
typedef ACTION_RETURN   CDECL_FAR16 loader_canload_fn(FDORNAME filename,ULONG);
typedef ACTION_RETURN   CDECL_FAR16 loader_getloadtable_fn(long FarPtr FarPtr loadtable);
typedef ACTION_RETURN   CDECL_FAR16 loader_getloadname_fn(long FarPtr loadtable_entry,char FarPtr entry_name,ULONG name_len);

#define LOADER_INIT(lv)         ((loader_init_fn *)(lv)->loader_actions[0])
#define LOADER_LOAD(lv)         ((loader_load_fn *)(lv)->loader_actions[1])
#define LOADER_REL(lv)          ((loader_rel_fn *)(lv)->loader_actions[2])
#define LOADER_UNREL(lv)        ((loader_unrel_fn *)(lv)->loader_actions[3])
#define LOADER_UNLOAD(lv)       ((loader_unload_fn *)(lv)->loader_actions[4])
#define LOADER_FREEMAP(lv)      ((loader_freemap_fn *)(lv)->loader_actions[5])
#define LOADER_CANLOAD(lv)      ((loader_canload_fn *)(lv)->loader_actions[6])
#define LOADER_GETLOADTABLE(lv) ((loader_getloadtable_fn *)(lv)->loader_actions[7])
#define LOADER_GETLOADNAME(lv)  ((loader_getloadname_fn *)(lv)->loader_actions[8])

typedef struct loader_vector {
    PACKAGE FarPtr  loader_package;
    ACTION          *loader_actions[9];
} LOADER_VECTOR;

typedef long LoaderCookie;

extern int  loader_bind( char FarPtr package_name, LOADER_VECTOR *lv );
