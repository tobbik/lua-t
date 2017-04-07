#!../out/bin/lua

---
-- \file    t_buf_seg.lua
-- \brief   Test for the T.Buffer.Segment implementation
--          The tests are largely the same as T.Buffer since the interface
--          between the two classes is unified to provide usage across T.Net
--          and T.Pack
T       = require( "t" )
Test    = require( "t.Test" )
Buffer  = require( "t.Buffer" )
Segment = Buffer.Segment
Rtvg    = T.require( 'rtvg' )
--math.randomseed(os.time())

local tests = {
	rtvg       = Rtvg( ),
	beforeEach = function( self )
		local  n = math.random( 1000,2000 )
		self.s   = self.rtvg:getWords( n )
		self.b   = Buffer( self.s )
		local  l = #self.s
		local st = math.random( math.floor(l/6), math.floor(l/2 ) )
		local ed = math.random( st, l )
		while ed==st do ed=math.random(st,l) end -- that's a very weird bug
		self.seg = Segment( self.b, st, ed-st )
	end,

	--afterEach = function( self )  -- not necessary for this suite
	--end,

	test_ConstructorFullSegment = function( self )
		Test.Case.describe( "Create a segment covering the entire buffer" )
		local seg         = Segment( self.b )
		local buf,ofs,len = seg:getBuffer()
		assert( #seg == #self.b, "Length of T.Buffer.Segment should be "..#self.b.." but was " ..#seg )
		assert( #seg == len,     "Length of T.Buffer.Segment should be "..#seg.." but was " ..len )
		assert( ofs  == 1,       "Offset of T.Buffer.Segment should be 1 but was " ..ofs )
		assert( seg:read() == self.b:read(),
			"Content of T.Buffer.Segment should be "..self.b:read().." but was "..seg:read() )
	end,

	test_ConstructorPartialSegment = function( self )
		local buf,ofs,len = self.seg:getBuffer()
		Test.Case.describe( "Create a segment starting at "..ofs.." byte of "..tostring(self.b) )
		local seg = Segment( self.b, ofs )
		assert( #seg == #self.b - ofs + 1, "Length of T.Buffer.Segment should be "..(#self.b-ofs+1).." but was "..#seg )
		assert( seg:read() == self.b:read( ofs ),
			"Content of T.Buffer.Segment should be "..self.b:read(ofs ).." but was " ..seg:read() )
	end,

	test_ConstructorPartialLengthSegment = function( self )
		local buf,ofs,len = self.seg:getBuffer()
		ofs,len = math.floor( ofs + len/3 ), math.ceil( len/2 )
		Test.Case.describe( "Create a "..len.." bytes long segment starting at "..ofs.." byte of "..tostring(self.b) )
		local seg = Segment( self.b, ofs, len )
		assert( #seg == len, "Length of T.Buffer.Segment should be "..len.." but was " ..#seg )
		assert( seg:read() == self.b:read( ofs, len ),
			"Content of T.Buffer.Segment should be "..self.b:read(ofs, len ).." but was " ..seg:read() )
	end,

	test_ConstructorPartialSegmentIndexOne = function( self )
		Test.Case.describe( "Segment starting at index 1 should equal Segment covering full Buffer" )
		local seg1 = Segment( self.b, 1 )
		local seg  = Segment( self.b )
		assert( #seg1 == #seg, "Length of T.Buffer.Segment should be "..#seg.." but was " ..#seg1 )
		assert( seg1  ==  seg, "T.Buffer.Segments should be eqyual" )
		assert( seg1:read() == seg:read(),
			"Content of T.Buffer.Segment should be "..seg:read( ).." but was " ..seg1:read() )
	end,

	test_ConstructorOffsetOverflowFails = function( self )
		Test.Case.describe( "Creating Segment with offset higher then #buffer fails" )
		local f   = function(s) local seg = Segment( s.b, #s.b+5 ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "Offset relative to length of T.Buffer out of bound" ), "Wrong Error message: "..e )
	end,

	test_ConstructorLengthOverflowFails = function( self )
		Test.Case.describe( "Creating Segment with length higher then #buffer fails" )
		local f   = function(s) local seg = Segment( s.b, 1, #s.b+5 ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer.Segment length out of bound" ), "Wrong Error message: "..e )
	end,

	test_ConstructorOffsetPlusLengthOverflowFails = function( self )
		Test.Case.describe( "Creating Segment with offset+length higher then #buffer fails" )
		local f   = function(s) local seg = Segment( s.b, #s.b-5, 10 ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer.Segment length out of bound" ), "Wrong Error message: "..e )
	end,
	
	test_ConstructorZeroPartialLengthSegmentStart = function( self )
		Test.Case.describe( "Create a 0 bytes long segment starting wherever" )
		local seg         = Segment( self.b, 1, 0 )
		local buf,idx,len = seg:getBuffer( )
		assert( #seg == 0, "Length of T.Buffer.Segment should be "..len.." but was " ..#seg )
		assert( seg:read() == self.b:read( idx, len ),
			"Content of T.Buffer.Segment should be "..self.b:read(idx, len ).." but was " ..seg:read() )
	end,
	
	test_ConstructorZeroPartialLengthSegmentMiddle = function( self )
		local starts = math.random( math.floor(#self.seg/3), #self.b - math.floor(#self.seg/3 ) )
		Test.Case.describe( "Create a 0 bytes long segment starting wherever" )
		local seg         = Segment( self.b, start, 0 )
		local buf,idx,len = seg:getBuffer( )
		assert( #seg == 0, "Length of T.Buffer.Segment should be "..len.." but was " ..#seg )
		assert( seg:read() == self.b:read( idx, len ),
			"Content of T.Buffer.Segment should be "..self.b:read(idx, len ).." but was " ..seg:read() )
	end,

	test_ConstructorZeroPartialLengthSegmentStart = function( self )
		Test.Case.describe( "Create a 0 bytes long segment starting wherever" )
		local seg         = Segment( self.b, #self.b, 0 )
		local buf,idx,len = seg:getBuffer( )
		assert( #seg == 0, "Length of T.Buffer.Segment should be "..len.." but was " ..#seg )
		assert( seg:read() == self.b:read( idx, len ),
			"Content of T.Buffer.Segment should be "..self.b:read(idx, len ).." but was " ..seg:read() )
	end,

	test_ClearBufferSegment = function( self )
		Test.Case.describe( "Clearing Buffer.Segment sets each byte to 0" )
		local buf,ofs,len = self.seg:getBuffer()
		for i=1,#self.seg do
			assert( string.byte( self.seg:read( i, 1 ), 1 ) ~= 0,
				"Value in segment at position "..i.. " should not be 0" )
		end
		self.seg:clear()
		for i=1,#self.seg do
			assert( string.byte( self.seg:read( i, 1 ), 1 ) == 0,
				"Value in segment at position "..i.. " should be 0" )
		end
		for i=ofs,#self.seg do
			assert( string.byte( self.b:read( i, 1 ), 1 ) == 0,
				"Value in buffer at position "..i.. " should be 0" )
		end
		-- test that the remaining buffer is untouched
		for i=1,ofs-1 do
			assert( string.byte( self.b:read( i, 1 ), 1 ) ~= 0,
				"Value in buffer at position "..i.. " should not be 0" )
		end
		for i=ofs+len,#self.b do
			assert( string.byte( self.b:read( i, 1 ), 1 ) ~= 0,
				"Value in buffer at position "..i.. " should not be 0" )
		end
	end,

	test_ReadIndexOutOfRange = function( self )
		Test.Case.describe( "Reading from Buffer.Segment past it's length fails" )
		local f = function(bs,n) return bs:read( n+2 ) end
		assert( not pcall( f, self.seg, #self.seg ),
			"Can't read from Buffer.Segment on index past it's length" )
	end,

	test_ReadLengthBeyondSegmentLength = function( self )
		Test.Case.describe( "Reading from Buffer beyond it's length fails" )
		local f = function(s,n,l) return s:read(n,l) end
		assert( not pcall( f, self.seg, #self.seg-100, 200 ),
			"Can't read a string longer than Buffer.Segment" )
	end,

	test_ReadFullSegment = function( self )
		Test.Case.describe( "Reading full Buffer.Segment content gets full string" )
		local buf,ofs,len = self.seg:getBuffer()
		assert( self.seg:read( ) == self.b:read( ofs, len ),
			"Read data from segment and buffer must be equal" )
	end,

	test_ReadPartialSegment = function( self )
		Test.Case.describe( "Reading partial Buffer.Segment content matches string" )
		local buf,ofs,len = self.seg:getBuffer()
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, #self.seg )
		local subS   = self.s:sub( starts+ofs-1, ends+ofs-1 )
		local readS  = self.seg:read( starts, ends-starts+1 )
		assert( #subS == #readS, "#Substring shall be "..#subS.." but is ".. #readS )
		assert( subS == readS,  "Substring shall be "..subS.." but is ".. readS )
	end,

	test_WriteFullSegment = function( self )
		Test.Case.describe( "Overwrite entire Buffer Segment content" )
		local buf,ofs,len = self.seg:getBuffer()
		local s = self.rtvg:getString( #self.seg )
		local before = buf:read( 1, ofs-1 )
		local after  = buf:read( ofs+len  )
		self.seg:write( s )
		assert( self.seg:read( )        == s, "Read data and new string must be equal" )
		assert( self.b:read( ofs, len ) == s, "Data in Buffer within segment must be equal" )
		assert( self.b:read( 1, ofs-1 ) == before, "Data in Buffer before segment must be untouched" )
		assert( self.b:read( ofs+len )  == after, "Data in Buffer after segment must be untouched" )
	end,

	test_WritePartial = function( self )
		Test.Case.describe( "Writing partial Buffer Segment" )
		local buf,ofs,len = self.seg:getBuffer()
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, #self.seg-3 )
		local s      = self.rtvg:getString( ends-starts )
		self.seg:write( s, starts )
		local readS  = self.seg:read( starts, ends-starts )
		assert( #s == #readS, "#Substring shall be "..#s.." but is ".. #readS )
		assert( s  ==  readS, "Substrings shall be equal" )
		assert( self.b:read( ofs+starts-1, #s ) == s, "Data in Buffer within segment must be equal" )
	end,

	test_WritePartialString = function( self )
		Test.Case.describe( "Writing partial string to Buffer content matches string" )
		local buf,ofs,len = self.seg:getBuffer()
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, starts+math.floor(#self.seg/3) )
		local s      = self.rtvg:getString( ends-starts )
		self.seg:write( s, starts, #s )
		local readS  = self.seg:read( starts, #s )
		assert( s == self.seg:read(starts, #s),
			"Overwritten part of Segment:\n"..self.seg:read(starts, #s).."\n shall equal new string:\n"..s )
	end,

	test_Equals = function( self )
		Test.Case.describe( "__eq metamethod properly compares for equality" )
		local buf,ofs,len = self.seg:getBuffer()
		local rseg = self.seg
		local nseg = Segment( self.b,ofs,len )
		assert( rseg == self.seg, "References and Original must be equal" )
		assert( nseg == self.seg, "Equal Segment and Original must be equal" )
	end,

	test_NotEquals = function( self )
		Test.Case.describe( "__eq metamethod properly compares for inequality" )
		local buf,ofs,len = self.seg:getBuffer()
		local oseg = Segment( self.b, ofs+1, len   )
		local lseg = Segment( self.b, ofs,   len+1 )
		assert( lseg ~= oseg,     "Different length and offset shall make Segment unequal" )
		assert( oseg ~= self.seg, "Different offset shall make Segment unequal" )
		assert( lseg ~= self.seg, "Different length shall make Segment unequal" )
	end,

	test_getBuffer = function( self )
		Test.Case.describe( "GetBuffer returns T.Buffer, offset and length" )
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, #self.seg-3 )
		local nlen   = ends-starts
		local seg    = Segment( self.b, starts, nlen )
		local buf,ofs,len = seg:getBuffer( )
		assert( buf == self.b, "Reported T.Buffer must equal original T.Buffer" )
		-- __tostring reports memory address
		assert( tostring(buf) == tostring(self.b), "Reported T.Buffer reference must equal original T.Buffer" )
		assert( starts == ofs, "Reported offset must equal input value" )
		assert( #seg   == len, "Reported length must equal calculated lenght" )
		assert( nlen   == len, "Reported length must equal input value" )
	end,
}

t = Test( tests )
t( )
print( t )
