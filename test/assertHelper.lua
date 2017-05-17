local T,Pack = require't',require't.Pack'

return {
	-- #########################################################################
	-- assert helpers
	Socket = function( sck, pro, fam, typ )
		assert( pro == sck.protocol, "Protocol should be "..pro.." but is ".. tostring(sck.protocol) )
		assert( fam == sck.family  , "Family should be "  ..fam.." but is ".. tostring(sck.family) )
		assert( typ == sck.type    , "Type should be "    ..typ.." but is ".. tostring(sck.type) )
	end,

	Address = function( adr, host, port )
		local h,p = adr:get( )
		assert( host == h, "Host should be "..host.." but is "..tostring(h) )
		if type( port ) ~= 'number' then
			assert( port ~= 0, "Port should not be 0" )
		else
			assert( port == p, "Port should be "..port.." but is "..tostring(p) )
		end
	end,

	Packer = function( pck, typ, siz, x )
		local ext      = ('number' == type(x) ) and ':'..x or x
		local expect   = 't.Pack.' .. typ .. (siz and siz or '') .. (ext and ext or '')
		local pck_name = tostring( pck ):match( '(.*):' )
		--print( 'PCK_NAME:', expect, pck_name )
		T.assert( expect == pck_name,
		          "Packer should be '%s' but was '%s'", expect, pck_name )
		if siz then
			T.assert( Pack.getSize( pck ) == siz, "Expected size %d was %d", siz, Pack.getSize( pck ) )
		end
	end
}
