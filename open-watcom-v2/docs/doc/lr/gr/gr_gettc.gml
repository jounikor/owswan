.gfunc _gettextcolor
.gsynop begin
.if '&lang' eq 'C' .do begin
short _FAR _gettextcolor( void );
.do end
.el .do begin
integer*2 function _gettextcolor()
.do end
.gsynop end
.desc begin
The
.id &funcb.
&routine returns the pixel value of the current text color.
This is the color used for displaying text with the
.reffunc _outtext
and
.reffunc _outmem
&routines..
The default text color value is set to 7 whenever a new video mode is selected.
.* .pp
.* .im gr_txtco
.desc end
.return begin
The
.id &funcb.
&routine returns the pixel value of the current text color.
.return end
.see begin
.seelist _settextcolor _setcolor _outtext _outmem
.see end
.grexam begin eg_gettc.&langsuff
.grexam end
.class &grfun
.system
