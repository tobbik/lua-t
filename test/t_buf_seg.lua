---
-- \file    t_buf_seg.lua
-- \brief   Test for the T.Buffer.Segment implementation
--          The tests are largely the same as T.Buffer since the interface
--          between the two classes is unified to provide usage across T.Net
--          and T.Pack
local T       = require( "t" )
local Test    = require( "t.Test" )
local Buffer  = require( "t.Buffer" )
local Rtvg    = T.require( 'rtvg' )

return {
	rtvg       = Rtvg( ),
	beforeEach = function( self )
		local  n = math.random( 1000,2000 )
		self.s   = self.rtvg:getWords( n )
		self.b   = Buffer( self.s )
		local  l = #self.s
		self.margin = math.floor(l/8)
		local st = math.random( self.margin, math.floor(l/2)-self.margin )
		self.seg = self.b:Segment( st, math.floor(l/2) )
	end,

	ConstructorFullSegment = function( self )
		Test.describe( "Create a segment covering the entire buffer" )
		local seg         = self.b:Segment( )
		local buf,ofs,len = seg.buffer, seg.start, seg.size
		assert( #seg == #self.b, "Length of T.Buffer.Segment should be "..#self.b.." but was " ..#seg )
		assert( #seg == len,     "Length of T.Buffer.Segment should be "..#seg.." but was " ..len )
		assert( ofs  == 1,       "Offset of T.Buffer.Segment should be 1 but was " ..ofs )
		assert( seg:read() == self.b:read(),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( self.b:read(), seg:read() ) )
	end,

	ConstructorPartialSegment = function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		Test.describe( "Create a segment starting at %d byte of %s", ofs, buf )
		local seg = self.b:Segment( ofs )
		assert( #seg == #self.b - ofs + 1, "Length of T.Buffer.Segment should be "..(#self.b-ofs+1).." but was "..#seg )
		assert( seg:read() == self.b:read( ofs ),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( self.b:read( ofs ), seg:read() ) )
	end,

	ConstructorPartialLengthSegment = function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		ofs,len = math.floor( ofs + len/3 ), math.ceil( len/2 )
		Test.describe( "Create a "..len.." bytes long segment starting at "..ofs.." byte of "..tostring(self.b) )
		local seg = self.b:Segment( ofs, len )
		assert( #seg == len, "Length of T.Buffer.Segment should be "..len.." but was " ..#seg )
		assert( seg:read() == self.b:read( ofs, len ),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( self.b:read( ofs, len ), seg:read() ) )
	end,

	ConstructorPartialSegmentStartOne = function( self )
		Test.describe( "Segment starting at 1 should equal Segment covering full Buffer" )
		local seg1 = self.b:Segment( 1 )
		local seg  = self.b:Segment( )
		assert( #seg1 == #seg, "Length of T.Buffer.Segment should be "..#seg.." but was " ..#seg1 )
		assert( seg1  ==  seg, "T.Buffer.Segments should be eqyual" )
		assert( seg1:read() == seg:read(),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( seg:read(), seg1:read() ) )
	end,

	ConstructorOffsetOverflowFails = function( self )
		Test.describe( "Creating Segment with offset higher then #buffer fails" )
		local f   = function(s) local seg = s.b:Segment( #s.b+5 ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "offset relative to length of T.Buffer out of bound" ), "Wrong Error message: "..e )
	end,

	ConstructorLengthOverflowFails = function( self )
		Test.describe( "Creating Segment with length higher then #buffer fails" )
		local f   = function(s) local seg = s.b:Segment( 1, #s.b+5 ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer.Segment length out of bound" ), "Wrong Error message: "..e )
	end,

	ConstructorOffsetPlusLengthOverflowFails = function( self )
		Test.describe( "Creating Segment with offset+length higher then #buffer fails" )
		local f   = function(s) local seg = s.b:Segment( #s.b-5, 10 ) end
		local r,e = pcall( f, self )
		assert( not r, "Segement constructor should have failed" )
		assert( e:match( "T.Buffer.Segment length out of bound" ), "Wrong Error message: "..e )
	end,

	ConstructorZeroPartialLengthSegmentStart = function( self )
		Test.describe( "Create a 0 bytes long segment starting at the beginning" )
		local seg         = self.b:Segment( 1, 0 )
		assert( #seg == 0, "Length of T.Buffer.Segment should be ".. 0 .." but was " ..#seg )
		assert( seg:read() == self.b:read( seg.start, #seg ),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( self.b:read( seg.start, #seg ), seg:read() ) )
	end,

	ConstructorZeroPartialLengthSegmentMiddle = function( self )
		local start       = math.random( math.floor(#self.seg/3), #self.b - math.floor(#self.seg/3 ) )
		Test.describe( "Create a 0 bytes long segment starting in the middle" )
		local seg         = self.b:Segment( start, 0 )
		assert( #seg == 0, "Length of T.Buffer.Segment should be ".. 0 .." but was " ..#seg )
		assert( seg:read() == self.b:read( seg.start, #seg ),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( self.b:read( seg.start, #seg ), seg:read() ) )
	end,

	ConstructorZeroPartialLengthSegmentEnd = function( self )
		Test.describe( "Create a 0 bytes long segment starting at the end" )
		local seg         = self.b:Segment( #self.b, 0 )
		assert( #seg == 0, "Length of T.Buffer.Segment should be ".. 0 .." but was " ..#seg )
		assert( seg.start == #self.b, "Start of T.Buffer.Segment should be ".. #self.b .." but was " .. seg.start )
		assert( seg:read() == self.b:read( seg.start, #seg ),
			("Content of T.Buffer.Segment should be `%s` but was `%s`."):format( self.b:read( seg.start, #seg ), seg:read() ) )
	end,

	ClearBufferSegment = function( self )
		Test.describe( "Clearing Buffer.Segment sets each byte to 0" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		for i=1,#self.seg do
			assert( string.byte( self.seg:read( i, 1 ), 1 ) ~= 0,
				"Value in segment at position "..i.. " should not be 0" )
		end
		self.seg:clear()
		for i=1,#self.seg do
			assert( string.byte( self.seg:read( i, 1 ), 1 ) == 0,
				"Value in segment at position "..i.. " should be 0" )
		end
		for i=ofs,ofs+#self.seg-1 do
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

	ReadIndexOutOfRangeReturnsEmptyString = function( self )
		Test.describe( "Reading from Buffer.Segment past it's length should retun empty string" )
		local r = self.seg:read( #self.seg+2 )
		assert( r == "", "Should be empty string but was " .. r )
	end,

	ReadLengthBeyondSegmentLengthShorterString = function( self )
		Test.describe( "Reading from Buffer beyond it's length returns shorter string" )
		local r = self.seg:read( 0-self.margin, 2*self.margin )
		assert( #r == self.margin, "Expected 100 but was " .. #r )
	end,

	ReadFullSegment = function( self )
		Test.describe( "Reading full Buffer.Segment content gets full string" )
		assert( self.seg:read( ) == self.b:read( self.seg.start, #self.seg ),
			"Read data from segment and buffer must be equal" )
	end,

	SetSizeIncreaseLength= function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local nLen        = #buf - ofs - 2
		Test.describe( ("Change length of %s in %s from %d to %d"):format( self.seg, buf, len, nLen ) )
		local seg = self.b:Segment( ofs, len )
		seg.size = nLen
		assert( #seg == nLen, ("Length of T.Buffer.Segment should be %d but was %d"):format( nLen, #seg ) )
		assert( seg:read() == self.b:read( ofs, nLen ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read(ofs, len ), seg:read() ) )
	end,

	SetSizeIncreaseToMaxLength= function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		Test.describe( ("Change length of %s from %d to %d"):format( self.seg, len, len ) )
		local seg = self.b:Segment( ofs, len )
		seg.size = len
		assert( #seg == len, ("Length of T.Buffer.Segment should be %d but was %d"):format( len, #seg ) )
		assert( seg:read() == self.b:read( ofs, len ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read(ofs, len ), seg:read() ) )
	end,

	SetSizeDecreaseLength= function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local nlen        = len - math.floor( len/3 )
		Test.describe( ("Change length of %s in %s from %d to %d"):format( self.seg, buf, len, nlen ) )
		local seg = self.b:Segment( ofs, len )
		seg.size = nlen
		assert( #seg == nlen, ("Length of T.Buffer.Segment should be %d but was %d"):format( nlen, #seg ) )
		assert( seg:read() == self.b:read( ofs, nlen ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read(ofs, len ), seg:read() ) )
	end,

	SetSizeIncreaseLengthPastBufferSizeFails= function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local errMsg      = "T.Buffer.Segment length out of bound"
		local nlen        = #buf-ofs+23
		Test.describe( ("Change length of %s in %s from %d to %d should fail"):format( self.seg, buf, len, nlen ) )
		local seg = self.b:Segment( ofs, len )
		local f   = function(s,l) s.size = l end
		local r,e = pcall( f, self.seg, nlen )
		assert( not r, "Increasing size past length of Buffer should fail" )
		assert( e:match( errMsg), ("Error message should contain `%s`, but was `%s`"):format( errMsg, e ) )
	end,

	SetLastLengthenSegment= function( self )
		local buf,ofs,last,len = self.seg.buffer, self.seg.start, self.seg.last, #self
		local nLast        = last + 6
		Test.describe( ("Change last index of %s in %s length from %d to %d lengthens segment"):format( self.seg, buf, last, nLast ) )
		self.seg.last      = nLast
		local xBuf,xOfs,xLen = self.seg.buffer, self.seg.start, #self.seg
		assert( xOfs == ofs, "Start should remain the same" )
		assert( xLen == #self.seg, ("Length should have increased to %d but was %d"):format( len+6, xLen ) )
		assert( self.seg:read() == self.b:read( ofs, xLen ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read( ofs, xLen ), self.seg:read() ) )
	end,

	SetLastShortenSegment = function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local nLast      = ofs + len - 6
		Test.describe( ("Change end of %s in %s length from %d to %d"):format( self.seg, buf, len+ofs, nLast ) )
		self.seg.last     = nLast
		local xBuf,xOfs,xLen = self.seg.buffer, self.seg.start, self.seg.size
		assert( xOfs == ofs, "Offset should remain the same" )
		assert( xLen == len-6, ("Length should have increased to %d but was %d"):format( len+6, xLen ) )
		assert( self.seg:read() == self.b:read( ofs, xLen ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read( ofs, xLen ), self.seg:read() ) )
	end,

	ShiftRight= function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local shft        = #buf - self.seg.last - 2
		Test.describe( ( "Shift %s in %d to the right"):format( self.seg, shft ) )
		self.seg:shift( shft )
		local xBuf,xOfs,xLen = self.seg.buffer, self.seg.start, self.seg.size
		assert( xOfs == ofs+shft, "Offset should increase" )
		assert( xLen == len,   "Length should remain the same" )
		assert( self.seg:read() == self.b:read( xOfs, len ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read( ofs, xLen ), self.seg:read() ) )
	end,

	ShiftLeft= function( self )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local shft        = 0 - ofs + 2
		Test.describe( ("Shift %s in %d to the left"):format( self.seg, shft ) )
		self.seg:shift( shft )
		local xBuf,xOfs,xLen = self.seg.buffer, self.seg.start, self.seg.size
		assert( xOfs == ofs+shft, "Offset should increase" )
		assert( xLen == len,   "Length should remain the same" )
		assert( self.seg:read() == self.b:read( xOfs, len ),
			("Content of T.Buffer.Segment should be %s but wa %s"):format( self.b:read( ofs, xLen ), self.seg:read() ) )
	end,

	Next= function( self )
		local seg  = self.b:Segment( 1, math.floor( #self.b/2 ) - 20 )
		Test.describe( ("Next Segment %s"):format( seg ) )
		local oLen,oStart = #seg, seg.start
		assert( seg:next( ), "Forwarding to next segment should be successful" )
		assert( seg.start == oLen + oStart, ("Next Segment should start at %d but started at %d"):format(
			 oLen + oStart, seg.start ) )
		assert( #seg == oLen, "nxt should have same length" )
	end,

	NextLastSegmentShorter = function( self )
		local seg  = self.b:Segment( 1, math.floor( #self.b/2 ) - 20 )
		Test.describe( ("Next Segment %s last seg:next() should be shorther"):format( seg ) )
		local len,nxtLen = #seg,#seg
		while seg:next() do
			nxtLen = #seg
		end
		assert( nxtLen < len, "Last seg:next() segment should be shorter than original" )
	end,

	ReadPartialSegment = function( self )
		Test.describe( "Reading partial Buffer.Segment content matches string" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, #self.seg )
		local subS   = self.s:sub( starts+ofs-1, ends+ofs-1 )
		local readS  = self.seg:read( starts, ends-starts+1 )
		assert( #subS == #readS, "#Substring shall be "..#subS.." but is ".. #readS )
		assert( subS == readS,  "Substring shall be "..subS.." but is ".. readS )
	end,

	WriteFullSegment = function( self )
		Test.describe( "Overwrite entire Buffer Segment content" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local s = self.rtvg:getString( #self.seg )
		local before = buf:read( 1, ofs-1 )
		local after  = buf:read( ofs+len  )
		self.seg:write( s )
		assert( self.seg:read( )        == s, "Read data and new string must be equal" )
		assert( self.b:read( ofs, len ) == s, "Data in Buffer within segment must be equal" )
		assert( self.b:read( 1, ofs-1 ) == before, "Data in Buffer before segment must be untouched" )
		assert( self.b:read( ofs+len )  == after, "Data in Buffer after segment must be untouched" )
	end,

	WritePartial = function( self )
		Test.describe( "Writing partial Buffer Segment" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, #self.seg-3 )
		local s      = self.rtvg:getString( ends-starts )
		self.seg:write( s, starts )
		local readS  = self.seg:read( starts, ends-starts )
		assert( #s == #readS, "#Substring shall be "..#s.." but is ".. #readS )
		assert( s  ==  readS, "Substrings shall be equal" )
		assert( self.b:read( ofs+starts-1, #s ) == s, "Data in Buffer within segment must be equal" )
	end,

	WritePartialString = function( self )
		Test.describe( "Writing partial string to Buffer content matches string" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local starts = math.random( math.floor(len/6), math.floor(len/2 ) )
		local s,wLen = self.rtvg:getString( len ), math.floor( len/2 )
		self.seg:write( s, starts, wLen )
		local readS  = self.seg:read( starts, wLen )
		assert( s:sub(1,wLen) == self.seg:read(starts, wLen),
			("Overwritten part of Segment:\n %s \n shall equal new string:\n%s"):format( self.seg:read(starts, #s), s:sub(1,wLen) ) )
	end,

	Equals = function( self )
		Test.describe( "__eq metamethod properly compares for equality" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local rseg = self.seg
		local nseg = self.b:Segment( ofs, len )
		assert( rseg == self.seg, "References and Original must be equal" )
		assert( nseg == self.seg, "Equal Segment and Original must be equal" )
	end,

	NotEquals = function( self )
		Test.describe( "__eq metamethod properly compares for inequality" )
		local buf,ofs,len = self.seg.buffer, self.seg.start, self.seg.size
		local oseg = self.b:Segment( ofs+1, len   )
		local lseg = self.b:Segment( ofs,   len+1 )
		assert( lseg ~= oseg,     "Different length and offset shall make Segment unequal" )
		assert( oseg ~= self.seg, "Different offset shall make Segment unequal" )
		assert( lseg ~= self.seg, "Different length shall make Segment unequal" )
	end,

	getBuffer = function( self )
		Test.describe( "GetBuffer returns T.Buffer, offset and length" )
		local starts = math.random( math.floor(#self.seg/5), math.floor(#self.seg/2 ) )
		local ends   = math.random( starts, #self.seg-3 )
		local nlen   = ends-starts
		local seg    = self.b:Segment( starts, nlen )
		local buf,ofs,len = seg.buffer, seg.start, seg.size
		assert( buf == self.b, "Reported T.Buffer must equal original T.Buffer" )
		-- __tostring reports memory address
		assert( tostring(buf) == tostring(self.b), "Reported T.Buffer reference must equal original T.Buffer" )
		assert( starts == ofs, "Reported offset must equal input value" )
		assert( #seg   == len, "Reported length must equal calculated lenght" )
		assert( nlen   == len, "Reported length must equal input value" )
	end,

	toHex = function( self )
		Test.describe( "T.Buffer.Segment:toHex() creates a proper hex string" )
		local a = { 65,66,67,68,69,70 }
		local x = string.char( a[1], a[2], a[3], a[4], a[5], a[6] )
		local X = ('%02X %02X %02X %02X %02X %02X'):format(  a[1], a[2], a[3], a[4], a[5], a[6] )
		local b = Buffer( x )
		local s = b:Segment( )
		assert( s:toHex() == X, ("Expected HexString: `%s` but got `%s`"):format( X, s:toHex() ) )
	end
}
