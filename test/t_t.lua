---
-- \file      test/t.lua
-- \brief     test helper functions in t.lua
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local   Test  = require( 't.Test' )
local   Time  = require( 't.Time' )
local   Set   = require( 't.Set' )
local   T     = require( 't' )
local   t_count = require( 't.Table' ).count

local tc = {
	test_NormalRequireFails = function( self )
		Test.Case.describe( "require('a.b') relative to file which calls fails" )
		local f    = function(s) local b = require'a.b' end
		local eMsg = "module 'a.b' not found"
		local x,e  = pcall( f, self )
		assert( not x, "Should hav failed" )
		T.assert( e:match( eMsg ), "Error massage should contain `%s` but was `%s`", eMsg, e )
	end,

	test_tRequiresRelative = function( self )
		Test.Case.describe( "t.require('a.b') relative to file which calls" )
		local b = T.require'a.b'
		assert( 'table' == type(b), "Required result should be a table" )
		assert( "I live 'a/b.lua'" == b.content, "should match proper phrase" )
		-- "unload" them so test_NormalRequireFails() doesn't find them cached
		package.loaded[ 'a.b' ] = nil
		_G[ 'a.b' ] = nil
	end,

	test_typeNormalObjects = function( self )
		Test.Case.describe( "type( {} ) gives `table`" )
		local tbl = {}
		assert( 'table' ==   type( tbl ), "type should be table" )
		assert( 'table' == T.type( tbl ), "type should be table" )
		local _mt = {}
		setmetatable( tbl, _mt )
		assert( 'table' ==   type( tbl ), "type should be table" )
		assert( 'table' == T.type( tbl ), "type should be table" )
		_mt.__name = "THING"
		assert( 'table' ==   type( tbl ), "type should be table" )
		assert( 'THING' == T.type( tbl ), "type should be table" )
	end,

	test_typeLuaObjects = function( self )
		Test.Case.describe( "type( FILE* ) gives `userdata` or `FILE*`" )
		local dir = debug.getinfo( 1, "S" ).short_src:match( "^(.*)/" )
		local f = io.open( dir .. "/t_t.lua", "r")
		T.assert( 'userdata' ==   type( f ), "type should be `userdata` but was `%s`",   type(f) )
		T.assert( 'FILE*'    == T.type( f ), "type should be `FILE*`    but was `%s`", T.type(f) )
	end,

	test_typeLuatObjects = function( self )
		Test.Case.describe( "type( t.Set() ) gives `t.Set` (Lua Table with metatable)" )
		local set = Set()
		T.assert( 'table' ==   type( set ), "type should be `table` but was `%s`",   type(set) )
		T.assert( 't.Set' == T.type( set ), "type should be `t.Set` but was `%s`", T.type(set) )
	end,

	test_typeLuatUserdata = function( self )
		Test.Case.describe( "type( t.Time() ) gives `t.Time` (userdata with metatable)" )
		local tim = Time()
		T.assert( 'userdata' ==   type( tim ), "type should be `userdata` but was `%s`",   type(tim) )
		T.assert( 'T.Time'   == T.type( tim ), "type should be `T.Time`   but was `%s`", T.type(tim) )
	end,

	test_assertFormatter = function( self )
		Test.Case.describe( "t.assert() should format var args" )
		local f = function(s) T.assert( 5==6, "In no Universe %d should equal %d", 5, 6 ) end
		local x,e = pcall( f, self )
		assert( not x, "call should have failed" )
		assert( e:match( "In no Universe 5 should equal 6"), "Error message should have been properly formatted" )
	end,

}

-- The tests in tc will be executed in random order
return Test( tc )
