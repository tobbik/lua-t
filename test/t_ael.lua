-- vim: ts=3 sw=3 sts=3 tw=80 sta noet list
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
	EpochTimeReasonable = function( self )
		Test.describe( "Make sure os.time() and t.Loop.time() are in their ballparks" )
		local os_epoch, t_epoch_ms = os.time(),Loop.time()
		local delta  = (t_epoch_ms//1000) - os_epoch
		assert( math.abs(delta) < 2, ("os and lua-t time should not differ more than %ds but did %ds"):format(1,delta) )
	end,

	EpochTimeAndMonoTime = function( self )
		Test.describe( "Make sure and t.Loop.timeepoch() and t.Loop.timemonotonic() are in their ballparks" )
		local m_s,e_s,delay = Loop.timemonotonic(), Loop.timeepoch(), math.random(600,1500)
		Loop.sleep(delay)
		local m_e,e_e = Loop.timemonotonic(), Loop.timeepoch()
		local m_j,e_j = (m_e-m_s) - delay, (e_e - e_s) - delay
		Test.info("Delay: %d -- Mono: %d(%d) -- Epoch: %d(%d) -- Jitter diff: %d", delay, m_e-m_s, m_j, e_e-e_s, e_j, e_j-m_j)
		--print("Delay:", delay, "Mono:",m_e-m_s, m_j,"Epoch:",e_e-e_s,e_j, "Jitterdiff:", e_j-m_j)
		assert( math.abs(m_j) < 4, ("monotonic sleep jitter should be less than %dms but was %dms"):format(4,m_j) )
		assert( math.abs(e_j) < 4, ("epoch sleep jitter should be less than %dms but was %dms"):format(4,e_j) )
		assert( math.abs(e_j-m_j) < 3,
			("epoch and monotonic sleep jitter shouldn't differ more than %dms but did %dms"):format(3,math.abs(e_j-m_j)) )
	end,

	SingleTimerAccuracy = function( self )
		-- seems kqueue or select on FreeBSD has some margin here, epoll is
		-- generally okay with 2-3ms sometimes less. FreeBSD ran in a VM
		-- though
		local delay,start, delta_max = math.random(800,1900), Loop.time(), 50
		Test.describe( "Test simple Timer(%dms) execution, less than %dms jitter", delay, delta_max )
		local success    = function( s, d, x )
			local now  = Loop:time()
			assert( d == delay, ("First argument should be %d but was %d"):format( delay, d ) )
			assert( s == start, ("Second argument should be %d but was %d"):format( start, s ) )
			assert( x == delta_max, ("Third argument should be %d but was %d"):format( delta_max, x ) )
			local delay_actual  = now - start
			local jitter         = delay_actual - delay
			Test.info("Jitter expected < %dms, actual: %dms", delta_max, jitter)
			assert( math.abs(jitter) < delta_max, ("Jitter should be less than %dms , but was %dms"):format(delta_max, jitter) )
		end
		local task = self.loop:addTask( delay, success, start, delay, delta_max )
		self.loop:run( )
	end,

	BulkTimerAccuracy = function( self )
		Test.describe( "Test Average Timer Accuracy" )
		--Test.todo( "This needs some more dev testing before we can set expectations" )
		Test.skip( "This needs some more dev testing before we can set expectations" )
		-- 5950X linux 2-4ms     sd: <1
		-- FreeBSD KVM 4-6ms     sd: >3
		-- RasPi       30-100ms  sd: 40-60
		-- It seems when compiling with select() on linux the precision is much
		-- higher, seems epoll() is messing up ... why?
		local runs,delta,sd,vari,start = 50, 0, 0, {}, 0
		local success = function( wanted )
			local ms_passed = Loop:time() - start
			local diff = ms_passed - wanted
			delta = delta + diff
			Test.info( ("Wanted: %d -- Actual: %d  -- Difference: %d -- Diff  percent: %f" ):format(wanted, ms_passed, diff, (100*diff)/wanted) )
			table.insert( vari, diff )
		end
		for n=1,runs do
			local delay = math.random(n*5, n*50)
			self.loop:addTask( delay, success, delay )
		end
		start = Loop.time()
		self.loop:run( )
		local mean = delta/runs
		for i,d in ipairs(vari) do sd = sd + ((d-mean) ^ 2) end
		sd = math.sqrt( sd/runs )
		Test.info( "Average delta over %d runs was %fms with standard deviation of %f", runs, mean, sd )
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

