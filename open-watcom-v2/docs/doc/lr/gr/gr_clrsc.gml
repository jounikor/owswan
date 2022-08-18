.gfunc _clearscreen
.gsynop begin
.if '&lang' eq 'C' .do begin
void _FAR _clearscreen( short area );
.do end
.el .do begin
subroutine _clearscreen( area )
integer*2 area
.do end
.gsynop end
.desc begin
The
.id &funcb.
&routine clears the indicated
.arg area
and fills it with the background color.
The
.arg area
argument must be one of the following values:
.begterm 15
.uterm _GCLEARSCREEN
area is entire screen
.uterm _GVIEWPORT
area is current viewport or clip region
.uterm _GWINDOW
area is current text window
.endterm
.desc end
.if '&lang' eq 'C' .do begin
.return begin
The
.id &funcb.
&routine does not return a value.
.return end
.do end
.see begin
.seelist _setbkcolor _setviewport _setcliprgn _settextwindow
.see end
.grexam begin eg_clear.&langsuff
.grexam end
.class &grfun
.system
