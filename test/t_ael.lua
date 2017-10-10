#!../out/bin/lua

---
-- \file    t_ael.lua
-- \brief   Test for the Set functionality
local t_assert  =   require"t".assert
local   Test    = require't.Test'
local   Loop    = require't.Loop'
local   Time    = require't.Time'
local Socket    = require't.Net.Socket'
local Address   = require't.Net.Address'
local Interface = require't.Net.Interface'

local dR = function()
	local d = debug.getregistry()
	for i=#d,3,-1 do
		if type(d[i]) =='table' then
			print( i, "TABLE", d[i] )
			for k,v in pairs(d[i]) do print('',k,v) end
		else
			print( i, "VALUE", d[i] )
		end
	end
	print("-----------------------------------------")
end

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
		local tm, tmx    = Time( 1500 ), Time( )
		local success    = function(a,b)
			tmx:since()
			local ms_passed = tmx:get()
			assert( a == arg1, "First argument should be " .. arg1 )
			assert( b == arg2, "Second argument should be " .. arg2 )
			assert( ms_passed > 1400 and ms_passed < 1600,
				"Time passed should be between 4500 and 5500 milliseconds" )
			done()
		end
		self.loop:addTimer( tm, success, arg1, arg2 )
	end,

	test_cb_OverwriteReadHandler = function( self, done )
		-- This test was written to dissect the debug.getregistry() and make sure
		-- that overwriting the handler does call luaL_unref( L, ... ) and no
		-- memory is leaked
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
		local f1  = function( sck )
			--dR()
			t_assert( rSck==sck, "Socket should be `%s` but was `%s`", rSck, sck )
			self.loop:addHandle( sck, 'r', f2, sck )
			sSck:send('bar', adr)
		end
		self.loop:addHandle( rSck, 'r', f1, rSck )
		sSck:send('foo', adr)
	end,

	test_cb_OverwriteTimerHandler = function( self, done )
		Test.Case.describe( "Overwrite Timer callback Handler" )
		local tm = Time( 1000 )
		local r1 = false
		local f1 = function( a )
			assert( false, "This function should not ran and have been replaced" )
		end
		local f2 = function( b )
			assert( true, "This is okay" )
			done( )
		end
		self.loop:addTimer( tm, f1, "First Adding of timer" )
		--dR()
		self.loop:addTimer( tm, f2, "Second Adding of timer" )
		--dR()
	end,

}

return Test( tests )
