local Buffer   = require( "t.buf" )

local buf    = Buffer( 1 )
local seg_mt = getmetatable( buf:Segment( ) )

seg_mt.shift = function( self, ofs )
	self.start = self.start + ofs
end

seg_mt.next = function( self )
	if self.start + #self + #self <= #self.buffer then
		nxt = self.buffer:Segment( self.start + #self, #self )
	else
		nxt = self.buffer:Segment( self.start + #self )
	end
	return nxt
end


return Buffer
