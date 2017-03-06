#!../out/bin/lua

---
-- \file    t_tst_asy_par.lua
-- \brief   Asynchronous tests in Lua; running all cases at the same time on
--          the loop
T     = require( 't' )
Test  = T.Test
Loop  = T.Loop
Timer = T.Time
t_async = function( self, done )
	Test.Case.describe( 'Running an asynchrounous Timer test' )
	local rTimeMs = math.random(500,3000)
	self.loop:addTimer( 
		Timer( rTimeMs ),
		function( tm )
			print( "Finished Timer with " ..tm.. " milliseconds" )
			done( )
		end,
		rTimeMs
	)
end

t_sync = function( self )
	Test.Case.describe( 'Running a synchrounous test' )
	assert( self.b==self.a*2, "Multiplication equals result" )
	print("TEST SYNC")
end

tbl = {
	beforeAll = function( self, done )
		self.loop    = Loop(20) 
		print("+++++++++++++++++++++++++++++++BEFORE ALL")
		local tm = Timer(1)
		print(tm)
		self.a = 10
		self.b = 20
		self.loop:addTimer( tm, done, self )
		-- loop:run() blocks further execution but the done function here is the
		-- test runner itself.  When it get's executed it will loop over all test
		-- cases which in turn puts all of them on the loop pretty much in parallel
		-- making the timers be worked on in parallel.
		self.loop:run()
	end,
	afterAll = function( self, done )
		self.loop:stop()
		print("-------------------------------AFTER ALL")
		done( self )
	end,
}
for i=1,25 do
	tbl[ 'test_cb_'..i ] = t_async
	tbl[ 'test_'..i ]    = t_sync
end

t = Test( tbl )

t( )
print(t)
