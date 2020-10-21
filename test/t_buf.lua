#!../out/bin/lua

---
-- \file    t_buf.lua
-- \brief   Test for the t.Buffer implementation
local T      = require( "t" )
local Test   = require( "t.Test" )
local Buffer = require( "t.Buffer" )
local Rtvg   = T.require( 'rtvg' )
local format = string.format

tests = {
	rtvg       = Rtvg( ),
	beforeEach = function( self )
		local      n = math.random( 1000, 2000 )
		      self.s = self.rtvg:getWords( n )
		      self.b = Buffer( self.s )
	end,

	--afterEach = function( self )  -- not necessary for this suite
	--end,
	test_ConstantBufsize = function( self )
		Test.Case.describe( "Check for t.Buffer.Size (aka. BUFSIZ) being known in lib" )
		local size   = Buffer.Size
		T.assert( type(size) == 'number', "`t.Buffer.Size` should be a `number` but is `%s`", type( size ) )
		T.assert( size > 0 , "`t.Buffer.Size` should be a greater than 0 but is `%s`", size )
	end,

	test_ConstructorZeroSizedBufferFails = function( self )
		Test.Case.describe( "Create a buffer of 0 length fails" )
		local f   = function(n) local b = Buffer(0) end
		local r,e = pcall( f, self )
		print(e)
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer size must be greater than 0" ), "Wrong Error message: "..e )
	end,

	test_ConstructorNegativeSizedBufferFails = function( self )
		Test.Case.describe( "Create a buffer of negative length fails" )
		local f   = function(n) local b = Buffer( 0-math.random(1,1000) ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer size must be greater than 0" ), "Wrong Error message: "..e )
	end,

	test_ConstructorEmptySizedBuffer = function( self )
		Test.Case.describe( "Create an empty buffer of certain length" )
		local  n = math.random( 100, 500 )
		local  b = Buffer( n )
		assert( #b == n, "Length of t.Buffer should be "..n.." but was " ..#b )
		for i=1,n do
			assert( b[ i ] == 0,
				"Value in buffer at position "..i.." should be 0" )
		end
	end,

	test_ConstructorFromString = function( self )
		Test.Case.describe( "T.Buffer constructed from string must match original" )
		local b = Buffer( self.s )
		assert( #b == #self.s, "Length of T.Buffer should be "..#self.s.." but was " ..#b )
		for i=1,#self.s do
			assert( b[ i ] == string.byte( self.s, i ),
				"Value in buffer at position "..i..
				" should be ".. string.char( string.byte(self.s,i) ) ..
				" but was ".. string.char( b[ i ] ) )
		end
	end,

	test_CopyConstructorFromBuffer = function( self )
		Test.Case.describe( "T.Buffer constructed by copy must match original" )
		local b = Buffer( self.b )
		assert( #b == #self.b, "Length of T.Buffer should be "..#self.b.." but was " ..#b )
		for i=1,#self.b do
			assert( b[ i ] == self.b[ i ],
				"Value in buffer at position "..i..
				" should be ".. string.char( self.b[ i ] ) ..
				" but was ".. string.char( b[ i ] ) )
		end
	end,

	test_ConstructorFromSegment = function( self )
		--TODO: this code can create a constructor tha calls with Length of -1
		--How do we create a Segment with that lengths?
		Test.Case.describe( "t.Buffer constructed from t.Buffer.Segment copy must match original" )
		local seg = self.b:Segment( 1, math.floor( #self.b/2) )
		local b   = Buffer( seg )
		assert( #b == #seg, "Length of T.Buffer should be "..#seg.." but was " ..#b )
		assert( b:read() == seg:read(), "Content of Buffer copy should be samme as copy" )
		for i=1,#seg do
			assert( b[ i ] == seg[ i ],
				"Value in buffer at position "..i..
				" should be ".. string.char( seg[ i ] ) ..
				" but was ".. string.char( b[ i ] )
			)
		end
	end,

	test_ConstructorBiggerThanString = function( self )
		Test.Case.describe( "t.Buffer( string, #string+1000 ) must have right length and values" )
		local b = Buffer( self.s, #self.s+1000 )
		local n = string.rep( string.char(0), 1000 )
		assert( #b      == #self.s+1000,       format( "Length of t.Buffer should be %d but was %d", #self.s+1000, #b ) )
		assert( self.s  == b:read(1, #self.s), format( "Content of t.Buffer should be `%s` \n  but was `%s`", self.s, b:read(1, #self.s) ) )
		assert( n       == b:read(#self.s+1),  "Content of t.Buffer after string should be all NULL" )
	end,

	test_ConstructorShorterThanString = function( self )
		Test.Case.describe( "t.Buffer( string, #string-234 ) should create shorter buffer" )
		local b   = Buffer( self.s, #self.s-234 )
		local exp = self.s:sub( 1, #self.s-234 )
		assert( #b  == #self.s-234,  format( "Length of t.Buffer should be %d but was %d", #self.s-234, #b ) )
		assert( exp == b:read(),     format( "Content of t.Buffer should be `%s` \n  but was `%s`", exp, b:read() ) )
	end,

	test_ConstructorBiggerThanBuffer = function( self )
		Test.Case.describe( "t.Buffer( buffer, #buffer+1000 ) must have right length and values" )
		local b = Buffer( self.b, #self.b+1000 )
		local n = string.rep( string.char(0), 1000 )
		assert( #b      == #self.b+1000,       format( "Length of t.Buffer should be %d but was %d", #self.b+1000, #b ) )
		assert( self.b:read( 1, #self.b ) == b:read(1, #self.s), format( "Content of t.Buffer should be `%s` \n  but was `%s`", self.s, b:read(1, #self.s) ) )
		assert( n       == b:read( #self.b+1 ),  "Content of t.Buffer after string should be all NULL" )
	end,

	test_ConstructorShorterThanBuffer = function( self )
		Test.Case.describe( "t.Buffer( buffer, #buffer-234 ) should create shorter buffer" )
		local b   = Buffer( self.b, #self.b-234 )
		local exp = self.s:sub( 1, #self.s-234 )
		assert( #b  == #self.s-234,  format( "Length of t.Buffer should be %d but was %d", #self.s-234, #b ) )
		assert( exp == b:read(),     format( "Content of t.Buffer should be `%s` \n  but was `%s`", exp, b:read() ) )
	end,

	test_ConstructorPartialFromBuffer = function( self )
		Test.Case.describe( "Buffer( size, buf ) must have right length and values" )
		local b = Buffer( self.b, #self.b+1000 )
		assert( #b == #self.b+1000, format( "Length of t.Buffer should be %d but was %d", #self.b+1000, #b ) )
		assert( self.b:read( )  == self.b:read(1, #self.b), format( "Content of Buffer should be `%s`\n   but was `%s`", self.b:read(), b:read(1, #self.b) ) )
	end,

	test_ConstructorPartialFromBufferShortens = function( self )
		Test.Case.describe( "Buffer( #buffer-5, buffer ) shall create shorter buffer" )
		local b = Buffer( self.b, #self.b-5 )
		assert( #b == #self.b - 5, format("Buffer expected to be %d long but was %d", #self.b - 5, #b) )
		assert( b:read() == self.b:read(1, #b), format("Buffer expected to be %s long but was %s", #self.b:read(1, #b), b:read()) )
	end,

	test_ConstructorPartialFromSegment = function( self )
		Test.Case.describe( "Buffer( seg, size ) must have right length and values" )
		local s = self.b:Segment( math.floor( #self.b/2 ) )
		local b = Buffer( s, #s+123 )
		local p = string.rep( string.char(0), 123 )
		assert( #b == #s+123, format( "Length of t.Buffer should be %d but was %d", #s+123, #b ) )
		assert( b:read( )  == s:read( ) .. p, format( "Content of Buffer should be `%s`\n   but was `%s`", s:read() .. p, b:read() ) )
	end,

	test_ConstructorShortPartialFromSegment = function( self )
		Test.Case.describe( "Buffer( buffer:Segment(), #buffer-5 ) creates shorter Buffer" )
		local s      = self.b:Segment( math.floor( #self.b/2 ) )
		local b      = Buffer( s, #s-5 )
		assert( #b == #s-5, format( "Length of t.Buffer should be %d but was %d", #s-5, #b ) )
		assert( b:read() == s:read(1, #b), format( "t.Buffer should be %s but was %s", s:read(1, #b), b:read() ) )
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

	test_ReadLengthBeyondBufferLengthGetEmptyString = function( self )
		Test.Case.describe( "Reading from Buffer past it's length gives empty string" )
		local e = self.b:read( #self.b + 40 )
		assert( "" == e, "Should be empty string but was: '" ..e.. "'" )
		--local f = function(b,n,l) return b:read(n,l) end
		--assert( not pcall( f, self.b, #self.b-100, 200 ),
		--	"Can't read a string longer than buffer" )
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
