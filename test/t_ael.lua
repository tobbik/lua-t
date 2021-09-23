---
-- \file    t_ael.lua
-- \brief   Test for the asynchronous Loop
local Test      = require't.Test'
local Loop      = require't.Loop'

-- TODO: ael should test some socket operation as well, even though a lot is
-- covered in the t_net_* tests
--local Socket    = require't.Net.Socket'
--local Address   = require't.Net.Address'
--local Interface = require't.Net.Interface'

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
		local delay,start = math.random(80,600), Loop.time()
		-- seems kqueue or select on FreeBSD has some margin here, epoll is
		-- generally okay with 2-3ms sometimes less. FreeBSD ran in a VM
		-- though
		local delta       = 50
		Test.describe( "Test simple Timer(%dms)", delay )
		local success    = function( s, d )
			local ms_passed = Loop:time() - start
			assert( d == delay, ("First argument should be %d but was %d"):format( delay, d ) )
			assert( d == delay, ("Second argument should be %d but was %d"):format( start, s ) )
			assert( ms_passed > delay-delta and ms_passed < delay+delta,
				("Time passed should be between %dms and %dms, but was %dms"):format(delay-delta, delay+delta, ms_passed) )
		end
		self.loop:addTask( delay, success, start, delay )
		self.loop:run( )
	end,

	TimerAccuracy = function( self )
		Test.describe( "Test Average Timer Accuracy" )
		Test.todo( "This needs some more dev testing befor we can set expectations" )
		-- 5950X linux 2-4ms sd: <1
		-- FreeBSD KVM 4-6ms sd: >3
		-- RasiPy      30-100ms  sd: 40-60
		local runs,delta,sd,vari,start = 200, 0, 0, {}, Loop.time()
		local success = function( s, d )
			local ms_passed = Loop:time() - start
			delta = delta + (ms_passed-d)
			table.insert( vari, ms_passed-d )
		end
		for n=1,runs do
			local delay = math.random(n*8, n*20)
			self.loop:addTask( delay, success, start, delay )
		end
		self.loop:run( )
		local mean = delta/runs
		for i,d in ipairs(vari) do sd = sd + ((d-mean) ^ 2) end
		sd = math.sqrt( sd/runs )
		print( ("Average delta over %d runs was %fms with standard deviation of %f"):format( runs, mean, sd ) )
		assert( delta/runs < 20, ("Average delta over %d runs was %fms, hoping for less than 20"):format( runs, delta/runs ) )
	end,

	CancelTask = function( self )
		local tsk1, tsk2, time = nil, nil, math.random(100,500)
		Test.describe( "Cancel existing from a running Task after %dms", time )
		local t1 = function( )
			self.loop:cancelTask( tsk2 )
		end
		local t2 = function( )
			assert( false, "This function should not have run since tsk1 cancelled it!" )
		end
		tsk1 = self.loop:addTask( time, t1 )
		tsk2 = self.loop:addTask( time*2, t2 )
		self.loop:run( )
	end,

	RepeatTask = function( self )
		local time, cnt = math.random(200,1000), 0
		Test.describe( "Repeat Task by returning milliseconds(%dms) from its function", time )
		local tr = function( diff )
			time = time - diff
			cnt  = cnt  + 1
			return time
		end
		tskr = self.loop:addTask( time, tr, math.ceil(time/4) )
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
		local cnt, time = 0, math.random(10,80)
		Test.describe( "Execute multiple timers in succession after %dms", time )
		local inc = function( ) cnt = cnt+1 end
		local chk = function( )
			assert( cnt == 8, ("%d timers should have been executed. Counted: %d"):format( 8, cnt ) )
		end
		self.loop:addTask( 1*time, inc )
		self.loop:addTask( 2*time, inc )
		self.loop:addTask( 3*time, inc )
		self.loop:addTask( 4*time, inc )
		self.loop:addTask( 5*time, inc )
		self.loop:addTask( 6*time, inc )
		self.loop:addTask( 7*time, inc )
		self.loop:addTask( 8*time, inc )
		self.loop:addTask( 9*time, chk )
		self.loop:run()
	end,

	TimerOrder = function( self )
		Test.describe( "Proper timer insertion requires ordering" )
		local order = { 5, 7, 1, 4, 2, 9, 3, 8, 6 }  -- insertion order
		local cnt, time, tasks = 0, math.random(10,60), { }
		local inc = function( ) cnt = cnt+1 end
		local chk = function( )
			assert( cnt == 9, ("%d timers should have been executed. Counted: %d"):format( 9, cnt ) )
		end
		for _, n in ipairs( order ) do
			tasks[n] = self.loop:addTask( n*time, inc )
		end
		tasks[#order+1] = self.loop:addTask( (#order+1)*time, chk )
		for i=1,#order do
			local t_nxt = debug.getuservalue( tasks[ i ], 1 )  -- get the next uservalue
			assert( t_nxt == tasks[ i+1 ], ("Expected next task <%s>, but was <%s>"):format( t_nxt, tasks[1+1] ) )
		end
		self.loop:run()
	end,
}

