#!../out/bin/lua

---
-- \file    t_buf.lua
-- \brief   Test for the T.Buffer implementation
T      = require( "t" )
Test   = require( "t.Test" )
Buffer = require( "t.Buffer" )
Rtvg   = T.require( 'rtvg' )

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

	test_ConstructorFromString = function( self )
		Test.Case.describe( "T.Buffer constructed from String must have right length and values" )
		local s = self.rtvg:getWords( math.random( 1000, 2000 ) )
		local b = Buffer( s )
		assert( #b == #s, "Length of T.Buffer should be "..#s.." but was " ..#b )
		for i=1,#b do
			assert( string.byte(b:read( i, 1 ),1) == string.byte( s, i ),
				"Value in buffer at position "..i..
				" should be ".. string.char( string.byte( s, i ) ) ..
				" but was ".. string.char( string.byte(b:read( i, 1 ),1 ) ) )
		end
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

	test_ReadLengthBeyondBufferLength = function( self )
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
}

return Test( tests )
