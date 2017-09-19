local T,Pack = require't',require't.Pack'

return {
	-- #########################################################################
	-- assert helpers
	Socket = function( sck, pro, fam, typ )
		T.assert( pro == sck.protocol, "Protocol should be `%s`but is `%s`", pro, sck.protocol )
		T.assert( fam == sck.family  , "Family should be `%s` but is `%s`", fam, sck.family )
		T.assert( typ == sck.type    , "Type should be `%s` but is `%s`", typ, sck.type )
	end,

	Address = function( adr, family, ip, port )
		T.assert( ip     == adr.ip,     "IP should be `%s` but is `%s`",   ip,     adr.ip )
		T.assert( family == adr.family, "Family should be `%s` but is `%s`", family, adr.family )
		if type( port ) ~= 'number' then
			assert( port ~= 0, "Port should not be 0" )
		else
			T.assert( port == adr.port, "Port should be %d but is %d", port, adr.port )
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
