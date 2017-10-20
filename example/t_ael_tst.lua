#!../out/bin/lua

---
-- \file    t_cb.lua
-- \brief   Example of asynchronous tests

local Test  = require('t.Test')
local Time  = require('t.Time')
local Loop  = require('t.Loop')

local tests = {
	beforeAll = function( self, done )
		self.loop = Loop( 5 )
		self.loop:addTimer( Time(1), done )
		self.loop:run( );
	end,

	afterAll = function( self, done )
		self.loop:stop( );
		done( )
	end,

	test_cb_sleep = function( self, done )
		Test.Case.describe( "Test calling done() after 3s sleep" )

		print( 'inside the test case' )
		Time.sleep( 3000 )
		done( )
	end,

	test_cb_timer = function( self, done )
		Test.Case.describe( "Test calling done() after 5s sleep via global loop" )

		print( 'inside the other test case' )
		self.loop:addTimer( Time(5000), done )
		-- this returns right away and lets the next test execute; done() however will
		-- be executed in 5 seconds
	end,
}


t = Test( tests )
t( )
--print( t )
