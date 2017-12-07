#!../out/bin/lua

---
-- \file    t_buf.lua
-- \brief   Test for the T.Buffer implementation
local T      = require( "t" )
local Test   = require( "t.Test" )
local Buffer = require( "t.Buffer" )
local Rtvg   = T.require( 'rtvg' )
local format = string.format

tests = {
	rtvg       = Rtvg( ),
	beforeEach = function( self )
		local  n = math.random( 1000,2000 )
		self.s   = self.rtvg:getWords( n )
		self.b   = Buffer( self.s )
	end,

	--afterEach = function( self )  -- not necessary for this suite
	--end,
	test_ConstantBufsize = function( self )
		Test.Case.describe( "Check for Buffer.Size (aka. BUFSIZ) being known in lib" )
		local size   = Buffer.Size
		T.assert( type(size) == 'number', "`Buffer.Size` should be a `number` but is `%s`", type( size ) )
		T.assert( size > 0 , "`Buffer.Size` should be a greater than 0 but is `%s`", size )
	end,

	test_ConstructorEmptySizedBuffer = function( self )
		Test.Case.describe( "Create an empty buffer of certain length" )
		local  n = math.random( 100, 500 )
		local  b = Buffer( n )
		assert( #b == n, "Length of T.Buffer should be "..n.." but was " ..#b )
		for i=1,n do
			assert( string.byte( b:read( i, 1 ), 1 ) == 0,
				"Value in buffer at position "..i.." should be 0" )
		end
	end,

	test_CopyConstructor = function( self )
		Test.Case.describe( "T.Buffer constructed by copy must match original" )
		local b = Buffer( self.b )
		assert( #b == #self.b, "Length of T.Buffer should be "..#self.b.." but was " ..#b )
		for i=1,#self.b do
			assert( string.byte(b:read( i,1 ),1) == string.byte( self.s, i ),
				"Value in buffer at position "..i..
				" should be ".. string.char( string.byte(self.s,i) ) ..
				" but was ".. string.char( string.byte(b:read(i, 1),1 ) ) )
		end
	end,

	test_CopyConstructorFromBuffer = function( self )
		Test.Case.describe( "T.Buffer constructed by copy must match original" )
		local b = Buffer( self.b )
		assert( #b == #self.b, "Length of T.Buffer should be "..#self.b.." but was " ..#b )
		for i=1,#self.b do
			assert( string.byte(b:read( i,1 ),1) == string.byte( self.s, i ),
				"Value in buffer at position "..i..
				" should be ".. string.char( string.byte(self.s,i) ) ..
				" but was ".. string.char( string.byte(b:read(i, 1),1 ) ) )
		end
	end,

	test_ConstructorPartialFromString = function( self )
		Test.Case.describe( "Buffer( size, string ) must have right length and values" )
		local b = Buffer( #self.s+1000, self.s )
		assert( #b      == #self.s+1000,      format( "Length of t.Buffer should be %d but was %d", #self.s+1000, #b ) )
		assert( self.s  == b:read(1,#self.s), format( "Content of Buffer should be `%s` \n  but was `%s`", self.s, b:read(1, #self.s) ) )
	end,

	test_ConstructorTooShortPartialFromStringFails = function( self )
		Test.Case.describe( "Buffer( #string-5, string ) shall fail" )
		local errMsg = "size must be at least as big as source to copy from"
		local f      = function( x ) local b = Buffer( #x-5, x ) end
		local r,e    = pcall( f, self.s )
		assert( not r, "Creating Buffer should have failed" )
		assert( e:match( errMsg ), format( "Error message should contain `%s`, but was `%s`", errMsg, e ) )
	end,

	test_ConstructorPartialFromBuffer = function( self )
		Test.Case.describe( "Buffer( size, buf ) must have right length and values" )
		local b = Buffer( #self.b+1000, self.b )
		assert( #b == #self.b+1000, format( "Length of t.Buffer should be %d but was %d", #self.b+1000, #b ) )
		assert( self.b:read( )  == self.b:read(1, #self.b), format( "Content of Buffer should be `%s`\n   but was `%s`", self.b:read(), b:read(1, #self.b) ) )
	end,

	test_ConstructorTooShortPartialFromBufferFails = function( self )
		Test.Case.describe( "Buffer( #buffer-5, buffer ) shall fail" )
		local errMsg = "size must be at least as big as source to copy from"
		local f      = function( x ) local b = Buffer( #x-5, x ) end
		local r,e    = pcall( f, self.b )
		assert( not r, "Creating Buffer should have failed" )
		assert( e:match( errMsg ), format( "Error message should contain `%s`, but was `%s`", errMsg, e ) )
	end,

	test_ConstructorPartialFromSegment = function( self )
		Test.Case.describe( "Buffer( size, seg ) must have right length and values" )
		local s = self.b:Segment( math.floor( #self.b/2 ) )
		local b = Buffer( #s+123, s )
		assert( #b == #s+123, format( "Length of t.Buffer should be %d but was %d", #s+123, #b ) )
		assert( s:read( )  == b:read(1, #s), format( "Content of Buffer should be `%s`\n   but was `%s`", s:read(), b:read(1, #s) ) )
	end,

	test_ConstructorTooShortPartialFromSegmentFails = function( self )
		Test.Case.describe( "Buffer( #buffer-5, buffer:Segment() ) shall fail" )
		local errMsg = "size must be at least as big as source to copy from"
		local s      = self.b:Segment( math.floor( #self.b/2 ) )
		local f      = function( x ) local b = Buffer( #x-5, x ) end
		local r,e    = pcall( f, s )
		assert( not r, "Creating Buffer should have failed" )
		assert( e:match( errMsg ), format( "Error message should contain `%s`, but was `%s`", errMsg, e ) )
	end,

	test_ClearBuffer = function( self )
		Test.Case.describe( "Clearing Buffer sets each byte to 0" )
		for i=1,#self.b do
			assert( string.byte( self.b:read( i, 1 ), 1 ) ~= 0,
				"Value in buffer at position "..i.. " should be 0" )
		end
		self.b:clear()
		for i=1,#self.b do
			assert( string.byte( self.b:read( i, 1 ), 1 ) == 0,
				"Value in buffer at position "..i.. " should be 0" )
		end
	end,

	test_ReadLengthBeyondBufferLengthFails = function( self )
		Test.Case.describe( "Reading from Buffer past it's length fails" )
		local f = function(b,n,l) return b:read(n,l) end
		assert( not pcall( f, self.b, #self.b-100, 200 ),
			"Can't read a string longer than buffer" )
	end,

	test_ReadFull = function( self )
		Test.Case.describe( "Reading full Buffer content gets full string" )
		assert( self.b:read( ) == self.s, "Read data and original string must be equal" )
	end,

	test_ReadPartial = function( self )
		Test.Case.describe( "Reading partial Buffer content matches string" )
		local starts = math.random( math.floor(#self.s/5), math.floor(#self.s/2 ) )
		local ends   = math.random( starts, #self.s )
		local subS   = self.s:sub( starts, ends )
		self.b:write( subS, starts, ends-starts )
		local readS  = self.b:read( starts, ends-starts+1 )
		assert( #subS == #readS, "#Substring shall be "..#subS.." but is ".. #readS )
		assert( subS == readS, "Substrings shall be equal" )
	end,

	test_WriteFull = function( self )
		Test.Case.describe( "Overwrite entire Buffer content" )
		local s = self.rtvg:getString( #self.s )
		self.b:write( s )
		assert( self.b:read( ) == s, "Read data and new string must be equal" )
	end,

	test_WritePartial = function( self )
		Test.Case.describe( "Writing partial Buffer content matches string" )
		local starts = math.random( math.floor(#self.s/5), math.floor(#self.s/2 ) )
		local ends   = math.random( starts, #self.s )
		local s      = self.rtvg:getString( ends-starts )
		self.b:write( s, starts )
		local readS  = self.b:read( starts, ends-starts )
		assert( #s == #readS, "#Substring shall be "..#s.." but is ".. #readS )
		assert( s  ==  readS, "Substrings shall be equal" )
	end,

	test_WritePartialString = function( self )
		Test.Case.describe( "Writing partial string to Buffer content matches string" )
		local starts = math.random( math.floor(#self.b/5), math.floor(#self.b/2 ) )
		local ends   = math.random( starts, starts+math.floor(#self.b/3) )
		local s      = self.rtvg:getString( ends-starts )
		self.b:write( s, starts, #s )
		local readS  = self.b:read( starts, #s )
		assert( self.s:sub(1,starts-1) == self.b:read(1,starts-1),
			"First part of Buffer shall equal original string" )
		assert( s == self.b:read(starts, #s),
			"Overwritten part of Buffer shall equal new string" )
	end,

	test_Equals = function( self )
		Test.Case.describe( "__eq metamethod properly compares for equality" )
		local b = Buffer( self.b )
		assert( b == self.b, "Original and clone must be equal" )
	end,

	test_NotEquals = function( self )
		Test.Case.describe( "__eq metamethod properly compares for inequality" )
		local b = Buffer( self.b )
		self.b:write( "2" )
		b:write( "1" )
		assert( b ~= self.b, "Original and clone mustn't be equal" )
	end,

	test_Add = function( self )
		Test.Case.describe( "__add metamethod properly adds T.Buffers" )
		local sA = self.rtvg:getWords( math.random( 1000, 2000 ) )
		local sB = self.rtvg:getWords( math.random( 1000, 2000 ) )
		local sR = sA .. sB
		local bR = Buffer( sR )
		local bA = Buffer( sA )
		local bB = Buffer( sB )
		local bT = bA + bB
		assert( bT==bR, "Composed buffer should equal buffer from full string" )
	end,

	test_toHex = function( self )
		local a = { 65,66,67,68,69,70 }
		local x = string.char( a[1], a[2], a[3], a[4], a[5], a[6] )
		local X = format( '%02X %02X %02X %02X %02X %02X',  a[1], a[2], a[3], a[4], a[5], a[6] )
		local b = Buffer( x )
		assert( b:toHex() == X, format( "Expected HexString: `%s` but got `%s`", X, b:toHex() ) )
	end
}

-- t = Test( tests )
-- t( "WritePartialString" )
-- print( t )

return Test( tests )
