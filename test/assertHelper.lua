local t_assert, t_type = require't'.assert,require't'.type
local Pack             = require't.Pack'

return {
	-- #########################################################################
	-- assert helpers
	Socket = function( sck, pro, fam, typ )
		t_assert( 'T.Net.Socket' == t_type( sck ), "Should be `t.Net.Socket` but was `%s`", t_type( sck ) )
		t_assert( pro == sck.protocol, "Protocol should be `%s`but is `%s`", pro, sck.protocol )
		t_assert( fam == sck.family  , "Family should be `%s` but is `%s`", fam, sck.family )
		t_assert( typ == sck.type    , "Type should be `%s` but is `%s`", typ, sck.type )
	end,

	Address = function( adr, family, ip, port )
		t_assert( 'T.Net.Address' == t_type( adr ), "Should be `t.Net.Address` but was `%s`", t_type( adr ) )
		t_assert( ip     == adr.ip,     "IP should be `%s` but is `%s`",   ip,     adr.ip )
		t_assert( family == adr.family, "Family should be `%s` but is `%s`", family, adr.family )
		if type( port ) ~= 'number' then
			assert( port ~= 0, "Port should not be 0" )
		else
			t_assert( port == adr.port, "Port should be %d but is %d", port, adr.port )
		end
	end,

	Packer = function( pck, typ, sizByte, sizBit )
		local expect   = 't.Pack.' .. typ
		local pck_name = tostring( pck ):match( '(.*):' )
		--print( 'PCK_NAME:', expect, pck_name )
		t_assert( expect == pck_name,
		          "Packer should be '%s' but was '%s'", expect, pck_name )
		local sByt,sBit = Pack.getSize( pck )
		t_assert( sByt == sizByte, "Expected Byte size %d was %d for `%s`", sizByte, sByt, typ )
		t_assert( sBit == sizBit,  "Expected Bit  size %d was %d for `%s`", sizByte, sBit, typ )
	end
}
