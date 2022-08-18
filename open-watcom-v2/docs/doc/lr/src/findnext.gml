.func _findnext _findnexti64 _wfindnext _wfindnexti64
.synop begin
#include <&iohdr>
int _findnext( intptr_t handle, struct _finddata_t *fileinfo );
.ixfunc2 '&DosFunc' &_func
int _findnexti64( intptr_t handle, struct _finddatai64_t *fileinfo );
.ixfunc2 '&DosFunc' &func64
.if &'length(&wfunc.) ne 0 .do begin
int _wfindnext( intptr_t handle, struct _wfinddata_t *fileinfo );
.ixfunc2 '&DosFunc' &wfunc
.ixfunc2 '&Wide' &wfunc
int _wfindnexti64( intptr_t handle, struct _wfinddatai64_t *fileinfo );
.ixfunc2 '&DosFunc' &wfunc64
.ixfunc2 '&Wide' &wfunc64
.do end
.synop end
.desc begin
The
.id &_func.
function returns information on the next file whose name
matches the
.arg filespec
argument that was specified in a call to the
.reffunc _findfirst
function.
The
.arg handle
argument was returned by the
.reffunc _findfirst
function.
The information is returned in a
.kw _finddata_t
structure pointed to by
.arg fileinfo
.period
.millust begin
struct _finddata_t {
    unsigned    attrib;
    time_t      time_create;     /* -1 for FAT file systems */
    time_t      time_access;     /* -1 for FAT file systems */
    time_t      time_write;
    _fsize_t    size;
    char        name[_MAX_PATH];
};
.millust end
.np
The
.id &func64
function returns information on the next file whose name
matches the
.arg filespec
argument that was specified in a call to the
.reffunc _findfirsti64
function.
It differs from the
.id &_func.
function in that it returns a 64-bit file
size.
The
.arg handle
argument was returned by the
.reffunc _findfirsti64
function.
The information is returned in a
.kw _finddatai64_t
structure pointed to by
.arg fileinfo
.period
.millust begin
struct _finddatai64_t {
    unsigned    attrib;
    time_t      time_create;     /* -1 for FAT file systems */
    time_t      time_access;     /* -1 for FAT file systems */
    time_t      time_write;
    __int64     size;            /* 64-bit size info        */
    char        name[_MAX_PATH];
};
.millust end
.widefunc &wfunc. &funcb.
.if &'length(&wfunc.) ne 0 .do begin
.millust begin
struct _wfinddata_t {
    unsigned    attrib;
    time_t      time_create;     /* -1 for FAT file systems */
    time_t      time_access;     /* -1 for FAT file systems */
    time_t      time_write;
    _fsize_t    size;
    wchar_t     name[_MAX_PATH];
};
.millust end
.np
The wide character
.id &wfunc64
function is similar to the
.id &func64
function but operates on wide character strings.
It differs from the
.id &wfunc.
function in that it returns a 64-bit file
size.
.millust begin
struct _wfinddatai64_t {
    unsigned    attrib;
    time_t      time_create;     /* -1 for FAT file systems */
    time_t      time_access;     /* -1 for FAT file systems */
    time_t      time_write;
    __int64     size;            /* 64-bit size info        */
    wchar_t     name[_MAX_PATH];
};
.millust end
.do end
.desc end
.return begin
If successful,
.id &_func.
returns 0; otherwise,
.id &_func.
and returns &minus.1
and sets
.kw errno
to one of the following values:
.begterm
.termhd1 Constant
.termhd2 Meaning
.term ENOENT
No matching files
.endterm
.return end
.see begin
.seelist _dos_find&grpsfx
.seelist _findclose _findfirst _findnext closedir opendir readdir
.see end
.exmp begin
#include <stdio.h>
#include <&iohdr>

void main()
  {
    struct _finddata_t  fileinfo;
    intptr_t            handle;
    int                 rc;
.exmp break
    /* Display name and size of "*.c" files */
    handle = _findfirst( "*.c", &fileinfo );
    rc = handle;
    while( rc != -1 ) {
      printf( "%14s %10ld\n", fileinfo.name,
                              fileinfo.size );
      rc = _findnext( handle, &fileinfo );
    }
    _findclose( handle );
  }
.exmp end
.listnew Classification:
DOS
.listend
.system
