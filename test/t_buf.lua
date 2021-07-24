---
-- \file    test/t_buf.lua
-- \brief   Test for the t.Buffer implementation
local T      = require( "t" )
local Test   = require( "t.Test" )
local Buffer = require( "t.Buffer" )
local Rtvg   = T.require( "rtvg" )

return {
	rtvg       = Rtvg( ),
	beforeEach = function( self )
		local      n = math.random( 1000, 2000 )
		      self.s = self.rtvg:getWords( n )
		      self.b = Buffer( self.s )
	end,

	--afterEach = function( self )  -- not necessary for this suite
	--end,
	ConstantBufsize = function( self )
		Test.describe( "Check for t.Buffer.Size (aka. BUFSIZ) being known in lib" )
		local size   = Buffer.Size
		assert( type(size) == 'number', ("`t.Buffer.Size` should be a `number` but is `%s`"):format( type( size ) ) )
		assert( size > 0 , ("`t.Buffer.Size` should be a greater than 0 but is `%s`"):format( size ) )
	end,

	ConstructorZeroSizedBufferFails = function( self )
		Test.describe( "Create a buffer of 0 length fails" )
		local f   = function(n) local b = Buffer(0) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer size must be greater than 0" ), "Wrong Error message: "..e )
	end,

	ConstructorZeroSizedBufferFailsNew = function( self )
		Test.describe( "Create a buffer of 0 length should return nil and error" )
		Test.skip( "Failure forgivin Buffer Constructor not implemented yet" )
		local b,e = Buffer(0)
		assert( nil == b, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer size must be greater than 0" ), "Wrong Error message: "..e )
	end,

	ConstructorNegativeSizedBufferFails = function( self )
		Test.describe( "Create a buffer of negative length fails" )
		local f   = function(n) local b = Buffer( 0-math.random(1,1000) ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer size must be greater than 0" ), "Wrong Error message: "..e )
	end,

	ConstructorEmptySizedBuffer = function( self )
		Test.describe( "Create an empty buffer of certain length" )
		local  n = math.random( 100, 500 )
		local  b = Buffer( n )
		assert( #b == n, ("Length of t.Buffer should be %d but was %d"):format( n, #b ) )
		for i=1,n do
			assert( b[ i ] == 0,
				"Value in buffer at position "..i.." should be 0" )
		end
	end,

	ConstructorFromString = function( self )
		Test.describe( "T.Buffer constructed from string must match original" )
		local b = Buffer( self.s )
		assert( #b == #self.s, ("Length of t.Buffer should be %d but was %d"):format( #self.s, #b ) )
		for i=1,#self.s do
			assert( b[ i ] == string.byte( self.s, i ),
				("Value in buffer at position %d should be <%s> but was <%s>"):format(
				i,  string.char( string.byte(self.s,i) ), string.char( b[ i ] ) )
			)
		end
	end,

	CopyConstructorFromBuffer = function( self )
		Test.describe( "T.Buffer constructed by copy must match original" )
		local b = Buffer( self.b )
		assert( #b == #self.b, ("Length of t.Buffer should be %d but was %d"):format( #self.b, #b ) )
		for i=1,#self.b do
			assert( b[ i ] == self.b[ i ],
				("Value in buffer at position %d should be <%s> but was <%s>"):format(
				i,  string.char( self.b[i] ), string.char( b[ i ] ) )
			)
		end
	end,

	ConstructorFromSegment = function( self )
		--TODO: occasionally, this code can create a constructor that calls with Length of -1
		--How does that happen?
		Test.describe( "t.Buffer constructed from t.Buffer.Segment copy must match original" )
		local seg = self.b:Segment( 1, math.floor( #self.b/2) )
		local b   = Buffer( seg )
		assert( #b == #seg, ("Length of T.Buffer should be %d but was %d"):format( #seg, #b) )
		assert( b:read() == seg:read(), "Content of Buffer copy should be samme as copy" )
		for i=1,#seg do
			assert( b[ i ] == seg[ i ],
				("Value in buffer at position %d should be <%s> but was <%s>"):format(
				i,  string.char( seg[i] ), string.char( b[ i ] ) )
			)
		end
	end,

	ConstructorBiggerThanString = function( self )
		Test.describe( "t.Buffer( string, #string+1000 ) must have right length and values" )
		local b = Buffer( self.s, #self.s+1000 )
		local n = string.rep( string.char(0), 1000 )
		assert( #b      == #self.s+1000,       ("Length of t.Buffer should be %d but was %d"):format( #self.s+1000, #b ) )
		assert( self.s  == b:read(1, #self.s), ("Content of t.Buffer should be `%s` \n  but was `%s`"):format( self.s, b:read(1, #self.s) ) )
		assert( n       == b:read(#self.s+1),  "Content of t.Buffer after string should be all NULL" )
	end,

	ConstructorShorterThanString = function( self )
		local d   = math.floor( #self.s/4 )
		Test.describe( "t.Buffer( string, #string-%d ) should create shorter buffer", d )
		local b   = Buffer( self.s, #self.s-d )
		local exp = self.s:sub( 1, #self.s-d )
		assert( #b  == #self.s-d, ("Length of t.Buffer should be %d but was %d"):format( #self.s-d, #b ) )
		assert( exp == b:read( ), ("Content of t.Buffer should be `%s` \n  but was `%s`"):format( exp, b:read() ) )
	end,

	ConstructorBiggerThanBuffer = function( self )
		Test.describe( "t.Buffer( buffer, #buffer+1000 ) must have right length and values" )
		local b = Buffer( self.b, #self.b+1000 )
		local n = string.rep( string.char(0), 1000 )
		assert( #b      == #self.b+1000, ( "Length of t.Buffer should be %d but was %d"):format( #self.b+1000, #b ) )
		assert( self.b:read( 1, #self.b ) == b:read(1, #self.s), ("Content of t.Buffer should be `%s` \n  but was `%s`"):format( self.s, b:read(1, #self.s) ) )
		assert( n       == b:read( #self.b+1 ),  "Content of t.Buffer after string should be all NULL" )
	end,

	ConstructorShorterThanBuffer = function( self )
		local d   = math.floor( #self.s/4 )
		Test.describe( "t.Buffer( buffer, #buffer-%d ) should create shorter buffer", d )
		local b   = Buffer( self.b, #self.b-d )
		local exp = self.s:sub( 1, #self.s-d )
		assert( #b  == #self.s-d, ("Length of t.Buffer should be %d but was %d"):format( #self.s-d, #b ) )
		assert( exp == b:read( ), ("Content of t.Buffer should be `%s` \n  but was `%s`"):format( exp, b:read( ) ) )
	end,

	ConstructorPartialFromBuffer = function( self )
		Test.describe( "Buffer( size, buf ) must have right length and values" )
		local b = Buffer( self.b, #self.b+1000 )
		assert( #b == #self.b+1000, ("Length of t.Buffer should be %d but was %d"):format( #self.b+1000, #b ) )
		assert( self.b:read( )  == self.b:read(1, #self.b), ("Content of Buffer should be `%s`\n   but was `%s`"):format( self.b:read(), b:read(1, #self.b) ) )
	end,

	ConstructorPartialFromBufferShortens = function( self )
		Test.describe( "Buffer( #buffer-5, buffer ) shall create shorter buffer" )
		local b = Buffer( self.b, #self.b-5 )
		assert( #b == #self.b - 5, ("Buffer expected to be %d long but was %d"):format( #self.b - 5, #b) )
		assert( b:read() == self.b:read(1, #b), ("Buffer expected to be %s long but was %s"):format( #self.b:read(1, #b), b:read()) )
	end,

	ConstructorPartialFromSegment = function( self )
		Test.describe( "Buffer( seg, size ) must have right length and values" )
		local s = self.b:Segment( math.floor( #self.b/2 ) )
		local b = Buffer( s, #s+123 )
		local p = string.rep( string.char(0), 123 )
		assert( #b == #s+123, ("Length of t.Buffer should be %d but was %d"):format( #s+123, #b ) )
		assert( b:read( )  == s:read( ) .. p, ("Content of Buffer should be `%s`\n   but was `%s`"):format( s:read() .. p, b:read() ) )
	end,

	ConstructorShortPartialFromSegment = function( self )
		Test.describe( "Buffer( buffer:Segment(), #buffer-5 ) creates shorter Buffer" )
		local s      = self.b:Segment( math.floor( #self.b/2 ) )
		local b      = Buffer( s, #s-5 )
		assert( #b == #s-5, ("Length of t.Buffer should be %d but was %d"):format( #s-5, #b ) )
		assert( b:read() == s:read(1, #b), ("t.Buffer should be %s but was %s"):format( s:read(1, #b), b:read() ) )
	end,

	ClearBuffer = function( self )
		Test.describe( "Clearing Buffer sets each byte to 0" )
		for i=1,#self.b do
			assert( string.byte( self.b:read( i, 1 ), 1 ) ~= 0,
				("Value in buffer at position %d should be 0"):format( i ) )
		end
		self.b:clear()
		for i=1,#self.b do
			assert( string.byte( self.b:read( i, 1 ), 1 ) == 0,
				("Value in buffer at position %d should be 0"):format( i ) )
		end
	end,

	ReadLengthBeyondBufferLengthGetEmptyString = function( self )
		Test.describe( "Reading from Buffer past it's length gives empty string" )
		local e = self.b:read( #self.b + 40 )
		assert( "" == e, "Should be empty string but was: '" ..e.. "'" )
		--local f = function(b,n,l) return b:read(n,l) end
		--assert( not pcall( f, self.b, #self.b-100, 200 ),
		--	"Can't read a string longer than buffer" )
	end,

	ReadFull = function( self )
		Test.describe( "Reading full Buffer content gets full string" )
		assert( self.b:read( ) == self.s, "Read data and original string must be equal" )
	end,

	ReadPartial = function( self )
		Test.describe( "Reading partial Buffer content matches string" )
		local starts = math.random( math.floor(#self.s/5), math.floor(#self.s/2 ) )
		local ends   = math.random( starts, #self.s )
		local subS   = self.s:sub( starts, ends )
		self.b:write( subS, starts, ends-starts )
		local readS  = self.b:read( starts, ends-starts+1 )
		assert( #subS == #readS, ("#Substring shall be %d but is %d "):format( #subS, #readS ) )
		assert( subS == readS, "Substrings shall be equal" )
	end,

	WriteFull = function( self )
		Test.describe( "Overwrite entire Buffer content" )
		local s = self.rtvg:getString( #self.s )
		self.b:write( s )
		assert( self.b:read( ) == s, "Read data and new string must be equal" )
	end,

	WritePartial = function( self )
		Test.describe( "Writing partial Buffer content matches string" )
		local starts = math.random( math.floor(#self.s/5), math.floor(#self.s/2 ) )
		local ends   = math.random( starts, #self.s )
		local s      = self.rtvg:getString( ends-starts )
		self.b:write( s, starts )
		local readS  = self.b:read( starts, ends-starts )
		assert( #s == #readS, ("#Substring shall be %d but is %d "):format( #s, #readS ) )
		assert( s  ==  readS, "Substrings shall be equal" )
	end,

	WritePartialString = function( self )
		Test.describe( "Writing partial string to Buffer content matches string" )
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

	Equals = function( self )
		Test.describe( "__eq metamethod properly compares for equality" )
		local b = Buffer( self.b )
		assert( b == self.b, "Original and clone must be equal" )
	end,

	NotEquals = function( self )
		Test.describe( "__eq metamethod properly compares for inequality" )
		local b = Buffer( self.b )
		self.b:write( "2" )
		b:write( "1" )
		assert( b ~= self.b, "Original and clone mustn't be equal" )
	end,

	Add = function( self )
		Test.describe( "__add metamethod properly adds T.Buffers" )
		local sA = self.rtvg:getWords( math.random( 1000, 2000 ) )
		local sB = self.rtvg:getWords( math.random( 1000, 2000 ) )
		local sR = sA .. sB
		local bR = Buffer( sR )
		local bA = Buffer( sA )
		local bB = Buffer( sB )
		local bT = bA + bB
		assert( bT==bR, "Composed buffer should equal buffer from full string" )
	end,

	ToHex = function( self )
		Test.describe( "T.Buffer:toHex() creates a proper hex string" )
		local a = { 65,66,67,68,69,70 }
		local x = string.char( a[1], a[2], a[3], a[4], a[5], a[6] )
		local X = ("%02X %02X %02X %02X %02X %02X"):format(  a[1], a[2], a[3], a[4], a[5], a[6] )
		local b = Buffer( x )
		assert( b:toHex() == X, ("Expected HexString: `%s` but got `%s`"):format( X, b:toHex() ) )
	end
}
