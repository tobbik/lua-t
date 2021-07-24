---
-- \file    t_ael.lua
-- \brief   Test for the Set functionality
local t_assert  = require't'.assert
local Test      = require't.Test'
local Loop      = require't.Loop'
local Socket    = require't.Net.Socket'
local Address   = require't.Net.Address'
local Interface = require't.Net.Interface'


return {
	beforeAll = function( self )
		self.loop = Loop( )
	end,


	afterEach = function( self )  -- not necessary for this suite
		self.loop:clean( );
	end,

	-- -----------------------------------------------------------------------
	-- Timer Tests
	-- -----------------------------------------------------------------------
	Timer = function( self )
		Test.describe( "Test simple Timer" )
		local arg1,arg2  = 100, 'this is a string'
		local start      = Loop.time()
		local success    = function( a, b )
			local ms_passed = Loop:time() - start
			assert( a == arg1, "First argument should be " .. arg1 )
			assert( b == arg2, "Second argument should be " .. arg2 )
			assert( ms_passed > 380 and ms_passed < 390,
				("Time passed should be between 380 and 390 milliseconds, but was %d"):format(ms_passed) )
		end
		self.loop:addTask( 385, success, arg1, arg2 )
		self.loop:run()
	end,

	CancelTask = function( self )
		Test.describe( "Cancel existing and running Task" )
		local tsk1, tsk2 = nil, nil
		local t1 = function( )
			self.loop:cancelTask( tsk2 )
		end
		local t2 = function( )
			assert( false, "This function should not have run since tsk1 cancelled it!" )
		end
		tsk1 = self.loop:addTask( 500 , t1 )
		tsk2 = self.loop:addTask( 1000, t2 )
		self.loop:run( )
	end,

	RepeatTask = function( self )
		Test.describe( "Repeat Task by returning milliseconds from its function" )
		local msg, time, cnt = 'Message1', 1000, 0
		local tr = function( a )
			assert( msg == a, ("Expected `%s`, got `%s`"):format( msg, a ) )
			time = time - 251
			cnt  = cnt  + 1
			return time
		end
		tskr = self.loop:addTask( time , tr, msg )
		self.loop:run()
		assert( cnt == 4, ("Expected to be execute `4`, got `%d`"):format( cnt ) )
	end,

	TaskFunctionReference = function( self )
		Test.describe( "Linked task function is proper" )
		local msg = "the functions argument"
		local tf = function( a )
			print(a)
		end
		local tsk = self.loop:addTask( 1234 , tf, msg )
		local tsk_r = debug.getuservalue( tsk, 2 ) -- 2 == T_AEL_TSK_FNCIDX in t_ael_l.h
		assert( 'table' == type(tsk_r), ("Expected reference to be `%s`, got `%s`"):format( 'table', type(tsk_r) ) )
		assert( tf   == tsk_r[1], ("Expected function to be `%s`, got `%s`"):format( tsk_r[1], tf ) )
		assert( msg  == tsk_r[2], ("Expected argument to be `%s`, got `%s`"):format( tsk_r[2], msg  ) )
	end,

	MultiTimer = function( self )
		Test.describe( "Execute multiple timers in succession" )
		local cnt, m_secs = 0, 50
		local inc = function( ) cnt = cnt+1 end
		local chk = function( )
			t_assert( cnt == 8, "%d timers should have been executed. Counted: %d", 8, cnt )
		end
		self.loop:addTask( 1*m_secs, inc )
		self.loop:addTask( 2*m_secs, inc )
		self.loop:addTask( 3*m_secs, inc )
		self.loop:addTask( 4*m_secs, inc )
		self.loop:addTask( 5*m_secs, inc )
		self.loop:addTask( 6*m_secs, inc )
		self.loop:addTask( 7*m_secs, inc )
		self.loop:addTask( 8*m_secs, inc )
		self.loop:addTask( 9*m_secs, chk )
		self.loop:run()
	end,

	TimerOrder = function( self )
		Test.describe( "Proper timer insertion requires ordering" )
		local order = { 5, 7, 1, 4, 2, 9, 3, 8, 6 }  -- insertion order
		local cnt, m_secs, tasks = 0, 50, { }
		local inc = function( ) cnt = cnt+1 end
		local chk = function( )
			assert( cnt == 9, ("%d timers should have been executed. Counted: %d"):format( 9, cnt ) )
		end
		for _, n in ipairs( order ) do
			tasks[n] = self.loop:addTask( n*m_secs, inc )
		end
		tasks[10] = self.loop:addTask( 10*m_secs, chk )
		for i=1,9 do
			local t_nxt = debug.getuservalue( tasks[ i ], 1 )  -- get the next uservalue
			assert( t_nxt == tasks[ i+1 ], ("Expected next task <%s>, but was <%s>"):format( t_nxt, tasks[1+1] ) )
		end
		self.loop:run()
	end,
}

