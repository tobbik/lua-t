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
		assert( host == adr.ip, "Host should be "..host.." but is "..tostring( adr.ip ) )
		if type( port ) ~= 'number' then
			assert( port ~= 0, "Port should not be 0" )
		else
			assert( port == adr.port, "Port should be "..port.." but is "..tostring( adr.port ) )
		end
	end,

	Packer = function( pck, typ, sizByte, sizBit, x )
		local ext      = ('number' == type(x) ) and ':'..x or x
		local expect   = 't.Pack.' .. typ .. (ext and ext or '')
		local pck_name = tostring( pck ):match( '(.*):' )
		--print( 'PCK_NAME:', expect, pck_name )
		T.assert( expect == pck_name,
		          "Packer should be '%s' but was '%s'", expect, pck_name )
		local sByt,sBit = Pack.getSize( pck )
		T.assert( sByt == sizByte, "Expected Byte size %d was %d for `%s`", sizByte, sByt, typ )
		T.assert( sBit == sizBit,  "Expected Bit  size %d was %d for `%s`", sizByte, sBit, typ )
	end
}
