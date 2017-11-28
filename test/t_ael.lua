#!../out/bin/lua

---
-- \file    t_ael.lua
-- \brief   Test for the Set functionality
local t_assert  = require"t".assert
local Test      = require't.Test'
local Loop      = require't.Loop'
local Time      = require't.Time'
local Socket    = require't.Net.Socket'
local Address   = require't.Net.Address'
local Interface = require't.Net.Interface'


local   tests = {
	beforeAll = function( self, done )
		self.loop = Loop( 5 )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Time(1), done )
		self.loop:run( );
	end,

	afterEach_cb = function( self, done )  -- not necessary for this suite
		self.loop:stop( );
		done()
	end,

	-- -----------------------------------------------------------------------
	-- Timer Tests
	-- -----------------------------------------------------------------------
	test_cb_Timer = function( self, done )
		Test.Case.describe( "Test simple Timer" )
		local arg1,arg2  = 100, 'this is a string'
		local tm, tmx    = Time( 200 ), Time( )
		local success    = function(a,b)
			tmx:since()
			local ms_passed = tmx:get()
			assert( a == arg1, "First argument should be " .. arg1 )
			assert( b == arg2, "Second argument should be " .. arg2 )
			assert( ms_passed > 150 and ms_passed < 250,
				"Time passed should be between 4500 and 5500 milliseconds" )
			done()
		end
		self.loop:addTimer( tm, success, arg1, arg2 )
	end,

	test_cb_OverwriteReadHandler = function( self, done )
		Test.Case.describe( "Overwrite Socket readHandler" )
		local adr  = Address( Interface( 'default' ).AF_INET.address.ip, 4000 )
		local rSck = Socket( 'udp' )
		local sSck = Socket( 'udp' )
		rSck:bind( adr )
		local f2  = function( sck )
			--dR()
			t_assert( rSck==sck, "Socket should be `%s` but was `%s`", rSck, sck )
			self.loop:removeHandle( sck, 'r' )
			rSck:close( )
			sSck:close( )
			done( )
		end
		local f1
		f1 = function( sck )
			--dR()
			t_assert( rSck==sck, "Socket should be `%s` but was `%s`", rSck, sck )
			t_assert( self.loop[sck].read[1] == f1,
			    "old function should be `%s` but was `%s`", f1, self.loop[sck].read[1] )
			self.loop:addHandle( sck, 'r', f2, sck )
			t_assert( self.loop[sck].read[1] == f2,
			    "new function should be `%s` but was `%s`", f2, self.loop[sck].read[1] )
			sSck:send( 'bar', adr )
		end
		self.loop:addHandle( rSck, 'r', f1, rSck )
		sSck:send('foo', adr)
	end,

	test_cb_OverwriteTimerHandler = function( self, done )
		Test.Case.describe( "Overwrite Timer callback Handler" )
		local tm = Time( 123 )
		local r1 = false
		local m1, m2 = 'Message1', 'Message2'
		local f1 = function( a )
			t_assert( m1 == a, "Expected `%s`, got `%s`", m1, a )
			assert( false, "This function should not ran and have been replaced" )
		end
		local f2 = function( b )
			t_assert( m2 == b, "Expected `%s`, got `%s`", m2, b )
			assert( true, "This is okay" )
			done( )
		end
		self.loop:addTimer( tm, f1, m1 )
		t_assert( self.loop[tm][1] == f1,
			    "function should be `%s` but was `%s`", f1, self.loop[tm ][1] )
		self.loop:addTimer( tm, f2, m2 )
		t_assert( self.loop[tm][1] == f2,
			    "new function should be `%s` but was `%s`", f2, self.loop[tm ][1] )
	end,

	test_cb_replaceTimerParameter = function( self, done )
		--Test.Case.skip( "" )
		Test.Case.describe( "Overwrite Timer callback Handlers parameter" )
		local args
		local msg1, msg2 = 'Message1', 'Message2'
		local msg = msg1
		local tm  = Time( 111 )
		local f1  = function( m )
			t_assert( m == msg, "Expected `%s`, got `%s`", msg, m )
			msg     = msg2
			args[2] = msg2
			if m == msg1 then
				tm:set( 99 )
				return tm
			else
				done( )
			end
		end
		self.loop:addTimer( tm, f1, msg1 )
		args = self.loop[ tm ]
	end,

	test_cb_replaceTimerFunction = function( self, done )
		--Test.Case.skip( "" )
		Test.Case.describe( "Overwrite Timer callback Handlers function" )
		local pickle
		local tbl = { }    -- a table reference only equal to itself
		local tm  = Time( 111 )
		local f2  = function( arg )
			t_assert( true, "Should execute")
			t_assert( tbl==arg, "arg expected `%s`; got `%s`", tbl, arg)
			done( )
		end
		local f1  = function( arg )
			t_assert( true, "Should execute")
			t_assert( tbl==arg, "arg expected `%s`; got `%s`", tbl, arg)
			pickle[ 1 ] = f2 -- overwrite function but not argument
			tm:set( 99 )
			return tm
		end
		self.loop:addTimer( tm, f1, tbl )
		pickle = self.loop[ tm ] -- { f1, tbl }
	end,

	test_cb_MultiTimer = function( self, done )
		Test.Case.describe( "Execute multiple timers in succession" )
		local cnt, m_secs = 0,50
		local inc = function( ) cnt = cnt+1 end
		local chk = function( )
			t_assert( cnt == 8, "%d timers should have been executed. Counted: %d", 8, cnt )
			done( )
		end
		self.loop:addTimer( Time(1*m_secs), inc )
		self.loop:addTimer( Time(2*m_secs), inc )
		self.loop:addTimer( Time(3*m_secs), inc )
		self.loop:addTimer( Time(4*m_secs), inc )
		self.loop:addTimer( Time(5*m_secs), inc )
		self.loop:addTimer( Time(6*m_secs), inc )
		self.loop:addTimer( Time(7*m_secs), inc )
		self.loop:addTimer( Time(8*m_secs), inc )
		self.loop:addTimer( Time(9*m_secs), chk )
	end,

}

return Test( tests )
