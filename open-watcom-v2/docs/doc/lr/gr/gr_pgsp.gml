.gfunc _pg_setpalette
.gsynop begin
.if '&lang' eq 'C' .do begin
short _FAR _pg_setpalette( paletteentry _FAR *pal );
.do end
.el .do begin
integer*2 function _pg_setpalette( pal )
record /paletteentry/ pal(*)
.do end
.gsynop end
.desc begin
The
.id &funcb.
&routine sets the internal palette of the presentation
graphics system.
The palette controls the colors, line styles, fill patterns
and plot characters used to display each series of data in a chart.
.pp
The argument
.arg pal
is an array of palette structures containing the new palette.
Each element of the palette is a structure containing the following fields:
.begterm 15
.termnx color
color used to display series
.termnx style
line style used for line and scatter charts
.termnx fill
fill pattern used to fill interior of bar and pie sections
.termnx plotchar
character plotted on line and scatter charts
.endterm
.desc end
.return begin
The
.id &funcb.
&routine returns zero
if successful; otherwise, a non-zero value is returned.
.return end
.see begin
.seelist _pg_defaultchart _pg_initchart _pg_chart _pg_chartpie _pg_chartscatter _pg_getpalette _pg_resetpalette
.see end
.grexam begin eg_pggp.&langsuff
.grexam end
.class &grfun
.system
