.gfunc _gettextposition
.gsynop begin
.if '&lang' eq 'C' .do begin
struct rccoord _FAR _gettextposition( void );
.do end
.el .do begin
record /rccoord/ function _gettextposition()
.do end
.gsynop end
.desc begin
The
.id &funcb.
&routine returns the current output position for text.
This position is in terms of characters, not pixels.
.pp
The current position defaults to the top left corner of the screen,
.coord 1 1 ,
when a new video mode is selected.
It is changed by successful calls to the
.reffunc _outtext
.ct ,
.reffunc _outmem
.ct ,
.reffunc _settextposition
and
.reffunc _settextwindow
&routines..
.pp
Note that the output position for graphics output differs from that
for text output.
The output position for graphics output can be set by use of the
.reffunc _moveto
&routine..
.desc end
.return begin
The
.id &funcb.
&routine returns, as an
.id rccoord
structure, the current output position for text.
.return end
.see begin
.seelist _outtext _outmem _settextposition _settextwindow _moveto
.see end
.grexam begin eg_gettp.&langsuff
.grexam end
.class &grfun
.system
