local Loop = require( "t.ael" )

local pack,unpack,pcall = table.pack,table.unpack,pcall

local addTimeNode = function( self, timer, ... )

end

local execDescNode = function( self, func )
	local x,e = pcall( unpack( fnc ) )
	

end

local execTimeNode = function( self, node )
	local x,e = pcall( unpack( node.fnc ) )
	if x and e and rawequal( node.tv, e ) then

	end
end

local addDescNode = function( self, hndl, dir, ... )
	local fnc = pack( select( 1, ... ) )
	fnc.n     = nil
	local dnd = self.dnodes[ hndl ] and self.dnodes[ hndl ] or { msk = msk }
	dnd.rfnc = (msk==1) and fnc or nil
	dnd.wfnc = (msk==2) and fnc or nil
	if not self.dnodes[ hndl ] then self.dnodes[ hndl ] = dnd end
end

return Loop
