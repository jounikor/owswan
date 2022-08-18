.gfuncw _getcurrentposition
.gsynop begin
.if '&lang' eq 'C' .do begin
struct xycoord _FAR _getcurrentposition( void );

struct _wxycoord _FAR _getcurrentposition_w( void );
.do end
.el .do begin
record /xycoord/ function _getcurrentposition()

record /_wxycoord/ function _getcurrentposition_w()
.do end
.gsynop end
.desc begin
The
.id &funcb.
&routines return the current
output position for graphics.
The
.id &funcb.
&routine returns the point in view coordinates.
The
.id &func2.
&routine returns the point in window coordinates.
.pp
The current position defaults to the origin,
.coord 0 0 ,
when a new video
mode is selected.
It is changed
by successful calls to the
.reffunc _arc
.ct ,
.reffunc _moveto
and
.reffunc _lineto
&routines as well as the
.reffunc _setviewport
&routine..
.pp
Note that the output position for graphics output differs from that
for text output.
The output position for text output can be set by use of the
.reffunc _settextposition
&routine..
.desc end
.return begin
The
.id &funcb.
&routines return the current output
position for graphics.
.return end
.see begin
.seelist _moveto _settextposition
.see end
.grexam begin eg_getcp.&langsuff
.grexam end
.class &grfun
.system
