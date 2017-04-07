#!../out/bin/lua

---
-- \file    t_cb.lua
-- \brief   Test asynchronous tests
local   Test = require('t.Test')
local   Time = require('t.Time')
--local   Loop = require ('t').Loop

local   tests = {
	setUp = function( self, done )
		--self.loop = Loop( 5 )
		--self.loop:run( );
		--done( )
	end,

	tearDown = function( self, done )
		--self.loop:stop( );
		--done( )
	end,

	test_cb_sleep = function( self, done )
		Test.Case.describe( "Test calling done() after 3s sleep" )

		print( 'inside the test case' )
		Time.sleep( 3000 )
		done( )
	end,
}


t = Test( tests )
t( )
--print( t )
