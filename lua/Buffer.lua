local Buffer = require( "t.buf" )

local seg_mt = debug.getregistry( )[ "T.Buffer.Segment" ]

seg_mt.next  = function( self )
	if self.start + #self + #self <= #self.buffer then
		nxt = self.buffer:Segment( self.start + #self, #self )
	else
		nxt = self.buffer:Segment( self.start + #self )
	end
	return nxt
end

--[[
seg_mt.shift = function( self, offset )
	local start, size = self:getSegment( )
	self:segSegment( self.start + offset, self.size )
end
--]]

return Buffer
