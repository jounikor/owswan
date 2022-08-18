.se __idx=0
.se freffnd=0
.se frefid=''
.if &e'&machsys eq 0 .ty ***ERROR*** machsys not defined
.*
.dm fnc begin
.se $$fnd=&'wordpos(&machsys,&*,4)
.if &$$fnd. ne 0 .do begin
.  .se __idx=&__idx.+1
.  .se fnclst(&__idx.)=&*1
.  .se freflst(&__idx.)=&*2
.  .se imblst(&__idx.)=&*3
.  .se __sysl(&__idx.)=0
.do end
.dm fnc end
.*
.* version 10.7 or greater functions
.*
.dm fn7 begin
.if &vermacro ge 1070 .do begin
.  .fnc &*
.do end
.dm fn7 end
.*
.* version 11.0 or greater functions
.*
.dm fn8 begin
.if &vermacro ge 1100 .do begin
.  .fnc &*
.do end
.dm fn8 end
.*
.* create far entries (e.g., _fmemccpy)
.* corresponding to regular entry (e.g., memccpy)
.*
.dm fnf begin
.if &farfnc ne 0 .do begin
.  .fnc &*
.do end
.dm fnf end
.*
.* create a multibyte character entry (e.g., mbscmp)
.* corresponding to regular entry (e.g., strcmp)
.*
.dm fnm begin
.if &vermacro ge 1070 .do begin
.  .fnc &*
.do end
.dm fnm end
.*
.* create a "far" multibyte character entry (e.g., _fmbsbtype)
.* corresponding to regular entry (e.g., _mbsbtype)
.*
.dm fnn begin
.if &farfnc ne 0 .do begin
.if &vermacro ge 1070 .do begin
.  .fnc &*
.do end
.do end
.dm fnn end
.*
.* create a wide character entry (e.g., wcscmp)
.* corresponding to regular entry (e.g., strcmp)
.*
.dm fnw begin
.if &vermacro ge 1070 .do begin
.  .fnc &*
.do end
.dm fnw end
.*
.dm funcref begin
.se freffnd=&'vecpos(&*.,fnclst)
.if &e'&dohelp eq 0 .do begin
.   .se frefid=&*.
.do end
.el .if '&freffnd.' eq '0' .do begin
.* .   .ty *** &*. - referenced but not defined ***
.   .se frefid=&*.
.do end
.el .do begin
.   .se frefid=&freflst(&freffnd.).
.do end
.dm funcref end
.*
.* DOS16 DOS32 WIN16 WIN386 WIN32 QNX16 QNX32 OS216 OS216MT OS216DL OS232 LNX32 RDOS
.* 1     2     4     8      16    32    64    128   256     512     1024  2048  4096
.*
.* DOSPM NET32 MATH  MACRO
.* 8192  16384 32768 65536
.*
.se __name()='DOS16'
.se __name()='DOS32'
.se __name()='WIN16'
.se __name()='WIN386'
.se __name()='WIN32'
.se __name()='QNX16'
.se __name()='QNX32'
.se __name()='OS216'
.se __name()='OS216MT'
.se __name()='OS216DL'
.se __name()='OS232'
.se __name()='LNX32'
.se __name()='RDOS'
.se __name()='DOSPM'
.se __name()='NET32'
.se __name()='MATH'
.se __name()='MACRO'
.*
.se __bits()=0
.se __bits()=1
.se __bits()=2
.se __bits()=4
.se __bits()=8
.se __bits()=16
.se __bits()=32
.se __bits()=64
.se __bits()=128
.se __bits()=256
.se __bits()=512
.se __bits()=1024
.se __bits()=2048
.se __bits()=4096
.se __bits()=8192
.se __bits()=16384
.se __bits()=32768
.se __bits()=65536
.*
.dm sys begin
.se *cnt=0
.se *fnd=&'vecpos(&*1,fnclst)
.if &*fnd. ne 0 .do begin
.  .se *i=2
.  .pe &*0.-1
.  .  .se *cnt=&*cnt+&__bits(&'vecpos(&*&*i.,__name)+1);.se *i=&*i.+1
.*  Everything except Linux, RDOS, DOSPM and Netware is All
.*  (i.e., add DOSPM to total to get new total = NET32-1)
.*  If everything is included but not Netware and DOSPM, make it include DOSPM
.* .if &*cnt. eq (8191-4096) .se *cnt=&*cnt.+4096
.*  If Netware is also included but not DOSPM, make it include DOSPM
.* .if &*cnt. eq (8192+(8191-4096)) .se *cnt=&*cnt.+4096
.  .se __sysl(&*fnd.)=&*cnt.
.do end
.dm sys end
.*
.dm sysstr begin
.se $$str=''
.se *bits=&__sysl(&'vecpos(&*,fnclst))
.if &*bits. ne 0 .do begin
.  .if &*bits. ge 65536 .do begin
.  .  .se $$str=MACRO, &$$str
.  .  .se *bits = &*bits. - 65536
.  .do end
.*  Math functions are on All systems
.  .if &*bits. ge 32768 .do begin
.  .  .se $$str=Math, &$$str
.  .  .se *bits = &*bits. - 32768
.  .do end
.  .if &*bits. ge 16384 .do begin
.  .  .se $$str=Netware, &$$str
.  .  .se *bits = &*bits. - 16384
.  .do end
.  .if &*bits. ge 8192 .do begin
.  .  .se $$str=DOS/PM, &$$str
.  .  .se *bits = &*bits. - 8192
.  .do end
:cmt.  .if &*bits. ge 8191 .do begin
:cmt.  .  .se $$str=All, &$$str
:cmt.  .  .se *bits = &*bits. - 8191
:cmt.  .do end
.  .if &*bits. ge 4096 .do begin
.  .  .se $$str=RDOS, &$$str
.  .  .se *bits = &*bits. - 4096
.  .do end
.  .if &*bits. ge 2048 .do begin
.  .  .se $$str=Linux, &$$str
.  .  .se *bits = &*bits. - 2048
.  .do end
.  .if &*bits. ge 2047 .do begin
.  .  .se $$str=All, &$$str
.  .  .se *bits = &*bits. - 2047
.  .do end
.  .if &*bits. ge 1024 .do begin
.  .  .se $$str=OS/2-32, &$$str
.  .  .se *bits = &*bits. - 1024
.  .do end
.  .if &*bits. ge 512+256+128 .do begin
.  .  .se $$str=OS/2 1.x(all), &$$str
.  .  .se *bits = &*bits. - (512+256+128)
.  .do end
.  .if &*bits. ge 512 .do begin
.  .  .se $$str=OS/2 1.x(DL), &$$str
.  .  .se *bits = &*bits. - 512
.  .do end
.  .if &*bits. ge 256 .do begin
.  .  .se $$str=OS/2 1.x(MT), &$$str
.  .  .se *bits = &*bits. - 256
.  .do end
.  .if &*bits. ge 128 .do begin
.  .  .se $$str=OS/2 1.x, &$$str
.  .  .se *bits = &*bits. - 128
.  .do end
.  .if &*bits. ge 64+32 .do begin
.  .  .if '&machsys' eq 'QNX' .do begin
.  .  .  .se $$str=QNX, &$$str
.  .  .do end
.  .  .se *bits = &*bits. - (64+32)
.  .do end
.  .if &*bits. ge 64 .do begin
.  .  .if '&machsys' eq 'QNX' .do begin
.  .  .  .se $$str=QNX/32, &$$str
.  .  .do end
.  .  .se *bits = &*bits. - 64
.  .do end
.  .if &*bits. ge 32 .do begin
.  .  .if '&machsys' eq 'QNX' .do begin
.  .  .  .se $$str=QNX/16, &$$str
.  .  .do end
.  .  .se *bits = &*bits. - 32
.  .do end
.  .if &*bits. ge 16 .do begin
.  .  .se $$str=Win32, &$$str
.  .  .se *bits = &*bits. - 16
.  .do end
.  .if &*bits. ge 8 .do begin
.  .  .se $$str=Win386, &$$str
.  .  .se *bits = &*bits. - 8
.  .do end
.  .if &*bits. ge 4 .do begin
.  .  .se $$str=Windows, &$$str
.  .  .se *bits = &*bits. - 4
.  .do end
.  .if &*bits. ge 2+1 .do begin
.  .  .se $$str=DOS, &$$str
.  .  .se *bits = &*bits. - (2+1)
.  .do end
.  .if &*bits. ge 2 .do begin
.  .  .se $$str=DOS/32, &$$str
.  .  .se *bits = &*bits. - 2
.  .do end
.  .if &*bits. ge 1 .do begin
.  .  .se $$str=DOS/16, &$$str
.  .  .se *bits = &*bits. - 1
.  .do end
.  .sr $$str="&'strip(&$$str,'T',',')"
.do end
.dm sysstr end
