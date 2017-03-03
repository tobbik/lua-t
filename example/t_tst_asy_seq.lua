#!../out/bin/lua

---
-- \file    t_tst_asy.lua
-- \brief   Asynchronous tests in lua
T     = require ('t')
Test  = T.Test
Loop  = T.Loop
Timer = T.Time
t_async = function( self, done )
	Test.describe( 'Running an asynchrounous Timer test' )
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
	Test.describe( 'Running a synchrounous test' )
	assert( self.b==self.a*2, "Multiplication equals result" )
	print("TEST SYNC")
end

tbl = {
	beforeAll = function( self, done )
		print("BEFOREALL")
		self.loop    = Loop(20)
		done( self )
	end,
	
	beforeEach_cb = function( self, done )
		print("BEFOREEACH_CB")
		self.loop:addTimer( Timer(1), done, self )
		-- loop:run() blocks further execution until the function on the loop
		-- runs the afterEach_cb and releases the block forcing all tests to be
		-- executed sequentially
		self.loop:run()
	end,
	
	afterEach_cb = function( self, done )
		print("AFTEREACH_CB")
		self.loop:stop()
		done( )
	end,
}
for i=1,25 do
	tbl[ 'test_cb_'..i ] = t_async
	tbl[ 'test_'..i ]    = t_sync
end

t = Test( tbl )

t( )
print(t)
