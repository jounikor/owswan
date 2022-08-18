:INCLUDE file='LYTCHG'.
.if &e'&dohelp eq 0 .do begin
:INCLUDE file='WNOHELP'.
.do end
.el .do begin
:INCLUDE file='WHELP'.
.do end
:INCLUDE file='FMTMACRO'.
:INCLUDE file='GMLMACS'.
:INCLUDE file='XDEFS'.
:set symbol="lang"      value="C/C++".
:INCLUDE file='DEFS'.
.*
:GDOC.
.*
:BODY.
.*
.if &e'&dohelp ne 0 .do begin
:exhelp
:include file='&book..idx'
:include file='&book..tbl'
:include file='&book..kw'
.do end
.*
.chap About Watcom Editor On-line Documentation
.*
.if '&lang' eq 'C/C++' .do begin
:HBMP 'cbooks.bmp' i
.do end
.if '&lang' eq 'FORTRAN 77' .do begin
:HBMP 'fbooks.bmp' i
.do end
:INCLUDE file='COPYRITE'.
.*
.cntents end_of_book
:eGDOC.
