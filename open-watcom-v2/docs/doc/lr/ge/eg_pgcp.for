	include 'graphapi.fi'
	include 'graph.fi'
	include 'pgapi.fi'
	include 'pg.fi'

	integer NUM_VALUES
	parameter (NUM_VALUES=4)

	integer*4 categories( NUM_VALUES )
	real values( NUM_VALUES )
     +		    / 20, 45, 30, 25 /
	integer*2 explode( NUM_VALUES )
     +		    / 1, 0, 0, 0 /

	record /chartenv/ env

	categories( 1 ) = loc( 'Jan'c )
	categories( 2 ) = loc( 'Feb'c )
	categories( 3 ) = loc( 'Mar'c )
	categories( 4 ) = loc( 'Apr'c )

	call _setvideomode( _VRES16COLOR )
	call _pg_initchart()
	call _pg_defaultchart( env,
     +			_PG_PIECHART, _PG_NOPERCENT )
	env.maintitle.title = 'Pie Chart'c
	call _pg_chartpie( env, categories,
     +			values, explode, NUM_VALUES )
	pause
	call _setvideomode( _DEFAULTMODE )
	end
