---
-- \file      test/t.lua
-- \brief     test helper functions in t.lua
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local   Test  = require( 't.Test' )
local   Set   = require( 't.Set' )
local   Socket= require( 't.Net.Socket' )
local   T     = require( 't' )
local   t_count = require( 't.Table' ).count

return {
	NormalRequireFails = function( self )
		Test.describe( "require('a.b') relative to file which calls fails" )
		local f    = function(s) local b = require'a.b' end
		local eMsg = "module 'a.b' not found"
		local x,e  = pcall( f, self )
		assert( not x, "Should hav failed" )
		assert( e:match( eMsg ), ("Error massage should contain `%s` but was `%s`"):format( eMsg, e ) )
	end,

	tRequiresRelative = function( self )
		Test.describe( "t.require('a.b') relative to file which calls" )
		local path,cpath = package.path, package.cpath
		local b = T.require'a.b'
		assert( 'table' == type(b), "Required result should be a table" )
		assert( "I live 'a/b.lua'" == b.content, "should match proper phrase" )
		assert( #package.path == #path, "Using T.require polluted the global 'package.path' namespace" )
		assert( #package.cpath == #cpath, "Using T.require polluted the global 'package.cpath' namespace" )
		-- "unload" them so NormalRequireFails() doesn't find them cached
		package.loaded[ 'a.b' ] = nil
		_G[ 'a.b' ] = nil
	end,

	typeNormalObjects = function( self )
		Test.describe( "type( {} ) gives `table`" )
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

	typeLuaObjects = function( self )
		Test.describe( "type( FILE* ) gives `userdata` or `FILE*`" )
		local dir = debug.getinfo( 1, "S" ).short_src:match( "^(.*)/" )
		local f = io.open( dir .. "/t_t.lua", "r")
		assert( 'userdata' ==   type( f ), ("type should be `userdata` but was `%s`"):format(   type(f) ) )
		assert( 'FILE*'    == T.type( f ), ("type should be `FILE*`    but was `%s`"):format( T.type(f) ) )
	end,

	typeLuatObjects = function( self )
		Test.describe( "type( t.Set() ) gives `t.Set` (Lua Table with metatable)" )
		local set = Set()
		assert( 'table' ==   type( set ), ("type should be `table` but was `%s`"):format(   type(set) ) )
		assert( 't.Set' == T.type( set ), ("type should be `t.Set` but was `%s`"):format( T.type(set) ) )
	end,

	typeLuatUserdata = function( self )
		Test.describe( "type( t.Net.Socket() ) gives `t.Net.Socket` (userdata with metatable)" )
		local sock = Socket()
		assert( 'userdata' ==   type( sock ), ("type should be `userdata` but was `%s`"):format( type(sock) ) )
		assert( 'T.Net.Socket'   == T.type( sock ), ("type should be `T.Net.Socket`   but was `%s`"):format( T.type(sock) ) )
	end,

	assertFormatter = function( self )
		Test.describe( "t.assert() should format var args" )
		local f = function(s) T.assert( 5==6, "In no Universe %d should equal %d", 5, 6 ) end
		local x,e = pcall( f, self )
		assert( not x, "call should have failed" )
		assert( e:match( "In no Universe 5 should equal 6"), "Error message should have been properly formatted" )
	end,

}
